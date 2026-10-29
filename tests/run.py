"""Real facade policy and native Zephyr cbprintf; output/lock transport is mocked."""

from pathlib import Path
import os
import subprocess
import tempfile


r = Path(__file__).resolve().parents[1]
z = Path(os.environ["ZEPHYR_BASE"])

sdk = subprocess.check_output(
    ["xcrun", "--sdk", "macosx", "--show-sdk-path"],
    env={**os.environ, "DEVELOPER_DIR": "/Applications/Xcode.app/Contents/Developer"},
    text=True,
).strip()

with tempfile.TemporaryDirectory(prefix="rt-log-tests-") as d:
    for enabled, maximum, dynamic in [(1, 5, 1), (1, 2, 1), (1, 5, 0), (0, 0, 0)]:
        flags = [
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-isysroot", sdk,
            "-g",
            "-fsanitize=address,undefined",
            "-DCONFIG_ARCH_POSIX=1",
            "-DCONFIG_64BIT=1",
            "-DCONFIG_CBPRINTF_COMPLETE=1",
            "-DCONFIG_CBPRINTF_FULL_INTEGRAL=1",
            "-DCONFIG_CBPRINTF_FP_SUPPORT=1",
            f"-DCONFIG_RT_LOG={enabled}",
            f"-DCONFIG_RT_LOG_DYNAMIC_LEVEL_CONTROL={dynamic}",
            f"-DCONFIG_RT_LOG_MAX_LEVEL={maximum}",
            "-DCONFIG_RT_LOG_DEFAULT_LEVEL=4",
            "-DCONFIG_RT_LOG_TAG_SLOTS=4",
            "-DCONFIG_RT_LOG_TAG_MAX_LEN=15",
            *[f"-I{x}" for x in [r / "tests/host", r / "include", z / "include"]],
        ]

        exe = Path(d) / f"test-{enabled}-{maximum}"
        sources = [
            z / "lib/os/cbprintf_complete.c",
            r / "tests/test_log.c",
            r / "tests/test_local_level.c",
        ]
        if enabled:
            sources.append(r / "src/log.c")

        subprocess.run(
            ["/usr/bin/clang", *flags, *map(str, sources), "-o", str(exe)],
            check=True,
        )
        subprocess.run([str(exe)], check=True)
