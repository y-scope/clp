import re
import pathlib
import logging


def get_profiled_memory_usage(profiler_output_file: str, logger: logging.Logger) -> int:
    peak_memory_in_kbytes: int = 0
    if not pathlib.Path(profiler_output_file).is_file():
        logger.error(f"Failed to locate {profiler_output_file}, report 0 memory usage")
    pattern = r"maxrss:\s*(\d+)"
    found_match = False
    with open("profiling.out", "r") as f:
        for line in f:
            match = re.search(pattern, line.strip())
            if match:
                peak_memory_in_kbytes = int(match.group(1))
                found_match = True
                break
    if not found_match:
        logger.error(f"Failed to find memory usage, report 0 memory usage")
    return peak_memory_in_kbytes
