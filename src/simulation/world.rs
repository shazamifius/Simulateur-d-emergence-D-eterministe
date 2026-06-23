use std::collections::HashMap;
use serde::{Serialize, Deserialize};
use rayon::prelude::*;

use super::cell::{Cell, CellType};
use super::chunk::{Chunk, CHUNK_SIZE};
use super::params::ParametresGlobaux;
use super::laws::{
    appliquer_loi_metabolisme, evaluer_mouvements, evaluer_division,
    evaluer_osmose, evaluer_psychisme, deterministic_mutation,
    euclidean_distance
};

#[derive(Serialize, Deserialize, Hash, Eq, PartialEq, Ord, PartialOrd, Clone, Copy, Debug)]
pub struct ChunkKey {
    pub x: i32,
    pub y: i32,
    pub z: i32,
}

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct WorldMap {
    pub chunks: HashMap<ChunkKey, Chunk>,
}

impl WorldMap {
    pub fn new() -> Self {
        Self {
            chunks: HashMap::new(),
        }
    }

    #[inline]
    pub fn world_to_chunk_coords(wx: i32, wy: i32, wz: i32) -> (i32, i32, i32, usize, usize, usize) {
        let cx = wx.div_euclid(CHUNK_SIZE as i32);
        let cy = wy.div_euclid(CHUNK_SIZE as i32);
        let cz = wz.div_euclid(CHUNK_SIZE as i32);

        let lx = wx.rem_euclid(CHUNK_SIZE as i32) as usize;
        let ly = wy.rem_euclid(CHUNK_SIZE as i32) as usize;
        let lz = wz.rem_euclid(CHUNK_SIZE as i32) as usize;

        (cx, cy, cz, lx, ly, lz)
    }

    pub fn get_chunk(&self, cx: i32, cy: i32, cz: i32) -> Option<&Chunk> {
        self.chunks.get(&ChunkKey { x: cx, y: cy, z: cz })
    }

    pub fn get_chunk_mut(&mut self, cx: i32, cy: i32, cz: i32) -> Option<&mut Chunk> {
        self.chunks.get_mut(&ChunkKey { x: cx, y: cy, z: cz })
    }

    pub fn get_or_create_chunk(&mut self, cx: i32, cy: i32, cz: i32) -> &mut Chunk {
        self.chunks.entry(ChunkKey { x: cx, y: cy, z: cz })
            .or_insert_with(|| Chunk::new(cx, cy, cz))
    }

    pub fn get_cell_global(&self, wx: i32, wy: i32, wz: i32) -> Option<&Cell> {
        let (cx, cy, cz, lx, ly, lz) = Self::world_to_chunk_coords(wx, wy, wz);
        self.get_chunk(cx, cy, cz).map(|chk| chk.get_cell(lx, ly, lz))
    }

    pub fn get_cell_global_mut(&mut self, wx: i32, wy: i32, wz: i32) -> Option<&mut Cell> {
        let (cx, cy, cz, lx, ly, lz) = Self::world_to_chunk_coords(wx, wy, wz);
        self.get_chunk_mut(cx, cy, cz).map(|chk| chk.get_cell_mut(lx, ly, lz))
    }

    pub fn clear(&mut self) {
        self.chunks.clear();
    }
}

// --- Actions Différées ---

#[derive(Clone, Debug)]
pub struct MouvementSouhaite {
    pub source: (i32, i32, i32),
    pub destination: (i32, i32, i32),
    pub dette_besoin_source: f32,
    pub index_source: usize,
}

#[derive(Clone, Debug)]
pub struct DivisionSouhaitee {
    pub source_mere: (i32, i32, i32),
    pub destination_fille: (i32, i32, i32),
    pub energie_mere: f32,
}

#[derive(Clone, Debug)]
pub struct EchangeEnergieSouhaite {
    pub source: (i32, i32, i32),
    pub destination: (i32, i32, i32),
    pub montant_energie: f32,
}

#[derive(Clone, Debug)]
pub struct EchangePsychiqueSouhaite {
    pub source: (i32, i32, i32),
    pub destination: (i32, i32, i32),
    pub montant_c: f32,
    pub montant_l: f32,
}

#[derive(Clone, Debug)]
pub struct ChunkIntentions {
    pub chunk_key: ChunkKey,
    pub mouvements: Vec<MouvementSouhaite>,
    pub divisions: Vec<DivisionSouhaitee>,
    pub echanges_energie: Vec<EchangeEnergieSouhaite>,
    pub echanges_psychiques: Vec<EchangePsychiqueSouhaite>,
}

impl ChunkIntentions {
    pub fn new(chunk_key: ChunkKey) -> Self {
        Self {
            chunk_key,
            mouvements: Vec::new(),
            divisions: Vec::new(),
            echanges_energie: Vec::new(),
            echanges_psychiques: Vec::new(),
        }
    }
}

// --- Struct MondeSED ---

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct MondeSED {
    pub size_x: i32,
    pub size_y: i32,
    pub size_z: i32,
    pub cycle_actuel: i32,
    pub current_seed: u32,
    pub world_map: WorldMap,
    #[serde(skip, default = "WorldMap::new")]
    pub read_map: WorldMap,
    pub params: ParametresGlobaux,
}

impl MondeSED {
    pub fn new(sx: i32, sy: i32, sz: i32) -> Self {
        let mut params = ParametresGlobaux::default();
        params.world_height = sy;
        Self {
            size_x: sx,
            size_y: sy,
            size_z: sz,
            cycle_actuel: 0,
            current_seed: 0,
            world_map: WorldMap::new(),
            read_map: WorldMap::new(),
            params,
        }
    }

    pub fn sync_read_map(&mut self) {
        for (key, chunk) in &self.world_map.chunks {
            let read_chunk = self.read_map.chunks.entry(*key)
                .or_insert_with(|| Chunk::new(key.x, key.y, key.z));
            read_chunk.cells.copy_from_slice(&chunk.cells);
            read_chunk.has_alive_cells = chunk.has_alive_cells;
            read_chunk.is_active = chunk.is_active;
        }
        if self.read_map.chunks.len() > self.world_map.chunks.len() {
            let world_chunks = &self.world_map.chunks;
            self.read_map.chunks.retain(|key, _| world_chunks.contains_key(key));
        }
    }

    pub fn initialiser_monde(&mut self, seed: u32, initial_density: f32) {
        self.current_seed = seed;
        self.world_map.clear();
        self.read_map.clear();
        self.cycle_actuel = 0;

        let mut rng = SmallRng::new(seed);

        // 1. Plancher de Bedrock à Y=0
        for z in 0..self.size_z {
            for x in 0..self.size_x {
                let (cx, cy, cz, lx, ly, lz) = WorldMap::world_to_chunk_coords(x, 0, z);
                let chunk = self.world_map.get_or_create_chunk(cx, cy, cz);
                let c = chunk.get_cell_mut(lx, ly, lz);
                *c = Cell::new_static();
            }
        }

        // 2. Remplissage des cellules
        for z in 0..self.size_z {
            for y in 1..self.size_y {
                for x in 0..self.size_x {
                    if rng.next_f32() < initial_density {
                        let (cx, cy, cz, lx, ly, lz) = WorldMap::world_to_chunk_coords(x, y, z);
                        let chunk = self.world_map.get_or_create_chunk(cx, cy, cz);
                        let cell = chunk.get_cell_mut(lx, ly, lz);
                        
                        cell.is_alive = true;
                        cell.e = 1.0;
                        cell.r = rng.next_f32();
                        cell.sc = rng.next_f32();
                        cell.t = CellType::Souche;
                    }
                }
            }
        }

        for chunk in self.world_map.chunks.values_mut() {
            chunk.update_active_flags();
        }
    }

    pub fn calculate_state_hash(&self) -> usize {
        let mut global_hash = 0usize;
        let mut keys: Vec<&ChunkKey> = self.world_map.chunks.keys().collect();
        keys.sort_by(|a, b| {
            if a.x != b.x { a.x.cmp(&b.x) }
            else if a.y != b.y { a.y.cmp(&b.y) }
            else { a.z.cmp(&b.z) }
        });

        for key in keys {
            if let Some(chk) = self.world_map.chunks.get(key) {
                let mut chunk_hash = 0usize;
                let pos_hash = (key.x as usize) ^ ((key.y as usize) << 1) ^ ((key.z as usize) << 2);
                chunk_hash ^= pos_hash;

                for c in &chk.cells {
                    if c.is_alive {
                        let e_bits = c.e.to_bits() as usize;
                        chunk_hash = chunk_hash.rotate_left(5) ^ e_bits ^ (c.t as usize);
                    }
                }
                global_hash ^= chunk_hash;
            }
        }
        global_hash
    }

    pub fn get_nombre_cellules_vivantes(&self) -> usize {
        self.world_map.chunks.values()
            .map(|chk| chk.cells.iter().filter(|c| c.is_alive).count())
            .sum()
    }

    // --- Loi 1 & Loi 5 Helpers ---

    pub fn get_neighbors(x: i32, y: i32, z: i32) -> [(i32, i32, i32, usize); 26] {
        let mut neighbors = [(0, 0, 0, 0); 26];
        let mut count = 0;
        for dz in -1..=1 {
            for dy in -1..=1 {
                for dx in -1..=1 {
                    if dx == 0 && dy == 0 && dz == 0 { continue; }
                    let w_idx = ((dz + 1) * 9 + (dy + 1) * 3 + (dx + 1)) as usize;
                    neighbors[count] = (x + dx, y + dy, z + dz, w_idx);
                    count += 1;
                }
            }
        }
        neighbors
    }

    pub fn calculer_barycentre(&self) -> (f32, f32, f32) {
        let mut sum_x = 0.0;
        let mut sum_y = 0.0;
        let mut sum_z = 0.0;
        let mut count = 0;

        for chunk in self.world_map.chunks.values() {
            if !chunk.has_alive_cells { continue; }
            for lx in 0..CHUNK_SIZE {
                for ly in 0..CHUNK_SIZE {
                    for lz in 0..CHUNK_SIZE {
                        let cell = chunk.get_cell(lx, ly, lz);
                        if cell.is_alive && cell.t != CellType::Static {
                            let wx = (chunk.cx * CHUNK_SIZE as i32 + lx as i32) as f32;
                            let wy = (chunk.cy * CHUNK_SIZE as i32 + ly as i32) as f32;
                            let wz = (chunk.cz * CHUNK_SIZE as i32 + lz as i32) as f32;
                            sum_x += wx;
                            sum_y += wy;
                            sum_z += wz;
                            count += 1;
                        }
                    }
                }
            }
        }

        if count > 0 {
            let c = count as f32;
            (sum_x / c, sum_y / c, sum_z / c)
        } else {
            (self.size_x as f32 / 2.0, self.size_y as f32 / 2.0, self.size_z as f32 / 2.0)
        }
    }

    // --- COEUR DE SIMULATION ---

    pub fn executer_cycle_neural(&mut self) {
        let neighbors_coords = {
            let mut neighbors = [(0, 0, 0, 0); 26];
            let mut count = 0;
            for dz in -1..=1 {
                for dy in -1..=1 {
                    for dx in -1..=1 {
                        if dx == 0 && dy == 0 && dz == 0 { continue; }
                        let w_idx = ((dz + 1) * 9 + (dy + 1) * 3 + (dx + 1)) as usize;
                        neighbors[count] = (dx, dy, dz, w_idx);
                        count += 1;
                    }
                }
            }
            neighbors
        };

        for sub_tick in 0..self.params.ticks_neuraux_par_physique {
            self.sync_read_map();
            let read_map = &self.read_map;
            let spikes = std::sync::Mutex::new(Vec::new());

            let seed = self.current_seed;
            let cycle = self.cycle_actuel;
            let params = &self.params;
            
            self.world_map.chunks.par_iter_mut().for_each(|(&_key, chunk)| {
                if !chunk.has_alive_cells { return; }

                let cx = chunk.cx;
                let cy = chunk.cy;
                let cz = chunk.cz;

                for lx in 0..CHUNK_SIZE {
                    for ly in 0..CHUNK_SIZE {
                        for lz in 0..CHUNK_SIZE {
                            let cell = chunk.get_cell_mut(lx, ly, lz);
                            if cell.is_alive && cell.t == CellType::Neurone {
                                if cell.r#ref > 0 {
                                    cell.r#ref -= 1;
                                    cell.p = -0.5;
                                } else {
                                    let wx = cx * CHUNK_SIZE as i32 + lx as i32;
                                    let wy = cy * CHUNK_SIZE as i32 + ly as i32;
                                    let wz = cz * CHUNK_SIZE as i32 + lz as i32;

                                    let mut sum_w = 0.0;
                                    let mut sum_pw = 0.0;

                                    for &(dx, dy, dz, w_idx) in &neighbors_coords {
                                        let nx = wx + dx;
                                        let ny = wy + dy;
                                        let nz = wz + dz;

                                        let neighbor_p = read_map.get_cell_global(nx, ny, nz)
                                            .map(|c| if c.is_alive { c.p } else { 0.0 })
                                            .unwrap_or(0.0);
                                        
                                        let w = cell.w[w_idx];
                                        sum_w += w;
                                        sum_pw += neighbor_p * w;
                                    }

                                    let integration = sum_pw / f32::max(1.0, sum_w);
                                    let noise = deterministic_noise(wx, wy, wz, cycle, sub_tick as i32, seed);
                                    let p_new = (cell.p * 0.9) + integration + noise;

                                    if p_new > params.seuil_fire {
                                        cell.p = 1.0;
                                        cell.r#ref = params.periode_refractaire;
                                        cell.e_cost += params.cout_spike;
                                        cell.h = (cell.h << 1) | 1;
                                        
                                        let mut s = spikes.lock().unwrap();
                                        s.push((wx, wy, wz));
                                    } else {
                                        cell.p = p_new.clamp(-1.0, 1.0);
                                        cell.h = (cell.h << 1) | 0;
                                    }
                                }
                            }
                        }
                    }
                }
            });

            let spikes_list = spikes.into_inner().unwrap();
            if !spikes_list.is_empty() {
                self.world_map.chunks.par_iter_mut().for_each(|(&_key, chunk)| {
                    if !chunk.has_alive_cells { return; }

                    let cx = chunk.cx;
                    let cy = chunk.cy;
                    let cz = chunk.cz;

                    for lx in 0..CHUNK_SIZE {
                        for ly in 0..CHUNK_SIZE {
                            for lz in 0..CHUNK_SIZE {
                                let cell = chunk.get_cell_mut(lx, ly, lz);
                                if cell.is_alive && cell.t == CellType::Neurone {
                                    let wx = cx * CHUNK_SIZE as i32 + lx as i32;
                                    let wy = cy * CHUNK_SIZE as i32 + ly as i32;
                                    let wz = cz * CHUNK_SIZE as i32 + lz as i32;

                                    let mut bonus = 0.0;
                                    for &spk in &spikes_list {
                                        let d = euclidean_distance((wx, wy, wz), spk);
                                        if d <= params.rayon_ignition {
                                            bonus += 0.1;
                                        }
                                    }
                                    if bonus > 0.0 {
                                        cell.p += bonus;
                                        cell.clamp_variables();
                                    }
                                }
                            }
                        }
                    }
                });
            }
        }
    }

    pub fn avancer_temps(&mut self) {
        self.cycle_actuel += 1;
        
        let barycentre = self.calculer_barycentre();

        self.executer_cycle_neural();

        self.sync_read_map();
        let read_map = &self.read_map;

        let mut chunk_intents: Vec<ChunkIntentions> = self.world_map.chunks.par_iter_mut().map(|(&key, chunk)| {
            let mut intents = ChunkIntentions::new(key);
            if !chunk.has_alive_cells { return intents; }

            let mut index_counter = 0;
            let cx = chunk.cx;
            let cy = chunk.cy;
            let cz = chunk.cz;

            for lx in 0..CHUNK_SIZE {
                for ly in 0..CHUNK_SIZE {
                    for lz in 0..CHUNK_SIZE {
                        let cell = chunk.get_cell_mut(lx, ly, lz);
                        if !cell.is_alive { continue; }

                        let wx = cx * CHUNK_SIZE as i32 + lx as i32;
                        let wy = cy * CHUNK_SIZE as i32 + ly as i32;
                        let wz = cz * CHUNK_SIZE as i32 + lz as i32;

                        appliquer_loi_metabolisme(cell, wy, &self.params);
                        if cell.e <= 0.0 || cell.c > cell.sc || cell.is_nan_or_inf() {
                            cell.is_alive = false;
                            continue;
                        }

                        let mut max_e = 0.0f32;
                        for dz in -1..=1 {
                            for dy in -1..=1 {
                                for dx in -1..=1 {
                                    if dx == 0 && dy == 0 && dz == 0 { continue; }
                                    if let Some(v) = read_map.get_cell_global(wx+dx, wy+dy, wz+dz) {
                                        if v.is_alive && v.e > max_e {
                                            max_e = v.e;
                                        }
                                    }
                                }
                            }
                        }
                        cell.m = f32::max(cell.m * self.params.facteur_oubli_m, max_e);

                        let dx_b = wx as f32 - barycentre.0;
                        let dy_b = wy as f32 - barycentre.1;
                        let dz_b = wz as f32 - barycentre.2;
                        let dist = (dx_b*dx_b + dy_b*dy_b + dz_b*dz_b).sqrt();
                        cell.g = (-self.params.lambda_gradient * dist).exp();

                        if cell.t == CellType::Souche {
                            if cell.g < self.params.seuil_soma {
                                cell.t = CellType::Soma;
                            } else if cell.g >= self.params.seuil_neuro {
                                cell.t = CellType::Neurone;
                            }
                        }

                        if cell.t == CellType::Neurone {
                            let self_fired = (cell.h & 1) != 0;
                            let neighbors_coords = Self::get_neighbors(wx, wy, wz);
                            for &(_nx, _ny, _nz, w_idx) in &neighbors_coords {
                                let mut voisin_active = false;
                                if let Some(voisin) = read_map.get_cell_global(_nx, _ny, _nz) {
                                    if voisin.is_alive && voisin.t == CellType::Neurone {
                                        voisin_active = (voisin.h & 0b111) != 0;
                                    }
                                }

                                if self_fired && voisin_active {
                                    cell.w[w_idx] += self.params.learn_rate;
                                } else {
                                    cell.w[w_idx] -= self.params.learn_rate * 0.1;
                                }

                                cell.w[w_idx] *= self.params.decay_synapse;
                                cell.w[w_idx] = cell.w[w_idx].clamp(0.0, 1.0);
                            }
                        }

                        let source_index = (wx as usize ^ (wy as usize * 33) ^ (wz as usize * 1024)) + index_counter;
                        index_counter += 1;
                        if let Some(mvt) = evaluer_mouvements(wx, wy, wz, cell, &read_map, &self.params, source_index) {
                            intents.mouvements.push(mvt);
                        }

                        if let Some(div) = evaluer_division(wx, wy, wz, cell, &read_map, &self.params) {
                            intents.divisions.push(div);
                        }

                        let echanges = evaluer_osmose(wx, wy, wz, cell, &read_map, &self.params);
                        intents.echanges_energie.extend(echanges);

                        let psy = evaluer_psychisme(wx, wy, wz, cell, &read_map);
                        intents.echanges_psychiques.extend(psy);
                    }
                }
            }
            intents
        }).collect();

        // Sort chunk_intents by coordinate to guarantee determinism
        chunk_intents.sort_by(|a, b| {
            if a.chunk_key.x != b.chunk_key.x { a.chunk_key.x.cmp(&b.chunk_key.x) }
            else if a.chunk_key.y != b.chunk_key.y { a.chunk_key.y.cmp(&b.chunk_key.y) }
            else { a.chunk_key.z.cmp(&b.chunk_key.z) }
        });

        // A. Résoudre l'Osmose
        for chunk_i in &chunk_intents {
            for e in &chunk_i.echanges_energie {
                let src_alive = self.world_map.get_cell_global(e.source.0, e.source.1, e.source.2)
                    .map(|c| c.is_alive && c.t != CellType::Static).unwrap_or(false);
                let dst_alive = self.world_map.get_cell_global(e.destination.0, e.destination.1, e.destination.2)
                    .map(|c| c.is_alive && c.t != CellType::Static).unwrap_or(false);

                if src_alive && dst_alive {
                    if let Some(src_cell) = self.world_map.get_cell_global_mut(e.source.0, e.source.1, e.source.2) {
                        src_cell.e -= e.montant_energie;
                    }
                    if let Some(dst_cell) = self.world_map.get_cell_global_mut(e.destination.0, e.destination.1, e.destination.2) {
                        dst_cell.e += e.montant_energie;
                    }
                }
            }
        }

        // B. Résoudre les Mouvements
        let mut tous_mouvements = Vec::new();
        for chunk_i in &chunk_intents {
            tous_mouvements.extend(chunk_i.mouvements.clone());
        }

        tous_mouvements.sort_by(|a, b| {
            if a.destination != b.destination { a.destination.cmp(&b.destination) }
            else if (a.dette_besoin_source - b.dette_besoin_source).abs() > 0.0001 {
                b.dette_besoin_source.partial_cmp(&a.dette_besoin_source).unwrap()
            } else {
                a.source.cmp(&b.source)
            }
        });

        let mut dest_occupees = HashMap::new();
        for m in tous_mouvements {
            if dest_occupees.contains_key(&m.destination) {
                continue;
            }
            dest_occupees.insert(m.destination, m.source);
        }

        for (dest, src) in dest_occupees {
            let (cx_d, cy_d, cz_d, lx_d, ly_d, lz_d) = WorldMap::world_to_chunk_coords(dest.0, dest.1, dest.2);
            let (cx_s, cy_s, cz_s, lx_s, ly_s, lz_s) = WorldMap::world_to_chunk_coords(src.0, src.1, src.2);
            
            let cell_src = self.world_map.get_chunk(cx_s, cy_s, cz_s).map(|chk| *chk.get_cell(lx_s, ly_s, lz_s));
            if let Some(mut c) = cell_src {
                if c.is_alive {
                    c.e -= self.params.cout_mouvement;
                    
                    let chunk_dst = self.world_map.get_or_create_chunk(cx_d, cy_d, cz_d);
                    *chunk_dst.get_cell_mut(lx_d, ly_d, lz_d) = c;

                    let chunk_src = self.world_map.get_chunk_mut(cx_s, cy_s, cz_s).unwrap();
                    *chunk_src.get_cell_mut(lx_s, ly_s, lz_s) = Cell::default();
                }
            }
        }

        // C. Résoudre les Divisions
        let mut toutes_divisions = Vec::new();
        for chunk_i in &chunk_intents {
            toutes_divisions.extend(chunk_i.divisions.clone());
        }

        toutes_divisions.sort_by(|a, b| {
            if a.destination_fille != b.destination_fille { a.destination_fille.cmp(&b.destination_fille) }
            else if (a.energie_mere - b.energie_mere).abs() > 0.0001 {
                b.energie_mere.partial_cmp(&a.energie_mere).unwrap()
            } else {
                a.source_mere.cmp(&b.source_mere)
            }
        });

        let mut div_dest_occupees = HashMap::new();
        for d in toutes_divisions {
            if div_dest_occupees.contains_key(&d.destination_fille) {
                continue;
            }
            div_dest_occupees.insert(d.destination_fille, d.source_mere);
        }

        for (dest, src) in div_dest_occupees {
            let mut daughter_cell = None;

            if let Some(cell_mere) = self.world_map.get_cell_global_mut(src.0, src.1, src.2) {
                if cell_mere.is_alive && cell_mere.e > self.params.seuil_energie_division {
                    let e_fille = cell_mere.e / 2.0;
                    cell_mere.e = e_fille;

                    // Fille repart en Souche (T=0) avec variables neurales réinitialisées
                    let mutation = deterministic_mutation(dest.0, dest.1, dest.2, 0, self.current_seed);
                    let fille = Cell {
                        is_alive: true,
                        t: CellType::Souche,  // Toujours Souche — se re-différenciera
                        e: e_fille,
                        r: (cell_mere.r + mutation).clamp(0.0, 1.0),
                        sc: cell_mere.sc,
                        d: 0.0,
                        c: 0.0,
                        l: 0.0,
                        m: 0.0,
                        a: 0,
                        p: 0.0,
                        r#ref: 0,
                        e_cost: 0.0,
                        w: [0.01; 27],  // Synapses faibles par défaut
                        h: 0,
                        g: 0.0,
                    };
                    
                    daughter_cell = Some(fille);
                }
            }

            if let Some(fille) = daughter_cell {
                let (cx, cy, cz, lx, ly, lz) = WorldMap::world_to_chunk_coords(dest.0, dest.1, dest.2);
                let chunk = self.world_map.get_or_create_chunk(cx, cy, cz);
                *chunk.get_cell_mut(lx, ly, lz) = fille;
            }
        }

        // D. Résoudre le Psychisme
        for chunk_i in &chunk_intents {
            for psy in &chunk_i.echanges_psychiques {
                if let Some(src_cell) = self.world_map.get_cell_global_mut(psy.source.0, psy.source.1, psy.source.2) {
                    if src_cell.is_alive {
                        src_cell.c += psy.montant_c;
                        src_cell.l -= psy.montant_l;
                    }
                }
            }
        }

        self.world_map.chunks.par_iter_mut().for_each(|(&_key, chunk)| {
            if !chunk.has_alive_cells { return; }

            for c in &mut chunk.cells {
                if c.is_alive {
                    c.clamp_variables();
                    if c.e <= 0.0 || c.c > c.sc || c.is_nan_or_inf() {
                        c.is_alive = false;
                        *c = Cell::default();
                    }
                }
            }
            chunk.update_active_flags();
        });
    }
}

pub struct SmallRng {
    state: u64,
}

impl SmallRng {
    pub fn new(seed: u32) -> Self {
        Self {
            state: seed as u64 + 1442695040888963407,
        }
    }

    pub fn next_u32(&mut self) -> u32 {
        self.state = self.state.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
        (self.state >> 32) as u32
    }

    pub fn next_f32(&mut self) -> f32 {
        (self.next_u32() as f32) / (u32::MAX as f32)
    }
}

pub fn deterministic_noise(x: i32, y: i32, z: i32, cycle: i32, sub_tick: i32, seed: u32) -> f32 {
    let hash = (x as u32).wrapping_mul(73856093)
        ^ (y as u32).wrapping_mul(19349663)
        ^ (z as u32).wrapping_mul(83492791)
        ^ (cycle as u32).wrapping_mul(55555)
        ^ (sub_tick as u32).wrapping_mul(1234)
        ^ seed.wrapping_mul(9781);
    ((hash % 100) as f32 / 1000.0) - 0.05
}
