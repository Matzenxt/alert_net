mod device;

use std::sync::MutexGuard;
use sqlx::{Error, PgPool, Pool, Postgres};

pub trait Database<T> {
    fn table_name() -> &'static str;

    async fn insert(&self, pool: &Pool<Postgres>) -> Result<T, Error>;

    async fn update(&self, pool: &Pool<Postgres>) -> Result<(), Error>;

    async fn get_all(pool: &Pool<Postgres>) -> Result<Vec<T>, Error>;

    async fn get_by_id(id: i64, pool: &Pool<Postgres>) -> Result<Vec<T>, Error>;
}
