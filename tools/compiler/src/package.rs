use crate::util::*;
use anyhow::{Context, Result, ensure};
use serde_json::json;
use std::{collections::BTreeMap, fs, path::Path};
use zip::{CompressionMethod, ZipWriter, write::SimpleFileOptions};

fn copy_tree(source: &Path, destination: &Path) -> Result<()> {
    ensure!(
        source.is_dir(),
        "missing package input: {}",
        source.display()
    );
    for entry in walkdir::WalkDir::new(source).sort_by_file_name() {
        let entry = entry?;
        let path = entry.path();
        let relative = path.strip_prefix(source)?;
        if entry.file_type().is_dir() {
            continue;
        }
        ensure!(
            entry.file_type().is_file() || entry.file_type().is_symlink(),
            "non-regular package input: {}",
            path.display()
        );
        if entry.file_type().is_symlink() {
            ensure!(
                path.canonicalize()?.starts_with(source.canonicalize()?),
                "package link escapes component: {}",
                path.display()
            );
        }
        ensure!(
            !matches!(
                path.extension().and_then(|x| x.to_str()),
                Some("py" | "pyc" | "pyd")
            ),
            "interpreter payload in native package input: {}",
            path.display()
        );
        let target = destination.join(relative);
        write(&target, fs::read(path)?)?;
        fs::set_permissions(target, fs::metadata(path)?.permissions())?;
    }
    Ok(())
}
fn inventory(root: &Path) -> Result<BTreeMap<String, String>> {
    let mut result = BTreeMap::new();
    for entry in walkdir::WalkDir::new(root).sort_by_file_name() {
        let entry = entry?;
        if entry.file_type().is_file() {
            let name = s(entry.path().strip_prefix(root)?).replace('\\', "/");
            if name != "MANIFEST.sha256" {
                result.insert(name, hash(entry.path())?);
            }
        }
    }
    Ok(result)
}
fn manifest(root: &Path) -> Result<()> {
    write(
        root.join("MANIFEST.sha256"),
        inventory(root)?
            .iter()
            .map(|(name, hash)| format!("{hash}  {name}\n"))
            .collect::<String>(),
    )
}
fn archive(root: &Path, output: &Path, root_name: &str) -> Result<()> {
    ensure!(
        !output.exists(),
        "package output already exists: {}",
        output.display()
    );
    fs::create_dir_all(output.parent().context("missing output directory")?)?;
    manifest(root)?;
    let mut pending = tempfile::NamedTempFile::new_in(output.parent().unwrap())?;
    {
        let mut zip = ZipWriter::new(pending.as_file_mut());
        for entry in walkdir::WalkDir::new(root).sort_by_file_name() {
            let entry = entry?;
            if !entry.file_type().is_file() {
                continue;
            }
            let name = format!(
                "{root_name}/{}",
                s(entry.path().strip_prefix(root)?).replace('\\', "/")
            );
            safe_relative(&name)?;
            let mode = if entry.path().extension().is_some_and(|e| e == "exe")
                || entry.path().file_name().is_some_and(|n| n == "stcxx")
            {
                0o755
            } else {
                0o644
            };
            #[cfg(unix)]
            let mode = {
                use std::os::unix::fs::PermissionsExt;
                if entry.metadata()?.permissions().mode() & 0o111 != 0 {
                    0o755
                } else {
                    mode
                }
            };
            zip.start_file(
                name,
                SimpleFileOptions::default()
                    .compression_method(CompressionMethod::Deflated)
                    .compression_level(Some(6))
                    .unix_permissions(mode),
            )?;
            let mut input = fs::File::open(entry.path())?;
            std::io::copy(&mut input, &mut zip)?;
        }
        zip.finish()?;
    }
    pending.persist_noclobber(output).map_err(|e| e.error)?;
    write_json(
        suffix(output, ".json"),
        &json!({"archiveFileName":output.file_name().unwrap().to_string_lossy(),"archiveRoot":root_name,"size":fs::metadata(output)?.len(),"sha256":hash(output)?,"manifest_sha256":hash(root.join("MANIFEST.sha256"))?,"driver_version":env!("CARGO_PKG_VERSION"),"runtime_interpreters":[]}),
    )?;
    Ok(())
}
pub fn platform(source: &Path, output: &Path) -> Result<()> {
    let temp = tempfile::tempdir()?;
    let stage = temp.path().join("platform");
    fs::create_dir_all(&stage)?;
    for name in [
        "boards.txt",
        "platform.txt",
        "README.md",
        "COMPATIBILITY.md",
        "RELEASE_NOTES.md",
        "SOURCES.md",
        "VALIDATION.md",
        "LICENSE",
    ] {
        write(stage.join(name), fs::read(source.join(name))?)?;
    }
    for name in ["cores", "variants", "libraries", "examples", "LICENSES"] {
        if source.join(name).is_dir() {
            copy_tree(&source.join(name), &stage.join(name))?;
        }
    }
    {
        let name = "devices.json";
        write(
            stage.join("tools/variants").join(name),
            fs::read(source.join("tools/variants").join(name))?,
        )?;
    }
    let driver = stage.join("tools/stcxx-driver");
    fs::create_dir_all(&driver)?;
    let current = std::env::current_exe()?;
    write(
        driver.join(current.file_name().unwrap()),
        fs::read(current)?,
    )?;
    for entry in fs::read_dir(source.join("tools/stcxx-driver"))? {
        let p = entry?.path();
        if p.file_name()
            .is_some_and(|n| n.to_string_lossy().starts_with("toolchain-lock."))
            && p.extension().is_some_and(|e| e == "json")
        {
            write(driver.join(p.file_name().unwrap()), fs::read(&p)?)?;
        }
    }
    write(
        driver.join("README.md"),
        fs::read(source.join("tools/stcxx-driver/README.md"))?,
    )?;
    copy_tree(
        &source.join("tools/stcxx-driver/LICENSES"),
        &driver.join("LICENSES"),
    )?;
    // Include the other host's binary when supplied by its native build job.
    let other = if cfg!(windows) { "stcxx" } else { "stcxx.exe" };
    let p = source.join("tools/stcxx-driver").join(other);
    if p.is_file() {
        write(driver.join(other), fs::read(p)?)?;
    }
    let version = text(source.join("platform.txt"))?
        .lines()
        .find_map(|l| l.strip_prefix("version="))
        .context("missing platform version")?
        .to_owned();
    archive(&stage, output, &format!("arduino-mcs51-{version}"))
}
pub fn run(args: &[String]) -> Result<()> {
    ensure!(args.len() == 3 && args[0] == "package-platform", "expected package-platform <source> <output.zip>");
    platform(&absolute(&args[1])?, &absolute(&args[2])?)
}
