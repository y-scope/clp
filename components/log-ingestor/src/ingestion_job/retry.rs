use std::time::Duration;

use clp_rust_utils::job_config::ingestion::s3::RetryConfig;
use tokio::select;
use tokio_util::sync::CancellationToken;

/// Tracks consecutive failure state and computes exponential backoff durations.
pub struct RetryState {
    config: RetryConfig,
    consecutive_failures: u32,
}

impl RetryState {
    pub const fn new(config: RetryConfig) -> Self {
        Self {
            config,
            consecutive_failures: 0,
        }
    }

    /// Records a successful operation, resetting the consecutive failure counter.
    pub const fn record_success(&mut self) {
        self.consecutive_failures = 0;
    }

    /// Records a failure. Returns `true` if retry is allowed (under the max),
    /// `false` if the max consecutive failures has been reached.
    pub const fn record_failure(&mut self) -> bool {
        self.consecutive_failures = self.consecutive_failures.saturating_add(1);
        self.consecutive_failures < self.config.max_consecutive_failures
    }

    /// Returns the current backoff duration based on the number of consecutive failures.
    pub fn current_backoff(&self) -> Duration {
        let exponent = self.consecutive_failures.saturating_sub(1);
        let backoff_ms = self
            .config
            .initial_backoff_ms
            .saturating_mul(u64::from(self.config.backoff_multiplier).saturating_pow(exponent))
            .min(self.config.max_backoff_ms);
        Duration::from_millis(backoff_ms)
    }

    /// Returns the current number of consecutive failures.
    pub const fn consecutive_failures(&self) -> u32 {
        self.consecutive_failures
    }

    /// Sleeps for the current backoff duration, respecting cancellation.
    /// Returns `true` if cancelled during sleep, `false` if sleep completed normally.
    pub async fn backoff_sleep(&self, cancel_token: &CancellationToken) -> bool {
        let backoff = self.current_backoff();
        select! {
            () = cancel_token.cancelled() => true,
            () = tokio::time::sleep(backoff) => false,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn default_config() -> RetryConfig {
        RetryConfig::default()
    }

    #[test]
    fn test_record_success_resets_counter() {
        let mut state = RetryState::new(default_config());
        state.record_failure();
        state.record_failure();
        assert_eq!(state.consecutive_failures(), 2);
        state.record_success();
        assert_eq!(state.consecutive_failures(), 0);
    }

    #[test]
    fn test_record_failure_returns_true_until_max() {
        let config = RetryConfig {
            max_consecutive_failures: 3,
            ..Default::default()
        };
        let mut state = RetryState::new(config);
        assert!(state.record_failure()); // 1 < 3
        assert!(state.record_failure()); // 2 < 3
        assert!(!state.record_failure()); // 3 == 3, exhausted
    }

    #[test]
    fn test_backoff_exponential_growth() {
        let config = RetryConfig {
            initial_backoff_ms: 1000,
            backoff_multiplier: 2,
            max_backoff_ms: 60_000,
            max_consecutive_failures: 10,
        };
        let mut state = RetryState::new(config);

        state.record_failure(); // consecutive_failures = 1
        assert_eq!(state.current_backoff(), Duration::from_millis(1000)); // 1000 * 2^0

        state.record_failure(); // consecutive_failures = 2
        assert_eq!(state.current_backoff(), Duration::from_millis(2000)); // 1000 * 2^1

        state.record_failure(); // consecutive_failures = 3
        assert_eq!(state.current_backoff(), Duration::from_millis(4000)); // 1000 * 2^2

        state.record_failure(); // consecutive_failures = 4
        assert_eq!(state.current_backoff(), Duration::from_millis(8000)); // 1000 * 2^3
    }

    #[test]
    fn test_backoff_caps_at_max() {
        let config = RetryConfig {
            initial_backoff_ms: 1000,
            backoff_multiplier: 2,
            max_backoff_ms: 5000,
            max_consecutive_failures: 20,
        };
        let mut state = RetryState::new(config);

        for _ in 0..10 {
            state.record_failure();
        }
        assert_eq!(state.current_backoff(), Duration::from_millis(5000));
    }

    #[test]
    fn test_backoff_zero_failures_returns_initial() {
        let state = RetryState::new(default_config());
        // With 0 failures, exponent would be saturating_sub(1) = 0 on underflow,
        // but this state shouldn't normally be queried before a failure.
        // Still, verify it doesn't panic.
        let _ = state.current_backoff();
    }

    #[tokio::test]
    async fn test_backoff_sleep_completes_normally() {
        let config = RetryConfig {
            initial_backoff_ms: 10,
            max_backoff_ms: 100,
            backoff_multiplier: 2,
            max_consecutive_failures: 5,
        };
        let mut state = RetryState::new(config);
        state.record_failure();

        let cancel_token = CancellationToken::new();
        let cancelled = state.backoff_sleep(&cancel_token).await;
        assert!(!cancelled);
    }

    #[tokio::test]
    async fn test_backoff_sleep_respects_cancellation() {
        let config = RetryConfig {
            initial_backoff_ms: 60_000,
            max_backoff_ms: 60_000,
            backoff_multiplier: 2,
            max_consecutive_failures: 5,
        };
        let mut state = RetryState::new(config);
        state.record_failure();

        let cancel_token = CancellationToken::new();
        let cancel_token_clone = cancel_token.clone();
        tokio::spawn(async move {
            tokio::time::sleep(Duration::from_millis(10)).await;
            cancel_token_clone.cancel();
        });

        let cancelled = state.backoff_sleep(&cancel_token).await;
        assert!(cancelled);
    }
}
