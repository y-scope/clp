# Search Coordinator (Rust) — Design

Porting the Python **query scheduler**
(`components/job-orchestration/job_orchestration/scheduler/query/query_scheduler.py`)
to a Rust **search coordinator** modeled on `components/compression-coordinator`.

Scope: the full orchestration lifecycle — concurrency/pool, poll loop, job
retirement/updates, sleep/wake cadence, query-table row reading, job
categorization, cancellation, aggregation (timeline + other), and
decompression. The current celery **reducer subsystem is deleted entirely** and
must not be ported (see §7).

---

## 1. Background

### 1.1 `QUERY_JOBS_TABLE_NAME` is a message bus, not just a table

The scheduler is fully decoupled from job producers. There is no RPC between
clients and the scheduler — everything flows through the `QUERY_JOBS_TABLE_NAME` table:

- **Submit a job** = `INSERT INTO QUERY_JOBS_TABLE_NAME (job_config, type) VALUES (?, ?)` with `status` defaulting to `PENDING`.
- **Cancel a job** = `UPDATE QUERY_JOBS_TABLE_NAME SET status=CANCELLING WHERE id=? AND status IN (PENDING, RUNNING)`.
- **Run a job** = the scheduler polls `status=PENDING`, dispatches, retires.
- **Act on a cancel** = the scheduler polls `status=CANCELLING`, aborts.

Because the protocol is the DB, any component with write access can submit or
cancel, in any language, with no scheduler-side coordination. This is why the
scheduler (and the new coordinator) is poll-driven, and why cancellation is a
scan rather than a push.

### 1.2 Job types and lifecycle

- `QueryJobType`: `SEARCH_OR_AGGREGATION`, `EXTRACT_IR`, `EXTRACT_JSON` (defined in `job_orchestration/scheduler/constants.py`).
- `QueryJobStatus`: `PENDING → RUNNING → SUCCEEDED | FAILED | CANCELLING → CANCELLED | KILLED`.

```
              dispatch             finish OK
  PENDING ───────────────► RUNNING ──────────────► SUCCEEDED
     │  │                     │
     │  │ bad config /        │ finish w/ errors
     │  │ dispatch failure    │
     │  └─────────────────────┴──────────────────► FAILED
     │
     │  producer cancel           scheduler acts on the cancel
     └───────────────► CANCELLING ───────────────► CANCELLED
        (from PENDING
         or RUNNING)

  PENDING | RUNNING ─────────────────────────────► KILLED
              startup kill_hanging_jobs
              (previous scheduler died mid-job)
```

Edge notes: `CANCELLING` is written by producers (api-server/webui `UPDATE ...
WHERE status IN (PENDING, RUNNING)` — §2.2), so it can be entered from either
non-terminal state; every other transition is scheduler-written. Terminal states:
`SUCCEEDED`, `FAILED`, `CANCELLED`, `KILLED`.

- `QUERY_JOBS_TABLE_NAME` columns: `id, type, status, creation_time, num_tasks, num_tasks_completed, start_time, duration, job_config` (**msgpack** MEDIUMBLOB).
- `QUERY_TASKS_TABLE_NAME` columns: `id, status, creation_time, start_time, duration, job_id, archive_id`. One row per archive searched.
- The `prev_status` guard on status updates avoids clobbering concurrent transitions (e.g. cancel-during-finish).

### 1.3 Three execution paths share one table

`SEARCH_OR_AGGREGATION` is one `type` but splits at the config level:
`SearchJobConfig.aggregation_config` is `None` (plain search) vs set (timeline
or other aggregation). `EXTRACT_IR`/`EXTRACT_JSON` are the decompression paths.
The scheduler routes rows by `type` and then by config content.

---

## 2. Current state

### 2.1 Search query sources (who submits query jobs)

"Aggregation" below means a `SEARCH_OR_AGGREGATION` job with `aggregation_config`
set (routes through the reducer today). "Search" means the same `type` with
`aggregation_config = None` (plain search).

| Source | Language | Submits | Mechanism |
|---|---|---|---|
| **webui server** | TS/Fastify | search **and** aggregation, as **two separate job rows** (`searchJobId` + `aggregationJobId`) | `QueryJobDbManager.submitJob` → `INSERT INTO QUERY_JOBS_TABLE_NAME (job_config, type)` (msgpack via `@msgpack/msgpack`) |
| **api-server** | Rust | **search only** (`QueryConfig` has no `aggregation_config` field) | `client.submit_query` → same INSERT (sqlx) |
| **CLI `search.py`** | Python | search, **optionally with aggregation** (`aggregation_config` set when `--count` / `--count-by-time` passed) — **one job row** | shared `submit_query_job` → same INSERT |
| **CLI `decompress.py`** | Python | decompression only (`EXTRACT_IR` / `EXTRACT_JSON`) — not search | same shared `submit_query_job` |
| **mcp-server** | Python | **search only** | `clp_connector.py` → same INSERT |

All producers write the same row shape; the scheduler does not distinguish them.
Note the two different aggregation styles: the **webui splits** search and
aggregation into two rows (separate result streams for UX — see §2.2), while the
**CLI folds** aggregation into a single row via `aggregation_config`. Either way
the aggregation job goes through the reducer today. The CLI and mcp-server are
submit-only (they wait synchronously / observe status); only the two HTTP
services also cancel.

### 2.2 Cancellation sources (who issues cancellations)

Cancellation is a DB write of `status=CANCELLING`; the scheduler polls it via
`fetch_cancelling_search_jobs` (filtered to `type=SEARCH_OR_AGGREGATION`, since
the cancel machinery — `cancel_job_except_reducer`, GroupResult revoke, reducer
release — is search-specific and not wired for `EXTRACT_*`).

| Source | Route | How the job id is conveyed | What it cancels |
|---|---|---|---|
| **api-server** | `POST /query/{search_job_id}` | id in the **URL path** | one search job → `client.cancel_search_job(id)` → `UPDATE ... SET status=CANCELLING WHERE id=? AND status IN (PENDING, RUNNING)` |
| **webui server** | `POST /api/search/cancel` | ids in the **JSON body** `{searchJobId, aggregationJobId}` | **both** the search job and the (reducer-era) aggregation job in one request → `QueryJobDbManager.cancelJob(id)` each → same UPDATE |

Both guard `status IN (PENDING, RUNNING)` — a terminal job cannot be cancelled.
The webui additionally flips the Mongo results-metadata `lastSignal` to
`RESP_DONE` so the client UI stops streaming.

**Note on the webui's `aggregationJobId`**: the current webui flow submits a
**separate** `QUERY_JOBS_TABLE_NAME` row for the aggregation (the reducer-era design).
With the reducer deleted (§7) and timeline aggregation done natively in clp-s
(§6), this separate aggregation-job row goes away — the webui cancellation
request will collapse to a single search-job id. That is a webui-side change to
track, not a coordinator concern.

### 2.3 Scheduler-side handling (Python, today)

- Poll loop: `handle_job_updates` calls `handle_cancelling_search_jobs` then `check_job_status_and_update_db` every `jobs_poll_delay`.
- Cancel: `cancel_job_except_reducer` (synchronous, atomic) revokes the celery `GroupResult`; `release_reducer_for_job` (async, called last) releases the reducer; then `QUERY_TASKS_TABLE_NAME` → `CANCELLED` and `QUERY_JOBS_TABLE_NAME` → `CANCELLED` (guarded by `prev_status=CANCELLING`).
- Retire: `check_job_status_and_update_db` polls each `RUNNING` job's rehydrated `GroupResult` and routes to `handle_finished_search_job` / `handle_finished_stream_extraction_job`.

### 2.4 clp-s search output handlers

**Scope:** everything in this subsection concerns the **clp-s binary's `search`
command** — its five output handlers (the *complete* set of user-facing output
destinations, registered at `CommandLineArguments.cpp:1023-1028` as the
`file`/`network`/`reducer`/`results-cache`/`stdout` subcommands) and, for each
§2.1 source, which of those handlers its submitted jobs can select. The five are
exhaustive for `clp-s search`; `VectorOutputHandler` and the `Aggregation*`
handlers are not CLI-selectable (aggregation is sub-selected within `reducer`).

Out of scope here: (a) **decompression** — `EXTRACT_*` jobs use the separate
`extract_stream` clp-s path, not a search output handler; and (b) **how a source
delivers results to its own client** — the webui reading Mongo, the api-server
streaming over HTTP, and `search.py` printing from its TCP server are
consumer-side mechanisms, not clp-s output handlers.

A clp-s search worker (one per archive) writes its matches through one of five
**output handlers**, selected by the search config. The handler names below are
the `cXxxOutputHandlerName` constants in
`components/core/src/clp_s/CommandLineArguments.cpp`; the classes are in
`components/core/src/clp_s/OutputHandlerImpl.hpp`.

| Handler | CLI name | Class | What it does | Used by |
|---|---|---|---|---|
| **stdout** | `stdout` | `StandardOutputHandler` | writes `archive_id: log_event_idx: timestamp message` to stdout | the `clp-s search` CLI default (terminal streaming); not used by the celery scheduler path |
| **file** | `file` | `FileOutputHandler` | writes results to a file path (`FileWriter::CreateForWriting`); the worker then optionally uploads to S3 | `search_config.write_to_file = true` (api-server `buffer_results_in_mongodb = false`) |
| **network** | `network` | `NetworkOutputHandler` | streams results over a TCP socket to `host:port` (`search_config.network_address`) | CLI `search.sh` / api-server live streaming to a client |
| **results-cache** | `results-cache` | `ResultsCacheOutputHandler` | writes results as documents into a MongoDB collection (`--uri`, `--collection <job_id>`, `--max-num-results`, optional `--dataset`) | the persistent results-cache path — webui reads collection `<job_id>`; scheduler's `found_max_num_latest_results` reads it for max-results short-circuit |
| **reducer** | `reducer` | `CountReducerOutputHandler` / `CountByTimeReducerOutputHandler` / `AggregationOutputHandler` | aggregation: streams **per-archive** aggregated results to the reducer process over a socket (`--host --port --job-id`). Sub-selected by `--count` (total count), `--count-by-time SIZE` (timeline buckets), or richer aggregation config | the aggregation jobs (webui `aggregationJobId`, CLI `--count`/`--count-by-time`) |

Selection logic in `fs_search_task` (the celery worker building the clp-s
command), in priority order:

1. `aggregation_config` is set → **reducer** (with `--count` / `--count-by-time`).
2. else `network_address` is set → **network**.
3. else `write_to_file` → **file**.
4. else → **results-cache**.

(`stdout` is the clp-s binary's own default when run from the CLI, not a path
the celery scheduler selects.)

### Which sources select which handler

Determined by which fields each source sets on `SearchJobConfig` (the selection
priority above). Verified from each source's config construction:

| Source | stdout | file | network | results-cache | reducer |
|---|---|---|---|---|---|
| CLI `search.py` | — ¹ | — ² | ✓ | ✓ | ✓ |
| webui server | — | — | — | ✓ (search job) | ✓ (aggregation job) |
| api-server | — | ✓ ³ | — | ✓ ³ | — |
| mcp-server | — | — | — | ✓ | — |
| CLI `decompress.py` | n/a (EXTRACT_* → `extract_stream`, not a search handler) |

Footnotes:

1. **CLI `search.py` stdout**: terminal output is produced via the **network** handler — clp-s streams results to a TCP server that `search.py` runs (`do_search_without_aggregation`), which prints to its own stdout. The clp-s `stdout` handler is only reached when the `clp-s search` binary is run directly from a terminal; no job-submitting source selects it.
2. **CLI `search.py` file**: `search.py` never sets `write_to_file` (its `--file-path` arg is an *input* file to search, not an output destination), so the `file` handler is never selected.
3. **api-server**: `file` vs `results-cache` is toggled by `buffer_results_in_mongodb` — `false` → `file`, `true` → `results-cache`.

Other notes:
- `network` is unique to the CLI `search.py` streaming path.
- `reducer` is reached only by sources that set `aggregation_config`: the webui (its aggregation job) and the CLI (`--count` / `--count-by-time`).
- `results-cache` is the common default and the only handler mcp-server ever uses.
- **S3 is a valid destination, not a separate handler.** The `file` handler's local output is uploaded to S3 when the worker's `stream_output.storage.type == S3` (and `write_to_file` and the task succeeded) — see `upload_results_to_s3` in `fs_search_task`, which writes to `{job_id}/{archive_id}`. So S3 is reachable via the `file` handler (e.g. api-server with `buffer_results_in_mongodb=false` on an S3-configured worker).

**Map-reduce note**: `reducer` is the **map** side — `CountByTimeReducerOutputHandler`
buckets matches per archive (`m_bucket_counts[bucket] += 1`) and flushes them to
the reducer over the socket. The reducer process (`CountOperator`) does the
**cross-archive sum** and writes the combined `{timestamp, count}` timeline to
Mongo. Deleting the reducer (§6) keeps the clp-s per-archive map; the
cross-archive reduce moves into the coordinator (MVP+2).

---

## 3. Phased roadmap

| Phase | Scope | Notes |
|---|---|---|
| **MVP** | Plain **search** jobs only. No aggregation, no cancellation, no decompression. | Get search dispatch + result retirement + DB updates working end-to-end through clp-s. |
| **MVP+1** | **Cancellation** | Background coroutine scanning `QUERY_JOBS_TABLE_NAME` for `status=CANCELLING`; per-job `CancellationToken` aborts the in-flight search. Consumes the same CANCELLING rows the api-server/webui already write — no coordination needed. |
| **MVP+2** | **Timeline aggregation** only | clp-s `--count-by-time` does per-archive bucketing (map); the coordinator does the cross-archive sum (reduce) the reducer used to do. No separate reducer process. |
| **MVP+3** | **Decompression** (EXTRACT_IR / EXTRACT_JSON) | Decide celery-handoff vs Spider submission; resolve resource-group sharing. |
| **MVP+N** | **Other aggregations** | Require new Spider functionality (not ready). Blocked on Spider. |

Guiding rule: the celery reducer (`reducer_connection_queue`, `acquire_reducer_for_job`, `ReducerHandlerMessage*`, the reducer TCP server, `release_reducer_for_job`, the `reducer` subcommand in `fs_search_task`) is **deleted, not ported**. Timeline aggregation goes through clp-s natively; other aggregations go through Spider later.

---

## 4. Translation table (old → new)

The port is a three-part project:

1. **search-coordinator** — polling, job categorization, and status-update logic. This is the focus of this section. A naive baseline exists on branch `search-coordinator/init` (`components/search-coordinator`).
2. **clp-tdl-package** — search task signatures, mirroring the existing compression tasks (out of scope for this doc).
3. **The bridge** — task-input construction and submission connecting 1 → 2 (out of scope for this doc).

### Part 1 baseline — branch `search-coordinator/init`

The branch is a structural port of the **compression-coordinator**, already
Spider-based (this settles the old "Celery vs Rust worker" question: tasks go to
Spider). What it already has:

| Area | On the branch | Ported from (compression-coordinator) |
|---|---|---|
| Poll loop | `SearchCoordinator::run`: `select!` on `CancellationToken`; `saturating_sub` sleep; deferred `mark_jobs_dispatched` | `Coordinator::run` |
| Two-phase fetch | `fetch_new_job_rows`: first fetch = PENDING + `dispatch_time IS NOT NULL` (re-dispatch, no LIMIT); subsequent = PENDING + `dispatch_time IS NULL` `LIMIT available_permits()` | same function, same queries |
| Concurrency | `Semaphore(max_concurrent_jobs)`; owned permit moved into each spawned handle | `schedule_new_jobs` |
| Recovery | `fetch_submitted_running_jobs` (RUNNING + `spider_id IS NOT NULL`) → `QueryJobHandle::recover` per job | same (replaces Python `kill_hanging_jobs`) |
| Handle lifecycle | `QueryJobHandle::{run, recover, submit_and_wait, to_completion, report_failure}` | `job_handle.rs` |
| Status updates | `persist_spider_job_id` (→ RUNNING + `start_time` + `num_tasks` + COALESCE `dispatch_time`), `update_job_status`, `mark_job_failed` | same |
| Submitter | `QueryJobSubmitter` trait; `run_query_job_to_completion` (idempotent start + exponential-backoff job-state polling) implemented; `submit_query_job` is `todo!()` (parts 2–3) | submitter trait pattern |
| Schema | `QUERY_JOBS_TABLE_NAME` gains `status_msg`, `update_time`, `spider_id`, `dispatch_time` + matching indices — aligned with `compression_jobs` | `compression_jobs` columns |
| Config | `SearchCoordinator` config: `max_concurrent_jobs`, `job_polling_interval_millisecs`, `result_polling` backoff, task retries/timeouts, `resource_group` | `CompressionCoordinator` config |
| ID/status types | `QueryJobId = i32`; `QueryJobStatus` as a typed `sqlx::Type` enum | `CompressionJobId` / `CompressionJobStatus` |

### Part 1 gaps — what the branch does not do yet

The branch treats every `QUERY_JOBS_TABLE_NAME` row identically; **job categorization is
entirely absent** — this is the main new work relative to the compression side,
because `compression_jobs` has no `type` column, so the compression-coordinator
never needed to categorize:

- `fetch_new_job_rows` projects only `id` — it never reads `type`, `job_config`, or `creation_time`.
- `QueryJobHandle::new` is a stub: no msgpack deserialization of `job_config`, no `SearchJobConfig` / `ExtractIrJobConfig` / `ExtractJsonJobConfig` variants, no `aggregation_config` branch. Contrast with compression's `S3CompressionJobHandle::new`, which deserializes `ClpIoConfig`, rejects non-S3 inputs with `Error::UnsupportedInputConfig`, and derives the clp-s options — the same shape search needs, plus the `type` dispatch in front.
- The `UnsupportedInputConfig` skip path is ported into `create_job_handle` (warn + leave the row for another handler) but nothing produces the error yet. It is the right hook for the phased rollout: aggregation (until MVP+2) and `EXTRACT_*` (until MVP+3) rows return it and stay with the legacy scheduler.
- `num_tasks` is a placeholder constant; no `QUERY_TASKS_TABLE_NAME` rows are written; `num_tasks_completed` and `duration` are never set.
- `update_job_status` has no previous-status CAS guard. Python guards with `set_job_or_task_status(prev_status=...)` (cancel-during-finish races), and compression's commit path CASes on `status = Running`; port the guard.
- No CANCELLING scan, no per-job cancellation wiring (MVP+1).

Part-1 work on top of the baseline, in order: widen the fetch projection
(`type`, `job_config`, `creation_time`); categorize in `QueryJobHandle::new`
(`type` → config variant → `aggregation_config` branch), returning
`UnsupportedInputConfig` for not-yet-supported categories; write `QUERY_TASKS_TABLE_NAME`
rows and a real `num_tasks`; add the CAS guard on status transitions; then the
MVP+1 cancellation scan.

### MVP — plain search end-to-end (detailed)

The core path for a `SEARCH_OR_AGGREGATION` job with `aggregation_config = None`.
This is what MVP must implement.

| Step | Old (Python query scheduler) | New (Rust search coordinator) |
|---|---|---|
| Discover | `fetch_new_query_jobs`: `SELECT ... FROM QUERY_JOBS_TABLE_NAME WHERE status=PENDING` (MySQL control plane) | unchanged — poll `PENDING` rows from MySQL |
| Read row | `job_config` MEDIUMBLOB, **plain msgpack**; `msgpack.unpackb` → `SearchJobConfig` (Pydantic) | serde + `rmp-serde` (**plain msgpack, not Brotli**) → `SearchJobConfig` |
| Categorize | by `type` + `aggregation_config is None` → plain search | same; MVP handles only this branch |
| Plan work | `get_archives_for_search` / `_get_archives_for_search_without_datasets` resolve target archives; retention lower-bound from `creation_time` + `archive_retention_period` | same query logic in sqlx |
| Dispatch | `insert_query_tasks_into_db` (one `QUERY_TASKS_TABLE_NAME` row per archive); `celery.group(search.s(...) per archive).apply_async()`; `GroupResult.save()` | insert task rows; submit the search task graph to **Spider** via `QueryJobSubmitter::submit_query_job` (parts 2–3; `todo!()` on the branch); **no `GroupResult` rehydration** — the handle stays in-process |
| Execute | clp-s `search` celery task per archive; output handler = `results-cache` → writes matches to Mongo collection `<job_id>` | **data plane unchanged**: clp-s workers still use the `results-cache` handler → Mongo collection `<job_id>` |
| Retire | `check_job_status_and_update_db` polls the rehydrated `GroupResult`; `handle_finished_search_job` updates `QUERY_JOBS_TABLE_NAME`/`QUERY_TASKS_TABLE_NAME` | per-job handle: `to_completion` polls Spider job state with exponential backoff (implemented on the branch); updates `QUERY_JOBS_TABLE_NAME` (SUCCEEDED/FAILED) + `QUERY_TASKS_TABLE_NAME`; permit released on handle exit |
| Max-results short-circuit | `found_max_num_latest_results` reads Mongo `<job_id>` (sort by `timestamp` desc, limit) | unchanged — read Mongo `<job_id>` |
| `job_id` type | `str` everywhere (it's the Mongo collection name) | typed integer `JobId` internally; stringify only at the Mongo boundary |
| Concurrency | `ProcessPoolExecutor(scheduler_concurrency)` for blocking dispatch | `Semaphore(max_concurrent_jobs)`; `available_permits()` bounds the fetch `LIMIT` (see concurrency table below) |

Key MVP invariant: the **Mongo results cache (data plane) is untouched** — clp-s
workers still write to collection `<job_id>`, the webui still reads it. Only the
**MySQL control-plane orchestration** (discover/dispatch/retire/update) moves
from Python to Rust.

### Aggregation — timeline (detailed)

`SEARCH_OR_AGGREGATION` with `aggregation_config` set. Timeline = count / count-by-time.
This is a **map-reduce**; the reduce moves into the coordinator (MVP+2).

| Aspect | Old (Python + reducer) | New (Rust coordinator) |
|---|---|---|
| Config | `AggregationConfig {job_id, reducer_host, reducer_port, do_count_aggregation, count_by_time_bucket_size}` | keep `job_id`, `do_count_aggregation`, `count_by_time_bucket_size`; **drop `reducer_host` / `reducer_port`** |
| Job state | `WAITING_FOR_REDUCER` + `acquire_reducer_for_job` pulls a reducer from `reducer_connection_queue` | **deleted** — no reducer acquisition, no `WAITING_FOR_REDUCER` state |
| Per-archive **map** | clp-s `reducer` subcommand: `CountByTimeReducerOutputHandler` buckets matches per archive (`m_bucket_counts[bucket] += 1`) and streams them to the reducer over a socket | clp-s still does the per-archive bucketing; but instead of streaming to a reducer socket, **returns per-archive bucket counts to the coordinator** (open: via task result vs. a shared store) |
| Cross-archive **reduce** | reducer process `CountOperator` sums buckets across all archives and writes `{timestamp, count}` documents to Mongo collection `<aggregationJobId>` | **the coordinator accumulates** per-archive bucket counts and writes the combined `{timestamp, count}` timeline to Mongo — no separate reducer process |
| clp-s handler selection | `aggregation_config` set → `reducer` handler (priority 1 in `fs_search_task`) | MVP+2: clp-s emits per-archive buckets (map); transport changes so results return to the coordinator instead of a reducer socket |
| Finish handshake | `handle_finished_search_job` reducer `SUCCESS`/`FAILURE` handshake | **deleted** — the coordinator already has the per-archive results |
| `is_reducer_job` gating | special-cased in finish/cancel paths | **deleted** — no reducer-job distinction for timeline |
| webui two-row split | webui submits a separate `aggregationJobId` row and cancels both ids | webui-side change (track separately): collapse to a single search-job row whose clp-s run also emits the timeline |
| Other (non-timeline) aggregations | reducer (`AggregationOutputHandler`) | **MVP+N via Spider** (not ready); explicitly **not a port of the reducer** |

Open (§7): how clp-s workers return per-archive bucket counts to the
coordinator, and how the coordinator accumulates + writes the combined timeline.

### Concurrency, polling, retirement, sleep

| Old query scheduler (Python) | New search coordinator (Rust) |
|---|---|
| `ProcessPoolExecutor(scheduler_concurrency)` for blocking dispatch | tokio tasks; no process pool |
| `scheduler_concurrency` (pool size) | `Semaphore(max_concurrent_jobs)`; `available_permits()` bounds the pending-fetch `LIMIT` |
| `DispatchExecutor.dispatch_job_and_update_db` in a worker process | one `tokio::spawn`'d handle per job/batch |
| `GroupResult.save()` / `GroupResult.restore(id, app=app)` rehydration | eliminated — result handle stays in-process in the spawned task |
| `handle_updating_task` background retire loop + inline `handle_pending_query_jobs` | fold retire into the per-job task; keep only the outer poll loop |
| `handle_job_updates` cadence: `handle_cancelling_search_jobs` + `check_job_status_and_update_db` then `sleep(jobs_poll_delay - elapsed)` | outer `Coordinator::run`-style loop with `CancellationToken` via `select!`; sleep `job_polling_interval.saturating_sub(elapsed)` |
| `asyncio.wait(FIRST_COMPLETED)` on {update task, sleep, reducer tasks} | plain `loop { schedule_new_jobs(); sleep }`; no multi-task rendezvous |
| `sleep(jobs_poll_delay)` task paces next poll; reducer tasks are the wake signal otherwise | one cadence; per-job task awaits its own wake events within its own `select!` |
| `handle_pending_query_jobs` is a plain `def` called inline, blocking the event loop | dispatch is async I/O on the single runtime (or `spawn_blocking` if needed) |
| `active_jobs` module-global dict, no lock | per-task owned state; guarded container (`DashMap`/`Mutex`) only for cross-job views |
| `kill_hanging_jobs` on startup | resume-style recovery **implemented on the branch**: `fetch_submitted_running_jobs` → `QueryJobHandle::recover` (confirm against search semantics / duplicate-results caveat) |
| partial DB-failure handling: reads → empty, writes silently fail → state drift | propagate `Result`; define partial-failure semantics explicitly |
| archive batching via `num_archives_to_search_per_sub_job`, re-adding to `pending_search_jobs` next iteration | batching lives inside the per-job task looping over `remaining_archives` |
| `dispatch_time` written inline | `mark_jobs_dispatched` deferred until after the sleep, so the grant-of-permission write doesn't contend with submissions |
| first fetch re-dispatches prior-instance jobs (no LIMIT); later fetches `LIMIT available_permits` | reuse the compression-coordinator two-phase fetch pattern |

### Query-table row reading

| Old query scheduler (Python) | New search coordinator (Rust) |
|---|---|
| `fetch_new_query_jobs`: `SELECT id, job_config, type, creation_time FROM QUERY_JOBS_TABLE_NAME WHERE status=PENDING` | branch's `fetch_new_job_rows` projects only `id` — **widen to `type`, `job_config`, `creation_time`** (part-1 gap) |
| `fetch_cancelling_search_jobs`: `SELECT id FROM QUERY_JOBS_TABLE_NAME WHERE status=CANCELLING AND type=SEARCH_OR_AGGREGATION` | cancellation handled by a background scan coroutine (see §5) |
| `job_config` column = MEDIUMBLOB, **msgpack**-serialized; `msgpack.unpackb(...)` | deserialize with serde + msgpack (`rmp-serde`) — mirror the compression-coordinator's blob pattern (note: query uses plain msgpack, not Brotli-wrapped) |
| config validated into `SearchJobConfig` / `ExtractIrJobConfig` / `ExtractJsonJobConfig` (Pydantic) | equivalent Rust structs via serde; route by `type` before deserializing into the right variant |
| `creation_time` read as `job_creation_time` for archive-retention cutoff (`archive_retention_period`) | keep the retention lower-bound computation (`SECOND_TO_MILLISECOND * (creation_time - retention_period*MIN_TO_SECONDS)`) |
| `insert_query_tasks_into_db`: one `QUERY_TASKS_TABLE_NAME` row per archive; `lastrowid` → task_id | same; per-archive task rows, task id returned to the search handle |
| `set_job_or_task_status` branches on `QUERY_JOBS_TABLE_NAME` (id, quoted kwargs) vs `QUERY_TASKS_TABLE_NAME` (job_id, raw kwargs) | replace with typed sqlx queries per table; drop the stringly-typed branch |
| `SELECT id AS job_id` aliases column to dict key | keep SQL plain; use `#[sqlx(rename = "...")]` on the struct (centralizes the mapping) |

### Job categorization (by `QUERY_JOBS_TABLE_NAME.type`)

| `QueryJobType` value | Old handling | New handling |
|---|---|---|
| `SEARCH_OR_AGGREGATION`, `aggregation_config=None` | plain search via `search` celery task | **MVP**: the supported path; per-job task dispatches clp-s search batches |
| `SEARCH_OR_AGGREGATION`, `aggregation_config` set, `do_count_aggregation`/`count_by_time_bucket_size` | reducer subprocess (celery) | **MVP+2**: clp-s native timeline aggregation; no reducer |
| `SEARCH_OR_AGGREGATION`, other aggregation | reducer subprocess (celery) | **MVP+N**: Spider-backed; blocked on Spider; until then, leave for celery or mark unsupported |
| `EXTRACT_IR` | `extract_stream` celery task (IR extraction) | **MVP+3** (decompression); wave to celery until then |
| `EXTRACT_JSON` | `extract_stream` celery task (JSON extraction) | **MVP+3** (decompression); wave to celery until then |

Categorization point: read `type` first, then deserialize `job_config` into the
matching variant; within `SearchJobConfig`, branch on `aggregation_config`
presence and on whether it's timeline (count/count-by-time) vs other. This lives
in `QueryJobHandle::new` (currently a stub on the branch) — mirror
`S3CompressionJobHandle::new`, which validates `ClpIoConfig` and returns
`Error::UnsupportedInputConfig` for inputs it doesn't handle; the coordinator's
`create_job_handle` already warns-and-skips on that error, which is the rollout
mechanism for leaving aggregation/`EXTRACT_*` rows to the legacy scheduler until
their phase lands. Note the compression side never needed this: `compression_jobs`
has no `type` column — categorization is net-new for search.

### Decompression

| Old (Python) | New (Rust) |
|---|---|
| `EXTRACT_IR`/`EXTRACT_JSON` are **query jobs** in `QUERY_JOBS_TABLE_NAME`, submitted by `decompress.py` via `submit_query_job(type=EXTRACT_*)`, dispatched to celery `extract_stream` | **MVP+3**: not handled by the coordinator until then |
| decompression is **not** a search job (different `type`, different celery task) | same conceptual split; do not fold decompression into the search path |
| `IrExtractionHandle` / `JsonExtractionHandle` resolve the target archive/file-split and dedup concurrent extractions (`active_file_split_ir_extractions`, `active_archive_json_extractions`) | when ported: per-job handle + a guarded dedup map; defer until MVP+3 |
| stream-already-extracted / in-progress checks against the results cache (MongoDB `document_exists`) | when ported: same checks against the results cache |

**Spider considerations (big picture, MVP+3):**
- Are decompression jobs submitted to Spider as the **same task type** as search, or a **distinct type**? Recommend distinct (different resource profile).
- Do they **share the resource group** with search? Sharing = shared concurrency/scheduling budget; separating = independent scheduling and isolation. Decide with the Spider scheduling design; default to a **separate resource group** so decompression cannot starve search (and vice versa).

### Cancellation

| Old query scheduler (Python) | New search coordinator (Rust) |
|---|---|
| `fetch_cancelling_search_jobs` polled inside `handle_job_updates` every `jobs_poll_delay` | **MVP+1**: a background coroutine scans `QUERY_JOBS_TABLE_NAME` for `status=CANCELLING` on its own cadence (or shares the outer poll) |
| `cancel_job_except_reducer` revokes the Celery task; `release_reducer_for_job` sends a reducer FAILURE | per-job `CancellationToken`; abort the in-flight search; **no reducer release** |
| `WAITING_FOR_REDUCER` cancellation cancels `reducer_acquisition_task` | n/a (no reducer) |
| updates `QUERY_TASKS_TABLE_NAME` → `CANCELLED` (PENDING and RUNNING) and `QUERY_JOBS_TABLE_NAME` → `CANCELLED` (guarded by `prev_status=CANCELLING`) | same DB transitions via typed sqlx queries |
| `asyncio.sleep(0)` yield between jobs to avoid monopolizing the loop | `yield_now` / cooperative scheduling between cancellation batches |

**Long-term:** if the architecture moves to a **server (push) form**, the scan-based cancellation is replaced by event-driven cancel requests pushed to the coordinator. Design the per-job `CancellationToken` now so this swap is local.

### Telemetry

| Old query scheduler (Python) | New search coordinator (Rust) |
|---|---|
| `clp.query.active_jobs`, `clp.query.outstanding_tasks` (observable up-down counters, callbacks read `active_jobs`) | same metrics via `opentelemetry` + `opentelemetry-otlp` Rust crates; `ObservableUpDownCounter` callbacks |
| `clp.query.tasks.completed`, `clp.query.tasks.failed` (counters) | same |
| `clp.query.job.duration`, `clp.query.task.duration` (histograms, seconds) | same |
| `init_telemetry` / `shutdown_telemetry`; disabled via `CLP_DISABLE_TELEMETRY` / `DO_NOT_TRACK` | replicate the env-gate; OTLP export is language-agnostic, so dashboards are unchanged |

---

## 5. Cancellation design (MVP+1)

- One background coroutine (or a phase of the outer poll loop) scans `QUERY_JOBS_TABLE_NAME WHERE status=CANCELLING` at a fixed cadence.
- For each, look up the in-flight job handle; trigger its `CancellationToken`.
- The per-job task aborts the in-flight clp-s search, updates `QUERY_TASKS_TABLE_NAME` → `CANCELLED` (for PENDING and RUNNING) and `QUERY_JOBS_TABLE_NAME` → `CANCELLED` (guarded by `prev_status=CANCELLING`), and releases its semaphore permit.
- Keep a per-job `CancellationToken` from MVP so the long-term swap to push-based (server-form) cancellation is local — only the trigger source changes.
- Consumes the same `CANCELLING` rows the api-server and webui already write (§2.2); no producer-side coordination.

---

## 6. Reducer deletion (explicit)

Do **not** port any of:
`reducer_connection_queue`, `asyncio.start_server` reducer handler, `handle_reducer_connection`, `ReducerHandlerMessage` / `ReducerHandlerMessageQueues` / `ReducerHandlerMessageType`, `acquire_reducer_for_job`, `release_reducer_for_job`, the reducer branches of `cancel_job_except_reducer`, the `WAITING_FOR_REDUCER` state, the `reducer` subcommand construction in `fs_search_task`, and the reducer handshake in `handle_finished_search_job`.

Timeline aggregation is a **map-reduce**: clp-s already does the per-archive map
(`--count-by-time` emits per-archive bucket counts); the cross-archive reduce
(summing buckets across all archives into one `{timestamp, count}` timeline) is
currently the reducer's job and **moves into the coordinator** — no separate
reducer process. Other aggregations are a future Spider-backed path (MVP+N), not
a port of the reducer. The webui's separate `aggregationJobId` (§2.2) is part of
this reducer-era design and is dropped alongside it.

---

## 7. Open questions

- **Retire model** *(settled on branch `search-coordinator/init`)*: per-handle self-retirement — `QueryJobHandle::to_completion` polls Spider job state inside the spawned task.
- **Result backend** *(settled)*: tasks are submitted to **Spider**, not Celery — `QueryJobSubmitter` polls Spider job state; no `GroupResult`-style handles.
- **Cross-job shared state**: container choice (`DashMap` vs. `Mutex`) for dedup/metrics; confirm what must be visible across jobs.
- **Decompression + Spider**: same task type as search or distinct; shared vs. separate resource group (default: separate). Resolve at MVP+3 with Spider scheduling design.
- **Other aggregations (MVP+N)**: Spider API surface and scheduling, not yet ready.
- **Partial DB-failure semantics**: define explicitly; no silent swallowing (the known Python limitation).
- **Timeline aggregation wiring**: clp-s `--count-by-time` already does per-archive bucketing (map); the coordinator must take over the cross-archive sum (reduce) the reducer used to do. Open: how each clp-s worker returns its per-archive bucket counts to the coordinator (task result vs. shared store), and how the coordinator accumulates + writes the combined `{timestamp, count}` timeline to the results cache.
- **webui aggregation-job removal**: the webui currently submits/cancels a separate aggregation job row; collapsing it to a single search-job id is a webui-side change to track when the reducer is deleted.