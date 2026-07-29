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

| Term                | Description                                                                                                                                                                |
|---------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| $sweep\_interval$   | The interval (in minutes) at which the garbage collector wakes up to check for expired data.                                                                               |
| $retention\_period$ | The duration (in minutes) for which data (an archive or search result) is retained before it is considered expired.                                                        |
| $current\_time$     | The time at which the garbage collector is performing a check.                                                                                                             |
| $data\_timestamp$   | The end of the time range for the data being evaluated for expiration (e.g., for an archive, this is the timestamp of the most recent log event contained in the archive). |

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
To configure it, modify the value of `archive_output.retention_period` in `etc/clp-config.yaml`.

For example, to configure an archive retention period of 30 days (43,200 minutes), use:

```yaml
archive_output:
  # ... Other archive_output settings...

  # Retention period for archives, in minutes. 
  # Set to null to disable automatic deletion.
  retention_period: 43200
```

By default, `archive_output.retention_period` is `null`, which means that archives will be retained
indefinitely.

:::{warning}
If your log events use timestamps that *aren't* in the UTC time zone, you will need to adjust the
configured retention period to ensure expired archives are deleted at the correct time. See
[Handling log events with non-UTC timestamps](#handling-log-events-with-non-utc-timestamps) for
details.
:::

#### Archive expiry criteria

For archives, $data\_timestamp$ (in the expiry criteria equation from [Figure 1](#figure-1)) is the
timestamp of the most recent log event contained in the archive.

:::{note}
This is not the timestamp at which your logs were compressed. Therefore, if you compress
particularly old logs that have already expired according to the expiry criteria, they will be
deleted the next time the garbage collector runs.
:::

#### Handling log events with non-UTC timestamps

If your log events use timestamps that **aren't** in the UTC time zone, you will need to adjust the
configured retention period to ensure expired archives are deleted at the correct time. This is
because CLP currently doesn't support parsing time zone information, and the garbage collector runs
based on the UTC time zone.

For example, let's say:

* your log events use timestamps in the AWST timezone (UTC+8);
* you set a retention period of 1 hour;
* you have an archive with $data\_timestamp = 08:00$ AWST; and
* the garbage collector runs at $current\_time = 09:01$ AWST.

When the garbage collector runs, it will evaluate the archive's expiry criteria, substituting
$08:00$ for $data\_timestamp$, and $01:01$ for $current\_time$, since $09:01$ AWST = $01:01$ UTC.
The equation then becomes...

$$is\_expired = (01:01 - 08:00 > 01:00)$$

... which evaluates to false. Thus, the garbage collector won't delete the archive; in fact, it
won't delete it until $09:01$ UTC, which is 8 hours later than it should've been deleted.

Similarly, archives may be deleted prematurely if your log events use timestamps in a time zone that
is behind UTC.

To avoid this issue, you can adjust the retention period to account for the offset of the log
events' time zone from UTC:

$$adjusted\_retention\_period = retention\_period - signed\_utc\_offset$$

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
If the value of `archive_output.retention_period` is `null`, the corresponding garbage collection
task will not run even if `garbage_collector.sweep_interval.archive` is configured. The same applies
for `results_cache.retention_period` and `garbage_collector.sweep_interval.search_result`.
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
