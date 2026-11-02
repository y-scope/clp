# Configuring retention periods

CLP can automatically delete *archives* and/or *search results* once they're older than a configured
retention period. This guide explains:

* [How retention works in CLP](#how-retention-works)
* [How to configure retention](#retention-settings)
* [Additional concerns worth noting](#additional-concerns)

## How retention works

To support retention periods, CLP's garbage collector component periodically scans for and deletes
expired data (archives or search results). To understand the high-level algorithm, first consider
the following definitions:

| Term                | Description                                                                                                         |
|---------------------|---------------------------------------------------------------------------------------------------------------------|
| $sweep\_interval$   | The interval (in minutes) at which the garbage collector wakes up to check for expired data.                        |
| $retention\_period$ | The duration (in minutes) for which data (an archive or search result) is retained before it is considered expired. |
| $current\_time$     | The time at which the garbage collector is performing a check.                                                      |
| $data\_timestamp$   | The time at which the data was created (e.g., for an archive, the time at which it finished being compressed).      |

When the garbage collector wakes up, it will scan for and delete any data that satisfies the expiry
criteria shown in [Figure 1](#figure-1):

(figure-1)=
:::{card}

$$is\_expired = (current\_time - data\_timestamp > retention\_period)$$

+++
**Figure 1**: The criteria for determining whether a piece of data has expired and should be
deleted.
:::

For example, if...

* some data has $data\_timestamp = 1440$;
* $retention\_period = 30$; and
* $current\_time = 1500$ when the garbage collector runs;

... then the garbage collector will determine that the data has expired and delete it.

---

## Retention settings

The following settings affect how CLP's data retention operates:

* [Archive retention period](#archive-retention-period)
* [Search result retention period](#search-result-retention-period)
* [Garbage collector sweep interval](#garbage-collector-sweep-interval)

All settings can be configured in `etc/clp-config.yaml` which is located in the CLP package
directory.

### Archive retention period

This setting determines how long an archive should be retained before it is automatically deleted.
Unlike the other settings on this page, it is configured **per dataset** rather than in
`etc/clp-config.yaml`, so different datasets can retain their archives for different durations.

A dataset's retention period is recorded when the dataset is created. If it isn't set, the dataset's
archives are retained indefinitely.

:::{note}
🚧 An interface for setting a dataset's retention period is still under construction. Until it's
available, archives are retained indefinitely.
:::

#### Archive expiry criteria

For archives, $data\_timestamp$ (in the expiry criteria equation from [Figure 1](#figure-1)) is the
time at which the archive was written, as recorded by CLP's metadata database.

:::{note}
This is not the timestamp of the log events contained in the archive. Compressing particularly old
logs therefore doesn't cause the resulting archives to be deleted early; they are retained for the
dataset's retention period like any other archive.
:::

### Search result retention period

This setting determines how long search results should be retained before they are automatically
deleted. To configure it, modify the value of `results_cache.retention_period` in
`etc/clp-config.yaml`.

For example, to configure a search result retention period of 1 day (1,440 minutes), use:

```yaml
results_cache:
  # ... Other results_cache settings...

  # Retention period for search results, in minutes. 
  # Set to null to disable automatic deletion.
  retention_period: 1440
```

By default, `results_cache.retention_period` is `60`, which means that search results will be
retained for 60 minutes (1 hour).

:::{note}
When a user runs consecutive queries in the webui without refreshing the page, the results of a
query will be deleted when the next query is run. This means that only the results of the last query
in a session are ever retained, and thus subject to the configured retention period.

In a future version of CLP, we may change this behavior so that the results of all queries are
retained until they are either evicted from the results cache, or their retention period expires,
whichever comes first.
:::

#### Search result expiry criteria

For search results, $data\_timestamp$ (in the expiry criteria equation from [Figure 1](#figure-1))
is the timestamp at which the search completed.

### Garbage collector sweep interval

This setting determines how often the garbage collector wakes up to check for and delete expired
data. To configure it, modify the value of `garbage_collector.sweep_interval` in
`etc/clp-config.yaml`.

For example, to configure a sweep interval of 3 hours (180 minutes) for archives and 15 minutes for
search results, use:

```yaml
garbage_collector:
  logging_level: "INFO"

  # Interval (in minutes) at which garbage collector jobs run
  sweep_interval:
    archive: 180
    search_result: 15
```

:::{note}
Since the garbage collector wakes up every $sweep\_interval$ minutes, data may be retained up to
$sweep\_interval$ minutes longer than the configured retention period.
:::

:::{note}
If the value of `results_cache.retention_period` is `null`, the search result garbage collection
task will not run even if `garbage_collector.sweep_interval.search_result` is configured. The
archive garbage collection task always runs, but has no effect on datasets that don't have a
retention period.
:::

---

## Additional concerns

It's worth understanding how CLP's retention system handles data races and ensures fault tolerance,
since these may affect the behavior of how long archives remain queryable and when they're deleted.

### Handling data races

CLP's retention system is designed to avoid deleting expired archives or search results that may
still be in use by active jobs. To do so, CLP employs the following mechanisms:

* If any query job is running, CLP conservatively calculates a **safe expiry timestamp** based on
  the earliest active search job. This ensures no archive which may be searched by the active job is
  deleted.

* CLP will **not** search an archive once it is considered expired, even if it has not yet been
  deleted by the garbage collector.

:::{warning}
A hanging search job will prevent CLP from deleting expired archives. Restarting the query scheduler
will mark such jobs as killed and allow garbage collection to resume.
:::

### Fault tolerance

The garbage collector can resume execution from where it left off if a previous run fails. This
design ensures that CLP does not fall into an inconsistent state due to partial deletions.

If the CLP package stops unexpectedly while a garbage collection task is running (for example, due
to a host machine shutdown), simply restart the package and the garbage collector will resume from
the point of failure.

:::{note}
During failure recovery, there may be a temporary period during which an archive no longer exists in
the metadata database, but still exists on disk or in object storage. Once recovery is complete, the
physical archive will also be deleted.
:::
