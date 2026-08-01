//! The search coordinator.

use tokio_util::sync::CancellationToken;

/// Coordinator for CLP search jobs.
pub struct SearchCoordinator {
    _cancellation_token: CancellationToken,
}

impl SearchCoordinator {
    /// Creates a search coordinator and a token that can be used to request its shutdown.
    #[must_use]
    pub fn new() -> (Self, CancellationToken) {
        let cancellation_token = CancellationToken::new();
        (
            Self {
                _cancellation_token: cancellation_token.clone(),
            },
            cancellation_token,
        )
    }

    /// Runs the search coordinator.
    ///
    /// This is currently a no-op.
    ///
    /// # Errors
    ///
    /// This implementation never returns an error.
    pub async fn run(self) -> anyhow::Result<()> {
        std::future::ready(()).await;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::SearchCoordinator;

    #[tokio::test]
    async fn run_succeeds() {
        let (coordinator, _cancellation_token) = SearchCoordinator::new();
        assert!(coordinator.run().await.is_ok());
    }
}
