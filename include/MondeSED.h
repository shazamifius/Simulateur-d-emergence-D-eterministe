#ifndef MONDESED_H
#define MONDESED_H

#include <string>
#include <tuple>
#include <vector>

// --- Structures de Données Publiques ---

/**
 * @struct ParametresGlobaux
 * @brief Contient tous les paramètres ajustables de la simulation.
 * Ces "leviers" permettent de modifier le comportement des lois sans recompiler.
 */
struct ParametresGlobaux {
// Loi 0: Thermodynamique & Survie
    float K_THERMO = 0.001f;      // Coût énergétique par cycle (Métabolisme).
    float D_PER_TICK = 0.002f;    // Augmentation de la dette (faim) par cycle.
    float L_PER_TICK = 0.001f;    // Augmentation de l'ennui par cycle.

    // Loi 1: Dynamique (Mouvement)
    float K_D = 1.0f;             // Poids gravitaire (Dette).
    float K_C = 0.5f;             // Poids répulsif (Stress).
    float K_M = 0.5f;             // Poids inertiel (Mémoire).
    float COUT_MOUVEMENT = 0.001f;// Coût énergétique d'un déplacement.

    // Loi 2: Reproduction (Mitose)
    float SEUIL_ENERGIE_DIVISION = 1.8f;
    float COUT_DIVISION = 0.0f;   // Coût extra (optionnel), sinon conservation stricte.

    // Loi 3: Champs (Action à distance)
    float RAYON_DIFFUSION = 2.0f; // Rayon d'influence des champs.
    float ALPHA_ATTENUATION = 1.0f; // Coefficient d'atténuation (exp(-alpha * dist)).

    // Loi 4: Osmose (Échange Énergétique)
    float FACTEUR_ECHANGE_ENERGIE = 0.05f;
    float SEUIL_DIFFERENCE_ENERGIE = 0.2f;
    float SEUIL_SIMILARITE_R = 0.1f; // Tolerance génétique.
    float MAX_FLUX_ENERGIE = 0.05f;  // Limite par échange pour stabilité.

    // Loi 5: Interaction Forte (Psychique)
    float FACTEUR_ECHANGE_PSYCHIQUE = 0.1f;

    // Mémoire
    float TAUX_OUBLI = 0.01f;     // Décroissance exponentielle de la mémoire.

    // Exportation
    int intervalle_export = 10;
};

/**
 * @struct Cellule
 * @brief Représente l'état complet d'un Voxel dans la grille du monde.
 * Les noms de variables (E, D, C, etc.) sont courts pour correspondre à la documentation mathématique.
 */
struct Cellule {
    // --- Constantes de Naissance (Morphologie) ---
    float R = 0.0f;  // Résistance Innée: "facteur rebelle", influence les interactions.
    float Sc = 0.0f; // Seuil Critique: tolérance maximale au stress.

    // --- Variables Dynamiques (État) ---
    float E = 0.0f;  // Énergie: ressource vitale.
    float D = 0.0f;  // Dette de Besoin: pression des besoins (faim, etc.), pilote le déplacement.
    float C = 0.0f;  // Charge Émotionnelle: niveau de stress.
    float L = 0.0f;  // Dette de Stimulus: niveau d'ennui.
    float M = 0.0f;  // Mémoire: mémorise la plus haute énergie vue dans le voisinage.
    int A = 0;       // Âge: compteur de cycles de vie.

    bool is_alive = false; // État de vie de la cellule.
};

// --- Structures pour Actions Différées ---
// Ces structures stockent les actions décidées pendant la phase de lecture
// pour les appliquer plus tard dans la phase d'écriture, garantissant le déterminisme.

struct MouvementSouhaite {
    std::tuple<int, int, int> source;
    std::tuple<int, int, int> destination;
    float dette_besoin_source; // Utilisé pour la résolution de conflit.
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

/**
 * @class MondeSED
 * @brief Classe principale gérant la grille de simulation, les lois et l'état global.
 */
class MondeSED {
public:
    // --- Configuration et Cycle de Vie ---
    MondeSED(int size_x, int size_y, int size_z);
    void InitialiserMonde(unsigned int seed, float initial_density = 0.5f);
    void AvancerTemps();
    void ExporterEtatMonde(const std::string& nom_de_base) const;

    // --- Sauvegarde et Chargement ---
    bool SauvegarderEtat(const std::string& nom_fichier) const;
    bool ChargerEtat(const std::string& nom_fichier);
    bool ChargerParametresDepuisFichier(const std::string& nom_fichier);

    // --- Accesseurs (Getters) ---
    const std::vector<Cellule>& getGrille() const;
    int getTailleX() const { return size_x; }
    int getTailleY() const { return size_y; }
    int getTailleZ() const { return size_z; }
    int getCycleActuel() const { return cycle_actuel; }
    unsigned int getSeed() const { return current_seed; }
    int getNombreCellulesVivantes() const;

    // --- Paramètres Publics ---
    ParametresGlobaux params; // Permet à l'UI de modifier les paramètres en temps réel.

private:
    // --- État Interne ---
    int size_x, size_y, size_z;
    int cycle_actuel = 0;
    unsigned int current_seed = 0;
    std::vector<Cellule> grille;

    // --- Vecteurs d'Actions Différées ---
    std::vector<MouvementSouhaite> mouvements_souhaites;
    std::vector<DivisionSouhaitee> divisions_souhaitees;
    std::vector<EchangeEnergieSouhaite> echanges_energie_souhaites;
    std::vector<EchangePsychiqueSouhaite> echanges_psychiques_souhaites;

    // --- Fonctions d'Application des Lois (Logique Interne) ---
    void AppliquerLoiZero(int x, int y, int z);
    void AppliquerLoiMouvement(int x, int y, int z, const std::vector<Cellule>& read_grid);
    void AppliquerLoiDivision(int x, int y, int z, const std::vector<Cellule>& read_grid);
    void AppliquerLoiEchange(int x, int y, int z, const std::vector<Cellule>& read_grid);
    void AppliquerLoiPsychisme(int x, int y, int z, const std::vector<Cellule>& read_grid);

    // --- Fonctions de Résolution (Logique Interne) ---
    void AppliquerMouvements();
    void AppliquerDivisions();
    void AppliquerEchangesEnergie();
    void AppliquerEchangesPsychiques();

    // --- Fonctions Utilitaires (Logique Interne) ---
    int getIndex(int x, int y, int z) const;
    Cellule& getCellule(int x, int y, int z);
    const Cellule& getCellule(int x, int y, int z, const std::vector<Cellule>& grid) const;

    // --- Fonctions Utilitaires (Optimisation) ---
    // Nouvelle méthode optimisée qui écrit directement dans un tableau pré-alloué.
    void GetCoordsVoisins_Optimized(int x, int y, int z, std::tuple<int, int, int>* voisins_array, int& count) const;
};

#endif // MONDESED_H