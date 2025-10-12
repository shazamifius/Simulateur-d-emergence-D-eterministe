#ifndef MONDESED_H
#define MONDESED_H

#include <vector>

// Définition de la structure Cellule
struct Cellule {
    float reserve_energie;      // E
    float dette_besoin;         // D
    float resistance_stress;    // R
    float score_survie;         // S
    int horloge_interne;        // H
    int age;                    // A
    bool est_vivante;           // V
};

class MondeSED {
public:
    MondeSED(int size_x, int size_y, int size_z);
    void InitialiserMonde();
    void AppliquerLoiZero(int x, int y, int z);
    Cellule& getCellule(int x, int y, int z);

private:
    int size_x, size_y, size_z;
    std::vector<Cellule> grille;
    int getIndex(int x, int y, int z);
};

#endif // MONDESED_H