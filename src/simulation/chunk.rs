use super::cell::Cell;
use serde::{Serialize, Deserialize};

pub const CHUNK_SIZE: usize = 16;
pub const CHUNK_VOLUME: usize = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct Chunk {
    pub cx: i32,
    pub cy: i32,
    pub cz: i32,
    pub cells: Vec<Cell>, // Flat vector of size CHUNK_VOLUME
    pub is_active: bool,
    pub has_alive_cells: bool,
}

impl Chunk {
    pub fn new(cx: i32, cy: i32, cz: i32) -> Self {
        Self {
            cx,
            cy,
            cz,
            cells: vec![Cell::default(); CHUNK_VOLUME],
            is_active: false,
            has_alive_cells: false,
        }
    }

    #[inline]
    pub fn get_index(lx: usize, ly: usize, lz: usize) -> usize {
        lx + (ly * CHUNK_SIZE) + (lz * CHUNK_SIZE * CHUNK_SIZE)
    }

    #[inline]
    pub fn get_cell(&self, lx: usize, ly: usize, lz: usize) -> &Cell {
        &self.cells[Self::get_index(lx, ly, lz)]
    }

    #[inline]
    pub fn get_cell_mut(&mut self, lx: usize, ly: usize, lz: usize) -> &mut Cell {
        &mut self.cells[Self::get_index(lx, ly, lz)]
    }

    pub fn update_active_flags(&mut self) {
        self.has_alive_cells = self.cells.iter().any(|c| c.is_alive);
        self.is_active = self.has_alive_cells;
    }
}
