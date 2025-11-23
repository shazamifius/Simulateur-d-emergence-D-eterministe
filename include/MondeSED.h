#ifndef MONDESED_H
#define MONDESED_H

#include <string>
#include <tuple>
#include <vector>

// --- Structures de Données Publiques ---

/**
 * @struct ParametresGlobaux
 * @brief Contient tous les paramètres ajustables de la simulation.
 * @details Ces "leviers" permettent de modifier le comportement des lois fondamentales
 * de la simulation sans avoir besoin de recompiler le code. Ils sont directement
 * modifiables depuis l'interface utilisateur.
 */
struct ParametresGlobaux {
    // Loi 1: Mouvement
    float K_D = 1.0f;    /**< @brief Poids de la motivation due à la faim (D) dans le calcul du score de déplacement. */
    float K_C = 0.5f;    /**< @brief Poids de l'aversion au stress (C) dans le calcul du score de déplacement. */
    float K_M = 0.5f;    /**< @brief Poids de l'influence de la mémoire (M) dans le calcul du score de déplacement. */

    // Loi 2: Division
    float SEUIL_ENERGIE_DIVISION = 1.8f; /**< @brief Quantité d'énergie (E) qu'une cellule doit posséder pour pouvoir se diviser. */

    // Loi 4: Échange Énergétique
    float FACTEUR_ECHANGE_ENERGIE = 0.05f;   /**< @brief Pourcentage de la différence d'énergie à transférer lors d'un échange. */
    float SEUIL_DIFFERENCE_ENERGIE = 0.2f; /**< @brief Différence d'énergie minimale requise pour déclencher un échange. */
    float SEUIL_SIMILARITE_R = 0.1f;       /**< @brief Différence maximale de Résistance (R) pour être considéré comme "génétiquement similaire". */

    // Loi 5: Interaction Psychique
    float TAUX_AUGMENTATION_ENNUI = 0.001f;   /**< @brief Vitesse à laquelle la Dette de Stimulus (L) augmente passivement à chaque cycle. */
    float FACTEUR_ECHANGE_PSYCHIQUE = 0.1f; /**< @brief Intensité du stress (C) et du soulagement (L) générés lors d'une interaction. */

    // Exportation
    int intervalle_export = 10; /**< @brief Fréquence (en cycles) à laquelle les données de simulation sont exportées. */
};

/**
 * @struct Cellule
 * @brief Représente l'état complet d'un Voxel dans la grille tridimensionnelle du monde.
 * @details Les noms de variables (E, D, C, etc.) sont délibérément courts pour correspondre
 * directement à la documentation mathématique du projet SED.
 */
struct Cellule {
    // --- Constantes de Naissance (Morphologie) ---
    float R = 0.0f;  /**< @brief Résistance Innée: Un "facteur rebelle" qui influence les interactions sociales et la prise de décision. */
    float Sc = 0.0f; /**< @brief Seuil Critique: La tolérance maximale au stress (C) avant que la cellule ne meure. */

    // --- Variables Dynamiques (État) ---
    float E = 0.0f;  /**< @brief Énergie: La ressource vitale de la cellule, consommée à chaque cycle et nécessaire à la division. */
    float D = 0.0f;  /**< @brief Dette de Besoin: Une mesure de la "faim" ou du besoin de ressources, qui pilote le déplacement. */
    float C = 0.0f;  /**< @brief Charge Émotionnelle: Le niveau de stress ou de surcharge de la cellule. */
    float L = 0.0f;  /**< @brief Dette de Stimulus: Le niveau d'"ennui" de la cellule, qui motive les interactions sociales. */
    float M = 0.0f;  /**< @brief Mémoire: Mémorise la plus haute énergie (E) vue dans le voisinage direct. */
    int A = 0;       /**< @brief Âge: Le nombre de cycles de simulation que la cellule a survécu. */

    bool is_alive = false; /**< @brief Indique si la cellule est vivante (true) ou morte (false). */
};


/**
 * @struct MouvementSouhaite
 * @brief Stocke les informations d'une action de mouvement planifiée.
 * @details Utilisé pour la résolution de conflits déterministe.
 */
struct MouvementSouhaite {
    std::tuple<int, int, int> source; /**< @brief Coordonnées de la cellule qui souhaite se déplacer. */
    std::tuple<int, int, int> destination; /**< @brief Coordonnées de la case vide ciblée. */
    float dette_besoin_source; /**< @brief Valeur de 'D' de la cellule source, utilisée pour résoudre les conflits. */
};

/**
 * @struct DivisionSouhaitee
 * @brief Stocke les informations d'une action de division planifiée.
 * @details Utilisé pour la résolution de conflits déterministe.
 */
struct DivisionSouhaitee {
    std::tuple<int, int, int> source_mere; /**< @brief Coordonnées de la cellule mère. */
    std::tuple<int, int, int> destination_fille; /**< @brief Coordonnées où la cellule fille sera créée. */
    float energie_mere; /**< @brief Valeur de 'E' de la cellule mère, utilisée pour résoudre les conflits. */
};

/**
 * @struct EchangeEnergieSouhaite
 * @brief Stocke les informations d'un transfert d'énergie planifié.
 */
struct EchangeEnergieSouhaite {
    std::tuple<int, int, int> source; /**< @brief Coordonnées de la cellule donatrice. */
    std::tuple<int, int, int> destination; /**< @brief Coordonnées de la cellule réceptrice. */
    float montant_energie; /**< @brief Quantité d'énergie (E) à transférer. */
};

/**
 * @struct EchangePsychiqueSouhaite
 * @brief Stocke les informations d'une interaction psychique planifiée.
 */
struct EchangePsychiqueSouhaite {
    std::tuple<int, int, int> source; /**< @brief Coordonnées de la cellule initiant l'interaction. */
    std::tuple<int, int, int> destination; /**< @brief Coordonnées de la cellule ciblée. */
    float montant_C; /**< @brief Quantité de stress (C) à échanger. */
    float montant_L; /**< @brief Quantité de soulagement (L) à échanger. */
};

/**
 * @class MondeSED
 * @brief Classe principale gérant la grille de simulation, les lois et l'état global.
 * @details Cette classe encapsule toute la logique de la simulation, y compris la grille
 * tridimensionnelle de cellules, l'application des lois déterministes et la gestion
 * du cycle de vie de la simulation.
 */
class MondeSED {
public:
    // --- Configuration et Cycle de Vie ---

    /**
     * @brief Construit un nouveau monde de simulation.
     * @param size_x La taille du monde sur l'axe X.
     * @param size_y La taille du monde sur l'axe Y.
     * @param size_z La taille du monde sur l'axe Z.
     */
    MondeSED(int size_x, int size_y, int size_z);

    /**
     * @brief Initialise ou réinitialise le monde avec une "soupe primordiale" de cellules.
     * @param seed La graine pour le générateur de nombres aléatoires, garantissant la reproductibilité.
     * @param initial_density La densité approximative de cellules vivantes à la création du monde (entre 0.0 et 1.0).
     */
    void InitialiserMonde(unsigned int seed, float initial_density = 0.5f);

    /**
     * @brief Fait avancer la simulation d'un seul cycle (tick).
     * @details C'est la fonction centrale qui exécute les trois phases du cycle de simulation :
     * 1. Phase de Décision (parallèle)
     * 2. Phase d'Action (séquentielle, avec résolution de conflits)
     * 3. Phase de Mise à Jour de l'État (parallèle)
     */
    void AvancerTemps();

    /**
     * @brief Exporte l'état actuel des cellules vivantes dans un fichier CSV.
     * @param nom_de_base Le préfixe du nom de fichier. Le numéro de cycle sera ajouté automatiquement.
     */
    void ExporterEtatMonde(const std::string& nom_de_base) const;

    // --- Sauvegarde et Chargement ---

    /**
     * @brief Sauvegarde l'état complet de la simulation (cellules, paramètres, cycle) dans un fichier binaire.
     * @param nom_fichier Le chemin du fichier où sauvegarder l'état.
     * @return true si la sauvegarde a réussi, false sinon.
     */
    bool SauvegarderEtat(const std::string& nom_fichier) const;

    /**
     * @brief Charge un état de simulation depuis un fichier binaire.
     * @param nom_fichier Le chemin du fichier à partir duquel charger l'état.
     * @return true si le chargement a réussi, false sinon.
     */
    bool ChargerEtat(const std::string& nom_fichier);

    /**
     * @brief Charge les paramètres de la simulation depuis un fichier texte (clé=valeur).
     * @param nom_fichier Le chemin du fichier de paramètres.
     * @return true si le chargement a réussi, false sinon.
     */
    bool ChargerParametresDepuisFichier(const std::string& nom_fichier);

    // --- Accesseurs (Getters) ---

    /**
     * @brief Récupère une référence constante vers la grille de cellules.
     * @return Une référence const à `std::vector<Cellule>`.
     */
    const std::vector<Cellule>& getGrille() const;

    /**
     * @brief Récupère la taille du monde sur l'axe X.
     * @return La taille X.
     */
    int getTailleX() const { return size_x; }

    /**
     * @brief Récupère la taille du monde sur l'axe Y.
     * @return La taille Y.
     */
    int getTailleY() const { return size_y; }

    /**
     * @brief Récupère la taille du monde sur l'axe Z.
     * @return La taille Z.
     */
    int getTailleZ() const { return size_z; }

    /**
     * @brief Récupère le numéro du cycle de simulation actuel.
     * @return Le numéro du cycle.
     */
    int getCycleActuel() const { return cycle_actuel; }

    /**
     * @brief Récupère la graine utilisée pour l'initialisation actuelle du monde.
     * @return La graine de simulation.
     */
    unsigned int getSeed() const { return current_seed; }

    /**
     * @brief Compte et retourne le nombre total de cellules vivantes dans le monde.
     * @return Le nombre de cellules vivantes.
     */
    int getNombreCellulesVivantes() const;

    // --- Paramètres Publics ---
    ParametresGlobaux params; /**< @brief Instance publique des paramètres globaux, modifiable en temps réel par l'UI. */

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