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

This section explains the terms and criteria CLP uses to decide when data should be deleted.

At a high level, CLP compares a data item's timestamp with the current time to determine whether
it has expired. The criteria used to assess this expiration differs slightly between archives and
search results.

### Terms

* **Current Time (`T`):** The current time (UTC) when a garbage collector job evaluates data
  expiration.
* **Retention Period (`TTL`):** The configured duration for which CLP retains data before it is
  considered expired.
* **Archive timestamp (`archive.T`):** The most recent timestamp among all log messages
  contained in the archive. Not related to the time at which the logs were compressed.

  Note that logs with outdated timestamps may be deleted immediately, depending on your retention
  settings.
* **Search result timestamp (`search_result.T`):** The timestamp when a search result is inserted
  into the results_cache.

  :::{Note}
  Archives whose log messages do not contain timestamps are not subject to retention.
  :::

### Expiry criteria

* **Archive Expiry:**  
  An archive is considered expired if its retention period has elapsed since archive's timestamp,
  i.e. that the difference between `T` and `archive.T` has surpassed `TTL`.

  ```text
  if (T - archive.T > TTL) then EXPIRED
  ```

  For example, if a compressed archive has `archive.T = 16:00`(for simplicity,
  we omit dates and seconds from the timestamp), and `TTL = 1 hour`, it will be
  considered expired after `T = 17:00` since `T - 16:00 > 1:00` for all `T > 17:00`.

  :::{caution}
  Retention control assumes that archive timestamps are given in **UTC** time. Using retention
  control on archives with local (i.e., non-UTC) timestamps can lead to an effective `TTL` that is
  different from the intended value.

  In the example above, if the package operates on a system in EDT (UTC-4) and `archive.T = 16:00`
  is a local timestamp, then a garbage collection job operating at 16:30 local time will convert
  `16:30 EDT` to `20:30 UTC`, and the expiry calculation will be `20:30 - 16:00 > 1:00`. In this
  case, the archive would be considered expired, and would be deleted, even though it wouldn't have
  actually reached its intended retention period.

  To avoid this issue, either generate logs with UTC timestamps or adjust the retention period to
  account for the offset:
  
  `adjusted_retention_period = retention_period - signed_UTC_offset`
  :::

* **Search Result Expiry:** 
  A search result is considered expired if its retention period has elapsed since the search was 
  completed, i.e. that the difference between T and `search_result.T` has surpassed TTL.

  ```text
  if (T - search_result.T > TTL) then EXPIRED
  ```

---

## Configuration

CLP allows users to specify different **retention_periods** for different types of data.
Additionally, the frequency of garbage collection job execution for each type of data can be
configured to a customized **sweep_interval**. These settings can be configured in
`etc/clp-config.yml`.

### Configure retention period

To configure a retention period, update the appropriate `.retention_period` key in
`etc/clp-config.yml` with the desired retention period in minutes.

For example, to configure an archive retention period of 30 days (43,200 minutes):

```yaml
archive_output:
  # Other archive_output settings

  # Retention period for archives, in minutes. 
  # Set to null to disable automatic deletion.
  retention_period: 43200
```

Similarly, to configure a search result retention period of 1 day (1440 minutes):

```yaml
results_cache:
  # Other results_cache settings

  # Retention period for search results, in minutes. 
  # Set to null to disable automatic deletion.
  retention_period: 1440
```

### Configure sweep interval

The **`garbage_collector.sweep_interval`** parameter specifies the time interval at which garbage
collector jobs run to collect and delete expired data.

To configure a custom sweep frequency for different retention targets, you can set the subfields
under `garbage_collector.sweep_interval` individually in `etc/clp-config.yml`. For example, to
configure a sweep interval of 15 minutes for search results and 3 hours (180 minutes) for archives,
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
If the `.retention_period` for a data type is set to `null`, the corresponding garbage collection
task will not run even if `garbage_collector.sweep_interval.<datatype>` is configured.
:::

---

## Internal

This section documents some of CLP’s internal behavior for retention and garbage collection.

### Handling data race conditions

CLP's retention system is designed to avoid data race conditions that may arise from the deletion of
archives or search results that may still be in use by active jobs. CLP employs the following
mechanisms to avoid these conditions:

* If any query job is running, CLP conservatively calculates a **safe expiry timestamp** based on 
  the earliest active search job. This ensures no archive that could be searched is deleted.

* CLP will **not** search an archive once it is considered expired, even if it has not yet been
  deleted by the garbage collector.

:::{warning}
A hanging search job will prevent CLP from deleting expired archives.
Restarting the query scheduler will mark such jobs as failed and allow garbage collection to resume.
:::

### Fault tolerance

The garbage collector can resume execution from where it left off if a previous run fails.
This design ensures that CLP does not fall into an inconsistent state due to partial deletions.

If the CLP package stops unexpectedly while a garbage collection task is running (for example, due
to a host machine shutdown), simply restart the package and the garbage collector will continue from
the point of failure.

:::{note}
During failure recovery, there may be a temporary period during which an archive no longer exists in
the database, but still exists on disk or in object storage. Once recovery is complete, the physical
archive will also be deleted.
:::
