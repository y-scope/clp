import argparse
import os
import pathlib
import subprocess


def parse_args() -> argparse.Namespace:
    """
    Parses command line arguments.
    :return: The parsed arguments.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--concurrency", type=int, default=1, help="Number of concurrent spider workers."
    )
    parser.add_argument("--storage-url", type=str, required=True, help="Spider storage URL.")
    parser.add_argument("--host", type=str, required=True, help="Worker host address.")
    return parser.parse_args()


def main() -> None:
    """Main function to start multiple spider workers."""
    # Parse arguments
    args = parse_args()
    concurrency = args.concurrency
    storage_url = args.storage_url
    host = args.host

    clp_home = os.getenv("CLP_HOME", "/opt/clp")
    spider_worker_path = pathlib.Path(clp_home) / "bin" / "spider_worker"

    # Start multiple spider workers
    processes = []
    for _ in range(concurrency):
        process = subprocess.Popen(
            [spider_worker_path, "--storage_url", storage_url, "--host", host]
        )
        processes.append(process)

    for process in processes:
        exit_code = process.wait()
        print(f"Spider worker exited with code {exit_code}")


if __name__ == "__main__":
    main()
