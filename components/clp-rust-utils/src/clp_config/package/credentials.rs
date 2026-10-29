use secrecy::SecretString;
use serde::Deserialize;

#[derive(Clone, Debug, Deserialize)]
pub struct Credentials {
    pub database: Database,
}

#[derive(Clone, Debug, Deserialize)]
pub struct Database {
    pub password: SecretString,
    pub user: String,
}
