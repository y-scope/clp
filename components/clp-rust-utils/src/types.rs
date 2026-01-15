pub mod non_empty_string {
    use non_empty_string::NonEmptyString;

    /// Trait for creating [`NonEmptyString`] instances from an expected non-empty string.
    pub trait ExpectedNonEmpty {
        /// # Returns
        ///
        /// A [`NonEmptyString`] of the provided static string slice.
        ///
        /// # Panics
        ///
        /// Panics if the provided static string slice is empty.
        #[must_use]
        fn from_static_str(str: &'static str) -> NonEmptyString {
            NonEmptyString::new(str.to_owned()).expect("static &str shouldn't be empty")
        }

        /// # Returns
        ///
        /// A [`NonEmptyString`] of the provided string.
        ///
        /// # Panics
        ///
        /// Panics if the provided string is empty.
        #[must_use]
        fn from_string(str: String) -> NonEmptyString {
            NonEmptyString::new(str).expect("string shouldn't be empty")
        }
    }

    impl ExpectedNonEmpty for NonEmptyString {}
}
