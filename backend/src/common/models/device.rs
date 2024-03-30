use serde::{Deserialize, Serialize};
use uuid::Uuid;

#[derive(Deserialize, Serialize, Clone)]
pub struct Device {
    pub id: usize,
    pub uuid: Uuid,
    pub name: String,
    pub area: String,
}
