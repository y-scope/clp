"""
Profiling utilities for CLP query execution performance analysis.

This module provides lightweight profiling decorators using pyinstrument.

Profile outputs include:
- HTML files with interactive flame graphs and call trees
- Text summaries showing call hierarchy and timing
"""

import datetime
import functools
import inspect
import logging
import os
from pathlib import Path
from typing import Any, Callable, Optional, Tuple, TypeVar

from pyinstrument import Profiler

logger = logging.getLogger(__name__)

F = TypeVar("F", bound=Callable[..., Any])

# Sampling interval tuned for low-overhead profiling (1ms)
PROFILING_INTERVAL = 0.001

__all__ = ["profile"]


def is_profiling_enabled() -> bool:
    """
    Check if profiling is enabled.
    TODO: Add `CLPConfig` mechanism to enable/disable profiling for each component.

    :return: False
    """
    return False


def get_profiling_output_dir() -> Path:
    """
    Get the directory for profile output files. Creates directory if needed.

    :return: Path object pointing to the profiles directory.
    :raises RuntimeError: If CLP_LOGS_DIR environment variable is not set.
    """
    custom_dir = os.getenv("CLP_PROFILING_OUTPUT_DIR")
    if custom_dir:
        return Path(custom_dir)

    clp_logs_dir_str = os.getenv("CLP_LOGS_DIR")
    if clp_logs_dir_str is None:
        raise RuntimeError(
            "CLP_LOGS_DIR environment variable must be set for profiling. "
            "This should be set automatically by CLP's startup scripts."
        )

    profiles_dir = Path(clp_logs_dir_str) / "profiles"
    profiles_dir.mkdir(parents=True, exist_ok=True)
    return profiles_dir


def _extract_context_from_args(
    func: Callable,
    args: tuple,
    kwargs: dict,
    job_id_param: Optional[str] = None,
    task_id_param: Optional[str] = None,
) -> Tuple[str, str]:
    """
    Extract job_id and task_id from function arguments.

    :param func: The function being profiled
    :param args: Positional arguments passed to the function
    :param kwargs: Keyword arguments passed to the function
    :param job_id_param: Name/path of the parameter containing job_id (default: "job_id").
    :param task_id_param: Name/path of the parameter containing task_id (default: "task_id").
    :return: Tuple of (job_id, task_id) as strings. Empty strings if not found.
    """
    job_id = ""
    task_id = ""

    try:
        # Get function signature
        sig = inspect.signature(func)
        param_names = list(sig.parameters.keys())

        def extract_value(param_spec: str) -> str:
            """Extract value from parameter, supporting dot notation for attributes."""
            if not param_spec:
                return ""

            # Split on '.' to handle attribute access
            parts = param_spec.split(".")
            param_name = parts[0]

            # Find the parameter value
            value = None
            if param_name in kwargs:
                value = kwargs[param_name]
            elif param_name in param_names:
                idx = param_names.index(param_name)
                if idx < len(args):
                    value = args[idx]

            if value is None:
                return ""

            # Navigate through attributes if dot notation was used
            for attr_name in parts[1:]:
                if hasattr(value, attr_name):
                    value = getattr(value, attr_name)
                else:
                    return ""

            return str(value)

        # Extract job_id
        job_id_key = job_id_param or "job_id"
        job_id = extract_value(job_id_key)

        # Extract task_id
        task_id_key = task_id_param or "task_id"
        task_id = extract_value(task_id_key)

    except Exception as e:
        # Don't let introspection failures break profiling
        logger.debug(f"Failed to extract context from {func.__name__}: {e}")

    return job_id, task_id


def _save_profile(
    profiler: Profiler, section_name: str, job_id: str = "", task_id: str = ""
) -> None:
    """
    Save profiler output to HTML and text formats. Generates .html and .txt files.

    :param profiler: The pyinstrument Profiler object containing profiling data
    :param section_name: Name identifying this profiling section
    :param job_id: Optional job identifier for filename
    :param task_id: Optional task identifier for filename
    """
    try:
        # Get the session for logging
        session = profiler.last_session
        if not session:
            logger.debug(f"No profiling session for {section_name}")
            return

        duration = session.duration
        sample_count = session.sample_count

        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S_%f")
        filename_parts = [section_name]

        if job_id:
            filename_parts.append(f"job{job_id}")
        if task_id:
            filename_parts.append(f"task{task_id}")

        filename_parts.append(timestamp)
        base_filename = "_".join(filename_parts)

        output_dir = get_profiling_output_dir()

        # Save HTML with interactive visualization (flame graph)
        html_path = output_dir / f"{base_filename}.html"
        with open(html_path, "w", encoding="utf-8") as f:
            f.write(profiler.output_html())

        # Save human-readable text summary with call hierarchy
        txt_path = output_dir / f"{base_filename}.txt"
        with open(txt_path, "w", encoding="utf-8") as f:
            # Header
            f.write("=" * 80 + "\n")
            f.write(f"CLP Query Profiling Report (pyinstrument)\n")
            f.write(f"Section: {section_name}\n")
            if job_id:
                f.write(f"Job ID: {job_id}\n")
            if task_id:
                f.write(f"Task ID: {task_id}\n")
            f.write(f"Timestamp: {timestamp}\n")
            f.write("=" * 80 + "\n\n")
            f.write(profiler.output_text(unicode=True, color=False))

        logger.info(
            f"Profile saved: {section_name} "
            f"(duration={duration:.6f}s, samples={sample_count}) "
            f"HTML={html_path}, TXT={txt_path}"
        )

    except Exception as e:
        logger.error(f"Failed to save profile for {section_name}: {e}", exc_info=True)


def profile(
    section_name: Optional[str] = None,
    job_id_param: Optional[str] = None,
    task_id_param: Optional[str] = None,
) -> Callable[[F], F]:
    """
    Decorator for profiling function execution with automatic context extraction.

    Output files are written to $CLP_LOGS_DIR/profiles/ (e.g., clp-package/var/log/query_worker/
    profiles/).

    :param section_name: Override for profile section name. If None, uses function name.
    :param job_id_param: Parameter name to extract job_id from (default: "job_id").
                         Can use dot notation for attributes, e.g., "job.id"
    :param task_id_param: Parameter name to extract task_id from (default: "task_id").
                          Can use dot notation for attributes, e.g., "task.id"
    :return: Decorated function with profiling capabilities
    """

    def decorator(func: F) -> F:
        name = section_name or func.__name__
        is_async = inspect.iscoroutinefunction(func)

        if is_async:

            @functools.wraps(func)
            async def async_wrapper(*args, **kwargs):
                # Fast path: profiling disabled
                if not is_profiling_enabled():
                    return await func(*args, **kwargs)

                # Profiling enabled: extract context and profile execution
                job_id, task_id = _extract_context_from_args(
                    func, args, kwargs, job_id_param, task_id_param
                )

                profiler = Profiler(interval=PROFILING_INTERVAL)
                try:
                    profiler.start()
                except RuntimeError as e:
                    # Another profiler is already running (nested profiling)
                    # Skip profiling this function to avoid conflicts
                    if "already a profiler running" in str(e):
                        logger.debug(
                            f"Skipping nested profiling for {name} "
                            f"(parent profiler already active)"
                        )
                        return await func(*args, **kwargs)
                    raise

                try:
                    result = await func(*args, **kwargs)
                    return result
                finally:
                    profiler.stop()
                    _save_profile(profiler, name, job_id, task_id)

            return async_wrapper  # type: ignore

        else:

            @functools.wraps(func)
            def sync_wrapper(*args, **kwargs):
                # Fast path: profiling disabled (single boolean check)
                if not is_profiling_enabled():
                    return func(*args, **kwargs)

                # Profiling enabled: extract context and profile execution
                job_id, task_id = _extract_context_from_args(
                    func, args, kwargs, job_id_param, task_id_param
                )

                profiler = Profiler(interval=PROFILING_INTERVAL)
                try:
                    profiler.start()
                except RuntimeError as e:
                    # Another profiler is already running (nested profiling)
                    # Skip profiling this function to avoid conflicts
                    if "already a profiler running" in str(e):
                        logger.debug(
                            f"Skipping nested profiling for {name} "
                            f"(parent profiler already active)"
                        )
                        return func(*args, **kwargs)
                    raise

                try:
                    result = func(*args, **kwargs)
                    return result
                finally:
                    profiler.stop()
                    _save_profile(profiler, name, job_id, task_id)

            return sync_wrapper  # type: ignore

    return decorator
