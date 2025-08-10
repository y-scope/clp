# Retention control in CLP

CLP supports retention control to free up storage space by periodically deleting outdated archives
and search results. Retention applies to both the local filesystem and object storage.

This process is managed by background **garbage collector** jobs, which scan for and delete expired
data based on configured retention settings in `etc/clp-config.yml`.

:::{note}
By default, retention control is disabled, and CLP retains data indefinitely.
:::

---

## Definitions

This section defines the terms and criteria used by CLP to determine when data should be 
deleted.

### Terms

- **Current Time (T):** The present wall-clock time when data expiry is being evaluated.
- **Retention Period (TTL):** The configured duration for which CLP retains data before it is
  considered expired.

### Expiry criteria

- **Archive Expiry:**  
  An archive is considered expired if the archive's TTL has elapsed since its most recent log event,
  i.e. that the difference between T and `archive.largest_msg_ts` has surpassed TTL.
  ```text
  if (T - archive.largest_msg_ts > TTL) then EXPIRED
  ```
  :::{note}
  Archives whose log messages do not contain timestamps are not subject to retention.
  :::
  
  :::{caution}
  Archive expiry is based on the **timestamp of the log messages**, not on when the archive is
  compressed. Compressing old logs with retention enabled could cause those archives to be deleted
  immediately after compression.
  :::

- **Search Result Expiry:** 
  A search result is considered expired if the result's TTL has elapsed since the search was 
  completed, i.e. that the difference between T and `search_job.completion_time` has surpassed TTL.
  ```text
  if (T - search_job.completion_time > TTL) then EXPIRED
  ```

---

## Configuration
Retention settings can be configured in `etc/clp-config.yml`.

### Configure retention period
To configure a retention period, update the `retention_period` key in `etc/clp-config.yml` with the
desired retention period in minutes.

For example, to configure an archive retention period of 30 days (43,200 minutes):
```yaml
archive_output:
  # Other archive output specific settings

  # Retention period for archives, in minutes. 
  # Set to null to disable automatic deletion.
  retention_period: 43200
```

### Configure sweep interval
**Sweep interval** specifies the time interval at which garbage collector jobs run to collect and
delete expired data.

To configure a custom sweep frequency for different retention targets, you can set the subfields
under `garbage_collector.sweep_interval` individually in `etc/clp-config.yml`. For example, to
configure a sweep interval of 15 minutes for search results but 3 hours (180 minutes) for archives,
enter the following:

```yaml
garbage_collector:
  logging_level: "INFO"

  # Interval (in minutes) at which garbage collector jobs run
  sweep_interval:
    archive: 180
    search_result: 15
```

:::{note}
If the retention period is set to `null`, the corresponding garbage collection task will not run 
even if `sweep_interval` is configured.
:::

## Handling data race conditions
CLP's retention system is designed to avoid data race conditions that may arise from the deletion of
archives or search results that may still be in use by active jobs. CLP employs the following
mechanisms to avoid these conditions:

- If any query job is running, CLP conservatively calculates a **safe expiry timestamp** based on 
  the earliest active search job. This ensures no archive that could be searched is deleted.

- CLP will **not** search an archive once it is considered expired, even if it has not yet been
  deleted by the garbage collector.

:::{warning}
A hanging search job will prevent CLP from deleting expired archives. 
Restarting the query scheduler will mark such jobs as failed and allow garbage collection to resume.
:::

## Fault tolerance
The garbage collector can resume execution from where it left off if a previous run fails. 
This design ensures that CLP does not fall into an inconsistent state due to partial deletions.

If the CLP package stops unexpectedly (for example, due to a host machine shutdown) while a garbage
collection task is running, simply restart the package and the garbage collector will continue 
from the point of failure.

:::{note}
During failure recovery, there may be a temporary period during which an archive no longer exists in
the database, but still exists on disk or in object storage. Once recovery is complete, the physical
archive will also be deleted.
:::