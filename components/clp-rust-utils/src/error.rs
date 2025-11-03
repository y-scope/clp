use thiserror::Error;

#[derive(Error, Debug)]
pub enum Error {
    #[error("`rmp_serde::encode::Error`: {0}")]
    MsgpackEncodeError(#[from] rmp_serde::encode::Error),

    #[error("`std::io::Error`: {0}")]
    IoError(#[from] std::io::Error),
}
