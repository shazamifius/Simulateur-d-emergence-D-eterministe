use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u8)]
#[derive(Default)]
pub enum CellType {
    #[default]
    Souche = 0,
    Soma = 1,
    Neurone = 2,
    Static = 3,
}

#[derive(Serialize, Deserialize, Clone, Copy, Debug)]
pub struct Cell {
    // --- Identité ---
    pub t: CellType,

    // --- Constantes de Naissance (Morphologie) ---
    pub r: f32,  // Compatibilité Osmose [0,1]
    pub sc: f32, // Seuil Critique (résistance au stress) [0,1]

    // --- Variables Dynamiques (État Physique) ---
    pub e: f32, // Énergie
    pub d: f32, // Dette de Besoin (hunger)
    pub c: f32, // Charge Émotionnelle (stress) [0,1]
    pub l: f32, // Dette de Stimulus (ennui)
    pub m: f32, // Mémoire (max E voisin)
    pub a: i32, // Énergie accumulée / Âge (cycles)

    // --- Variables Neurales & Spatiales ---
    pub p: f32,       // Potentiel électrique [-1,1]
    pub r#ref: i32,   // Compteur réfractaire
    pub e_cost: f32,  // Coût énergétique accumulé (spikes)
    pub w: [f32; 27], // Poids synaptiques (voisins)
    pub h: u32,       // Historique des spikes (Bitfield 32 bits)
    pub g: f32,       // Gradient spatial

    pub is_alive: bool,
}

impl Default for Cell {
    fn default() -> Self {
        Self {
            t: CellType::Souche,
            r: 0.0,
            sc: 0.5,
            e: 0.0,
            d: 0.0,
            c: 0.0,
            l: 0.0,
            m: 0.0,
            a: 0,
            p: 0.0,
            r#ref: 0,
            e_cost: 0.0,
            w: [0.01; 27], // Connexions faibles par défaut
            h: 0,
            g: 0.0,
            is_alive: false,
        }
    }
}

impl Cell {
    pub fn new_static() -> Self {
        Self {
            t: CellType::Static,
            e: 1000.0,
            is_alive: true,
            ..Default::default()
        }
    }

    pub fn clamp_variables(&mut self) {
        self.p = self.p.clamp(-1.0, 1.0);
        self.c = self.c.clamp(0.0, 1.0);
        self.r = self.r.clamp(0.0, 1.0);
        self.sc = self.sc.clamp(0.0, 1.0);
        self.g = self.g.clamp(0.0, 1.0);

        // E, D, L, M doivent rester positifs ou nuls
        if self.e < 0.0 {
            self.e = 0.0;
        }
        if self.d < 0.0 {
            self.d = 0.0;
        }
        if self.l < 0.0 {
            self.l = 0.0;
        }
        if self.m < 0.0 {
            self.m = 0.0;
        }

        for weight in self.w.iter_mut() {
            *weight = weight.clamp(0.0, 1.0);
        }
    }

    pub fn is_nan_or_inf(&self) -> bool {
        self.e.is_nan()
            || self.e.is_infinite()
            || self.d.is_nan()
            || self.d.is_infinite()
            || self.c.is_nan()
            || self.c.is_infinite()
            || self.l.is_nan()
            || self.l.is_infinite()
            || self.p.is_nan()
            || self.p.is_infinite()
    }
}
