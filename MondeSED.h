#ifndef MONDESED_H
#define MONDESED_H

#include <vector>
#include <tuple>

// Définition de la structure Cellule
struct Cellule {
    float reserve_energie;      // E
    float dette_besoin;         // D
    float resistance_stress;    // R
    float score_survie;         // S
    float charge_emotionnelle;  // C
    int horloge_interne;        // H
    int age;                    // A
    bool est_vivante;           // V
};

// Structure pour stocker un mouvement potentiel
struct MouvementSouhaite {
    std::tuple<int, int, int> source;
    std::tuple<int, int, int> destination;
    float dette_besoin_source; // Pour la résolution de conflit
};

class MondeSED {
public:
    MondeSED(int size_x, int size_y, int size_z);
    void InitialiserMonde();
    void AppliquerLoiZero(int x, int y, int z);
    void AppliquerLoiMouvement(int x, int y, int z);
    void AppliquerMouvements();
    Cellule& getCellule(int x, int y, int z);
    int getTailleX() const { return size_x; }
    int getTailleY() const { return size_y; }
    int getTailleZ() const { return size_z; }


private:
    int size_x, size_y, size_z;
    std::vector<Cellule> grille;
    std::vector<MouvementSouhaite> mouvements_souhaites;

    // Constantes pour la Loi 1 (Mouvement)
    static constexpr float K_E = 2.0f;
    static constexpr float K_D = 1.0f;
    static constexpr float K_C = 0.5f;

    int getIndex(int x, int y, int z);
    std::vector<std::tuple<int, int, int>> GetCoordsVoisins(int x, int y, int z);
};

#endif // MONDESED_H