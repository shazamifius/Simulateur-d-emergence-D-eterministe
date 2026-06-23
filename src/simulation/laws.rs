use super::cell::{Cell, CellType};
use super::params::ParametresGlobaux;
use super::world::{
    WorldMap,
    MouvementSouhaite, DivisionSouhaitee, EchangeEnergieSouhaite, EchangePsychiqueSouhaite
};

// --- Calculs de Champs et d'Adhésion ---

#[inline]
pub fn euclidean_distance(p1: (i32, i32, i32), p2: (i32, i32, i32)) -> f32 {
    let dx = p1.0 - p2.0;
    let dy = p1.1 - p2.1;
    let dz = p1.2 - p2.2;
    ((dx * dx + dy * dy + dz * dz) as f32).sqrt()
}

pub fn calculer_champs_locaux(
    pos_v: (i32, i32, i32),
    read_map: &WorldMap,
    params: &ParametresGlobaux,
) -> (f32, f32) {
    let r_diff = params.rayon_diffusion.ceil() as i32;
    let mut sum_e = 0.0;
    let mut sum_c = 0.0;

    // On parcourt une boîte locale autour de la case vide
    for dz in -r_diff..=r_diff {
        for dy in -r_diff..=r_diff {
            for dx in -r_diff..=r_diff {
                if dx == 0 && dy == 0 && dz == 0 { continue; }
                let nx = pos_v.0 + dx;
                let ny = pos_v.1 + dy;
                let nz = pos_v.2 + dz;

                let dist = euclidean_distance(pos_v, (nx, ny, nz));
                if dist <= params.rayon_diffusion {
                    if let Some(cell) = read_map.get_cell_global(nx, ny, nz) {
                        if cell.is_alive && cell.t != CellType::Static {
                            let att = (-params.alpha_attenuation * dist).exp();
                            sum_e += cell.e * att;
                            sum_c += cell.c * att;
                        }
                    }
                }
            }
        }
    }
    (sum_e, sum_c)
}

pub fn calculer_adhesion(
    pos_v: (i32, i32, i32),
    cell_type: CellType,
    read_map: &WorldMap,
) -> f32 {
    let mut count = 0;
    for dz in -1..=1 {
        for dy in -1..=1 {
            for dx in -1..=1 {
                if dx == 0 && dy == 0 && dz == 0 { continue; }
                let nx = pos_v.0 + dx;
                let ny = pos_v.1 + dy;
                let nz = pos_v.2 + dz;
                if let Some(cell) = read_map.get_cell_global(nx, ny, nz) {
                    if cell.is_alive && cell.t == cell_type {
                        count += 1;
                    }
                }
            }
        }
    }
    count as f32
}

// --- Métabolisme ---

pub fn appliquer_loi_metabolisme(
    cell: &mut Cell,
    wy: i32,
    params: &ParametresGlobaux,
) {
    if !cell.is_alive || cell.t == CellType::Static { return; }

    // Dette et Ennui augmentent
    cell.d += params.d_per_tick;
    cell.l += params.l_per_tick;

    // Photosynthèse (si assez haut et pas Neurone)
    if cell.t != CellType::Neurone {
        let height_threshold = params.world_height as f32 * params.hauteur_soleil;
        if wy as f32 >= height_threshold {
            cell.e += params.sensibilite_soleil;
        }
    }

    // Facturation énergétique
    let cost = params.k_thermo + cell.e_cost;
    cell.e -= cost;
    cell.e_cost = 0.0; // Reset pour le prochain cycle physique

    cell.a += 1; // Âge
}

// --- Mouvement ---

pub fn evaluer_mouvements(
    wx: i32, wy: i32, wz: i32,
    cell: &Cell,
    read_map: &WorldMap,
    params: &ParametresGlobaux,
    index_source: usize,
) -> Option<MouvementSouhaite> {
    if !cell.is_alive || cell.t == CellType::Static { return None; }

    let mut best_target = None;
    let mut max_score = f32::NEG_INFINITY;

    let gravity = params.k_d * cell.d;
    let pressure = params.k_c * cell.c;
    let inertia = params.k_m * (cell.m / (cell.a + 1) as f32);

    // Evaluer les 26 voisins
    for dz in -1..=1 {
        for dy in -1..=1 {
            for dx in -1..=1 {
                if dx == 0 && dy == 0 && dz == 0 { continue; }
                let nx = wx + dx;
                let ny = wy + dy;
                let nz = wz + dz;

                // On ne sort pas des limites logiques de hauteur (bedrock à y=0)
                if ny <= 0 { continue; }

                // None = pas de chunk = espace vide libre
                let is_empty = match read_map.get_cell_global(nx, ny, nz) {
                    Some(target) => !target.is_alive,
                    None => true, // Espace libre (chunk non-existant)
                };

                if is_empty {
                    // Case vide : calcul des champs locaux et de l'adhésion
                    let (field_e, field_c) = calculer_champs_locaux((nx, ny, nz), read_map, params);
                    let bonus_champ = (params.k_champ_e * field_e) - (params.k_champ_c * field_c);
                    let adhesion = calculer_adhesion((nx, ny, nz), cell.t, read_map);
                    let bonus_adh = params.k_adh * adhesion;

                    let score = gravity - pressure + inertia + bonus_champ + bonus_adh - params.cout_mouvement;
                    if score > max_score {
                        max_score = score;
                        best_target = Some((nx, ny, nz));
                    }
                }
            }
        }
    }

    best_target.map(|dest| MouvementSouhaite {
        source: (wx, wy, wz),
        destination: dest,
        dette_besoin_source: cell.d,
        index_source,
    })
}

// --- Division ---

pub fn evaluer_division(
    wx: i32, wy: i32, wz: i32,
    cell: &Cell,
    read_map: &WorldMap,
    params: &ParametresGlobaux,
) -> Option<DivisionSouhaitee> {
    if !cell.is_alive || cell.t == CellType::Static || cell.e <= params.seuil_energie_division {
        return None;
    }

    // Trouver le premier voisin vide pour se diviser
    for dz in -1..=1 {
        for dy in -1..=1 {
            for dx in -1..=1 {
                if dx == 0 && dy == 0 && dz == 0 { continue; }
                let nx = wx + dx;
                let ny = wy + dy;
                let nz = wz + dz;
                if ny <= 0 { continue; }

                // None = pas de chunk = espace vide libre
                let is_empty = match read_map.get_cell_global(nx, ny, nz) {
                    Some(target) => !target.is_alive,
                    None => true,
                };

                if is_empty {
                    return Some(DivisionSouhaitee {
                        source_mere: (wx, wy, wz),
                        destination_fille: (nx, ny, nz),
                        energie_mere: cell.e,
                    });
                }
            }
        }
    }
    None
}

// --- Osmose (Échange d'énergie) ---

pub fn evaluer_osmose(
    wx: i32, wy: i32, wz: i32,
    cell: &Cell,
    read_map: &WorldMap,
    params: &ParametresGlobaux,
) -> Vec<EchangeEnergieSouhaite> {
    let mut echanges = Vec::new();
    if !cell.is_alive || cell.t == CellType::Static { return echanges; }

    let t_src = (wx, wy, wz);

    for dz in -1..=1 {
        for dy in -1..=1 {
            for dx in -1..=1 {
                if dx == 0 && dy == 0 && dz == 0 { continue; }
                let nx = wx + dx;
                let ny = wy + dy;
                let nz = wz + dz;
                let t_tgt = (nx, ny, nz);

                // Unicité des paires : on traite uniquement si ID(source) < ID(voisin)
                if t_src < t_tgt {
                    if let Some(target) = read_map.get_cell_global(nx, ny, nz) {
                        if target.is_alive && target.t != CellType::Static {
                            // Compatibilité génétique R
                            if (cell.r - target.r).abs() < params.seuil_similarite_r {
                                let delta = (cell.e - target.e) * params.facteur_echange_energie;
                                let delta_safe = delta.clamp(-params.max_flux_energie, params.max_flux_energie);
                                if delta_safe.abs() > 0.0001 {
                                    echanges.push(EchangeEnergieSouhaite {
                                        source: t_src,
                                        destination: t_tgt,
                                        montant_energie: delta_safe,
                                    });
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    echanges
}

// --- Interaction Forte (Psychisme) ---

pub fn evaluer_psychisme(
    wx: i32, wy: i32, wz: i32,
    cell: &Cell,
    read_map: &WorldMap,
) -> Vec<EchangePsychiqueSouhaite> {
    let mut echanges = Vec::new();
    if !cell.is_alive || cell.t == CellType::Static { return echanges; }

    for dz in -1..=1 {
        for dy in -1..=1 {
            for dx in -1..=1 {
                if dx == 0 && dy == 0 && dz == 0 { continue; }
                let nx = wx + dx;
                let ny = wy + dy;
                let nz = wz + dz;
                if let Some(target) = read_map.get_cell_global(nx, ny, nz) {
                    if target.is_alive && target.t != CellType::Static {
                        let dc = 0.1 * target.c; // Friction sociale
                        let dl = 0.1 * target.l; // Satisfaction sociale
                        echanges.push(EchangePsychiqueSouhaite {
                            source: (wx, wy, wz),
                            destination: (nx, ny, nz),
                            montant_c: dc,
                            montant_l: dl,
                        });
                    }
                }
            }
        }
    }
    echanges
}

// --- Hachage Déterministe pour Mutation ---

pub fn deterministic_mutation(x: i32, y: i32, z: i32, age: i32, seed: u32) -> f32 {
    let hash = (x as u32).wrapping_mul(18397)
        .wrapping_add((y as u32).wrapping_mul(20441))
        .wrapping_add((z as u32).wrapping_mul(22543))
        .wrapping_add((age as u32).wrapping_mul(24671))
        ^ seed.wrapping_mul(34567);
    match hash % 3 {
        0 => 0.01,
        1 => -0.01,
        _ => 0.0,
    }
}
