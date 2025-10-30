use serde::Deserialize;

#[derive(Deserialize)]
pub struct Credentials {
    pub database: Database,
}

#[derive(Deserialize)]
pub struct Database {
    pub password: String,
    pub user: String,
}
