use std::sync::LazyLock;

use regex::Regex;

pub static VALID_DATASET_NAME_REGEX: LazyLock<Regex> =
    LazyLock::new(|| Regex::new(r"^[a-zA-Z0-9_]+$").unwrap());
