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
    if concurrency < 1:
        print("Error: Concurrency must be at least 1.")
        exit(1)
    storage_url = args.storage_url
    host = args.host

    clp_home = os.getenv("CLP_HOME", "/opt/clp")
    spider_worker_path = pathlib.Path(clp_home) / "bin" / "spider_worker"
    if not spider_worker_path.exists():
        print(f"Error: spider_worker not found at {spider_worker_path}")
        exit(1)

    # Start multiple spider workers
    processes = []
    try:
        for _ in range(concurrency):
            process = subprocess.Popen(
                [spider_worker_path, "--storage_url", storage_url, "--host", host]
            )
            processes.append(process)
    except OSError as e:
        print(f"Failed to start spider worker: {e}")
        for process in processes:
            process.terminate()
        exit(1)

    failed = False
    for process in processes:
        exit_code = process.wait()
        if exit_code != 0:
            print(f"Spider worker exited with code {exit_code}")
            failed = True
    if failed:
        exit(1)


if __name__ == "__main__":
    main()
