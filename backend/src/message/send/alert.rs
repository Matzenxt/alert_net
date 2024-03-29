use serde::{Deserialize, Serialize};

#[derive(Deserialize, Serialize, Clone, Copy)]
pub struct Alert {
    pub led: bool,
    pub noise: bool,
}

impl Default for Alert {
    fn default() -> Self {
        Alert {
            led: true,
            noise: true,
        }
    }
}
