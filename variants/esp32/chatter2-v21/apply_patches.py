Import("env")

from pathlib import Path
import shutil

project_dir = Path(env["PROJECT_DIR"])
pioenv = env["PIOENV"]
libdeps_dir = Path(env.subst("$PROJECT_LIBDEPS_DIR"))

source_file = (
    project_dir
    / "variants"
    / "esp32"
    / "chatter2-v21"
    / "patches"
    / "ble_store_nvs.c"
)

target_file = (
    libdeps_dir
    / pioenv
    / "NimBLE-Arduino"
    / "src"
    / "nimble"
    / "nimble"
    / "host"
    / "store"
    / "config"
    / "src"
    / "ble_store_nvs.c"
)

if not source_file.exists():
    raise RuntimeError(
        "Chatter V2.1 BLE patch source is missing: "
        + str(source_file)
    )

if not target_file.exists():
    raise RuntimeError(
        "NimBLE target file is missing: "
        + str(target_file)
    )

if source_file.read_bytes() != target_file.read_bytes():
    print("Applying Chatter V2.1 NimBLE bond/NVS compatibility patch")
    shutil.copy2(source_file, target_file)
else:
    print("Chatter V2.1 NimBLE bond/NVS patch already applied")