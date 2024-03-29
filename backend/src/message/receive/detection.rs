use chrono::Utc;
use serde::{Deserialize, Serialize};
use crate::common::models::device::Device;

#[derive(Deserialize, Serialize)]
pub struct DetectionMessage {
    pub id: usize,
    pub device: Device,
    pub source: String,
    pub timestamp: Utc,
}
