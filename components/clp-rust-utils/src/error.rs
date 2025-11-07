use thiserror::Error;

#[derive(Error, Debug)]
pub enum Error {
    #[error("`rmp_serde::encode::Error`: {0}")]
    MsgpackEncode(#[from] rmp_serde::encode::Error),

    #[error("`std::io::Error`: {0}")]
    Io(#[from] std::io::Error),

    #[error("`serde_yaml::Error`: {0}")]
    SerdeYaml(#[from] serde_yaml::Error),

    #[error("`sqlx::Error`: {0}")]
    Sqlx(#[from] sqlx::Error),
}
