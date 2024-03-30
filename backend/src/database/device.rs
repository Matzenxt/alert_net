use futures::executor::block_on;
use sqlx::{Arguments, Error, PgPool, Pool, Postgres};
use sqlx::postgres::PgArguments;
use crate::common::models::device::Device;
use crate::database::Database;

impl Database<Device> for Device {
    fn table_name() -> &'static str {
        "device"
    }

    fn insert(&self, pool: &PgPool) -> Result<usize, Error> {
        println!("Insert device: {}", Self::table_name());

        let mut args = PgArguments::default();
        args.add(&self.uuid);
        args.add(&self.name);
        args.add(&self.area);

        let statement = format!("INSERT INTO {} (uuid, description, area) VALUES ($1, $2, $3)", Self::table_name());

        println!("state: {}", statement);

        let mut con = block_on(pool.acquire()).unwrap();

        let res = block_on(sqlx::query_with(statement.as_str(), args).execute(&mut *con));

        println!("Res: {:#?}", res);

        Ok(0)
    }

    async fn update(&self, pool: &Pool<Postgres>) -> Result<(), Error> {
        println!("Update device: {}", Self::table_name());

        let mut args = PgArguments::default();
        args.add(&self.uuid);
        args.add(&self.name);
        args.add(&self.area);

        let statement = format!("INSERT INTO {} (uuid, description, area) VALUES ($1, $2, $3)", Self::table_name());

        println!("state: {}", statement);

        //let mut con = block_on(pool.acquire()).unwrap();
        let mut con = pool.acquire().await?;

        let res = sqlx::query_with(statement.as_str(), args).execute(&mut *con).await;

        println!("Res: {:#?}", res);

        Ok(())
    }

    fn get_all(pool: &Pool<Postgres>) -> Result<Vec<Device>, Error> {
        todo!()
    }

    fn get_by_id(id: usize, pool: &Pool<Postgres>) -> Result<Vec<Device>, Error> {
        todo!()
    }
}