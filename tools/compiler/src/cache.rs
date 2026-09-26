use crate::util::*;
use anyhow::{Context, Result, ensure};
use serde_json::json;
use std::{
    collections::{BTreeMap, BTreeSet},
    fs,
    io::{Cursor, Read, Write},
    path::{Path, PathBuf},
};
use zip::{CompressionMethod, ZipArchive, ZipWriter, write::SimpleFileOptions};

fn unpack(archive: &Path) -> Result<BTreeMap<String, Vec<u8>>> {
    let mut packed = ZipArchive::new(fs::File::open(archive)?)?;
    unpack_checked(&mut packed)
}
fn unpack_checked(packed: &mut ZipArchive<fs::File>) -> Result<BTreeMap<String, Vec<u8>>> {
    let mut files = BTreeMap::new();
    for i in 0..packed.len() {
        let mut f = packed.by_index(i)?;
        safe_relative(f.name())?;
        ensure!(f.is_file(), "non-file core entry");
        let name = f.name().to_owned();
        let mut bytes = Vec::new();
        f.read_to_end(&mut bytes)?;
        ensure!(
            files.insert(name.clone(), bytes).is_none(),
            "duplicate core entry: {name}"
        );
    }
    let manifest: serde_json::Value = serde_json::from_slice(
        &files
            .remove("manifest.json")
            .context("missing core manifest")?,
    )?;
    ensure!(
        manifest["schema_version"] == 1,
        "unsupported core cache format"
    );
    let inventory = manifest["files"]
        .as_object()
        .context("missing core inventory")?;
    ensure!(
        files.contains_key("core.lib")
            && files.keys().collect::<BTreeSet<_>>() == inventory.keys().collect(),
        "incomplete core cache"
    );
    for (name, bytes) in &files {
        ensure!(
            digest(bytes) == string(&inventory[name])?,
            "core cache SHA-256 mismatch: {name}"
        );
    }
    Ok(files)
}
fn pack_updated(
    files: BTreeMap<String, Vec<u8>>,
    mut previous: Option<&mut ZipArchive<fs::File>>,
    changed: &BTreeSet<String>,
) -> Result<Vec<u8>> {
    let mut packed = ZipWriter::new(Cursor::new(Vec::new()));
    for (name, bytes) in files {
        if !changed.contains(&name)
            && let Some(previous) = previous.as_mut()
        {
            // unpack_checked validated the same open archive before mutation.
            // Preserve compressed data for untouched members, without Deflate.
            packed.raw_copy_file(previous.by_name(&name)?)?;
        } else {
            packed.start_file(
                name,
                SimpleFileOptions::default().compression_method(CompressionMethod::Deflated),
            )?;
            packed.write_all(&bytes)?;
        }
    }
    Ok(packed.finish()?.into_inner())
}
pub fn archive(sdar: &Path, archive: &Path, object: &Path, flags: &[String]) -> Result<()> {
    archive_many(sdar, archive, &[object.to_path_buf()], flags)
}
pub fn archive_many(
    sdar: &Path,
    archive: &Path,
    objects: &[PathBuf],
    flags: &[String],
) -> Result<()> {
    ensure!(!objects.is_empty(), "archive requires at least one object");
    let sdar = if sdar.is_file() {
        sdar.to_path_buf()
    } else {
        suffix(sdar, std::env::consts::EXE_SUFFIX)
    };
    let library = archive.with_extension("lib");
    let base = archive.parent().context("missing archive directory")?;
    let mut previous = if archive.exists() {
        Some(ZipArchive::new(fs::File::open(archive)?)?)
    } else {
        None
    };
    let mut changed = BTreeSet::from(["core.lib".into(), "manifest.json".into()]);
    let mut files = if let Some(previous) = &mut previous {
        unpack_checked(previous)?
    } else {
        BTreeMap::new()
    };
    let mut native = Vec::new();
    for object in objects {
        let rel = if object.extension().is_some_and(|e| e == "o") {
            object.with_extension("rel")
        } else {
            object.to_path_buf()
        };
        nonempty(&rel)?;
        let obj = rel.with_extension("o");
        let cpp = crate::artifact::validate(&obj)?;
        if rel.file_name().is_some_and(|n| n == "stcxx_heap.c.rel") {
            ensure!(
                archive.file_name().is_some_and(|n| n == "core.a"),
                "heap cannot enter a non-core archive"
            );
        } else {
            native.push(s(&rel));
        }
        let mut paths = vec![rel.clone(), obj.clone()];
        if rel.with_extension("lst").is_file() {
            paths.push(rel.with_extension("lst"));
        }
        for ext in [".stcxx.json", ".stcxx.bc", ".stcxx.ll", ".stcxx.module.cbe"] {
            let p = suffix(&obj, ext);
            if p.exists() {
                paths.push(p);
            }
        }
        let metadata = suffix(&rel, ".stcxx-c.json");
        if metadata.exists() {
            paths.push(metadata);
        }
        for p in paths {
            let name = p
                .strip_prefix(base)
                .context("object outside core directory")?
                .to_string_lossy()
                .replace('\\', "/");
            safe_relative(&name)?;
            changed.insert(name.clone());
            files.insert(name, fs::read(p)?);
        }
        if cpp {
            // A precompiled library must survive removal of its original checkout.
            let meta = crate::util::json(suffix(&obj, ".stcxx.json"))?;
            let source = PathBuf::from(string(&meta["source"])?);
            check_hash(&source, string(&meta["source_sha256"])?)?;
            let name = suffix(&obj, ".stcxx.source")
                .strip_prefix(base)?
                .to_string_lossy()
                .replace('\\', "/");
            changed.insert(name.clone());
            files.insert(name, fs::read(source)?);
        }
    }
    // Validate every member before touching the native archive. A cold cache
    // materialization can then create its library and ZIP in one pass.
    if let Some(bytes) = files.get("core.lib") {
        write(&library, bytes)?;
    } else {
        remove(&library)?;
    }
    let mut base_args = flags.to_vec();
    base_args.push(s(&library));
    for args in archive_batches(&sdar, &base_args, &native) {
        run(&sdar, &args, None, false, None)?;
    }
    files.insert(
        "core.lib".into(),
        if library.exists() {
            fs::read(&library)?
        } else {
            b"!<arch>\n".to_vec()
        },
    );
    let inventory: BTreeMap<_, _> = files.iter().map(|(n, b)| (n.clone(), digest(b))).collect();
    files.insert(
        "manifest.json".into(),
        serde_json::to_vec_pretty(&json!({"schema_version":1,"files":inventory}))?,
    );
    let packed = pack_updated(files, previous.as_mut(), &changed)?;
    // Close the read handle before atomically replacing the ZIP on Windows.
    drop(previous);
    atomic_write(archive, &packed)
}
fn archive_batches(sdar: &Path, base: &[String], objects: &[String]) -> Vec<Vec<String>> {
    // Windows limits CreateProcess to 32767 UTF-16 code units, including the
    // quoted executable and arguments. Keep headroom and preserve input order.
    let width = |value: &str| quote(value).encode_utf16().count() + 1;
    let fixed = width(&s(sdar)) + base.iter().map(|a| width(a)).sum::<usize>();
    let mut batches = Vec::new();
    let mut args = base.to_vec();
    let mut length = fixed;
    for object in objects {
        let size = width(object);
        if length + size > 30000 && args.len() > base.len() {
            batches.push(args);
            args = base.to_vec();
            length = fixed;
        }
        args.push(object.clone());
        length += size;
    }
    if args.len() > base.len() {
        batches.push(args);
    }
    batches
}
pub fn materialize(archive: &Path, work: &Path) -> Result<PathBuf> {
    if !fs::read(archive)?.starts_with(b"PK") {
        // SDAR archives containing native RELs are valid standalone libraries.
        crate::link::archive_members(archive)?;
        let destination = work.join("native-cache").join(hash(archive)?);
        let native = destination.join("native.a");
        write(&native, fs::read(archive)?)?;
        write(native.with_extension("lib"), fs::read(archive)?)?;
        return Ok(native);
    }
    let files = unpack(archive)?;
    let destination = work.join("core-cache").join(hash(archive)?);
    for (name, bytes) in &files {
        let path = destination.join(safe_relative(name)?);
        let mut bytes = bytes.clone();
        if name.ends_with(".o.stcxx.json") {
            let mut meta: serde_json::Value = serde_json::from_slice(&bytes)?;
            let obj = destination.join(name.trim_end_matches(".stcxx.json"));
            meta["object"] = json!(s(&obj));
            meta["bitcode"] = json!(s(suffix(obj, ".stcxx.bc")));
            let source = name.trim_end_matches(".stcxx.json").to_owned() + ".stcxx.source";
            if files.contains_key(&source) {
                meta["source"] = json!(s(destination.join(source)));
            }
            bytes = serde_json::to_vec_pretty(&meta)?;
        }
        if name.ends_with(".rel.stcxx-c.json") {
            let mut meta: serde_json::Value = serde_json::from_slice(&bytes)?;
            // Preserve the library classification before relocating its REL
            // from build/libraries into the content-addressed archive cache.
            meta["library_source"] = json!(crate::trim::is_library_source(&meta));
            meta["original_rel"] =
                json!(s(destination.join(name.trim_end_matches(".stcxx-c.json"))));
            bytes = serde_json::to_vec_pretty(&meta)?;
        }
        write(path, bytes)?;
    }
    let native = destination.join("core.a");
    write(&native, &files["core.lib"])?;
    Ok(native)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn batch_archive_preserves_heap_rules_and_validates_before_mutating() {
        let temp = tempfile::tempdir().unwrap();
        let heap = temp.path().join("stcxx_heap.c.o");
        let archive = temp.path().join("core.a");
        let sdar = temp.path().join("no-tool-needed");
        write(&heap, b"S _heap Def000000\n").unwrap();
        fs::copy(&heap, heap.with_extension("rel")).unwrap();
        archive_many(&sdar, &archive, &[heap.clone()], &["rcs".into()]).unwrap();
        let files = unpack(&archive).unwrap();
        assert_eq!(files["core.lib"], b"!<arch>\n");
        assert!(files.contains_key("stcxx_heap.c.rel"));
        assert!(
            archive_many(&sdar, &temp.path().join("library.a"), &[heap.clone()], &[])
                .unwrap_err()
                .to_string()
                .contains("heap cannot enter")
        );
        let good = fs::read(&archive).unwrap();
        let library = fs::read(archive.with_extension("lib")).ok();
        let bad = temp.path().join("bad.o");
        write(&bad, b"S _bad Def000000\n").unwrap();
        write(bad.with_extension("rel"), b"mismatched").unwrap();
        assert!(
            archive_many(&sdar, &archive, &[heap, bad], &[])
                .unwrap_err()
                .to_string()
                .contains("object/REL mismatch")
        );
        assert_eq!(fs::read(&archive).unwrap(), good);
        assert_eq!(fs::read(archive.with_extension("lib")).ok(), library);
    }
    #[test]
    fn archive_batches_bound_command_lines_without_losing_order() {
        let base = vec!["rcs".into(), "library path/core.lib".into()];
        let objects: Vec<_> = (0..40)
            .map(|i| format!("{}-{i}.rel", "中 long path/".repeat(100)))
            .collect();
        let tool = Path::new("tool path/sdar.exe");
        let batches = archive_batches(tool, &base, &objects);
        assert!(batches.len() > 1);
        let mut flattened = Vec::new();
        for args in batches {
            assert_eq!(&args[..base.len()], &base);
            let line = format!(
                "{} {}",
                quote(&s(tool)),
                args.iter().map(|a| quote(a)).collect::<Vec<_>>().join(" ")
            );
            assert!(line.encode_utf16().count() < 30000);
            flattened.extend_from_slice(&args[base.len()..]);
        }
        assert_eq!(flattened, objects);
        assert!(archive_batches(tool, &base, &[]).is_empty());
    }
    #[test]
    fn archive_updates_preserve_unchanged_compressed_members() {
        let temp = tempfile::tempdir().unwrap();
        let archive = temp.path().join("core.a");
        let mut zip = ZipWriter::new(fs::File::create(&archive).unwrap());
        let files = BTreeMap::from([
            ("core.lib".to_owned(), b"old library".to_vec()),
            ("unchanged.o".to_owned(), b"unchanged object".repeat(100)),
        ]);
        let inventory: BTreeMap<_, _> = files.iter().map(|(n, b)| (n, digest(b))).collect();
        for (name, bytes) in &files {
            // Stored deliberately: recompressing this entry changes the method.
            zip.start_file(
                name,
                SimpleFileOptions::default().compression_method(CompressionMethod::Stored),
            )
            .unwrap();
            zip.write_all(bytes).unwrap();
        }
        zip.start_file("manifest.json", SimpleFileOptions::default())
            .unwrap();
        zip.write_all(&serde_json::to_vec(&json!({"schema_version":1,"files":inventory})).unwrap())
            .unwrap();
        zip.finish().unwrap();
        let mut previous = ZipArchive::new(fs::File::open(&archive).unwrap()).unwrap();
        let mut updated = unpack_checked(&mut previous).unwrap();
        updated.insert("core.lib".into(), b"new library".to_vec());
        updated.insert("new.o".into(), b"new object".to_vec());
        let inventory: BTreeMap<_, _> = updated
            .iter()
            .map(|(n, b)| (n.clone(), digest(b)))
            .collect();
        updated.insert(
            "manifest.json".into(),
            serde_json::to_vec(&json!({"schema_version":1,"files":inventory})).unwrap(),
        );
        let bytes = pack_updated(
            updated,
            Some(&mut previous),
            &BTreeSet::from(["core.lib".into(), "new.o".into(), "manifest.json".into()]),
        )
        .unwrap();
        drop(previous);
        atomic_write(&archive, &bytes).unwrap();
        let readback = unpack(&archive).unwrap();
        assert_eq!(readback["core.lib"], b"new library");
        assert_eq!(readback["new.o"], b"new object");
        assert_eq!(readback["unchanged.o"], files["unchanged.o"]);
        let mut zip = ZipArchive::new(fs::File::open(&archive).unwrap()).unwrap();
        assert_eq!(
            zip.by_name("unchanged.o").unwrap().compression(),
            CompressionMethod::Stored
        );
    }
    #[test]
    fn native_library_classification_survives_archive_relocation() {
        let temp = tempfile::tempdir().unwrap();
        let archive = temp.path().join("test.a");
        let mut files = BTreeMap::new();
        files.insert("core.lib", b"!<arch>\n".to_vec());
        files.insert(
            "test.c.rel.stcxx-c.json",
            serde_json::to_vec(&json!({
                "source":"/custom path/Test/test.c",
                "original_rel":"/build/libraries/Test/test.c.rel"
            }))
            .unwrap(),
        );
        let inventory: BTreeMap<_, _> = files.iter().map(|(n, b)| (*n, digest(b))).collect();
        files.insert(
            "manifest.json",
            serde_json::to_vec(&json!({"schema_version":1,"files":inventory})).unwrap(),
        );
        let mut zip = ZipWriter::new(fs::File::create(&archive).unwrap());
        for (name, bytes) in files {
            zip.start_file(name, SimpleFileOptions::default()).unwrap();
            zip.write_all(&bytes).unwrap();
        }
        zip.finish().unwrap();
        let output = materialize(&archive, &temp.path().join("cache")).unwrap();
        let meta =
            crate::util::json(output.parent().unwrap().join("test.c.rel.stcxx-c.json")).unwrap();
        assert_eq!(meta["library_source"], true);
        assert!(crate::trim::is_library_source(&meta));
        assert!(
            !string(&meta["original_rel"])
                .unwrap()
                .contains("/libraries/")
        );
        assert!(!crate::trim::is_library_source(&json!({
            "source":"/core/runtime.c", "original_rel":"/build/core/runtime.c.rel"
        })));
    }
    #[test]
    fn cache_rejects_tampered_payload_and_missing_members() {
        let temp = tempfile::tempdir().unwrap();
        for (name, payload, inventory) in [
            (
                "valid",
                b"original".as_slice(),
                json!({"core.lib":digest("original")}),
            ),
            (
                "tampered",
                b"changed".as_slice(),
                json!({"core.lib":digest("original")}),
            ),
            (
                "missing",
                b"original".as_slice(),
                json!({"core.lib":digest("original"),"missing.o":digest("x")}),
            ),
        ] {
            let path = temp.path().join(name);
            let mut zip = ZipWriter::new(fs::File::create(&path).unwrap());
            zip.start_file("core.lib", SimpleFileOptions::default())
                .unwrap();
            zip.write_all(payload).unwrap();
            zip.start_file("manifest.json", SimpleFileOptions::default())
                .unwrap();
            zip.write_all(
                &serde_json::to_vec(&json!({"schema_version":1,"files":inventory})).unwrap(),
            )
            .unwrap();
            zip.finish().unwrap();
            assert_eq!(unpack(&path).is_ok(), name == "valid");
        }
    }
}
