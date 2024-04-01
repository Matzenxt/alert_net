use chrono::Utc;
use sqlx::{Arguments, Error, Pool, Postgres, Row};
use sqlx::postgres::{PgArguments, PgRow};
use crate::common::models::detection::Detection;
use crate::common::models::device::Device;
use crate::database::Database;
use crate::message::receive::detection::DetectionMessage;

async fn test(row: &'_ PgRow, pool: &Pool<Postgres>) -> Result<Detection, Error> {
    let device_id: i64 = row.try_get(1)?;
    let device = Device::get_by_id(device_id, pool).await?;
    let d = device.first().unwrap().clone();

    Ok(Detection {
        id: row.try_get(0)?,
        device: d,
        source: row.try_get(2)?,
        timestamp: row.try_get(3)?,
    })
}

impl Database<Detection> for DetectionMessage {
    fn table_name() -> &'static str {
        "detection"
    }

    async fn insert(&self, pool: &Pool<Postgres>) -> Result<Detection, Error> {
        let mut args = PgArguments::default();
        args.add(&self.device.id);
        args.add(&self.source);
        args.add(Utc::now());

        let statement = format!("INSERT INTO {} (device_id, source, timestamp) VALUES ($1, $2, $3) RETURNING id, device_id, source, timestamp", Self::table_name());

        let mut con = pool.acquire().await?;
        let res = sqlx::query_with(statement.as_str(), args).fetch_one(&mut *con).await;

        match res {
            Ok(row) => {
                let a = test(&row, pool).await;
                match a {
                    Ok(detection) => {
                        Ok(detection)
                    }
                    Err(err) => {
                        Err(err)
                    }
                }
            }
            Err(err) => {
                Err(err)
            }
        }
    }

    async fn update(&self, pool: &Pool<Postgres>) -> Result<(), Error> {
        todo!()
    }

    async fn get_all(pool: &Pool<Postgres>) -> Result<Vec<Detection>, Error> {
        todo!()
    }

    async fn get_by_id(id: i64, pool: &Pool<Postgres>) -> Result<Vec<Detection>, Error> {
        todo!()
    }
}