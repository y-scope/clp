//! Query deduplication cache.
//!
//! Maintains an in-memory map of query fingerprints to recently submitted job IDs.
//! When an identical query is submitted within the TTL window, the existing job ID is
//! returned instead of creating a new job, avoiding redundant archive scans across
//! concurrent users.

use std::collections::HashMap;
use std::sync::Mutex;
use std::time::{Duration, Instant};

use sha2::{Digest, Sha256};

use crate::client::QueryConfig;

/// Default TTL for cached query entries (60 seconds).
const DEFAULT_DEDUP_TTL: Duration = Duration::from_secs(60);

/// Maximum number of entries before triggering eviction of expired entries.
const MAX_CACHE_ENTRIES: usize = 10_000;

/// A cached query entry with its associated job ID and expiration time.
struct CacheEntry {
    job_id: u64,
    expires_at: Instant,
}

/// Thread-safe query deduplication cache.
pub struct QueryDedupCache {
    entries: Mutex<HashMap<String, CacheEntry>>,
    ttl: Duration,
}

impl QueryDedupCache {
    /// Creates a new dedup cache with the default TTL.
    #[must_use]
    pub fn new() -> Self {
        Self {
            entries: Mutex::new(HashMap::new()),
            ttl: DEFAULT_DEDUP_TTL,
        }
    }

    /// Computes a fingerprint for a query config by hashing its canonical fields.
    #[must_use]
    pub fn fingerprint(config: &QueryConfig) -> String {
        let mut hasher = Sha256::new();
        hasher.update(config.query_string.as_bytes());

        // Normalize datasets: sort and join.
        if let Some(ref datasets) = config.datasets {
            let mut sorted = datasets.clone();
            sorted.sort();
            for ds in &sorted {
                hasher.update(ds.as_bytes());
            }
        } else {
            hasher.update(b"__none__");
        }

        // Include time range boundaries.
        if let Some(begin) = config.time_range_begin_millisecs {
            hasher.update(begin.to_le_bytes());
        } else {
            hasher.update(b"__no_begin__");
        }
        if let Some(end) = config.time_range_end_millisecs {
            hasher.update(end.to_le_bytes());
        } else {
            hasher.update(b"__no_end__");
        }

        hasher.update(if config.ignore_case { &[1u8] } else { &[0u8] });
        hasher.update(config.max_num_results.to_le_bytes());

        let result = hasher.finalize();
        hex::encode(result)
    }

    /// Looks up a query fingerprint in the cache.
    ///
    /// Returns `Some(job_id)` if a non-expired entry exists, `None` otherwise.
    pub fn get(&self, fingerprint: &str) -> Option<u64> {
        let mut entries = self.entries.lock().unwrap();
        if let Some(entry) = entries.get(fingerprint) {
            if Instant::now() < entry.expires_at {
                return Some(entry.job_id);
            }
            // Expired — remove it.
            entries.remove(fingerprint);
        }
        None
    }

    /// Inserts a query fingerprint → job ID mapping into the cache.
    pub fn insert(&self, fingerprint: String, job_id: u64) {
        let mut entries = self.entries.lock().unwrap();

        // Evict expired entries if the cache is getting large.
        if entries.len() >= MAX_CACHE_ENTRIES {
            let now = Instant::now();
            entries.retain(|_, entry| entry.expires_at > now);
        }

        entries.insert(
            fingerprint,
            CacheEntry {
                job_id,
                expires_at: Instant::now() + self.ttl,
            },
        );
    }
}

impl Default for QueryDedupCache {
    fn default() -> Self {
        Self::new()
    }
}
