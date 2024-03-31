use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use crate::common::models::device::Device;

#[derive(Deserialize, Serialize, sqlx::FromRow)]
pub struct DetectionMessage {
    pub id: usize,
    pub device: Device,
    pub source: String,
    pub timestamp: DateTime<Utc>,
}
