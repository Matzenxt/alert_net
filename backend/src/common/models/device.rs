use serde::{Deserialize, Serialize};
use uuid::Uuid;

#[derive(Deserialize, Serialize, Clone, sqlx::FromRow, Debug)]
pub struct Device {
    pub id: i64,
    pub uuid: Uuid,
    pub description: String,
    pub area: String,
}
