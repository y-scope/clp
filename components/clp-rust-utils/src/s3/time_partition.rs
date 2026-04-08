/// Utilities for time-based S3 key partitioning.
///
/// Archives are stored under keys like:
///   `<prefix>/<dataset>/YYYY/MM/DD/HH/<archive_id>.clp`
///
/// This enables S3 prefix-based filtering so that time-bounded queries can skip
/// entire hours/days of data without listing all objects.

/// Generates a time-partitioned S3 key for an archive.
///
/// # Arguments
///
/// * `base_prefix` - The base S3 key prefix (e.g., `"archives/"`).
/// * `dataset` - The dataset name (e.g., `"default"`).
/// * `timestamp_ms` - The archive's representative timestamp in epoch milliseconds.
/// * `archive_id` - The unique archive identifier.
///
/// # Returns
///
/// A fully-qualified S3 key string.
#[must_use]
pub fn generate_time_partitioned_key(
    base_prefix: &str,
    dataset: &str,
    timestamp_ms: i64,
    archive_id: &str,
) -> String {
    let secs = timestamp_ms / 1000;
    let (year, month, day, hour) = epoch_secs_to_components(secs);
    format!(
        "{base_prefix}{dataset}/{year:04}/{month:02}/{day:02}/{hour:02}/{archive_id}.clp"
    )
}

/// Generates S3 key prefixes that cover the given time range.
///
/// Returns hourly prefixes for ranges up to 48 hours, daily prefixes for ranges up to 365 days,
/// and monthly prefixes for anything larger. Falls back to the dataset-level prefix if the range
/// is unbounded.
///
/// # Arguments
///
/// * `base_prefix` - The base S3 key prefix.
/// * `dataset` - The dataset name.
/// * `begin_ms` - Start of the time range in epoch milliseconds (inclusive), or `None`.
/// * `end_ms` - End of the time range in epoch milliseconds (inclusive), or `None`.
///
/// # Returns
///
/// A vector of S3 key prefixes to scan.
#[must_use]
pub fn generate_time_range_prefixes(
    base_prefix: &str,
    dataset: &str,
    begin_ms: Option<i64>,
    end_ms: Option<i64>,
) -> Vec<String> {
    let (Some(begin), Some(end)) = (begin_ms, end_ms) else {
        // Unbounded range: return the dataset-level prefix.
        return vec![format!("{base_prefix}{dataset}/")];
    };

    if end < begin {
        return vec![format!("{base_prefix}{dataset}/")];
    }

    let begin_secs = begin / 1000;
    let end_secs = end / 1000;
    let range_hours = (end_secs - begin_secs) / 3600 + 1;

    if range_hours <= 48 {
        generate_hourly_prefixes(base_prefix, dataset, begin_secs, end_secs)
    } else {
        let range_days = (end_secs - begin_secs) / 86400 + 1;
        if range_days <= 365 {
            generate_daily_prefixes(base_prefix, dataset, begin_secs, end_secs)
        } else {
            generate_monthly_prefixes(base_prefix, dataset, begin_secs, end_secs)
        }
    }
}

fn generate_hourly_prefixes(
    base_prefix: &str,
    dataset: &str,
    begin_secs: i64,
    end_secs: i64,
) -> Vec<String> {
    let mut prefixes = Vec::new();
    // Align to the start of the hour.
    let mut current = begin_secs - (begin_secs % 3600);
    while current <= end_secs {
        let (year, month, day, hour) = epoch_secs_to_components(current);
        prefixes.push(format!(
            "{base_prefix}{dataset}/{year:04}/{month:02}/{day:02}/{hour:02}/"
        ));
        current += 3600;
    }
    prefixes
}

fn generate_daily_prefixes(
    base_prefix: &str,
    dataset: &str,
    begin_secs: i64,
    end_secs: i64,
) -> Vec<String> {
    let mut prefixes = Vec::new();
    let mut current = begin_secs - (begin_secs % 86400);
    while current <= end_secs {
        let (year, month, day, _) = epoch_secs_to_components(current);
        prefixes.push(format!(
            "{base_prefix}{dataset}/{year:04}/{month:02}/{day:02}/"
        ));
        current += 86400;
    }
    prefixes
}

fn generate_monthly_prefixes(
    base_prefix: &str,
    dataset: &str,
    begin_secs: i64,
    end_secs: i64,
) -> Vec<String> {
    let mut prefixes = Vec::new();
    let (mut year, mut month, _, _) = epoch_secs_to_components(begin_secs);
    let (end_year, end_month, _, _) = epoch_secs_to_components(end_secs);

    loop {
        prefixes.push(format!(
            "{base_prefix}{dataset}/{year:04}/{month:02}/"
        ));
        if year > end_year || (year == end_year && month >= end_month) {
            break;
        }
        month += 1;
        if month > 12 {
            month = 1;
            year += 1;
        }
    }
    prefixes
}

/// Converts epoch seconds to (year, month, day, hour) components.
///
/// Uses a simple arithmetic approach (no leap-second handling) suitable for log partitioning.
fn epoch_secs_to_components(epoch_secs: i64) -> (i32, u32, u32, u32) {
    // Days since Unix epoch.
    let total_days = if epoch_secs >= 0 {
        epoch_secs / 86400
    } else {
        (epoch_secs - 86399) / 86400
    };
    let hour = ((epoch_secs % 86400 + 86400) % 86400 / 3600) as u32;

    // Civil date from day count (algorithm from Howard Hinnant).
    let z = total_days + 719_468;
    let era = (if z >= 0 { z } else { z - 146_096 }) / 146_097;
    let doe = (z - era * 146_097) as u32;
    let yoe = (doe - doe / 1460 + doe / 36524 - doe / 146_096) / 365;
    let y = yoe as i64 + era * 400;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = doy - (153 * mp + 2) / 5 + 1;
    let m = if mp < 10 { mp + 3 } else { mp - 9 };
    let y = if m <= 2 { y + 1 } else { y };

    (y as i32, m, d, hour)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_generate_time_partitioned_key() {
        // 2026-03-31 14:30:00 UTC = 1774976600000 ms
        let key = generate_time_partitioned_key(
            "archives/",
            "default",
            1_774_976_600_000,
            "abc123",
        );
        assert!(key.starts_with("archives/default/2026/"));
        assert!(key.ends_with("/abc123.clp"));
    }

    #[test]
    fn test_generate_hourly_prefixes_short_range() {
        // 2 hour range
        let begin = 1_774_972_800_000_i64; // some epoch ms
        let end = begin + 2 * 3_600_000;
        let prefixes = generate_time_range_prefixes("archives/", "default", Some(begin), Some(end));
        // Should produce hourly prefixes (3 hours to cover the range)
        assert!(prefixes.len() >= 2);
        assert!(prefixes.len() <= 4);
        for p in &prefixes {
            assert!(p.starts_with("archives/default/"));
        }
    }

    #[test]
    fn test_unbounded_range_returns_dataset_prefix() {
        let prefixes = generate_time_range_prefixes("archives/", "mydata", None, None);
        assert_eq!(prefixes, vec!["archives/mydata/"]);
    }

    #[test]
    fn test_epoch_secs_to_components_known_date() {
        // 2024-01-01 00:00:00 UTC = 1704067200
        let (y, m, d, h) = epoch_secs_to_components(1_704_067_200);
        assert_eq!((y, m, d, h), (2024, 1, 1, 0));
    }

    #[test]
    fn test_epoch_secs_to_components_mid_day() {
        // 2024-06-15 13:00:00 UTC = 1718452800
        let (y, m, d, h) = epoch_secs_to_components(1_718_452_800);
        assert_eq!((y, m, d, h), (2024, 6, 15, 13));
    }
}
