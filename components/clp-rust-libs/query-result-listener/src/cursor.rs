//! Coordinates result claims across repeated attempts of one query task.

use std::sync::atomic::AtomicU64;
use std::sync::atomic::Ordering;

use clp_rust_utils::types::ArchiveId;

/// The outcome of claiming a result index on a [`TaskCursor`].
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Claim {
    /// The caller is the first to claim the result.
    Claimed,

    /// Another attempt already claimed the result.
    Duplicate,
}

/// Tracks the next unclaimed result of one query task across all of its attempts.
#[derive(Debug)]
pub struct TaskCursor {
    next_result_index: AtomicU64,
    archive_id: ArchiveId,
}

impl TaskCursor {
    /// Creates a cursor for `archive_id`.
    ///
    /// # Returns
    ///
    /// A cursor whose next unclaimed result index is 0.
    #[must_use]
    pub const fn new(archive_id: ArchiveId) -> Self {
        Self {
            next_result_index: AtomicU64::new(0),
            archive_id,
        }
    }

    #[must_use]
    pub const fn archive_id(&self) -> ArchiveId {
        self.archive_id
    }

    /// Claims the result at `result_index` for the calling attempt.
    ///
    /// # Returns
    ///
    /// [`Claim::Claimed`] if the caller is the first attempt to claim the result, or
    /// [`Claim::Duplicate`] if another attempt already claimed it.
    ///
    /// # Panics
    ///
    /// Panics if `result_index` is greater than the next unclaimed result index.
    #[must_use]
    pub fn claim(&self, result_index: u64) -> Claim {
        match self.next_result_index.compare_exchange(
            result_index,
            result_index.wrapping_add(1),
            Ordering::Relaxed,
            Ordering::Relaxed,
        ) {
            Ok(_) => Claim::Claimed,
            Err(current) => {
                assert!(
                    current > result_index,
                    "result index {result_index} is greater than the next unclaimed result index \
                     {current}"
                );
                Claim::Duplicate
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use clp_rust_utils::types::ArchiveId;

    use super::Claim;
    use super::TaskCursor;

    /// # Returns
    ///
    /// A cursor for the test archive.
    fn new_cursor() -> TaskCursor {
        const ARCHIVE_ID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";

        TaskCursor::new(
            ARCHIVE_ID
                .parse::<ArchiveId>()
                .expect("archive ID fixture should be a valid UUID"),
        )
    }

    #[test]
    fn cursor_returns_duplicate_without_advancing_for_claimed_indices() {
        let cursor = new_cursor();
        for result_index in 0..3 {
            assert_eq!(cursor.claim(result_index), Claim::Claimed);
        }

        for result_index in 0..3 {
            assert_eq!(cursor.claim(result_index), Claim::Duplicate);
        }
        assert_eq!(cursor.claim(3), Claim::Claimed);
    }

    #[test]
    #[should_panic(expected = "is greater than the next unclaimed result index")]
    fn cursor_panics_if_claim_skips_unclaimed_index() {
        let cursor = new_cursor();
        let _claim = cursor.claim(1);
    }

    #[test]
    fn concurrent_attempts_claim_each_index_exactly_once() {
        const NUM_ATTEMPTS: usize = 8;
        const NUM_RESULTS: u64 = 10_000;

        use std::sync::Barrier;

        let cursor = new_cursor();
        let start = Barrier::new(NUM_ATTEMPTS);
        let mut claimed: Vec<u64> = std::thread::scope(|scope| {
            let attempts: Vec<_> = (0..NUM_ATTEMPTS)
                .map(|_| {
                    scope.spawn(|| {
                        start.wait();
                        (0..NUM_RESULTS)
                            .filter(|&result_index| Claim::Claimed == cursor.claim(result_index))
                            .collect::<Vec<_>>()
                    })
                })
                .collect();
            attempts
                .into_iter()
                .flat_map(|attempt| {
                    attempt
                        .join()
                        .expect("concurrent claim attempt should not panic")
                })
                .collect()
        });
        claimed.sort_unstable();
        assert_eq!(claimed, (0..NUM_RESULTS).collect::<Vec<_>>());
    }
}
