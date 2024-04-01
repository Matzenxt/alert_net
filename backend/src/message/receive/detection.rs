use serde::{Deserialize, Serialize};
use crate::common::models::device::Device;

#[derive(Deserialize, Serialize)]
pub struct DetectionMessage {
    pub device: Device,
    pub source: String,
}
