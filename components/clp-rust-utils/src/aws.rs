//! AWS utilities shared across CLP components.

pub const AWS_DEFAULT_REGION: &str = "us-east-1";

#[cfg(test)]
pub(crate) mod test_utils {
    use std::sync::Arc;
    use std::sync::Mutex;

    use aws_sdk_s3::config::ConfigBag;
    use aws_sdk_s3::config::Intercept;
    use aws_sdk_s3::config::RuntimeComponents;
    use aws_sdk_s3::config::interceptors::BeforeTransmitInterceptorContextRef;
    use aws_sdk_s3::error::BoxError;

    /// An interceptor that records the `Authorization` header of a signed request and then aborts
    /// the request before it is transmitted.
    #[derive(Debug, Clone, Default)]
    pub struct AuthorizationCapture {
        authorization: Arc<Mutex<Option<String>>>,
    }

    impl AuthorizationCapture {
        /// Gets the captured `Authorization` header.
        ///
        /// # Returns
        ///
        /// The captured `Authorization` header, or `None` if no signed request was intercepted.
        ///
        /// # Panics
        ///
        /// Panics if the capture lock is poisoned.
        pub fn authorization(&self) -> Option<String> {
            self.authorization
                .lock()
                .expect("authorization capture lock must not be poisoned")
                .clone()
        }
    }

    impl Intercept for AuthorizationCapture {
        fn name(&self) -> &'static str {
            "AuthorizationCapture"
        }

        fn read_before_transmit(
            &self,
            context: &BeforeTransmitInterceptorContextRef<'_>,
            _runtime_components: &RuntimeComponents,
            _cfg: &mut ConfigBag,
        ) -> Result<(), BoxError> {
            let authorization = context
                .request()
                .headers()
                .get("authorization")
                .map(str::to_owned);
            *self
                .authorization
                .lock()
                .expect("authorization capture lock must not be poisoned") = authorization;
            Err("request intercepted before transmission".into())
        }
    }
}
