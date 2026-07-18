use macroquad::prelude::*;
use rust_sed::simulation::cell::CellType;
use rust_sed::simulation::chunk::CHUNK_SIZE;
use rust_sed::simulation::world::MondeSED;
use serde::{Deserialize, Serialize};

// ===========================================================================
// Configuration de la fenêtre macroquad
// ===========================================================================
fn window_conf() -> Conf {
    Conf {
        window_title: "SED — Simulateur d'Émergence Déterministe".to_string(),
        window_width: 1280,
        window_height: 720,
        fullscreen: false,
        ..Default::default()
    }
}

// ===========================================================================
// Structures d'action pour le Replay et Fichiers Scénarios
// ===========================================================================

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct SimulationAction {
    pub cycle: i32,
    pub action_type: String,
    pub target: String,
    pub val1: f32,
    pub val2: f32,
    pub val3: f32,
    pub val4: f32,
}

#[derive(Serialize, Deserialize, Clone, Debug, Default)]
pub struct ReplaySystem {
    pub is_recording: bool,
    pub is_replaying: bool,
    pub actions: Vec<SimulationAction>,
    pub playback_index: usize,
}

impl ReplaySystem {
    pub fn record_action(&mut self, action: SimulationAction) {
        if self.is_recording {
            self.actions.push(action);
        }
    }

    pub fn save_recording(&self, path: &str) -> std::io::Result<()> {
        let json = serde_json::to_string_pretty(&self.actions)?;
        std::fs::write(path, json)?;
        Ok(())
    }

    pub fn load_recording(&mut self, path: &str) -> Result<(), Box<dyn std::error::Error>> {
        let content = std::fs::read_to_string(path)?;
        let actions: Vec<SimulationAction> = serde_json::from_str(&content)?;
        self.actions = actions;
        self.actions.sort_by_key(|a| a.cycle);
        self.playback_index = 0;
        self.is_replaying = true;
        Ok(())
    }
}

#[derive(Serialize, Deserialize)]
struct ScenarioCell {
    x: i32,
    y: i32,
    z: i32,
    r#type: u8,
    energy: f32,
    resistance: Option<f32>,
}

#[derive(Serialize, Deserialize)]
struct ScenarioFile {
    name: String,
    description: String,
    parameters: Option<serde_json::Value>,
    cells: Vec<ScenarioCell>,
}

// ===========================================================================
// Fonctions utilitaires de Fichiers
// ===========================================================================

fn list_scenarios() -> Vec<String> {
    let mut list = Vec::new();
    let _ = std::fs::create_dir_all("scenarios"); // ensure dir exists
    if let Ok(entries) = std::fs::read_dir("scenarios") {
        for entry in entries.flatten() {
            if let Some(ext) = entry.path().extension()
                && ext == "json"
                && let Some(name) = entry.path().file_name()
            {
                list.push(name.to_string_lossy().to_string());
            }
        }
    }
    list
}

fn charger_scenario(monde: &mut MondeSED, path: &str) -> Result<(), Box<dyn std::error::Error>> {
    let content = std::fs::read_to_string(path)?;
    let scenario: ScenarioFile = serde_json::from_str(&content)?;

    monde.world_map.clear();
    monde.read_map.clear();
    monde.cycle_actuel = 0;

    // (Plancher de Bedrock implicite à Y=0 géré globalement)

    // Placement
    for cell in scenario.cells {
        let (cx, cy, cz, lx, ly, lz) =
            rust_sed::simulation::world::WorldMap::world_to_chunk_coords(cell.x, cell.y, cell.z);
        let chunk = monde.world_map.get_or_create_chunk(cx, cy, cz);
        let c = chunk.get_cell_mut(lx, ly, lz);

        c.is_alive = true;
        c.e = cell.energy;
        c.sc = cell.resistance.unwrap_or(0.5);
        c.t = match cell.r#type {
            1 => CellType::Soma,
            2 => CellType::Neurone,
            3 => CellType::Static,
            _ => CellType::Souche,
        };
    }

    for chunk in monde.world_map.chunks.values_mut() {
        chunk.update_active_flags();
    }

    Ok(())
}

fn save_snapshot(monde: &MondeSED, path: &str) -> Result<(), Box<dyn std::error::Error>> {
    let json = serde_json::to_string_pretty(monde)?;
    std::fs::write(path, json)?;
    Ok(())
}

fn load_snapshot(monde: &mut MondeSED, path: &str) -> Result<(), Box<dyn std::error::Error>> {
    let content = std::fs::read_to_string(path)?;
    let loaded: MondeSED = serde_json::from_str(&content)?;
    *monde = loaded;
    Ok(())
}

// ===========================================================================
// Logique Mathématique de Raycasting 3D
// ===========================================================================

fn ray_aabb_intersection(
    ray_origin: Vec3,
    ray_dir: Vec3,
    aabb_min: Vec3,
    aabb_max: Vec3,
) -> Option<(f32, Vec3)> {
    let mut tmin = f32::NEG_INFINITY;
    let mut tmax = f32::INFINITY;
    let mut normal = Vec3::ZERO;

    if ray_dir.x.abs() > 0.000001 {
        let t1 = (aabb_min.x - ray_origin.x) / ray_dir.x;
        let t2 = (aabb_max.x - ray_origin.x) / ray_dir.x;
        let (t_axis_min, t_axis_max, norm_axis) = if t1 < t2 {
            (t1, t2, vec3(-1.0, 0.0, 0.0))
        } else {
            (t2, t1, vec3(1.0, 0.0, 0.0))
        };
        if t_axis_min > tmin {
            tmin = t_axis_min;
            normal = norm_axis;
        }
        tmax = tmax.min(t_axis_max);
    } else if ray_origin.x < aabb_min.x || ray_origin.x > aabb_max.x {
        return None;
    }

    if ray_dir.y.abs() > 0.000001 {
        let t1 = (aabb_min.y - ray_origin.y) / ray_dir.y;
        let t2 = (aabb_max.y - ray_origin.y) / ray_dir.y;
        let (t_axis_min, t_axis_max, norm_axis) = if t1 < t2 {
            (t1, t2, vec3(0.0, -1.0, 0.0))
        } else {
            (t2, t1, vec3(0.0, 1.0, 0.0))
        };
        if t_axis_min > tmin {
            tmin = t_axis_min;
            normal = norm_axis;
        }
        tmax = tmax.min(t_axis_max);
    } else if ray_origin.y < aabb_min.y || ray_origin.y > aabb_max.y {
        return None;
    }

    if ray_dir.z.abs() > 0.000001 {
        let t1 = (aabb_min.z - ray_origin.z) / ray_dir.z;
        let t2 = (aabb_max.z - ray_origin.z) / ray_dir.z;
        let (t_axis_min, t_axis_max, norm_axis) = if t1 < t2 {
            (t1, t2, vec3(0.0, 0.0, -1.0))
        } else {
            (t2, t1, vec3(0.0, 0.0, 1.0))
        };
        if t_axis_min > tmin {
            tmin = t_axis_min;
            normal = norm_axis;
        }
        tmax = tmax.min(t_axis_max);
    } else if ray_origin.z < aabb_min.z || ray_origin.z > aabb_max.z {
        return None;
    }

    if tmin <= tmax && tmax >= 0.0 {
        Some((tmin, normal))
    } else {
        None
    }
}

fn get_mouse_ray(camera: &Camera3D) -> (Vec3, Vec3) {
    let view_proj = camera.matrix();
    let inv_view_proj = view_proj.inverse();

    let (mouse_x, mouse_y) = mouse_position();
    let ndc_x = (2.0 * mouse_x) / screen_width() - 1.0;
    let ndc_y = 1.0 - (2.0 * mouse_y) / screen_height();

    let near_point_ndc = vec4(ndc_x, ndc_y, -1.0, 1.0);
    let near_point_world_homo = inv_view_proj * near_point_ndc;
    let near_point_world = near_point_world_homo.xyz() / near_point_world_homo.w;

    let far_point_ndc = vec4(ndc_x, ndc_y, 1.0, 1.0);
    let far_point_world_homo = inv_view_proj * far_point_ndc;
    let far_point_world = far_point_world_homo.xyz() / far_point_world_homo.w;

    let ray_direction = (far_point_world - near_point_world).normalize();
    let ray_origin = camera.position;

    (ray_origin, ray_direction)
}

fn raycast_world(
    state: &AppState,
    ray_origin: Vec3,
    ray_dir: Vec3,
) -> Option<(i32, i32, i32, Vec3)> {
    let mut min_t = f32::INFINITY;
    let mut best_hit = None;
    let scale = state.cell_scale;

    for chunk in state.monde.world_map.chunks.values() {
        if !chunk.has_alive_cells {
            continue;
        }

        for lx in 0..CHUNK_SIZE {
            for ly in 0..CHUNK_SIZE {
                for lz in 0..CHUNK_SIZE {
                    let cell = chunk.get_cell(lx, ly, lz);
                    if !cell.is_alive {
                        continue;
                    }

                    match cell.t {
                        CellType::Souche if !state.show_souche => continue,
                        CellType::Soma if !state.show_soma => continue,
                        CellType::Neurone if !state.show_neurone => continue,
                        CellType::Static if !state.show_static => continue,
                        _ => {}
                    }

                    let wx = (chunk.cx * CHUNK_SIZE as i32 + lx as i32) as f32;
                    let wy = (chunk.cy * CHUNK_SIZE as i32 + ly as i32) as f32;
                    let wz = (chunk.cz * CHUNK_SIZE as i32 + lz as i32) as f32;

                    let aabb_min = vec3(wx - scale / 2.0, wy - scale / 2.0, wz - scale / 2.0);
                    let aabb_max = vec3(wx + scale / 2.0, wy + scale / 2.0, wz + scale / 2.0);

                    if let Some((t, normal)) =
                        ray_aabb_intersection(ray_origin, ray_dir, aabb_min, aabb_max)
                        && t < min_t
                    {
                        min_t = t;
                        best_hit = Some((
                            chunk.cx * CHUNK_SIZE as i32 + lx as i32,
                            chunk.cy * CHUNK_SIZE as i32 + ly as i32,
                            chunk.cz * CHUNK_SIZE as i32 + lz as i32,
                            normal,
                        ));
                    }
                }
            }
        }
    }

    best_hit
}

// ===========================================================================
// Structures d'état de l'application
// ===========================================================================

struct AppState {
    monde: MondeSED,
    is_running: bool,
    speed: f32,        // Cycles par seconde
    accum: f32,        // Accumulateur de temps
    cam_yaw: f32,      // Rotation horizontale (degrés)
    cam_pitch: f32,    // Rotation verticale (degrés)
    cam_distance: f32, // Distance de la caméra
    cam_target: Vec3,  // Point visé
    show_gui: bool,
    render_mode: RenderMode,
    seed: u32,
    world_size: i32,
    initial_density: f32,
    // Statistiques
    pop_history: Vec<f32>,
    energy_history: Vec<f32>,
    last_fps: f32,
    mouse_captured: bool,
    last_mouse: Vec2,
    mouse_on_gui: bool,
    // Filtrage visuel
    show_souche: bool,
    show_soma: bool,
    show_neurone: bool,
    show_static: bool,
    cell_scale: f32,

    // Édition et Pinceau
    brush_mode: usize, // 0 = Inspect/View, 1 = Soma, 2 = Souche, 3 = Neurone, 4 = Delete
    brush_tool: usize, // 0 = Remplacer/Modifier, 1 = Ajouter Voisin
    selected_cell: Option<(i32, i32, i32)>,

    // Fichiers et Scénarios
    scenarios_list: Vec<String>,
    selected_scenario: String,

    // Monitoring CSV
    csv_recording: bool,
    csv_filename: String,
    csv_file: Option<std::fs::File>,

    // Replay
    replay: ReplaySystem,

    // Pre-loading / Replay Cache
    preloaded_states: Vec<MondeSED>,
    preload_index: usize,
    is_in_preload_mode: bool,
    preload_cycles_count: i32,
    preload_is_playing: bool,
}

#[derive(PartialEq, Clone, Copy)]
enum RenderMode {
    Type,
    Energy,
    Stress,
    Gradient,
    Potential,
}

impl AppState {
    fn new() -> Self {
        let size = 24;
        let seed = 42;
        let density = 0.12;
        let mut monde = MondeSED::new(size, size, size);
        monde.initialiser_monde(seed, density);

        let scenarios = list_scenarios();
        let selected_sc = if !scenarios.is_empty() {
            scenarios[0].clone()
        } else {
            "".to_string()
        };

        Self {
            monde,
            is_running: false,
            speed: 2.0,
            accum: 0.0,
            cam_yaw: 45.0,
            cam_pitch: 40.0,
            cam_distance: 50.0,
            cam_target: vec3(size as f32 / 2.0, size as f32 / 4.0, size as f32 / 2.0),
            show_gui: true,
            render_mode: RenderMode::Type,
            seed,
            world_size: size,
            initial_density: density,
            pop_history: Vec::new(),
            energy_history: Vec::new(),
            last_fps: 60.0,
            mouse_captured: false,
            last_mouse: Vec2::ZERO,
            mouse_on_gui: false,
            show_souche: true,
            show_soma: true,
            show_neurone: true,
            show_static: false,
            cell_scale: 0.4,

            brush_mode: 0,
            brush_tool: 0,
            selected_cell: None,
            scenarios_list: scenarios,
            selected_scenario: selected_sc,
            csv_recording: false,
            csv_filename: "metrics_log.csv".to_string(),
            csv_file: None,
            replay: ReplaySystem::default(),

            preloaded_states: Vec::new(),
            preload_index: 0,
            is_in_preload_mode: false,
            preload_cycles_count: 800,
            preload_is_playing: false,
        }
    }

    fn reset(&mut self) {
        self.monde = MondeSED::new(self.world_size, self.world_size, self.world_size);
        self.monde
            .initialiser_monde(self.seed, self.initial_density);
        self.pop_history.clear();
        self.energy_history.clear();
        self.accum = 0.0;
        self.cam_target = vec3(
            self.world_size as f32 / 2.0,
            self.world_size as f32 / 4.0,
            self.world_size as f32 / 2.0,
        );
        self.selected_cell = None;
        self.replay.actions.clear();
        self.replay.playback_index = 0;
        self.replay.is_replaying = false;
        self.replay.is_recording = false;

        self.preloaded_states.clear();
        self.preload_index = 0;
        self.is_in_preload_mode = false;
        self.preload_is_playing = false;
    }

    fn record_param_change(&mut self, name: &str, value: f32) {
        if self.replay.is_recording {
            self.replay.record_action(SimulationAction {
                cycle: self.monde.cycle_actuel,
                action_type: "PARAM".to_string(),
                target: name.to_string(),
                val1: value,
                val2: 0.0,
                val3: 0.0,
                val4: 0.0,
            });
        }
    }

    fn update_stats(&mut self) {
        let pop = self.monde.get_nombre_cellules_vivantes() as f32;
        let total_energy: f32 = self
            .monde
            .world_map
            .chunks
            .values()
            .flat_map(|chk| chk.cells.iter())
            .filter(|c| c.is_alive)
            .map(|c| c.e)
            .sum();
        self.pop_history.push(pop);
        self.energy_history.push(total_energy);
        if self.pop_history.len() > 500 {
            self.pop_history.remove(0);
            self.energy_history.remove(0);
        }
    }
}

// ===========================================================================
// Couleurs pour les cellules
// ===========================================================================

fn color_for_cell(cell: &rust_sed::simulation::cell::Cell, mode: RenderMode) -> Color {
    match mode {
        RenderMode::Type => match cell.t {
            CellType::Souche => Color::new(0.2, 0.8, 0.3, 0.85),
            CellType::Soma => Color::new(0.9, 0.6, 0.1, 0.85),
            CellType::Neurone => Color::new(0.3, 0.5, 1.0, 0.85),
            CellType::Static => Color::new(0.4, 0.4, 0.4, 0.4),
        },
        RenderMode::Energy => {
            let t = (cell.e / 2.0).clamp(0.0, 1.0);
            Color::new(1.0 - t, t, 0.2, 0.85)
        }
        RenderMode::Stress => {
            let t = cell.c.clamp(0.0, 1.0);
            Color::new(t, 0.2, 1.0 - t, 0.85)
        }
        RenderMode::Gradient => {
            let t = cell.g.clamp(0.0, 1.0);
            Color::new(t, t * 0.5, 1.0 - t, 0.85)
        }
        RenderMode::Potential => {
            let t = ((cell.p + 1.0) / 2.0).clamp(0.0, 1.0);
            Color::new(1.0, t, 1.0 - t, 0.85)
        }
    }
}

// ===========================================================================
// Fonctions d'interaction (Pinceau, Replay et Logs)
// ===========================================================================

fn apply_brush_action(state: &mut AppState, coords: (i32, i32, i32)) {
    let (x, y, z) = coords;
    if y <= 0 {
        return;
    }

    let (cx, cy, cz, lx, ly, lz) =
        rust_sed::simulation::world::WorldMap::world_to_chunk_coords(x, y, z);

    let chunk = state.monde.world_map.get_or_create_chunk(cx, cy, cz);
    let cell = chunk.get_cell_mut(lx, ly, lz);

    if state.brush_mode == 4 {
        *cell = rust_sed::simulation::cell::Cell::default();
    } else {
        cell.is_alive = true;
        cell.e = 5.0;
        cell.r = 0.5;
        cell.sc = 0.5;
        cell.t = match state.brush_mode {
            1 => CellType::Soma,
            2 => CellType::Souche,
            3 => CellType::Neurone,
            _ => CellType::Souche,
        };
    }
    chunk.update_active_flags();

    if state.replay.is_recording {
        state.replay.record_action(SimulationAction {
            cycle: state.monde.cycle_actuel,
            action_type: "BRUSH".to_string(),
            target: "".to_string(),
            val1: x as f32,
            val2: y as f32,
            val3: z as f32,
            val4: state.brush_mode as f32,
        });
    }
}

fn handle_interaction(state: &mut AppState, camera: &Camera3D) {
    if !state.mouse_on_gui && is_mouse_button_pressed(MouseButton::Left) {
        let (ray_origin, ray_direction) = get_mouse_ray(camera);
        if let Some((x, y, z, normal)) = raycast_world(state, ray_origin, ray_direction) {
            if state.brush_mode == 0 {
                state.selected_cell = Some((x, y, z));
            } else {
                let target_coords = if state.brush_tool == 1 {
                    (
                        x + normal.x as i32,
                        y + normal.y as i32,
                        z + normal.z as i32,
                    )
                } else {
                    (x, y, z)
                };
                apply_brush_action(state, target_coords);
            }
        }
    }
}

fn play_replay_actions(state: &mut AppState) {
    if !state.replay.is_replaying {
        return;
    }

    let current_cycle = state.monde.cycle_actuel;

    while state.replay.playback_index < state.replay.actions.len() {
        let act = &state.replay.actions[state.replay.playback_index];
        if act.cycle == current_cycle {
            if act.action_type == "PARAM" {
                let val1 = act.val1;
                match act.target.as_str() {
                    "k_thermo" => state.monde.params.k_thermo = val1,
                    "sensibilite_soleil" => state.monde.params.sensibilite_soleil = val1,
                    "hauteur_soleil" => state.monde.params.hauteur_soleil = val1,
                    "seuil_energie_division" => state.monde.params.seuil_energie_division = val1,
                    "cout_mouvement" => state.monde.params.cout_mouvement = val1,
                    "facteur_echange_energie" => state.monde.params.facteur_echange_energie = val1,
                    "seuil_similarite_r" => state.monde.params.seuil_similarite_r = val1,
                    "ticks_neuraux_par_physique" => {
                        state.monde.params.ticks_neuraux_par_physique = val1 as usize
                    }
                    "seuil_fire" => state.monde.params.seuil_fire = val1,
                    "cout_spike" => state.monde.params.cout_spike = val1,
                    "learn_rate" => state.monde.params.learn_rate = val1,
                    "decay_synapse" => state.monde.params.decay_synapse = val1,
                    _ => {}
                }
            } else if act.action_type == "BRUSH" {
                let x = act.val1 as i32;
                let y = act.val2 as i32;
                let z = act.val3 as i32;
                let mode = act.val4 as usize;

                let (cx, cy, cz, lx, ly, lz) =
                    rust_sed::simulation::world::WorldMap::world_to_chunk_coords(x, y, z);
                let chunk = state.monde.world_map.get_or_create_chunk(cx, cy, cz);
                let cell = chunk.get_cell_mut(lx, ly, lz);

                if mode == 4 {
                    *cell = rust_sed::simulation::cell::Cell::default();
                } else {
                    cell.is_alive = true;
                    cell.e = 5.0;
                    cell.r = 0.5;
                    cell.sc = 0.5;
                    cell.t = match mode {
                        1 => CellType::Soma,
                        2 => CellType::Souche,
                        3 => CellType::Neurone,
                        _ => CellType::Souche,
                    };
                }
                chunk.update_active_flags();
            }
            state.replay.playback_index += 1;
        } else if act.cycle < current_cycle {
            state.replay.playback_index += 1;
        } else {
            break;
        }
    }

    if state.replay.playback_index >= state.replay.actions.len() {
        state.replay.is_replaying = false;
        println!("[Replay] Playback terminé.");
    }
}

fn handle_csv_logging(state: &mut AppState) {
    if !state.csv_recording {
        state.csv_file = None;
        return;
    }

    if state.csv_file.is_none() {
        let path = std::path::Path::new(&state.csv_filename);
        if let Ok(file) = std::fs::File::create(path) {
            state.csv_file = Some(file);
            use std::io::Write;
            if let Some(ref mut f) = state.csv_file {
                let _ = writeln!(
                    f,
                    "Cycle,Total_Energy,Pop_Count,Souche_Count,Soma_Count,Neurone_Count,Mean_Stress,Spike_Rate"
                );
            }
        }
    }

    if let Some(ref mut f) = state.csv_file {
        let pop = state.monde.get_nombre_cellules_vivantes();
        let total_e: f32 = state
            .monde
            .world_map
            .chunks
            .values()
            .flat_map(|chk| chk.cells.iter())
            .filter(|c| c.is_alive)
            .map(|c| c.e)
            .sum();

        let mut n_souche = 0;
        let mut n_soma = 0;
        let mut n_neurone = 0;
        let mut sum_stress = 0.0;
        let mut sum_spike = 0;
        let mut active_neurons = 0;

        for chunk in state.monde.world_map.chunks.values() {
            for c in &chunk.cells {
                if c.is_alive {
                    match c.t {
                        CellType::Souche => n_souche += 1,
                        CellType::Soma => n_soma += 1,
                        CellType::Neurone => {
                            n_neurone += 1;
                            active_neurons += 1;
                            if (c.h & 1) != 0 {
                                sum_spike += 1;
                            }
                        }
                        CellType::Static => {}
                    }
                    sum_stress += c.c;
                }
            }
        }

        let mean_stress = if pop > 0 {
            sum_stress / pop as f32
        } else {
            0.0
        };
        let spike_rate = if active_neurons > 0 {
            sum_spike as f32 / active_neurons as f32
        } else {
            0.0
        };

        use std::io::Write;
        let _ = writeln!(
            f,
            "{},{},{},{},{},{},{},{}",
            state.monde.cycle_actuel,
            total_e,
            pop,
            n_souche,
            n_soma,
            n_neurone,
            mean_stress,
            spike_rate
        );
    }
}

// ===========================================================================
// Rendu 3D des cellules
// ===========================================================================

fn render_cells(state: &AppState) {
    let scale = state.cell_scale;

    for chunk in state.monde.world_map.chunks.values() {
        if !chunk.has_alive_cells {
            continue;
        }

        for lx in 0..CHUNK_SIZE {
            for ly in 0..CHUNK_SIZE {
                for lz in 0..CHUNK_SIZE {
                    let cell = chunk.get_cell(lx, ly, lz);
                    if !cell.is_alive {
                        continue;
                    }

                    match cell.t {
                        CellType::Souche if !state.show_souche => continue,
                        CellType::Soma if !state.show_soma => continue,
                        CellType::Neurone if !state.show_neurone => continue,
                        CellType::Static if !state.show_static => continue,
                        _ => {}
                    }

                    let wx_i = chunk.cx * CHUNK_SIZE as i32 + lx as i32;
                    let wy_i = chunk.cy * CHUNK_SIZE as i32 + ly as i32;
                    let wz_i = chunk.cz * CHUNK_SIZE as i32 + lz as i32;

                    // Occlusion Check : évite de dessiner les cellules complètement entourées
                    let n_up = state.monde.world_map.get_cell_global(wx_i, wy_i + 1, wz_i);
                    let n_down = state.monde.world_map.get_cell_global(wx_i, wy_i - 1, wz_i);
                    let n_left = state.monde.world_map.get_cell_global(wx_i - 1, wy_i, wz_i);
                    let n_right = state.monde.world_map.get_cell_global(wx_i + 1, wy_i, wz_i);
                    let n_front = state.monde.world_map.get_cell_global(wx_i, wy_i, wz_i + 1);
                    let n_back = state.monde.world_map.get_cell_global(wx_i, wy_i, wz_i - 1);

                    let is_occluded = n_up.map_or(false, |c| c.is_alive)
                        && n_down.map_or(false, |c| c.is_alive)
                        && n_left.map_or(false, |c| c.is_alive)
                        && n_right.map_or(false, |c| c.is_alive)
                        && n_front.map_or(false, |c| c.is_alive)
                        && n_back.map_or(false, |c| c.is_alive);

                    if is_occluded {
                        continue;
                    }

                    let wx = wx_i as f32;
                    let wy = wy_i as f32;
                    let wz = wz_i as f32;

                    let col = color_for_cell(cell, state.render_mode);

                    draw_cube(vec3(wx, wy, wz), vec3(scale, scale, scale), None, col);
                    draw_cube_wires(
                        vec3(wx, wy, wz),
                        vec3(scale, scale, scale),
                        Color::new(col.r * 0.6, col.g * 0.6, col.b * 0.6, 0.4),
                    );
                }
            }
        }
    }

    // Rendu dynamique du plancher de bedrock implicite à Y=0 sous les colonnes actives
    if state.show_static {
        let mut active_cols = std::collections::HashSet::new();
        for key in state.monde.world_map.chunks.keys() {
            if let Some(chunk) = state.monde.world_map.chunks.get(key)
                && chunk.has_alive_cells
            {
                active_cols.insert((key.x, key.z));
            }
        }
        for (cx, cz) in active_cols {
            for lx in 0..CHUNK_SIZE {
                for lz in 0..CHUNK_SIZE {
                    let wx = (cx * CHUNK_SIZE as i32 + lx as i32) as f32;
                    let wz = (cz * CHUNK_SIZE as i32 + lz as i32) as f32;
                    let col = Color::new(0.4, 0.4, 0.4, 0.4);
                    draw_cube(vec3(wx, 0.0, wz), vec3(scale, scale, scale), None, col);
                    draw_cube_wires(
                        vec3(wx, 0.0, wz),
                        vec3(scale, scale, scale),
                        Color::new(col.r * 0.6, col.g * 0.6, col.b * 0.6, 0.4),
                    );
                }
            }
        }
    }

    if let Some((x, y, z)) = state.selected_cell {
        draw_cube_wires(
            vec3(x as f32, y as f32, z as f32),
            vec3(scale * 1.1, scale * 1.1, scale * 1.1),
            Color::new(1.0, 1.0, 1.0, 1.0),
        );
    }
}

// ===========================================================================
// Rendu de la grille de référence (Dynamique & Infinie)
// ===========================================================================

fn render_grid(monde: &MondeSED, size: i32) {
    let b = monde.calculer_barycentre();
    // Arrondir le centre de la grille à la taille de bloc pour une stabilité visuelle
    let cx = b.0.round() as i32;
    let cz = b.2.round() as i32;

    let half_size = size / 2;
    let grid_color = Color::new(0.3, 0.3, 0.3, 0.3);

    let start_x = cx - half_size;
    let end_x = cx + half_size;
    let start_z = cz - half_size;
    let end_z = cz + half_size;

    for i in start_x..=end_x {
        draw_line_3d(
            vec3(i as f32, 0.0, start_z as f32),
            vec3(i as f32, 0.0, end_z as f32),
            grid_color,
        );
    }
    for i in start_z..=end_z {
        draw_line_3d(
            vec3(start_x as f32, 0.0, i as f32),
            vec3(end_x as f32, 0.0, i as f32),
            grid_color,
        );
    }

    // Repères des axes à l'origine (0, 0, 0)
    let axis_len = (size as f32 * 0.3).min(10.0);
    draw_line_3d(vec3(0.0, 0.0, 0.0), vec3(axis_len, 0.0, 0.0), RED);
    draw_line_3d(vec3(0.0, 0.0, 0.0), vec3(0.0, axis_len, 0.0), GREEN);
    draw_line_3d(vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, axis_len), BLUE);
}

// ===========================================================================
// Mini graphique custom avec egui Painter
// ===========================================================================

fn draw_sparkline(ui: &mut egui::Ui, data: &[f32], color: egui::Color32, label: &str) {
    if data.is_empty() {
        return;
    }

    ui.label(label);
    let (response, painter) =
        ui.allocate_painter(egui::vec2(ui.available_width(), 50.0), egui::Sense::hover());
    let rect = response.rect;

    // Background
    painter.rect_filled(rect, 2.0, egui::Color32::from_gray(30));

    let max_val = data.iter().cloned().fold(1.0f32, f32::max);
    let n = data.len();
    if n < 2 {
        return;
    }

    let points: Vec<egui::Pos2> = data
        .iter()
        .enumerate()
        .map(|(i, &v)| {
            let x = rect.left() + (i as f32 / (n - 1) as f32) * rect.width();
            let y = rect.bottom() - (v / max_val) * rect.height();
            egui::pos2(x, y)
        })
        .collect();

    for pair in points.windows(2) {
        painter.line_segment([pair[0], pair[1]], egui::Stroke::new(1.5, color));
    }

    // Dernière valeur affichée
    if let Some(&last) = data.last() {
        painter.text(
            egui::pos2(rect.right() - 2.0, rect.top() + 2.0),
            egui::Align2::RIGHT_TOP,
            format!("{:.0}", last),
            egui::FontId::proportional(10.0),
            egui::Color32::WHITE,
        );
    }
}

// ===========================================================================
// Interface utilisateur avec egui
// ===========================================================================

fn draw_gui(state: &mut AppState) {
    egui_macroquad::ui(|egui_ctx| {
        state.mouse_on_gui = egui_ctx.wants_pointer_input() || egui_ctx.is_pointer_over_area();

        egui::SidePanel::left("control_panel")
            .default_width(300.0)
            .show(egui_ctx, |ui| {
                ui.heading("🧬 SED — Panneau de Contrôle");
                ui.separator();

                // --- Simulation ---
                ui.collapsing("⚙ Simulation", |ui| {
                    ui.horizontal(|ui| {
                        if ui
                            .button(if state.is_running {
                                "⏸ Pause"
                            } else {
                                "▶ Lancer"
                            })
                            .clicked()
                        {
                            state.is_running = !state.is_running;
                        }
                        if ui.button("⏩ +1 Cycle").clicked() {
                            avancer_un_cycle(state);
                        }
                    });

                    ui.add(egui::Slider::new(&mut state.speed, 0.1..=30.0).text("Vitesse (Hz)"));
                    ui.label(format!("Cycle: {}", state.monde.cycle_actuel));
                    ui.label(format!("FPS: {:.0}", state.last_fps));

                    ui.separator();
                    ui.label("Nouveau Monde:");
                    ui.add(egui::Slider::new(&mut state.seed, 0..=9999).text("Seed"));
                    ui.add(egui::Slider::new(&mut state.world_size, 8..=64).text("Taille"));
                    ui.add(
                        egui::Slider::new(&mut state.initial_density, 0.01..=0.5).text("Densité"),
                    );
                    if ui.button("🔄 Réinitialiser").clicked() {
                        state.reset();
                    }
                });

                // --- Pinceau & Édition ---
                ui.collapsing("🖌 Édition & Pinceau", |ui| {
                    ui.label("Mode pinceau :");
                    ui.radio_value(&mut state.brush_mode, 0, "🔍 Inspecter / Sélectionner");
                    ui.radio_value(&mut state.brush_mode, 1, "🟠 Placer Soma");
                    ui.radio_value(&mut state.brush_mode, 2, "🟢 Placer Souche");
                    ui.radio_value(&mut state.brush_mode, 3, "🔵 Placer Neurone");
                    ui.radio_value(&mut state.brush_mode, 4, "❌ Supprimer (Gomme)");

                    if state.brush_mode > 0 && state.brush_mode < 4 {
                        ui.separator();
                        ui.label("Outil :");
                        ui.radio_value(&mut state.brush_tool, 0, "Remplacer");
                        ui.radio_value(&mut state.brush_tool, 1, "Ajouter sur la face cliquée");
                    }
                });

                // --- Inspecteur de cellule ---
                ui.collapsing("🔍 Inspecteur de cellule", |ui| {
                    if let Some((x, y, z)) = state.selected_cell {
                        ui.label(format!("Coordonnées: ({}, {}, {})", x, y, z));

                        let (cx, cy, cz, lx, ly, lz) =
                            rust_sed::simulation::world::WorldMap::world_to_chunk_coords(x, y, z);
                        let cell_opt = state
                            .monde
                            .world_map
                            .get_chunk(cx, cy, cz)
                            .map(|chk| chk.get_cell(lx, ly, lz));

                        if let Some(cell) = cell_opt {
                            if cell.is_alive {
                                ui.label(format!("Type: {:?}", cell.t));
                                ui.label(format!("Âge: {} cycles", cell.a));
                                ui.separator();

                                ui.label("État physique :");
                                ui.label(format!("Énergie: {:.3}", cell.e));
                                ui.label(format!("Dette de besoin: {:.3}", cell.d));
                                ui.label(format!("Stress (C): {:.3}", cell.c));
                                ui.label(format!("Osmose (R): {:.3}", cell.r));
                                ui.label(format!("Résistance (Sc): {:.3}", cell.sc));

                                ui.separator();
                                ui.label("État neural :");
                                ui.label(format!("Potentiel (P): {:.3}", cell.p));
                                ui.label(format!("Réfractaire: {}", cell.r#ref));
                                ui.label(format!("Coût spike: {:.3}", cell.e_cost));
                                ui.label(format!("Spike history (hex): 0x{:08X}", cell.h));
                                ui.label(format!("Gradient: {:.3}", cell.g));

                                ui.separator();
                                ui.horizontal(|ui| {
                                    if ui.button("Forcer Spike").clicked()
                                        && let Some(chunk) =
                                            state.monde.world_map.get_chunk_mut(cx, cy, cz)
                                    {
                                        let cell_mut = chunk.get_cell_mut(lx, ly, lz);
                                        cell_mut.p = 1.0;
                                        cell_mut.h = (cell_mut.h << 1) | 1;
                                        println!(
                                            "[Inspecteur] Spike forcé en ({}, {}, {})",
                                            x, y, z
                                        );
                                    }
                                    if ui.button("Réinitialiser").clicked()
                                        && let Some(chunk) =
                                            state.monde.world_map.get_chunk_mut(cx, cy, cz)
                                    {
                                        let cell_mut = chunk.get_cell_mut(lx, ly, lz);
                                        let old_t = cell_mut.t;
                                        *cell_mut = rust_sed::simulation::cell::Cell::default();
                                        cell_mut.t = old_t;
                                        cell_mut.is_alive = true;
                                        cell_mut.e = 5.0;
                                        cell_mut.clamp_variables();
                                        println!(
                                            "[Inspecteur] Cellule réinitialisée en ({}, {}, {})",
                                            x, y, z
                                        );
                                    }
                                });

                                ui.separator();
                                ui.label("Modifier variables :");
                                if let Some(chunk) = state.monde.world_map.get_chunk_mut(cx, cy, cz)
                                {
                                    let cell_mut = chunk.get_cell_mut(lx, ly, lz);
                                    let mut temp_e = cell_mut.e;
                                    if ui
                                        .add(
                                            egui::Slider::new(&mut temp_e, 0.0..=10.0)
                                                .text("Énergie"),
                                        )
                                        .changed()
                                    {
                                        cell_mut.e = temp_e;
                                    }
                                    let mut temp_p = cell_mut.p;
                                    if ui
                                        .add(
                                            egui::Slider::new(&mut temp_p, -1.0..=1.0)
                                                .text("Potentiel"),
                                        )
                                        .changed()
                                    {
                                        cell_mut.p = temp_p;
                                    }
                                    let mut temp_c = cell_mut.c;
                                    if ui
                                        .add(
                                            egui::Slider::new(&mut temp_c, 0.0..=1.0)
                                                .text("Stress"),
                                        )
                                        .changed()
                                    {
                                        cell_mut.c = temp_c;
                                    }
                                    cell_mut.clamp_variables();
                                }
                            } else {
                                ui.label("Cellule morte ou vide.");
                                ui.separator();
                                ui.label("Créer une cellule :");
                                ui.horizontal(|ui| {
                                    if ui.button("🟢 Souche").clicked() {
                                        apply_brush_action(state, (x, y, z));
                                    }
                                    if ui.button("🟠 Soma").clicked() {
                                        let old_m = state.brush_mode;
                                        state.brush_mode = 1;
                                        apply_brush_action(state, (x, y, z));
                                        state.brush_mode = old_m;
                                    }
                                    if ui.button("🔵 Neurone").clicked() {
                                        let old_m = state.brush_mode;
                                        state.brush_mode = 3;
                                        apply_brush_action(state, (x, y, z));
                                        state.brush_mode = old_m;
                                    }
                                });
                            }
                        } else {
                            ui.label("Cellule hors des chunks actifs.");
                        }
                    } else {
                        ui.label("Aucune cellule sélectionnée.");
                        ui.label("Utilisez le mode 'Inspecter' et cliquez sur une cellule.");
                    }
                });

                // --- Scénarios & Snapshots ---
                ui.collapsing("📂 Scénarios & Snapshots", |ui| {
                    ui.label("Scénarios :");
                    if state.scenarios_list.is_empty() {
                        ui.label("Aucun scénario dans /scenarios");
                    } else {
                        egui::ComboBox::from_label("Choisir")
                            .selected_text(&state.selected_scenario)
                            .show_ui(ui, |ui| {
                                for sc in &state.scenarios_list {
                                    ui.selectable_value(
                                        &mut state.selected_scenario,
                                        sc.clone(),
                                        sc,
                                    );
                                }
                            });

                        if ui.button("📥 Charger Scénario").clicked() {
                            let path = format!("scenarios/{}", state.selected_scenario);
                            if let Err(e) = charger_scenario(&mut state.monde, &path) {
                                println!("[Erreur] Impossible de charger le scénario: {}", e);
                            } else {
                                println!("[Scénario] Chargé: {}", state.selected_scenario);
                                state.update_stats();
                            }
                        }
                    }

                    ui.separator();
                    ui.label("Snapshots (État complet) :");
                    ui.horizontal(|ui| {
                        if ui.button("💾 Sauvegarder").clicked() {
                            if let Err(e) = save_snapshot(&state.monde, "snapshot.json") {
                                println!("[Erreur] Impossible de sauvegarder le snapshot: {}", e);
                            } else {
                                println!("[Snapshot] Sauvegardé sous snapshot.json");
                            }
                        }
                        if ui.button("📂 Charger").clicked() {
                            if let Err(e) = load_snapshot(&mut state.monde, "snapshot.json") {
                                println!("[Erreur] Impossible de charger le snapshot: {}", e);
                            } else {
                                println!("[Snapshot] Chargé depuis snapshot.json");
                                state.update_stats();
                            }
                        }
                    });
                });

                // --- Replay & Logs CSV ---
                ui.collapsing("⏺ Replay & Logs CSV", |ui| {
                    ui.label("Enregistrement & Replay :");
                    ui.horizontal(|ui| {
                        if state.replay.is_recording {
                            if ui.button("⏹ Arrêter Rec").clicked() {
                                state.replay.is_recording = false;
                                let _ = state.replay.save_recording("test_replay.json");
                                println!(
                                    "[Replay] Enregistrement sauvegardé dans test_replay.json"
                                );
                            }
                        } else {
                            if ui.button("🔴 Démarrer Rec").clicked() {
                                state.replay.actions.clear();
                                state.replay.playback_index = 0;
                                state.replay.is_recording = true;
                                state.replay.is_replaying = false;
                                println!("[Replay] Enregistrement démarré.");
                            }
                        }

                        if state.replay.is_replaying {
                            if ui.button("⏹ Arrêter Replay").clicked() {
                                state.replay.is_replaying = false;
                                println!("[Replay] Lecture arrêtée.");
                            }
                        } else {
                            if ui.button("▶ Rejouer Replay").clicked() {
                                if let Err(e) = state.replay.load_recording("test_replay.json") {
                                    println!("[Erreur Replay] Impossible de charger: {}", e);
                                } else {
                                    println!("[Replay] Lecture démarrée.");
                                }
                            }
                        }
                    });

                    if state.replay.is_recording {
                        ui.label("🔴 Enregistrement actif...");
                    }
                    if state.replay.is_replaying {
                        ui.label(format!(
                            "▶ Lecture active : {}/{}",
                            state.replay.playback_index,
                            state.replay.actions.len()
                        ));
                    }

                    ui.separator();
                    ui.label("Monitoring CSV :");
                    ui.text_edit_singleline(&mut state.csv_filename);
                    if ui
                        .checkbox(&mut state.csv_recording, "Enregistrer dans CSV")
                        .changed()
                    {
                        if !state.csv_recording {
                            state.csv_file = None;
                            println!("[CSV] Enregistrement arrêté.");
                        } else {
                            println!("[CSV] Enregistrement activé dans {}", state.csv_filename);
                        }
                    }
                });

                // --- Statistiques ---
                ui.collapsing("📊 Statistiques", |ui| {
                    let pop = state.monde.get_nombre_cellules_vivantes();
                    let total_e: f32 = state
                        .monde
                        .world_map
                        .chunks
                        .values()
                        .flat_map(|chk| chk.cells.iter())
                        .filter(|c| c.is_alive)
                        .map(|c| c.e)
                        .sum();

                    let mut n_souche = 0;
                    let mut n_soma = 0;
                    let mut n_neurone = 0;
                    for chunk in state.monde.world_map.chunks.values() {
                        for c in &chunk.cells {
                            if c.is_alive {
                                match c.t {
                                    CellType::Souche => n_souche += 1,
                                    CellType::Soma => n_soma += 1,
                                    CellType::Neurone => n_neurone += 1,
                                    CellType::Static => {}
                                }
                            }
                        }
                    }

                    ui.label(format!("Population: {}", pop));
                    ui.label(format!("  🟢 Souche:  {}", n_souche));
                    ui.label(format!("  🟠 Soma:    {}", n_soma));
                    ui.label(format!("  🔵 Neurone: {}", n_neurone));
                    ui.label(format!("Énergie totale: {:.1}", total_e));

                    // Mini sparklines
                    draw_sparkline(
                        ui,
                        &state.pop_history,
                        egui::Color32::from_rgb(100, 200, 100),
                        "Population:",
                    );
                    draw_sparkline(
                        ui,
                        &state.energy_history,
                        egui::Color32::from_rgb(255, 200, 80),
                        "Énergie:",
                    );
                });

                // --- Visualisation ---
                ui.collapsing("🎨 Visualisation", |ui| {
                    ui.label("Mode couleur :");
                    ui.radio_value(&mut state.render_mode, RenderMode::Type, "Type cellulaire");
                    ui.radio_value(&mut state.render_mode, RenderMode::Energy, "Énergie");
                    ui.radio_value(&mut state.render_mode, RenderMode::Stress, "Stress (C)");
                    ui.radio_value(&mut state.render_mode, RenderMode::Gradient, "Gradient (G)");
                    ui.radio_value(
                        &mut state.render_mode,
                        RenderMode::Potential,
                        "Potentiel Neural (P)",
                    );

                    ui.separator();
                    ui.label("Filtres :");
                    ui.checkbox(&mut state.show_souche, "🟢 Souche");
                    ui.checkbox(&mut state.show_soma, "🟠 Soma");
                    ui.checkbox(&mut state.show_neurone, "🔵 Neurone");
                    ui.checkbox(&mut state.show_static, "⬜ Static (bedrock)");

                    ui.separator();
                    ui.add(
                        egui::Slider::new(&mut state.cell_scale, 0.1..=1.0).text("Taille cubes"),
                    );
                });

                // --- Paramètres Physiques ---
                ui.collapsing("🔧 Paramètres Physiques", |ui| {
                    let mut val = state.monde.params.k_thermo;
                    if ui
                        .add(egui::Slider::new(&mut val, 0.0..=0.05).text("k_thermo"))
                        .changed()
                    {
                        state.monde.params.k_thermo = val;
                        state.record_param_change("k_thermo", val);
                    }

                    let mut val = state.monde.params.sensibilite_soleil;
                    if ui
                        .add(egui::Slider::new(&mut val, 0.0..=0.1).text("Soleil (gain)"))
                        .changed()
                    {
                        state.monde.params.sensibilite_soleil = val;
                        state.record_param_change("sensibilite_soleil", val);
                    }

                    let mut val = state.monde.params.hauteur_soleil;
                    if ui
                        .add(egui::Slider::new(&mut val, 0.0..=1.0).text("Hauteur soleil"))
                        .changed()
                    {
                        state.monde.params.hauteur_soleil = val;
                        state.record_param_change("hauteur_soleil", val);
                    }

                    let mut val = state.monde.params.seuil_energie_division;
                    if ui
                        .add(egui::Slider::new(&mut val, 0.5..=5.0).text("Seuil division"))
                        .changed()
                    {
                        state.monde.params.seuil_energie_division = val;
                        state.record_param_change("seuil_energie_division", val);
                    }

                    let mut val = state.monde.params.cout_mouvement;
                    if ui
                        .add(egui::Slider::new(&mut val, 0.0..=0.1).text("Coût mouvement"))
                        .changed()
                    {
                        state.monde.params.cout_mouvement = val;
                        state.record_param_change("cout_mouvement", val);
                    }

                    let mut val = state.monde.params.facteur_echange_energie;
                    if ui
                        .add(egui::Slider::new(&mut val, 0.0..=0.5).text("Osmose"))
                        .changed()
                    {
                        state.monde.params.facteur_echange_energie = val;
                        state.record_param_change("facteur_echange_energie", val);
                    }

                    let mut val = state.monde.params.seuil_similarite_r;
                    if ui
                        .add(egui::Slider::new(&mut val, 0.01..=0.5).text("Seuil R"))
                        .changed()
                    {
                        state.monde.params.seuil_similarite_r = val;
                        state.record_param_change("seuil_similarite_r", val);
                    }
                });

                // --- Paramètres Neuraux ---
                ui.collapsing("🧠 Paramètres Neuraux", |ui| {
                    let mut val_u = state.monde.params.ticks_neuraux_par_physique;
                    if ui
                        .add(egui::Slider::new(&mut val_u, 1..=20).text("Ticks neuraux"))
                        .changed()
                    {
                        state.monde.params.ticks_neuraux_par_physique = val_u;
                        state.record_param_change("ticks_neuraux_par_physique", val_u as f32);
                    }

                    let mut val = state.monde.params.seuil_fire;
                    if ui
                        .add(egui::Slider::new(&mut val, 0.1..=2.0).text("Seuil fire"))
                        .changed()
                    {
                        state.monde.params.seuil_fire = val;
                        state.record_param_change("seuil_fire", val);
                    }

                    let mut val = state.monde.params.cout_spike;
                    if ui
                        .add(egui::Slider::new(&mut val, 0.0..=0.05).text("Coût spike"))
                        .changed()
                    {
                        state.monde.params.cout_spike = val;
                        state.record_param_change("cout_spike", val);
                    }

                    let mut val = state.monde.params.learn_rate;
                    if ui
                        .add(egui::Slider::new(&mut val, 0.001..=0.2).text("Learn rate"))
                        .changed()
                    {
                        state.monde.params.learn_rate = val;
                        state.record_param_change("learn_rate", val);
                    }

                    let mut val = state.monde.params.decay_synapse;
                    if ui
                        .add(egui::Slider::new(&mut val, 0.9..=1.0).text("Decay synapse"))
                        .changed()
                    {
                        state.monde.params.decay_synapse = val;
                        state.record_param_change("decay_synapse", val);
                    }
                });

                // --- Caméra ---
                ui.collapsing("📷 Caméra", |ui| {
                    ui.label("Clic droit + glisser = orbiter");
                    ui.label("Molette = zoom");
                    ui.add(
                        egui::Slider::new(&mut state.cam_distance, 5.0..=150.0).text("Distance"),
                    );
                    if ui.button("Recentrer").clicked() {
                        let b = state.monde.calculer_barycentre();
                        state.cam_target = vec3(b.0, b.1, b.2);
                    }
                });

                // --- Pré-chargement (Replay Cache) ---
                ui.collapsing("⚡ Pré-chargement (Replay Cache)", |ui| {
                    if state.is_in_preload_mode {
                        ui.label("🔴 Mode Replay Cache activé");
                        ui.label(format!(
                            "Cycle courant : {} / {}",
                            state.preload_index,
                            if state.preloaded_states.is_empty() {
                                0
                            } else {
                                state.preloaded_states.len() - 1
                            }
                        ));

                        let mut idx = state.preload_index;
                        let max_idx = if state.preloaded_states.is_empty() {
                            0
                        } else {
                            state.preloaded_states.len() - 1
                        };
                        if ui
                            .add(egui::Slider::new(&mut idx, 0..=max_idx).text("Temps"))
                            .changed()
                        {
                            state.preload_index = idx;
                            state.monde = state.preloaded_states[state.preload_index].clone();
                        }

                        ui.horizontal(|ui| {
                            if ui
                                .button(if state.preload_is_playing {
                                    "⏸ Pause"
                                } else {
                                    "▶ Lire"
                                })
                                .clicked()
                            {
                                state.preload_is_playing = !state.preload_is_playing;
                            }
                            if ui.button("◀ Étape").clicked() && state.preload_index > 0 {
                                state.preload_index -= 1;
                                state.monde = state.preloaded_states[state.preload_index].clone();
                            }
                            if ui.button("Étape ▶").clicked()
                                && !state.preloaded_states.is_empty()
                                && state.preload_index < state.preloaded_states.len() - 1
                            {
                                state.preload_index += 1;
                                state.monde = state.preloaded_states[state.preload_index].clone();
                            }
                        });

                        ui.separator();
                        if ui.button("❌ Quitter le mode Replay").clicked() {
                            state.is_in_preload_mode = false;
                            state.preload_is_playing = false;
                            if !state.preloaded_states.is_empty() {
                                state.monde = state.preloaded_states[0].clone();
                            }
                            state.preloaded_states.clear();
                        }
                    } else {
                        ui.label("Calculer et pré-charger N cycles :");
                        ui.add(
                            egui::Slider::new(&mut state.preload_cycles_count, 10..=2000)
                                .text("Cycles"),
                        );
                        if ui.button("🚀 Lancer le pré-chargement").clicked() {
                            lancer_prechargement(state);
                        }
                    }
                });

                ui.separator();
                ui.label("SED v9.0 — Rust Edition");
                ui.label("Déterminisme bit-exact garanti");
            });
    });

    egui_macroquad::draw();
}

// ===========================================================================
// Gestion de la caméra orbitale et avancement du temps
// ===========================================================================

fn avancer_un_cycle(state: &mut AppState) {
    play_replay_actions(state);
    state.monde.avancer_temps();
    state.update_stats();
    handle_csv_logging(state);
}

fn lancer_prechargement(state: &mut AppState) {
    if state.preload_cycles_count <= 0 {
        return;
    }

    state.preloaded_states.clear();

    // Enregistrer l'état de départ initial
    state.preloaded_states.push(state.monde.clone());

    // Calculer les cycles suivants et les cloner
    for _ in 0..state.preload_cycles_count {
        // Avancement manuel du monde sans affecter les logs ou l'état global direct si non voulu,
        // mais avancer_un_cycle est parfait car il met à jour le monde et les stats.
        avancer_un_cycle(state);
        state.preloaded_states.push(state.monde.clone());
    }

    state.preload_index = 0;
    state.is_in_preload_mode = true;
    state.is_running = false;
    state.preload_is_playing = false;

    println!(
        "[Preload] {} cycles pré-chargés en mémoire.",
        state.preload_cycles_count
    );
}

fn handle_camera_input(state: &mut AppState) {
    if is_mouse_button_pressed(MouseButton::Right) {
        set_cursor_grab(true);
        show_mouse(false);
        state.last_mouse = Vec2::new(mouse_position().0, mouse_position().1);
        state.mouse_captured = true;
    }

    if is_mouse_button_down(MouseButton::Right) {
        let mouse_pos = Vec2::new(mouse_position().0, mouse_position().1);
        if state.mouse_captured {
            let delta = mouse_pos - state.last_mouse;
            state.cam_yaw = (state.cam_yaw + delta.x * 0.3) % 360.0;
            state.cam_pitch = (state.cam_pitch + delta.y * 0.3).clamp(-89.0, 89.0);
        }
        state.last_mouse = mouse_pos;
    }

    if is_mouse_button_released(MouseButton::Right) {
        set_cursor_grab(false);
        show_mouse(true);
        state.mouse_captured = false;
    }

    let (_, wheel_y) = mouse_wheel();
    if wheel_y != 0.0 {
        state.cam_distance = (state.cam_distance - wheel_y * 2.0).clamp(5.0, 150.0);
    }

    if is_key_pressed(KeyCode::Space) {
        if state.is_in_preload_mode {
            state.preload_is_playing = !state.preload_is_playing;
        } else {
            state.is_running = !state.is_running;
        }
    }
    if is_key_pressed(KeyCode::Tab) {
        state.show_gui = !state.show_gui;
    }
    if is_key_pressed(KeyCode::Right) {
        if state.is_in_preload_mode {
            if !state.preloaded_states.is_empty() {
                state.preload_index =
                    (state.preload_index + 1).min(state.preloaded_states.len() - 1);
            }
        } else {
            avancer_un_cycle(state);
        }
    }
}

fn compute_camera(state: &AppState) -> Camera3D {
    let yaw_rad = state.cam_yaw.to_radians();
    let pitch_rad = state.cam_pitch.to_radians();

    let cam_pos = state.cam_target
        + vec3(
            state.cam_distance * pitch_rad.cos() * yaw_rad.sin(),
            state.cam_distance * pitch_rad.sin(),
            state.cam_distance * pitch_rad.cos() * yaw_rad.cos(),
        );

    Camera3D {
        position: cam_pos,
        target: state.cam_target,
        up: vec3(0.0, -1.0, 0.0),
        fovy: 55.0,
        projection: Projection::Perspective,
        ..Default::default()
    }
}

// ===========================================================================
// Rendu HUD 2D
// ===========================================================================

fn draw_hud(state: &AppState) {
    let pop = state.monde.get_nombre_cellules_vivantes();
    let status = if state.is_running {
        "▶ EN COURS"
    } else if state.replay.is_replaying {
        "▶ REPLAY"
    } else {
        "⏸ PAUSE"
    };

    let start_x = if state.show_gui { 315.0 } else { 10.0 };

    draw_text(
        format!(
            "{} | Cycle: {} | Pop: {} | FPS: {:.0}",
            status, state.monde.cycle_actuel, pop, state.last_fps
        ),
        start_x,
        20.0,
        20.0,
        WHITE,
    );

    if !state.show_gui {
        draw_text(
            "[TAB] Interface | [ESPACE] Play/Pause | [→] +1 Cycle",
            10.0,
            40.0,
            16.0,
            GRAY,
        );
    }
}

// ===========================================================================
// Boucle principale
// ===========================================================================

#[macroquad::main(window_conf)]
async fn main() {
    // --- Parsing des arguments CLI ---
    let args: Vec<String> = std::env::args().collect();
    let mut headless = false;
    let mut scenario_path = None;
    let mut cycles = 100;
    let mut csv_output = "headless_log.csv".to_string();
    let mut json_output = "headless_final.json".to_string();

    let mut arg_idx = 1;
    while arg_idx < args.len() {
        match args[arg_idx].as_str() {
            "--headless" => {
                headless = true;
            }
            "--scenario" => {
                if arg_idx + 1 < args.len() {
                    scenario_path = Some(args[arg_idx + 1].clone());
                    arg_idx += 1;
                }
            }
            "--cycles" => {
                if arg_idx + 1 < args.len() {
                    if let Ok(c) = args[arg_idx + 1].parse::<i32>() {
                        cycles = c;
                    }
                    arg_idx += 1;
                }
            }
            "--output" => {
                if arg_idx + 1 < args.len() {
                    csv_output = args[arg_idx + 1].clone();
                    arg_idx += 1;
                }
            }
            "--save-final" => {
                if arg_idx + 1 < args.len() {
                    json_output = args[arg_idx + 1].clone();
                    arg_idx += 1;
                }
            }
            _ => {}
        }
        arg_idx += 1;
    }

    if headless {
        println!("[Headless] Mode sans interface activé.");
        let mut state = AppState::new();

        if let Some(path) = scenario_path {
            println!("[Headless] Chargement du scénario depuis : {}", path);
            if let Err(e) = charger_scenario(&mut state.monde, &path) {
                eprintln!("[Erreur] Impossible de charger le scénario : {}", e);
                std::process::exit(1);
            }
        } else {
            println!("[Headless] Aucun scénario spécifié. Initialisation d'un monde aléatoire par défaut.");
            state.reset();
        }

        // Configuration du log CSV
        state.csv_recording = true;
        state.csv_filename = csv_output.clone();

        // Premier log avec l'état initial
        handle_csv_logging(&mut state);

        println!("[Headless] Exécution de {} cycles de simulation...", cycles);
        let start_time = std::time::Instant::now();

        for cycle in 1..=cycles {
            state.monde.avancer_temps();
            handle_csv_logging(&mut state);

            if cycle % 50 == 0 || cycle == cycles {
                let pop = state.monde.get_nombre_cellules_vivantes();
                println!(
                    "[Headless] Cycle {} / {} — Population active : {}",
                    cycle, cycles, pop
                );
            }
        }

        let duration = start_time.elapsed();
        println!(
            "[Headless] Simulation complétée en {:.2} secondes ({:.2} Hz).",
            duration.as_secs_f32(),
            cycles as f32 / duration.as_secs_f32()
        );

        // Sauvegarde de l'état final
        println!("[Headless] Sauvegarde de l'état final dans : {}", json_output);
        if let Err(e) = save_snapshot(&state.monde, &json_output) {
            eprintln!("[Erreur] Échec de la sauvegarde de l'état final : {}", e);
        }

        println!("[Headless] Terminé avec succès. Fichier CSV : {}", csv_output);
        std::process::exit(0);
    }

    let mut state = AppState::new();
    state.update_stats();

    loop {
        state.last_fps = get_fps() as f32;

        handle_camera_input(&mut state);

        if state.is_in_preload_mode {
            if state.preload_is_playing {
                state.accum += get_frame_time();
                let interval = 1.0 / state.speed;
                while state.accum >= interval {
                    if !state.preloaded_states.is_empty() {
                        let next_idx = state.preload_index + 1;
                        if next_idx < state.preloaded_states.len() {
                            state.preload_index = next_idx;
                            state.monde = state.preloaded_states[state.preload_index].clone();
                        } else {
                            state.preload_index = 0;
                            state.monde = state.preloaded_states[0].clone();
                        }
                    }
                    state.accum -= interval;
                }
            }
        } else if state.is_running {
            state.accum += get_frame_time();
            let interval = 1.0 / state.speed;
            while state.accum >= interval {
                avancer_un_cycle(&mut state);
                state.accum -= interval;
            }
        }

        if !state.show_gui {
            state.mouse_on_gui = false;
        }

        let camera = compute_camera(&state);
        handle_interaction(&mut state, &camera);

        // --- Rendu 3D ---
        clear_background(Color::new(0.05, 0.05, 0.08, 1.0));

        set_camera(&camera);

        render_grid(&state.monde, state.monde.size_x);
        render_cells(&state);

        // --- Retour au 2D ---
        set_default_camera();
        draw_hud(&state);

        // --- GUI egui ---
        if state.show_gui {
            draw_gui(&mut state);
        }

        next_frame().await;
    }
}
