#!/usr/bin/env node
// Offline compatibility contract between the generated boards and stc-cli.
import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import { mkdirSync, mkdtempSync, readdirSync, readFileSync, writeFileSync } from "node:fs";
import { join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = fileURLToPath(new URL("..", import.meta.url));
const args = process.argv.slice(2);
assert(args.length === 0 || (args.length === 2 && args[0] === "--cli"),
  "Usage: node scripts/test-upload.mjs [--cli path/to/stc-cli]");
const cli = resolve(args[1] ?? join(root, "../stc-cli/target/release", process.platform === "win32" ? "stc-cli.exe" : "stc-cli"));
const read = path => readFileSync(path, "utf8");
const boards = new Map(read(join(root, "boards.txt")).split(/\r?\n/)
  .filter(line => line && !line.startsWith("#")).map(line => {
    const at = line.indexOf("=");
    assert(at > 0, "Invalid board property: " + line);
    return [line.slice(0, at), line.slice(at + 1)];
  }));
const platform = read(join(root, "platform.txt"));
for (const token of ["--expect \"{upload.model}\"", "--execution-mode mcs51", "--allow-experimental",
  "{upload.model_check_flags}", "--transport {upload.transport}", "--baud {upload.speed}"]) {
  assert(platform.includes(token), "Missing upload recipe parameter: " + token);
}
mkdirSync(join(root, ".build"), { recursive: true });
const output = mkdtempSync(join(root, ".build/upload-validation-"));
let checks = 0;
function run(argv, code = 0) {
  const result = spawnSync(cli, argv, { encoding: "utf8", timeout: 15000, windowsHide: true });
  if (result.error) throw result.error;
  assert.equal(result.status, code, argv.join(" ") + "\n" + result.stdout + result.stderr);
  checks++;
  return code === 0 ? JSON.parse(result.stdout) : result;
}
function record(type, address, bytes) {
  const data = [bytes.length, address >> 8, address & 255, type, ...bytes];
  data.push((-data.reduce((sum, byte) => sum + byte, 0)) & 255);
  return ":" + Buffer.from(data).toString("hex").toUpperCase() + "\n";
}
function hexAt(address) {
  return record(4, 0, [address >>> 24, (address >>> 16) & 255])
    + record(0, address & 65535, [0xA5]) + record(1, 0, []);
}
const results = [];
for (const entry of readdirSync(join(root, "variants"), { withFileTypes: true })) {
  if (!entry.isDirectory() || entry.name === "_common") continue;
  const variant = JSON.parse(read(join(root, "variants", entry.name, "variant.json")));
  const boardIds = [...boards].filter(([key, value]) => key.endsWith(".build.variant") && value === entry.name);
  assert.equal(boardIds.length, 1, "Board mapping: " + entry.name);
  const id = boardIds[0][0].slice(0, -".build.variant".length);
  const prop = suffix => boards.get(id + "." + suffix);
  assert.equal(prop("upload.model"), variant.model);
  assert.equal(prop("upload.transport"), "uart");
  assert.equal(Number(prop("upload.maximum_size")), variant.memory.maximum_code_bytes);
  assert.equal(variant.compiler_target, "mcs51");
  const validate = file => ["validate", "--expect", variant.model, "--file", file, "--execution-mode", "mcs51", "--json"];
  const files = {};
  for (const [name, address] of Object.entries({
    first: 0, linkedLast: variant.memory.maximum_code_bytes - 1,
    physicalLast: variant.memory.flash_bytes - 1, outside: variant.memory.flash_bytes,
    mcs251: 0xFF0000,
  })) {
    files[name] = join(output, id + "-" + name + ".hex");
    writeFileSync(files[name], hexAt(address));
  }
  const valid = run(validate(files.first));
  const device = valid.device;
  assert.equal(device.core, "mcs51");
  assert.equal(device.code_base, 0);
  assert.equal(device.code_flash, variant.memory.flash_bytes);
  assert(["stable", "experimental"].includes(device.support));
  assert(["stc89", "stc12", "stc15", "stc8", "stc8d", "stc8g", "ai8h"].includes(device.protocol));
  const force = prop("upload.model_check_flags").split(/\s+/).filter(Boolean);
  assert.deepEqual(force, device.magic === null ? ["--force-unverified-target"] : []);
  const baud = Number(prop("upload.speed"));
  assert.equal(baud, ["stc89", "stc12"].includes(device.protocol) ? 19200 : 115200);
  for (const file of [files.linkedLast, files.physicalLast]) {
    const result = run(validate(file));
    assert.equal(result.program_base_address, 0);
    assert.equal(result.program_bytes, device.code_flash);
  }
  run(validate(files.outside), 8);
  run(validate(files.mcs251), 8);
  const binary = join(output, id + ".bin");
  writeFileSync(binary, Buffer.from([0x02, 0x00, 0x10]));
  assert.equal(run(validate(binary)).program_bytes, 512);
  run(["validate", "--expect", variant.model, "--file", files.first, "--execution-mode", "mcs251"], 8);
  // Invalid firmware is rejected before opening a port. No physical I/O occurs.
  const flash = ["flash", "--port", "STC_CLI_OFFLINE_TEST_PORT", "--transport", prop("upload.transport"),
    "--expect", prop("upload.model"), "--execution-mode", "mcs51", "--reset", "manual",
    "--baud", String(baud), "--wait", "1", "--json"];
  run([...flash, "--file", files.outside, "--allow-experimental", ...force], 8);
  if (device.support === "experimental") run([...flash, "--file", files.first], 9);
  if (device.magic === null) run([...flash, "--file", files.first, "--allow-experimental"], 9);
  results.push({ variant: entry.name, model: device.name, magic: device.magic,
    protocol: device.protocol, support: device.support, flash: device.code_flash,
    linkLimit: variant.memory.maximum_code_bytes, baud, force: force.length !== 0 });
}
assert(results.length > 0, "No variants found");
writeFileSync(join(output, "results.json"), JSON.stringify({ cli, checks, results }, null, 2) + "\n");
console.table(results);
console.log(results.length + " variants, " + checks + " offline CLI checks passed. " + output);
