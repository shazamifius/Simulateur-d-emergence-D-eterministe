#ifndef MONDESED_H
#define MONDESED_H

#include <vector>
#include <tuple>

// Définition de la structure Cellule
struct Cellule {
    float reserve_energie;      // E
    float dette_besoin;         // D
    float dette_stimulus;       // L (new)
    float resistance_stress;    // R
    float seuil_critique;       // Sc
    float score_survie;         // S
    float charge_emotionnelle;  // C
    int horloge_interne;        // H
    int age;                    // A
    bool est_vivante;           // V
};

// --- Structures for deferred updates ---
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
    void InitialiserMonde();
    void AvancerTemps();

    // Law application functions
    void AppliquerLoiZero(int x, int y, int z);
    void AppliquerLoiMouvement(int x, int y, int z, const std::vector<Cellule>& read_grid);
    void AppliquerLoiDivision(int x, int y, int z, const std::vector<Cellule>& read_grid);
    void AppliquerLoiEchange(int x, int y, int z, const std::vector<Cellule>& read_grid);
    void AppliquerLoiPsychisme(int x, int y, int z, const std::vector<Cellule>& read_grid);

    // Deferred action functions
    void AppliquerMouvements();
    void AppliquerDivisions();
    void AppliquerEchangesEnergie();
    void AppliquerEchangesPsychiques();

    Cellule& getCellule(int x, int y, int z);
    const Cellule& getCellule(int x, int y, int z, const std::vector<Cellule>& grid) const;
    int getTailleX() const { return size_x; }
    int getTailleY() const { return size_y; }
    int getTailleZ() const { return size_z; }
    const std::vector<Cellule>& getGrille() const { return grille; }

private:
    int size_x, size_y, size_z;
    std::vector<Cellule> grille;
    std::vector<MouvementSouhaite> mouvements_souhaites;
    std::vector<DivisionSouhaitee> divisions_souhaitees;
    std::vector<EchangeEnergieSouhaite> echanges_energie_souhaites;
    std::vector<EchangePsychiqueSouhaite> echanges_psychiques_souhaites;

    // --- LAW CONSTANTS ---
    static constexpr float K_E = 2.0f, K_D = 1.0f, K_C = 0.5f;
    static constexpr float SEUIL_ENERGIE_DIVISION = 1.8f;
    static constexpr float TAUX_AUGMENTATION_ENNUI = 0.001f;
    static constexpr float FACTEUR_ECHANGE_ENERGIE = 0.05f;
    static constexpr float SEUIL_DIFFERENCE_ENERGIE = 0.2f;
    static constexpr float SEUIL_SIMILARITE_R = 0.1f;
    static constexpr float FACTEUR_ECHANGE_PSYCHIQUE = 0.1f;

    int getIndex(int x, int y, int z) const;
    std::vector<std::tuple<int, int, int>> GetCoordsVoisins(int x, int y, int z) const;
};

#endif // MONDESED_H