use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct ParametresGlobaux {
    // Loi 0: Thermodynamique & Survie
    pub k_thermo: f32,
    pub d_per_tick: f32,
    pub l_per_tick: f32,

    // Environnement (Soleil)
    pub sensibilite_soleil: f32,
    pub hauteur_soleil: f32,

    // Loi 1: Dynamique (Mouvement)
    pub k_d: f32,
    pub k_c: f32,
    pub k_m: f32,
    pub k_adh: f32,
    pub cout_mouvement: f32,

    // Loi 2: Reproduction (Mitose)
    pub seuil_energie_division: f32,
    pub cout_division: f32,

    // Loi 3: Champs (Action à distance)
    pub rayon_diffusion: f32,
    pub alpha_attenuation: f32,
    pub k_champ_e: f32,
    pub k_champ_c: f32,

    // Loi 4: Osmose
    pub facteur_echange_energie: f32,
    pub seuil_difference_energie: f32,
    pub seuil_similarite_r: f32,
    pub max_flux_energie: f32,

    // Loi 5: Interaction Forte (Psychique)
    pub facteur_echange_psychique: f32,

    // Morphogenèse
    pub lambda_gradient: f32,
    pub seuil_soma: f32,
    pub seuil_neuro: f32,

    // Dynamique Neurale
    pub ticks_neuraux_par_physique: usize,
    pub cout_spike: f32,
    pub periode_refractaire: i32,
    pub seuil_fire: f32,
    pub decay_synapse: f32,
    pub learn_rate: f32,
    pub rayon_ignition: f32,

    // Mémoire
    pub facteur_oubli_m: f32,

    // Limites
    pub max_cells: usize,
    pub world_height: i32,
}

impl Default for ParametresGlobaux {
    fn default() -> Self {
        Self {
            k_thermo: 0.001,
            d_per_tick: 0.002,
            l_per_tick: 0.001,
            sensibilite_soleil: 0.01,
            hauteur_soleil: 0.5,
            k_d: 1.0,
            k_c: 0.5,
            k_m: 0.5,
            k_adh: 0.5,
            cout_mouvement: 0.01,
            seuil_energie_division: 1.8,
            cout_division: 0.0,
            rayon_diffusion: 2.0,
            alpha_attenuation: 1.0,
            k_champ_e: 1.0,
            k_champ_c: 1.0,
            facteur_echange_energie: 0.1,
            seuil_difference_energie: 0.2,
            seuil_similarite_r: 0.1,
            max_flux_energie: 0.05,
            facteur_echange_psychique: 0.1,
            lambda_gradient: 0.1,
            seuil_soma: 0.3,
            seuil_neuro: 0.7,
            ticks_neuraux_par_physique: 5,
            cout_spike: 0.005,
            periode_refractaire: 2,
            seuil_fire: 0.85,
            decay_synapse: 0.999,
            learn_rate: 0.05,
            rayon_ignition: 4.0,
            facteur_oubli_m: 0.99,
            max_cells: 1_000_000,
            world_height: 32,
        }
    }
}
