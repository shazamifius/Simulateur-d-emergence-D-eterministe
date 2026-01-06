#ifndef SIMULATION_DATA_H
#define SIMULATION_DATA_H

#include <array>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

// --- Structures de Données Publiques ---

/**
 * @struct ParametresGlobaux
 * @brief Contient tous les paramètres ajustables de la simulation.
 * Ces "leviers" permettent de modifier le comportement des lois sans
 * recompiler.
 */
struct ParametresGlobaux {
  // Loi 0: Thermodynamique & Survie
  float K_THERMO = 0.001f;   // Coût énergétique par cycle (Métabolisme).
  float D_PER_TICK = 0.002f; // Augmentation de la dette (faim) par cycle.
  float L_PER_TICK = 0.001f; // Augmentation de l'ennui par cycle.

  // Environnement (Soleil / Photosynthèse)
  float SENSIBILITE_SOLEIL = 0.005f; // Gain d'énergie par tick si exposé.
  float HAUTEUR_SOLEIL =
      0.8f; // % de la hauteur max (Y) pour recevoir le soleil (Simple Mode).

  // Regulation Dynamique (Homeostasie) - REMOVED for Strict Determinism (Spec
  // V8) float TARGET_TOTAL_ENERGY = 10000.0f; float ADAPTATION_RATE = 0.01f;
  // float K_THERMO_INITIAL = 0.001f;
  // float COUT_MOUVEMENT_INITIAL = 0.01f;

  // Loi 1: Dynamique (Mouvement)
  float K_D = 1.0f;             // Poids gravitaire (Dette).
  float K_C = 0.5f;             // Poids répulsif (Stress).
  float K_M = 0.5f;             // Poids inertiel (Mémoire).
  float K_ADH = 0.5f;           // Poids adhésion (Cohésion sociale/tissulaire).
  float COUT_MOUVEMENT = 0.01f; // Coût énergétique d'un déplacement.

  // Loi 2: Reproduction (Mitose)
  float SEUIL_ENERGIE_DIVISION = 1.8f;
  float COUT_DIVISION =
      0.0f; // Coût extra (optionnel), sinon conservation stricte.

  // Loi 3: Champs (Action à distance)
  float RAYON_DIFFUSION = 2.0f; // Rayon d'influence des champs.
  float ALPHA_ATTENUATION =
      1.0f;               // Coefficient d'atténuation (exp(-alpha * dist)).
  float K_CHAMP_E = 1.0f; // Poids du champ d'énergie.
  float K_CHAMP_C = 1.0f; // Poids du champ de stress.

  // Loi 4: Osmose (Échange Énergétique)
  float FACTEUR_ECHANGE_ENERGIE = 0.1f;
  float SEUIL_DIFFERENCE_ENERGIE =
      0.2f;                        // Peut-être pas dans la doc V8, mais utile ?
  float SEUIL_SIMILARITE_R = 0.1f; // Tolerance génétique.
  float MAX_FLUX_ENERGIE = 0.05f;  // Limite par échange pour stabilité.

  // Loi 5: Interaction Forte (Psychique)
  float FACTEUR_ECHANGE_PSYCHIQUE = 0.1f;

  // Morphogenèse (Lois Structurelles)
  float LAMBDA_GRADIENT = 0.1f; // Pour calcul du gradient G.
  float SEUIL_SOMA = 0.3f;      // Seuil G pour devenir Soma.
  float SEUIL_NEURO = 0.7f;     // Seuil G pour devenir Neurone.

  // Dynamique Neurale (Temps Rapide)
  int TICKS_NEURAUX_PAR_PHYSIQUE = 5; // N
  float COUT_SPIKE = 0.005f;
  int PERIODE_REFRACTAIRE = 2; // En ticks neuraux.
  float SEUIL_FIRE = 0.85f;    // Seuil activation neurone. (Spec: 0.85)
  float DECAY_SYNAPSE = 0.999f;
  float LEARN_RATE = 0.05f;    // (Spec: 0.05)
  float RAYON_IGNITION = 4.0f; // (Spec: 4)

  // Mémoire
  float TAUX_OUBLI =
      0.01f; // Décroissance exponentielle de la mémoire (matches 0.99 decay).

  // Exportation
  int intervalle_export = 10;

  // --- Sécurité & Limites (Optimisation) ---
  // Limites strictes pour éviter les crashs sur configs modestes.
  size_t MAX_CELLS = 1000000; // 1M cellules par défaut (Override par init)
  size_t MAX_RAM_MB = 1024;   // 1GO RAM Max pour la simu
  int WORLD_HEIGHT = 32; // Hauteur du monde (pour Soleil). Override au start.
  bool limit_safety_override =
      false; // "God Mode": Désactive les limites de sécurité.
};

/**
 * @struct Cellule
 * @brief Représente l'état complet d'un Voxel dans la grille du monde.
 * Les noms de variables (E, D, C, etc.) sont courts pour correspondre à la
 * documentation mathématique.
 */
struct Cellule {
  // --- Identité ---
  uint8_t T = 0; // Type: 0=Souche, 1=Soma, 2=Neurone, 3=Static Block (Floor)

  // --- Constantes de Naissance (Morphologie) ---
  float R =
      0.0f; // Résistance Innée: "facteur rebelle", influence les interactions.
  float Sc = 0.0f; // Seuil Critique: tolérance maximale au stress.

  // --- Variables Dynamiques (État Physique) ---
  float E = 0.0f; // Énergie: ressource vitale.
  float D = 0.0f; // Dette de Besoin: pression des besoins (faim, etc.), pilote
                  // le déplacement.
  float C = 0.0f; // Charge Émotionnelle: niveau de stress.
  float L = 0.0f; // Dette de Stimulus: niveau d'ennui.
  float M =
      0.0f;  // Mémoire: mémorise la plus haute énergie vue dans le voisinage.
  int A = 0; // Âge: compteur de cycles de vie.

  // --- Variables Neurales & Spatiales ---
  float P = 0.0f;       // Potentiel électrique [-1, 1].
  int Ref = 0;          // Compteur réfractaire.
  float E_cost = 0.0f;  // Coût énergétique accumulé (spikes).
  float W[27] = {0.0f}; // Poids synaptiques (26 voisins + centre inutilisé).
  uint32_t H = 0;       // Historique des spikes (Bitfield).
  float G = 0.0f;       // Gradient spatial.

  bool is_alive = false; // État de vie de la cellule.
};

// --- Structures pour Actions Différées ---
struct MouvementSouhaite {
  std::tuple<int, int, int> source;
  std::tuple<int, int, int> destination;
  float dette_besoin_source; // Utilisé pour la résolution de conflit.
  int index_source;          // Pour tie-breaker stable
};

struct DivisionSouhaitee {
  std::tuple<int, int, int> source_mere;
  std::tuple<int, int, int> destination_fille;
  float energie_mere; // Utilisé pour la résolution de conflit.
};

struct EchangeEnergieSouhaite {
  std::tuple<int, int, int> source;
  std::tuple<int, int, int> destination;
  float montant_energie;
};

struct EchangePsychiqueSouhaite {
  std::tuple<int, int, int> source;
  std::tuple<int, int, int> destination;
  float montant_C;
  float montant_L;
};

struct MetriquesMonde {
  int population_totale = 0;
  float taille_moyenne_organismes =
      0.0f; // Pour l'instant = population (chaque cellule est un invidivu
            // autonome si pas de lien structurel)
  float taux_survie_moyen = 0.0f; // Basé sur l'âge moyen ou l'énergie moyenne
  float connectivite_neurale_moyenne = 0.0f;

  // Heatmap simplifiée (comptage par zone, reset à chaque calcul ou cumulatif?)
  // On va le gérer dynamiquement dans l'export pour l'instant
};

#endif // SIMULATION_DATA_H
