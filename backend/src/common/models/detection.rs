use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use crate::common::models::device::Device;

#[derive(Deserialize, Serialize, sqlx::FromRow, Debug)]
pub struct Detection {
    pub id: i64,
    pub device: Device,
    pub source: String,
    pub timestamp: DateTime<Utc>,
}
