#ifndef MONDESED_H
#define MONDESED_H

#include <vector>
#include <tuple>
#include <string>

// --- Global Parameters Struct ---
struct ParametresGlobaux {
    // Loi 1 (Mouvement)
    float K_E = 2.0f;
    float K_D = 1.0f;
    float K_C = 0.5f;
    // Loi 2 (Division)
    float SEUIL_ENERGIE_DIVISION = 1.8f;
    // Loi 4 (Echange)
    float FACTEUR_ECHANGE_ENERGIE = 0.05f;
    float SEUIL_DIFFERENCE_ENERGIE = 0.2f;
    float SEUIL_SIMILARITE_R = 0.1f;
    // Loi 5 (Psychisme)
    float TAUX_AUGMENTATION_ENNUI = 0.001f;
    float FACTEUR_ECHANGE_PSYCHIQUE = 0.1f;
    // Loi 6 (Mémoire)
    float K_M = 0.5f;
    // Export
    int intervalle_export = 10;
};

// Définition de la structure Cellule
struct Cellule {
    float reserve_energie = 0.0f;
    float dette_besoin = 0.0f;
    float dette_stimulus = 0.0f;
    float resistance_stress = 0.0f;
    float seuil_critique = 0.0f;
    float score_survie = 0.0f;
    float charge_emotionnelle = 0.0f;
    float memoire_energie_max = 0.0f; // Loi 6
    int horloge_interne = 0;
    int age = 0;
    bool est_vivante = false;
};

// --- Deferred Action Structs ---
struct MouvementSouhaite {
    std::tuple<int, int, int> source;
    std::tuple<int, int, int> destination;
    float dette_besoin_source;
};

struct DivisionSouhaitee {
    std::tuple<int, int, int> source_mere;
    std::tuple<int, int, int> destination_fille;
    float energie_mere;
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

class MondeSED {
public:
    MondeSED(int size_x, int size_y, int size_z);
    void InitialiserMonde(float initial_density = 0.5f); // Default density
    void AvancerTemps();
    void ExporterEtatMonde(const std::string& nom_de_base) const;

    // Accesseur public pour la grille (lecture seule) pour la visualisation
    const std::vector<Cellule>& getGrille() const;

    // Paramètres modifiables de la simulation
    ParametresGlobaux params;

    void AppliquerLoiZero(int x, int y, int z);
    void AppliquerLoiMouvement(int x, int y, int z, const std::vector<Cellule>& read_grid);
    void AppliquerLoiDivision(int x, int y, int z, const std::vector<Cellule>& read_grid);
    void AppliquerLoiEchange(int x, int y, int z, const std::vector<Cellule>& read_grid);
    void AppliquerLoiPsychisme(int x, int y, int z, const std::vector<Cellule>& read_grid);

    void AppliquerMouvements();
    void AppliquerDivisions();
    void AppliquerEchangesEnergie();
    void AppliquerEchangesPsychiques();

    Cellule& getCellule(int x, int y, int z);
    const Cellule& getCellule(int x, int y, int z, const std::vector<Cellule>& grid) const;
    int getTailleX() const { return size_x; }
    int getTailleY() const { return size_y; }
    int getTailleZ() const { return size_z; }

private:
    int size_x, size_y, size_z;
    int cycle_actuel = 0;
    std::vector<Cellule> grille;
    std::vector<MouvementSouhaite> mouvements_souhaites;
    std::vector<DivisionSouhaitee> divisions_souhaitees;
    std::vector<EchangeEnergieSouhaite> echanges_energie_souhaites;
    std::vector<EchangePsychiqueSouhaite> echanges_psychiques_souhaites;

    int getIndex(int x, int y, int z) const;
    std::vector<std::tuple<int, int, int>> GetCoordsVoisins(int x, int y, int z) const;
};

#endif // MONDESED_H