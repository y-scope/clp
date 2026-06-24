#!/usr/bin/env bash
# Computes the maximum number of parallel C++ compilation jobs based on CPU count
# and available memory. Each job gets at least MIN_MEMORY_PER_JOB_GB GB of memory.
# The result is floored at 1.
#
# Usage: source this script, then use $compute_cpp_max_parallelism_result
#   OR:   call compute_cpp_max_parallelism directly
#
# Output: prints the computed job count to stdout (no trailing newline)
#
# Detection order for memory limits:
#   1. cgroup v2: /sys/fs/cgroup/memory.max (containers)
#   2. cgroup v1: /sys/fs/cgroup/memory/memory.limit_in_bytes (containers)
#   3. /proc/meminfo MemTotal (Linux hosts)
# If none are available, falls back to the CPU count with no memory cap.

MIN_MEMORY_PER_JOB_GB=2
# 2 GB in KB
MIN_MEMORY_PER_JOB_KB=$((MIN_MEMORY_PER_JOB_GB * 1024 * 1024))

compute_cpp_max_parallelism() {
    local cpus
    cpus=$(nproc 2>/dev/null || grep -c ^processor /proc/cpuinfo 2>/dev/null || echo 1)

    local mem_limit_kb=""
    local mem_source=""

    # 1. Try cgroup v2 (containers)
    if [ -f /sys/fs/cgroup/memory.max ]; then
        local cgroup_max
        cgroup_max=$(cat /sys/fs/cgroup/memory.max 2>/dev/null || true)
        # "max" means unlimited
        if [ -n "$cgroup_max" ] && [ "$cgroup_max" != "max" ]; then
            # Convert bytes to KB
            mem_limit_kb=$((cgroup_max / 1024))
            mem_source="cgroup-v2"
        fi
    fi

    # 2. Try cgroup v1 (containers)
    if [ -z "$mem_limit_kb" ] && [ -f /sys/fs/cgroup/memory/memory.limit_in_bytes ]; then
        local cgroup_limit
        cgroup_limit=$(cat /sys/fs/cgroup/memory/memory.limit_in_bytes 2>/dev/null || true)
        # Some systems set a very large limit (e.g., 9223372036854771712) for "unlimited"
        if [ -n "$cgroup_limit" ] && [ "$cgroup_limit" -lt 9223372036854771712 ] 2>/dev/null; then
            mem_limit_kb=$((cgroup_limit / 1024))
            mem_source="cgroup-v1"
        fi
    fi

    # 3. Try /proc/meminfo (Linux hosts)
    if [ -z "$mem_limit_kb" ]; then
        local total_kb
        total_kb=$(awk '/MemTotal/ {print $2}' /proc/meminfo 2>/dev/null || true)
        if [ -n "$total_kb" ]; then
            mem_limit_kb=$total_kb
            mem_source="meminfo"
        fi
    fi

    # Compute the job cap
    local mem_jobs=$cpus
    if [ -n "$mem_limit_kb" ]; then
        mem_jobs=$((mem_limit_kb / MIN_MEMORY_PER_JOB_KB))
    fi

    # Floor at 1
    if [ "$mem_jobs" -lt 1 ]; then
        mem_jobs=1
    fi

    if [ "$mem_jobs" -lt "$cpus" ]; then
        echo "$mem_jobs"
    else
        echo "$cpus"
    fi
}

# For convenience when sourcing: set the result variable
compute_cpp_max_parallelism_result=$(compute_cpp_max_parallelism)
