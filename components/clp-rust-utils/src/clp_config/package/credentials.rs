use secrecy::SecretString;
use serde::Deserialize;

#[derive(Deserialize)]
pub struct Credentials {
    pub database: Database,
}

#[derive(Deserialize)]
pub struct Database {
    pub password: SecretString,
    pub user: String,
}
