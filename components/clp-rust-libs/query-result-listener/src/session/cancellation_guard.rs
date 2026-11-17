//! A guard that requests the cancellation of a session's query job unless disarmed.

use std::future::Future;

/// A guard that fires the cancellation on drop.
///
/// # Type Parameters
///
/// * `CancellationType` - The type of the future that requests the cancellation.
pub(super) struct CancellationGuard<CancellationType: Future<Output = ()> + Send + 'static> {
    cancellation: Option<CancellationType>,
}

impl<CancellationType: Future<Output = ()> + Send + 'static> CancellationGuard<CancellationType> {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created guard that requests the cancellation by running `cancellation`.
    pub(super) const fn new(cancellation: CancellationType) -> Self {
        Self {
            cancellation: Some(cancellation),
        }
    }

    /// Disarms the guard, dropping it without firing the cancellation.
    pub(super) fn disarm(mut self) {
        self.cancellation = None;
    }

    /// Requests the cancellation, and waits for the request to finish.
    pub(super) async fn request(mut self) {
        if let Some(cancellation) = self.cancellation.take() {
            cancellation.await;
        }
    }
}

impl<CancellationType: Future<Output = ()> + Send + 'static> Drop
    for CancellationGuard<CancellationType>
{
    fn drop(&mut self) {
        let Some(cancellation) = self.cancellation.take() else {
            return;
        };
        let Ok(runtime) = tokio::runtime::Handle::try_current() else {
            tracing::warn!(
                "Can't request the cancellation of a session's query job outside a Tokio runtime."
            );
            return;
        };
        runtime.spawn(cancellation);
    }
}
