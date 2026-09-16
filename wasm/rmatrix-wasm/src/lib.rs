use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub fn greet() -> String {
    "Halo dari Rust WASM!".to_string()
}