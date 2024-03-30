mod device;

use std::sync::MutexGuard;
use sqlx::{Error, PgPool, Pool, Postgres};

pub trait Database<T> {
    fn table_name() -> &'static str;

    fn insert(&self, pool: &Pool<Postgres>) -> Result<usize, Error>;

    async fn update(&self, pool: &Pool<Postgres>) -> Result<(), Error>;

    fn get_all(pool: &Pool<Postgres>) -> Result<Vec<T>, Error>;

    fn get_by_id(id: usize, pool: &Pool<Postgres>) -> Result<Vec<T>, Error>;
}
