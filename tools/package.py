"""Create the release ZIP from the source tree and PlatformIO build metadata."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unicodedata
import zipfile

ROOT = Path(__file__).resolve().parents[1]
VERSION = "1.0.0"
NAME = f"Cardputer-Voice-{VERSION}"
TARGET = "cardputer_adv"
SONGS = ("さくらさくら", "君が代", "ふるさと")


def run(*command):
    subprocess.run(command, cwd=ROOT, check=True)


def universal(source, output, work):
    # Swift requires the .swift extension when compiling a Finder launcher.
    unit = work / "main.swift"
    unit.write_bytes(source.read_bytes())
    slices = []
    for arch in ("arm64", "x86_64"):
        thin = work / arch
        run("swiftc", "-O", "-target", arch + "-apple-macosx13.0",
            "-file-prefix-map", f"{work}=.", "-module-name", "CardputerVoice", unit, "-o", thin)
        slices.append(thin)
    run("lipo", "-create", *slices, "-output", output)
    run("codesign", "--force", "--sign", "-", output)
    archs = subprocess.check_output(["lipo", "-archs", output], text=True).split()
    if set(archs) != {"arm64", "x86_64"}:
        raise ValueError(f"Unexpected macOS architectures: {archs}")
    run("codesign", "--verify", "--all-architectures", output)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def filename(path):
    return unicodedata.normalize("NFC", path.as_posix())


def bundle(stage, work):
    source = stage / "source"
    for directory, pattern in (("src", "*"), ("tests", "*.cpp"), ("tools", "*.py"),
                               ("licenses", "*"), ("mac", "*.command"), ("mac", "*.mid"),
                               ("mac", "*.swift"), ("mac", "*.md")):
        dest = source / directory
        dest.mkdir(parents=True, exist_ok=True)
        for path in (ROOT / directory).glob(pattern):
            if path.is_file():
                shutil.copy2(path, dest / path.name)
    for name in ("README.md", "THIRD_PARTY.md", "platformio.ini", "Makefile", ".gitignore"):
        shutil.copy2(ROOT / name, source / name)
    for name in ("README.md", "THIRD_PARTY.md"):
        shutil.copy2(ROOT / name, stage / name)

    metadata = json.loads(subprocess.check_output(["pio", "project", "metadata", "--json-output"],
                                                  cwd=ROOT))[TARGET]
    extra = metadata["extra"]
    images = {int(i["offset"], 0): Path(i["path"]) for i in extra["flash_images"]}
    images[int(extra["application_offset"], 0)] = Path(metadata["prog_path"]).with_suffix(".bin")
    expected = {0: "bootloader.bin", 0x8000: "partitions.bin", 0xe000: "boot_app0.bin",
                0x10000: "firmware.bin"}
    if {addr: path.name for addr, path in images.items()} != expected:
        raise ValueError("Unexpected flash layout")
    firmware = stage / "firmware"
    firmware.mkdir()
    for address, path in images.items():
        shutil.copyfile(path, firmware / expected[address])
    merged = firmware / "Cardputer-Voice-Adv.bin"
    pairs = [v for offset, path in sorted(images.items()) for v in (hex(offset), path)]
    run(sys.executable, "-m", "esptool", "--chip", "esp32s3", "merge_bin", "-o", merged, *pairs)
    data = merged.read_bytes()
    for offset, path in images.items():
        segment = path.read_bytes()
        if data[offset:offset + len(segment)] != segment:
            raise ValueError(f"Merged flash image mismatch at {offset:#x}")
    framework = images[0xe000].parents[2]
    # Supply corresponding Arduino core and linked library sources for rebuilding.
    for part in ("cores", "libraries/SPI", "libraries/Wire", "variants/esp32s3"):
        shutil.copytree(framework / part, stage / "third-party" / "arduino-esp32" / part)

    mac = stage / "mac"
    mac.mkdir()
    for path in (source / "mac").iterdir():
        if path.suffix in (".mid", ".md") or path.name == "歌詞を送る.command":
            shutil.copy2(path, mac / path.name)
    universal(source / "mac/VoiceMIDI.swift", mac / "voice-midi", work)
    run(mac / "voice-midi", "--test")
    for i, song in enumerate(SONGS, 1):
        launcher = song + "を歌う.command"
        binary = f"play-score-{i}"
        universal(source / "mac" / launcher, mac / binary, work)
        wrapper = mac / launcher
        wrapper.write_text('#!/bin/zsh\nexec "${0:A:h}/' + binary + '" "$@"\n', encoding="utf-8")
        wrapper.chmod(0o755)
        run(wrapper, "--check")


def main():
    dist = ROOT / "dist"
    dist.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=dist) as folder:
        work = Path(folder)
        stage = work / NAME
        stage.mkdir()
        bundle(stage, work)
        files = sorted(p for p in stage.rglob("*") if p.is_file())
        manifest = stage / "SHA256SUMS"
        manifest.write_text("".join(f"{digest(p)}  {filename(p.relative_to(stage))}\n" for p in files),
                            encoding="utf-8")
        output = work / (NAME + ".zip")
        with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as archive:
            for path in [*files, manifest]:
                archive.write(path, filename(path.relative_to(work)))
        checksum = work / (output.name + ".sha256")
        checksum.write_text(f"{digest(output)}  {output.name}\n", encoding="ascii")
        os.replace(output, dist / output.name)
        os.replace(checksum, dist / checksum.name)
        print(dist / output.name)


if __name__ == "__main__":
    main()
