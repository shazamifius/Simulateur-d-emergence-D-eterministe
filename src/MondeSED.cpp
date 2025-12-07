// --- Fichiers d'en-tête ---
#include "MondeSED.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <sstream>
#include <vector>

// --- Implémentation de la classe MondeSED ---

// Calcule et retourne le nombre total de cellules vivantes dans la grille.
int MondeSED::getNombreCellulesVivantes() const {
  // Utilise std::accumulate pour sommer les cellules vivantes de manière
  // efficace.
  return std::accumulate(grille.begin(), grille.end(), 0,
                         [](int count, const Cellule &cell) {
                           return count + (cell.is_alive ? 1 : 0);
                         });
}

// --- Constructeur ---
// Initialise le monde avec les dimensions données et pré-alloue la grille.
MondeSED::MondeSED(int sx, int sy, int sz)
    : size_x(sx), size_y(sy), size_z(sz), cycle_actuel(0) {
  grille.resize(size_x * size_y * size_z);
  params = ParametresGlobaux(); // Initialise les paramètres avec les valeurs
                                // par défaut.
}

// Convertit les coordonnées 3D en un index 1D pour l'accès au vecteur `grille`.
// Retourne -1 si les coordonnées sont hors limites.
int MondeSED::getIndex(int x, int y, int z) const {
  if (x < 0 || x >= size_x || y < 0 || y >= size_y || z < 0 || z >= size_z) {
    return -1; // Coordonnées hors limites
  }
  // Formule pour un stockage "row-major"
  return x + y * size_x + z * size_x * size_y;
}

// Retourne une référence modifiable à une cellule à partir de ses coordonnées
// 3D.
Cellule &MondeSED::getCellule(int x, int y, int z) {
  return grille[getIndex(x, y, z)];
}

// Retourne une référence constante à une cellule d'une grille donnée (lecture
// seule).
const Cellule &MondeSED::getCellule(int x, int y, int z,
                                    const std::vector<Cellule> &grid) const {
  return grid[getIndex(x, y, z)];
}

// --- Initialisation et Exportation ---

// Remplit la grille avec une "soupe primordiale" de cellules de manière
// déterministe.
void MondeSED::InitialiserMonde(unsigned int seed, float initial_density) {
  current_seed = seed; // Sauvegarde la graine utilisée.
  // Le générateur est maintenant initialisé avec la graine fournie,
  // garantissant la reproductibilité.
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> random_float(0.0f, 1.0f);
  std::uniform_real_distribution<float> random_r_s(0.1f, 0.9f);

  for (int z = 0; z < size_z; ++z) {
    for (int y = 0; y < size_y; ++y) {
      for (int x = 0; x < size_x; ++x) {
        Cellule &cell = getCellule(x, y, z);
        cell = {}; // Réinitialise la cellule à son état par défaut (morte)
        if (random_float(rng) < initial_density) {
          // Crée une nouvelle cellule vivante avec des propriétés initiales.
          cell.is_alive = true;
          cell.E = 1.0f;             // Énergie de départ
          cell.R = random_r_s(rng);  // Résistance innée aléatoire
          cell.Sc = random_r_s(rng); // Seuil critique aléatoire
        }
      }
    }
  }
}

// Exporte l'état actuel des cellules vivantes dans un fichier CSV.
void MondeSED::ExporterEtatMonde(const std::string &nom_de_base) const {
  // Construit un nom de fichier unique basé sur le numéro de cycle.
  std::string nom_fichier =
      nom_de_base + "_cycle_" + std::to_string(cycle_actuel) + ".csv";
  std::ofstream outfile(nom_fichier);
  if (!outfile.is_open()) {
    std::cerr << "Erreur: Impossible d'ouvrir le fichier d'exportation '"
              << nom_fichier << "'" << std::endl;
    return;
  }

  // En-tête du fichier CSV
  outfile << "x,y,z,E,C,R,A,M\n";

  // Parcours de la grille et écriture des données pour chaque cellule vivante.
  for (int z = 0; z < size_z; ++z) {
    for (int y = 0; y < size_y; ++y) {
      for (int x = 0; x < size_x; ++x) {
        const Cellule &cell = getCellule(x, y, z, grille);
        if (cell.is_alive) {
          outfile << x << "," << y << "," << z << "," << cell.E << "," << cell.C
                  << "," << cell.R << "," << cell.A << "," << cell.M << "\n";
        }
      }
    }
  }
  outfile.close();
}

// --- Lois de Simulation (Phase de Mise à Jour d'État) ---

// Applique la Loi 0 (Survie, Vieillissement) et la Loi 6 (Mémorisation).
// Cette fonction est appliquée à la fin du cycle, après les décisions et
// actions.
void MondeSED::AppliquerLoiZero(int x, int y, int z) {
  Cellule &cell = getCellule(x, y, z);
  if (!cell.is_alive)
    return;

  // Loi 0 (Survie, Vieillissement)

  // 1. Thermodynamique et Entropie
  cell.E -= params.K_THERMO;   // Coût métabolique
  cell.D += params.D_PER_TICK; // La faim augmente
  cell.L += params.L_PER_TICK; // L'ennui augmente naturellement (vide froid)

  // 2. Vieillissement
  cell.A++;

  // 3. Oubli (Mémoire qui s'efface)
  cell.M *= (1.0f - params.TAUX_OUBLI);

  // 4. Mémorisation (Loi 6 intégrée)
  float max_energie_voisin = 0.0f;
  std::tuple<int, int, int> voisins[32];
  int nombre_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, nombre_voisins);

  for (int i = 0; i < nombre_voisins; ++i) {
    const auto &coords_voisin = voisins[i];
    const Cellule &voisin =
        getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin),
                   std::get<2>(coords_voisin));
    if (voisin.is_alive && voisin.E > max_energie_voisin) {
      max_energie_voisin = voisin.E;
    }
  }
  // Apprentissage : La mémorisation dépend de l'écart (surprise)
  if (max_energie_voisin > cell.M) {
    cell.M = std::max(cell.M, max_energie_voisin);
  }

  // 5. Clamping (Sécurité numérique)
  cell.C = std::clamp(cell.C, 0.0f, 1.0f);
  cell.R = std::clamp(cell.R, 0.0f, 1.0f);
  cell.Sc = std::clamp(cell.Sc, 0.0f, 1.0f);
  cell.E = std::max(0.0f, cell.E); // Pas d'énergie négative

  // 6. Mort (Effondrement)
  if (cell.E <= 0.0f || cell.C > cell.Sc) {
    cell = {}; // Réinitialise à un état mort par défaut (E=0, Alive=false).
  }
}

// --- Fonctions Utilitaires ---

// Version optimisée de GetCoordsVoisins qui évite les allocations de vecteur.
void MondeSED::GetCoordsVoisins_Optimized(
    int x, int y, int z, std::tuple<int, int, int> *voisins_array,
    int &count) const {
  count = 0;
  for (int dz = -1; dz <= 1; ++dz) {
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0 && dz == 0)
          continue;
        int nx = x + dx;
        int ny = y + dy;
        int nz = z + dz;
        if (getIndex(nx, ny, nz) != -1) {
          voisins_array[count++] = std::make_tuple(nx, ny, nz);
        }
      }
    }
  }
}

// --- Lois de Simulation (Phase de Décision) ---

// Applique la Loi 1 (Mouvement) : Calcule la meilleure destination pour une
// cellule.
void MondeSED::AppliquerLoiMouvement(int x, int y, int z,
                                     const std::vector<Cellule> &read_grid) {
  const Cellule &source_cell = getCellule(x, y, z, read_grid);
  if (!source_cell.is_alive)
    return;

  std::tuple<int, int, int> meilleure_cible;
  float max_score = -std::numeric_limits<float>::infinity();
  bool cible_trouvee = false;

  // Calcul du Champ (Loi 3)
  // On ne calcule le champ complet que si nécessaire pour le score,
  // mais ici on l'intègre dans le score de chaque voisin.
  // Note: La spec V2 demande de vérifier les voisins VIDES.

  std::tuple<int, int, int> voisins[26];
  int nombre_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, nombre_voisins);

  for (int i = 0; i < nombre_voisins; ++i) {
    const auto &coords_voisin = voisins[i];
    int vx = std::get<0>(coords_voisin);
    int vy = std::get<1>(coords_voisin);
    int vz = std::get<2>(coords_voisin);

    const Cellule &voisin_cell = getCellule(vx, vy, vz, read_grid);

    if (!voisin_cell.is_alive) { // Case vide

      // Calcul des champs locaux à la case cible (vx, vy, vz)
      // Cela simule ce que la cellule "sentirait" si elle y était
      float champ_E = 0.0f; // Opportunité
      float champ_C = 0.0f; // Stress/Danger

      int r = static_cast<int>(std::ceil(params.RAYON_DIFFUSION));
      for (int dz = -r; dz <= r; ++dz) {
        for (int dy = -r; dy <= r; ++dy) {
          for (int dx = -r; dx <= r; ++dx) {
            if (dx == 0 && dy == 0 && dz == 0)
              continue;
            int nx = vx + dx, ny = vy + dy, nz = vz + dz;
            if (getIndex(nx, ny, nz) != -1) {
              const Cellule &n_cell = getCellule(nx, ny, nz, read_grid);
              if (n_cell.is_alive) {
                float dist =
                    std::sqrt(static_cast<float>(dx * dx + dy * dy + dz * dz));
                if (dist <= params.RAYON_DIFFUSION) {
                  float weight = std::exp(-params.ALPHA_ATTENUATION * dist);
                  champ_E += n_cell.E * weight;
                  champ_C += n_cell.C * weight;
                }
              }
            }
          }
        }
      }

      // Score Vectoriel (Loi 1 + Loi 3)
      // Score = Gravité(D) - Pression(C) + Inertie(M) - Coût + Champ_E -
      // Champ_C
      float gravity = params.K_D * source_cell.D;
      float pressure =
          params.K_C * source_cell.C; // Pression interne de la cellule
      float inertia = params.K_M * (source_cell.M / (source_cell.A + 1.0f));

      // La répulsion thermique (Stress) local + le champ de stress externe
      float score = gravity - pressure + inertia - params.COUT_MOUVEMENT;

      // Ajout des champs : Attraction par Énergie, Répulsion par Stress
      score += (champ_E - champ_C);

      if (score > max_score) {
        max_score = score;
        meilleure_cible = coords_voisin;
        cible_trouvee = true;
      }
    }
  }

  if (cible_trouvee) {
#pragma omp critical
    mouvements_souhaites.push_back(
        {std::make_tuple(x, y, z), meilleure_cible, source_cell.D});
  }
}

// --- Fonctions d'Application des Actions (Phase d'Action) ---

// Résout les conflits de mouvement et applique les mouvements validés.
void MondeSED::AppliquerMouvements() {
  // TRI DÉTERMINISTE :
  // On trie le vecteur par coordonnées de source.
  // Comme chaque cellule ne peut proposer qu'un seul mouvement, la source est
  // un identifiant unique. Cela garantit que l'ordre d'itération (et donc la
  // résolution des conflits) est toujours le même.
  std::sort(mouvements_souhaites.begin(), mouvements_souhaites.end(),
            [](const MouvementSouhaite &a, const MouvementSouhaite &b) {
              return a.source < b.source;
            });

  // Gère les conflits : si plusieurs cellules visent la même destination,
  // seule celle avec la "Dette de Besoin" (D) la plus élevée l'emporte.
  // Cette approche garantit le déterminisme.
  std::map<std::tuple<int, int, int>, MouvementSouhaite> mouvements_gagnants;
  for (const auto &mouvement : mouvements_souhaites) {
    auto it = mouvements_gagnants.find(mouvement.destination);
    // En cas d'égalité de dette, l'ordre de tri (basé sur la source) fait
    // office de tie-breaker strict.
    if (it == mouvements_gagnants.end() ||
        mouvement.dette_besoin_source > it->second.dette_besoin_source) {
      mouvements_gagnants[mouvement.destination] = mouvement;
    }
  }

  // Applique les mouvements qui ont gagné la résolution de conflit.
  for (const auto &pair : mouvements_gagnants) {
    const auto &mouvement = pair.second;
    Cellule &source_cell =
        getCellule(std::get<0>(mouvement.source), std::get<1>(mouvement.source),
                   std::get<2>(mouvement.source));
    Cellule &dest_cell = getCellule(std::get<0>(mouvement.destination),
                                    std::get<1>(mouvement.destination),
                                    std::get<2>(mouvement.destination));

    dest_cell = source_cell; // Copie la cellule vers la destination
    source_cell = {};        // Réinitialise la cellule source
  }
  mouvements_souhaites.clear();
}

// Génère une mutation déterministe pour R et Sc lors de la division.
// La mutation dépend des coordonnées de la case cible et de l'âge de la
// nouvelle cellule, garantissant que le résultat est toujours le même pour une
// situation donnée. Ceci est crucial pour la reproductibilité de la simulation.
float deterministic_mutation(int x, int y, int z, int age) {
  // Utilise des nombres premiers pour améliorer la distribution du hash.
  unsigned int hash = (x * 18397) + (y * 20441) + (z * 22543) + (age * 24671);
  int decision = hash % 3; // Décide entre +0.01, -0.01, ou 0.0
  if (decision == 0)
    return 0.01f;
  if (decision == 1)
    return -0.01f;
  return 0.0f;
}

// Applique la Loi 2 (Division) : Détermine si une cellule doit se diviser et
// où.
void MondeSED::AppliquerLoiDivision(int x, int y, int z,
                                    const std::vector<Cellule> &read_grid) {
  const Cellule &mere = getCellule(x, y, z, read_grid);
  if (!mere.is_alive || mere.E <= params.SEUIL_ENERGIE_DIVISION)
    return;

  std::tuple<int, int, int> meilleure_cible;
  float max_resistance_voisin = -1.0f;
  bool cible_trouvee = false;

  std::tuple<int, int, int> voisins[26];
  int nombre_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, nombre_voisins);

  // Cherche la case voisine vide avec la plus haute "Résistance Innée" (R).
  // Cibler une `R` élevée (même si la case est vide) favorise la cohésion
  // génétique.
  for (int i = 0; i < nombre_voisins; ++i) {
    const auto &coords_voisin = voisins[i];
    const Cellule &voisin_cell =
        getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin),
                   std::get<2>(coords_voisin), read_grid);
    if (!voisin_cell.is_alive) { // Cible une case vide
      if (voisin_cell.R > max_resistance_voisin) {
        max_resistance_voisin = voisin_cell.R;
        meilleure_cible = coords_voisin;
        cible_trouvee = true;
      }
    }
  }

  if (cible_trouvee) {
#pragma omp critical
    divisions_souhaitees.push_back(
        {std::make_tuple(x, y, z), meilleure_cible, mere.E});
  }
}

// Résout les conflits de division et applique les divisions validées.
void MondeSED::AppliquerDivisions() {
  // TRI DÉTERMINISTE
  std::sort(divisions_souhaitees.begin(), divisions_souhaitees.end(),
            [](const DivisionSouhaitee &a, const DivisionSouhaitee &b) {
              return a.source_mere < b.source_mere;
            });

  // Gère les conflits : si plusieurs cellules visent la même case,
  // seule celle avec l'Énergie (E) la plus élevée l'emporte.
  std::map<std::tuple<int, int, int>, DivisionSouhaitee> divisions_gagnantes;
  for (const auto &division : divisions_souhaitees) {
    auto it = divisions_gagnantes.find(division.destination_fille);
    if (it == divisions_gagnantes.end() ||
        division.energie_mere > it->second.energie_mere) {
      divisions_gagnantes[division.destination_fille] = division;
    }
  }

  // Applique les divisions qui ont gagné la résolution de conflit.
  for (const auto &pair : divisions_gagnantes) {
    const auto &division = pair.second;
    Cellule &mere = getCellule(std::get<0>(division.source_mere),
                               std::get<1>(division.source_mere),
                               std::get<2>(division.source_mere));
    Cellule &fille = getCellule(std::get<0>(division.destination_fille),
                                std::get<1>(division.destination_fille),
                                std::get<2>(division.destination_fille));

    // Divise l'énergie et copie les propriétés.
    // Divise l'énergie avec conservation STRICTE
    // Mère perd la moitié (ou un coût)
    // Fille gagne la moitié
    float energie_initiale = mere.E;
    mere.E = energie_initiale / 2.0f;
    mere.E -= params.COUT_DIVISION;

    fille = mere; // Copie R, Sc, et aussi le E divisé
    fille.E =
        energie_initiale / 2.0f; // Assure que la fille a exactement la moitié
    fille.A = 0;
    fille.D = 0.0f;        // Nouvelle "faim"
    fille.is_alive = true; // IMPORTANT

    // Applique une mutation déterministe à la fille.
    int dest_x = std::get<0>(division.destination_fille);
    int dest_y = std::get<1>(division.destination_fille);
    int dest_z = std::get<2>(division.destination_fille);
    fille.R += deterministic_mutation(dest_x, dest_y, dest_z, fille.A);
    fille.Sc += deterministic_mutation(dest_x, dest_y, dest_z,
                                       fille.A + 1); // +1 pour varier le hash

    // S'assure que les valeurs restent dans un intervalle valide [0, 1].
    fille.R = std::clamp(fille.R, 0.0f, 1.0f);
    fille.Sc = std::clamp(fille.Sc, 0.0f, 1.0f);
  }
  divisions_souhaitees.clear();
}

// Applique la Loi 4 (Échange Énergétique) : Partage d'énergie entre cellules
// similaires.
void MondeSED::AppliquerLoiEchange(int x, int y, int z,
                                   const std::vector<Cellule> &read_grid) {
  const Cellule &source = getCellule(x, y, z, read_grid);
  if (!source.is_alive)
    return;

  std::tuple<int, int, int> voisins[26];
  int nombre_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, nombre_voisins);

  for (int i = 0; i < nombre_voisins; ++i) {
    const auto &coords_voisin = voisins[i];
    const Cellule &voisin =
        getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin),
                   std::get<2>(coords_voisin), read_grid);
    // Condition: le voisin est vivant, génétiquement similaire, et a moins
    // d'énergie.
    // Condition: le voisin est vivant, génétiquement similaire, et a moins
    // d'énergie.
    if (voisin.is_alive &&
        std::abs(source.R - voisin.R) < params.SEUIL_SIMILARITE_R) {

      float diff_energie = source.E - voisin.E;
      if (diff_energie > params.SEUIL_DIFFERENCE_ENERGIE) {
        // Flux limité pour la stabilité (Osmose douce)
        float flux_theorique = diff_energie * params.FACTEUR_ECHANGE_ENERGIE;
        float montant = std::min(flux_theorique, params.MAX_FLUX_ENERGIE);

#pragma omp critical
        echanges_energie_souhaites.push_back(
            {std::make_tuple(x, y, z), coords_voisin, montant});
      }
    }
  }
}

// Applique la Loi 5 (Interaction Psychique) : Gestion de l'ennui et du stress.
void MondeSED::AppliquerLoiPsychisme(int x, int y, int z,
                                     const std::vector<Cellule> &read_grid) {
  const Cellule &source = getCellule(x, y, z, read_grid);
  if (!source.is_alive)
    return;

  std::tuple<int, int, int> voisin_le_plus_calme;
  float min_dette_stimulus = std::numeric_limits<float>::max();
  bool voisin_trouve = false;

  std::tuple<int, int, int> voisins[26];
  int nombre_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, nombre_voisins);

  // Recherche le voisin vivant avec le plus faible niveau d'ennui (L).
  for (int i = 0; i < nombre_voisins; ++i) {
    const auto &coords_voisin = voisins[i];
    const Cellule &voisin =
        getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin),
                   std::get<2>(coords_voisin), read_grid);
    if (voisin.is_alive && voisin.L < min_dette_stimulus) {
      min_dette_stimulus = voisin.L;
      voisin_le_plus_calme = coords_voisin;
      voisin_trouve = true;
    }
  }

  // Si un voisin est trouvé, planifie une interaction psychique.
  if (voisin_trouve) {
    float echange_C =
        source.C * params.FACTEUR_ECHANGE_PSYCHIQUE; // Le stress augmente
    float echange_L =
        source.L * params.FACTEUR_ECHANGE_PSYCHIQUE; // L'ennui diminue
#pragma omp critical
    echanges_psychiques_souhaites.push_back(
        {std::make_tuple(x, y, z), voisin_le_plus_calme, echange_C, echange_L});
  }
}

// Applique les échanges d'énergie planifiés.
void MondeSED::AppliquerEchangesEnergie() {
  // TRI DÉTERMINISTE :
  // Indispensable pour que l'ordre des additions/soustractions flottantes soit
  // constant. On trie par source PUIS destination (car une source peut avoir
  // plusieurs destinations).
  std::sort(
      echanges_energie_souhaites.begin(), echanges_energie_souhaites.end(),
      [](const EchangeEnergieSouhaite &a, const EchangeEnergieSouhaite &b) {
        if (a.source != b.source)
          return a.source < b.source;
        return a.destination < b.destination;
      });

  for (const auto &echange : echanges_energie_souhaites) {
    // La cellule source donne de l'énergie, la destination en reçoit.
    getCellule(std::get<0>(echange.source), std::get<1>(echange.source),
               std::get<2>(echange.source))
        .E -= echange.montant_energie;
    getCellule(std::get<0>(echange.destination),
               std::get<1>(echange.destination),
               std::get<2>(echange.destination))
        .E += echange.montant_energie;
  }
  echanges_energie_souhaites.clear();
}

// Applique les échanges psychiques planifiés.
void MondeSED::AppliquerEchangesPsychiques() {
  // TRI DÉTERMINISTE :
  std::sort(
      echanges_psychiques_souhaites.begin(),
      echanges_psychiques_souhaites.end(),
      [](const EchangePsychiqueSouhaite &a, const EchangePsychiqueSouhaite &b) {
        if (a.source != b.source)
          return a.source < b.source;
        return a.destination < b.destination;
      });

  for (const auto &echange : echanges_psychiques_souhaites) {
    Cellule &source =
        getCellule(std::get<0>(echange.source), std::get<1>(echange.source),
                   std::get<2>(echange.source));
    Cellule &dest = getCellule(std::get<0>(echange.destination),
                               std::get<1>(echange.destination),
                               std::get<2>(echange.destination));
    // Le stress (C) augmente pour les deux, l'ennui (L) diminue pour les deux.
    source.C += echange.montant_C;
    dest.C += echange.montant_C;
    source.L -= echange.montant_L;
    dest.L -= echange.montant_L;

    // Clamping immédiat pour sécurité
    source.C = std::clamp(source.C, 0.0f, 1.0f);
    dest.C = std::clamp(dest.C, 0.0f, 1.0f);
    source.L = std::max(0.0f, source.L);
    dest.L = std::max(0.0f, dest.L);
  }
  echanges_psychiques_souhaites.clear();
}

// --- Boucle de Simulation Principale ---

// Fait avancer la simulation d'un cycle.
// L'ordre des opérations est crucial pour le déterminisme.
void MondeSED::AvancerTemps() {
  cycle_actuel++;

  // --- 1. Phase de Décision (Lecture seule) ---
  // Une copie de la grille est utilisée pour que toutes les décisions soient
  // basées sur l'état du monde AU MÊME moment. Cela garantit le déterminisme
  // même en parallèle.
  const std::vector<Cellule> read_grid = grille;

  // Réinitialise les listes d'actions souhaitées.
  mouvements_souhaites.clear();
  divisions_souhaitees.clear();
  echanges_energie_souhaites.clear();
  echanges_psychiques_souhaites.clear();

// Chaque cellule prend ses décisions en parallèle.
// Chaque cellule prend ses décisions en parallèle.
// NOTE: 'collapse(3)' removed as MSVC OpenMP 2.0 does not support it.
// Parallelizing outer loop is sufficient.
#pragma omp parallel for
  for (int z = 0; z < size_z; ++z) {
    for (int y = 0; y < size_y; ++y) {
      for (int x = 0; x < size_x; ++x) {
        AppliquerLoiMouvement(x, y, z, read_grid);
        AppliquerLoiDivision(x, y, z, read_grid);
        AppliquerLoiEchange(x, y, z, read_grid);
        AppliquerLoiPsychisme(x, y, z, read_grid);
      }
    }
  }

  // --- 2. Phase d'Action (Écriture séquentielle) ---
  // Les actions sont appliquées séquentiellement après la résolution des
  // conflits.
  AppliquerMouvements();
  AppliquerDivisions();
  AppliquerEchangesEnergie();
  AppliquerEchangesPsychiques();

// --- 3. Phase de Mise à Jour de l'État (Écriture parallèle) ---
// Les lois de survie et de vieillissement sont appliquées en dernier.
// Chaque cellule prend ses décisions en parallèle.
// NOTE: 'collapse(3)' removed as MSVC OpenMP 2.0 does not support it.
// Parallelizing outer loop is sufficient.
#pragma omp parallel for
  for (int z = 0; z < size_z; ++z) {
    for (int y = 0; y < size_y; ++y) {
      for (int x = 0; x < size_x; ++x) {
        AppliquerLoiZero(x, y, z);
      }
    }
  }
}

// --- Accesseurs ---

const std::vector<Cellule> &MondeSED::getGrille() const { return grille; }

// --- Sauvegarde et Chargement ---

// Sauvegarde l'état complet de la simulation dans un fichier binaire.
bool MondeSED::SauvegarderEtat(const std::string &nom_fichier) const {
  std::cout << "[DEBUG] SauvegarderEtat: Debut de sauvegarde vers "
            << nom_fichier << std::endl;
  std::ofstream fichier_sortie(nom_fichier, std::ios::binary);
  if (!fichier_sortie) {
    std::cerr << "Erreur: Impossible d'ouvrir le fichier de sauvegarde '"
              << nom_fichier << "'" << std::endl;
    return false;
  }

  // Écrit les métadonnées (dimensions, cycle, paramètres)
  fichier_sortie.write(reinterpret_cast<const char *>(&size_x), sizeof(size_x));
  fichier_sortie.write(reinterpret_cast<const char *>(&size_y), sizeof(size_y));
  fichier_sortie.write(reinterpret_cast<const char *>(&size_z), sizeof(size_z));
  fichier_sortie.write(reinterpret_cast<const char *>(&cycle_actuel),
                       sizeof(cycle_actuel));
  fichier_sortie.write(reinterpret_cast<const char *>(&params), sizeof(params));

  // Écrit l'intégralité de la grille.
  fichier_sortie.write(reinterpret_cast<const char *>(grille.data()),
                       grille.size() * sizeof(Cellule));

  fichier_sortie.close();
  std::cout << "État de la simulation sauvegardé dans '" << nom_fichier << "'."
            << std::endl;
  return true;
}

// Charge un état de simulation depuis un fichier binaire.
bool MondeSED::ChargerEtat(const std::string &nom_fichier) {
  std::ifstream fichier_entree(nom_fichier, std::ios::binary);
  if (!fichier_entree) {
    std::cerr << "Erreur: Impossible d'ouvrir le fichier de chargement '"
              << nom_fichier << "'" << std::endl;
    return false;
  }

  // Lit les métadonnées.
  fichier_entree.read(reinterpret_cast<char *>(&size_x), sizeof(size_x));
  fichier_entree.read(reinterpret_cast<char *>(&size_y), sizeof(size_y));
  fichier_entree.read(reinterpret_cast<char *>(&size_z), sizeof(size_z));
  fichier_entree.read(reinterpret_cast<char *>(&cycle_actuel),
                      sizeof(cycle_actuel));
  fichier_entree.read(reinterpret_cast<char *>(&params), sizeof(params));

  // Redimensionne la grille si nécessaire et lit les données des cellules.
  grille.resize(size_x * size_y * size_z);
  fichier_entree.read(reinterpret_cast<char *>(grille.data()),
                      grille.size() * sizeof(Cellule));

  fichier_entree.close();
  std::cout << "État de la simulation chargé depuis '" << nom_fichier << "'."
            << std::endl;
  return true;
}

// Charge les paramètres de simulation depuis un fichier texte de type
// clé=valeur.
bool MondeSED::ChargerParametresDepuisFichier(const std::string &nom_fichier) {
  std::ifstream fichier_params(nom_fichier);
  if (!fichier_params.is_open()) {
    std::cerr << "Avertissement: Fichier de paramètres '" << nom_fichier
              << "' introuvable. Utilisation des valeurs par défaut."
              << std::endl;
    return false;
  }

  std::string ligne;
  std::cout << "Chargement des paramètres depuis '" << nom_fichier << "'..."
            << std::endl;
  while (std::getline(fichier_params, ligne)) {
    // Ignore les lignes vides ou les commentaires.
    if (ligne.empty() || ligne[0] == '#') {
      continue;
    }

    std::istringstream iss(ligne);
    std::string cle, valeur_str;
    if (std::getline(iss, cle, '=') && std::getline(iss, valeur_str)) {
      try {
        // Associe la clé lue à la variable de paramètre correspondante.
        float valeur = std::stof(valeur_str);
        if (cle == "K_D")
          params.K_D = valeur;
        else if (cle == "K_C")
          params.K_C = valeur;
        else if (cle == "K_M")
          params.K_M = valeur;
        else if (cle == "K_THERMO")
          params.K_THERMO = valeur;
        else if (cle == "D_PER_TICK")
          params.D_PER_TICK = valeur;
        else if (cle == "L_PER_TICK")
          params.L_PER_TICK = valeur;
        else if (cle == "COUT_MOUVEMENT")
          params.COUT_MOUVEMENT = valeur;
        else if (cle == "SEUIL_ENERGIE_DIVISION")
          params.SEUIL_ENERGIE_DIVISION = valeur;
        else if (cle == "COUT_DIVISION")
          params.COUT_DIVISION = valeur;
        else if (cle == "RAYON_DIFFUSION")
          params.RAYON_DIFFUSION = valeur;
        else if (cle == "ALPHA_ATTENUATION")
          params.ALPHA_ATTENUATION = valeur;
        else if (cle == "FACTEUR_ECHANGE_ENERGIE")
          params.FACTEUR_ECHANGE_ENERGIE = valeur;
        else if (cle == "SEUIL_DIFFERENCE_ENERGIE")
          params.SEUIL_DIFFERENCE_ENERGIE = valeur;
        else if (cle == "SEUIL_SIMILARITE_R")
          params.SEUIL_SIMILARITE_R = valeur;
        else if (cle == "MAX_FLUX_ENERGIE")
          params.MAX_FLUX_ENERGIE = valeur;
        else if (cle == "FACTEUR_ECHANGE_PSYCHIQUE")
          params.FACTEUR_ECHANGE_PSYCHIQUE = valeur;
        else if (cle == "TAUX_OUBLI")
          params.TAUX_OUBLI = valeur;
        else if (cle == "intervalle_export")
          params.intervalle_export = static_cast<int>(valeur);
      } catch (const std::invalid_argument &e) {
        std::cerr
            << "Avertissement: Ligne invalide dans le fichier de paramètres: "
            << ligne << std::endl;
      }
    }
  }
  std::cout << "Paramètres chargés." << std::endl;
  return true;
}