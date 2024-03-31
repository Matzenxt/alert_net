use futures::executor::block_on;
use sqlx::{Arguments, Error, PgPool, Pool, Postgres};
use sqlx::postgres::PgArguments;
use crate::common::models::device::Device;
use crate::database::Database;

impl Database<Device> for Device {
    fn table_name() -> &'static str {
        "device"
    }

    async fn insert(&self, pool: &PgPool) -> Result<Device, Error> {
        println!("Insert device: {}", Self::table_name());

        let mut args = PgArguments::default();
        args.add(&self.uuid);
        args.add(&self.description);
        args.add(&self.area);

        let statement = format!("INSERT INTO {} (uuid, description, area) VALUES ($1, $2, $3) RETURNING id, uuid, description, area", Self::table_name());

        let mut con = pool.acquire().await?;

        // TODO: Add error handling
        let res = sqlx::query_as_with(statement.as_str(), args).fetch_one(&mut *con).await?;

        Ok(res)
    }

    async fn update(&self, pool: &Pool<Postgres>) -> Result<(), Error> {
        todo!()
    }

    async fn get_all(pool: &Pool<Postgres>) -> Result<Vec<Device>, Error> {
        todo!()
    }

    async fn get_by_id(id: i64, pool: &Pool<Postgres>) -> Result<Vec<Device>, Error> {
        todo!()
    }
}