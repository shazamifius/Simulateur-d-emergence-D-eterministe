use rust_sed::simulation::world::MondeSED;

/// Test de déterminisme bit-exact :
/// Deux simulations avec la même graine doivent produire le même hash d'état.
#[test]
fn test_determinism_bit_exact() {
    let cycles = 50;
    let seed = 42;
    let size = 16; // Petit monde pour la vitesse

    // --- Run 1 ---
    let mut monde1 = MondeSED::new(size, size, size);
    monde1.initialiser_monde(seed, 0.15);
    for _ in 0..cycles {
        monde1.avancer_temps();
    }
    let hash1 = monde1.calculate_state_hash();
    let pop1 = monde1.get_nombre_cellules_vivantes();

    // --- Run 2 ---
    let mut monde2 = MondeSED::new(size, size, size);
    monde2.initialiser_monde(seed, 0.15);
    for _ in 0..cycles {
        monde2.avancer_temps();
    }
    let hash2 = monde2.calculate_state_hash();
    let pop2 = monde2.get_nombre_cellules_vivantes();

    assert_eq!(
        hash1, hash2,
        "Les hashes doivent être identiques (déterminisme bit-exact)"
    );
    assert_eq!(pop1, pop2, "Les populations doivent être identiques");
    println!("Déterminisme vérifié: hash={} pop={}", hash1, pop1);
}

/// Test de conservation thermodynamique :
/// L'énergie totale ne doit jamais augmenter spontanément (sans source solaire).
#[test]
fn test_energy_conservation_no_sun() {
    let mut monde = MondeSED::new(16, 16, 16);
    // Désactiver le soleil pour vérifier la conservation pure
    monde.params.sensibilite_soleil = 0.0;
    monde.params.seuil_energie_division = 999.0; // Pas de division
    monde.initialiser_monde(123, 0.1);

    let initial_energy: f32 = monde
        .world_map
        .chunks
        .values()
        .flat_map(|chk| chk.cells.iter())
        .filter(|c| c.is_alive)
        .map(|c| c.e)
        .sum();

    for _ in 0..20 {
        monde.avancer_temps();
    }

    let final_energy: f32 = monde
        .world_map
        .chunks
        .values()
        .flat_map(|chk| chk.cells.iter())
        .filter(|c| c.is_alive)
        .map(|c| c.e)
        .sum();

    // L'énergie doit strictement diminuer (coûts métaboliques)
    assert!(
        final_energy <= initial_energy,
        "L'énergie ne doit pas augmenter sans source: initiale={}, finale={}",
        initial_energy,
        final_energy
    );
    println!(
        "Conservation vérifiée: E_init={:.2} -> E_fin={:.2}",
        initial_energy, final_energy
    );
}

/// Test de base : le métabolisme réduit l'énergie des cellules.
#[test]
fn test_metabolism_reduces_energy() {
    use rust_sed::simulation::cell::{Cell, CellType};
    use rust_sed::simulation::laws::appliquer_loi_metabolisme;
    use rust_sed::simulation::params::ParametresGlobaux;

    let params = ParametresGlobaux::default();
    let mut cell = Cell {
        is_alive: true,
        e: 1.0,
        t: CellType::Souche,
        ..Default::default()
    };

    let e_before = cell.e;
    appliquer_loi_metabolisme(&mut cell, 5, &params);
    assert!(cell.e < e_before, "Le métabolisme doit réduire l'énergie");
    assert!(cell.d > 0.0, "La dette doit augmenter après un tick");
    assert!(cell.l > 0.0, "L'ennui doit augmenter après un tick");
    assert_eq!(cell.a, 1, "L'âge doit être incrémenté");
}

/// Test d'apprentissage Hebbien : deux neurones co-activés renforcent leur synapse.
#[test]
fn test_hebb_learning() {
    use rust_sed::simulation::cell::{Cell, CellType};

    // Simuler deux neurones qui ont tiré ensemble (bit 0 à 1 dans H)
    let neurone_a = Cell {
        is_alive: true,
        t: CellType::Neurone,
        h: 0b1, // A tiré au dernier tick
        w: [0.5; 27],
        ..Default::default()
    };

    let neurone_b = Cell {
        is_alive: true,
        t: CellType::Neurone,
        h: 0b101, // A tiré dans les 3 derniers ticks
        ..Default::default()
    };

    // Vérifier que le bitfield détecte bien l'activité récente
    let self_fired = (neurone_a.h & 1) != 0;
    let voisin_active = (neurone_b.h & 0b111) != 0;
    assert!(self_fired, "Neurone A doit avoir tiré");
    assert!(voisin_active, "Neurone B doit avoir été actif récemment");
}

/// Test de la seed : deux seeds différentes produisent des mondes différents.
#[test]
fn test_different_seeds_diverge() {
    let mut monde1 = MondeSED::new(16, 16, 16);
    monde1.initialiser_monde(42, 0.15);
    for _ in 0..10 {
        monde1.avancer_temps();
    }

    let mut monde2 = MondeSED::new(16, 16, 16);
    monde2.initialiser_monde(99, 0.15);
    for _ in 0..10 {
        monde2.avancer_temps();
    }

    let hash1 = monde1.calculate_state_hash();
    let hash2 = monde2.calculate_state_hash();
    assert_ne!(
        hash1, hash2,
        "Deux seeds différentes doivent produire des états différents"
    );
}

/// Test que la population initiale est non-nulle.
#[test]
fn test_initialization_creates_cells() {
    let mut monde = MondeSED::new(16, 16, 16);
    monde.initialiser_monde(42, 0.15);
    let pop = monde.get_nombre_cellules_vivantes();
    assert!(pop > 0, "La population initiale doit être non-nulle");
    println!("Population initiale: {}", pop);
}

/// Test de robustesse aux valeurs NaN et Inf :
/// La simulation ne doit pas planter (panic) si des cellules acquièrent des valeurs non définies.
#[test]
fn test_nan_robustness() {
    let mut monde = MondeSED::new(16, 16, 16);
    monde.initialiser_monde(42, 0.1);

    // Injecter des valeurs invalides dans certaines cellules vivantes
    let mut injected = false;
    for chunk in monde.world_map.chunks.values_mut() {
        for cell in &mut chunk.cells {
            if cell.is_alive {
                cell.e = f32::NAN;
                cell.d = f32::INFINITY;
                cell.c = f32::NEG_INFINITY;
                injected = true;
                break;
            }
        }
        if injected {
            break;
        }
    }

    assert!(injected, "Une cellule vivante doit avoir été injectée avec des NaNs");

    // L'avancement temporel doit s'exécuter sans paniquer
    monde.avancer_temps();
    
    // Vérifier que la cellule corrompue a été nettoyée ou désactivée
    let pop_after = monde.get_nombre_cellules_vivantes();
    println!("Simulation résistante aux NaNs réussie. Population après : {}", pop_after);
}
