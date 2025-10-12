#include "MondeSED.h"
#include <vector>
#include <random>
#include <ctime>

MondeSED::MondeSED(int sx, int sy, int sz) : size_x(sx), size_y(sy), size_z(sz) {
    grille.resize(size_x * size_y * size_z);
}

int MondeSED::getIndex(int x, int y, int z) {
    return x + y * size_x + z * size_x * size_y;
}

Cellule& MondeSED::getCellule(int x, int y, int z) {
    return grille[getIndex(x, y, z)];
}

void MondeSED::InitialiserMonde() {
    std::mt19937 rng(static_cast<unsigned int>(std::time(0)));
    std::uniform_real_distribution<float> random_float(0.0f, 1.0f);
    std::uniform_real_distribution<float> random_r_s(0.1f, 0.9f);

    for (int z = 0; z < size_z; ++z) {
        for (int y = 0; y < size_y; ++y) {
            for (int x = 0; x < size_x; ++x) {
                Cellule& cell = getCellule(x, y, z);
                if (random_float(rng) < 0.5f) {
                    // Cellule vivante
                    cell.reserve_energie = 1.0f;
                    cell.dette_besoin = 0.0f;
                    cell.resistance_stress = random_r_s(rng);
                    cell.score_survie = random_r_s(rng);
                    cell.horloge_interne = 0;
                    cell.age = 0;
                    cell.est_vivante = true;
                } else {
                    // Voxel vide
                    cell.reserve_energie = 0.0f;
                    cell.dette_besoin = 0.0f;
                    cell.resistance_stress = 0.0f;
                    cell.score_survie = 0.0f;
                    cell.horloge_interne = 0;
                    cell.age = 0;
                    cell.est_vivante = false;
                }
            }
        }
    }
}

void MondeSED::AppliquerLoiZero(int x, int y, int z) {
    Cellule& cell = getCellule(x, y, z);
    if (cell.reserve_energie > 0) {
        cell.reserve_energie -= 0.001f;
        cell.dette_besoin += 0.002f;

        if (cell.reserve_energie <= 0) {
            cell.reserve_energie = 0; // S'assurer qu'elle ne devienne pas négative
            cell.est_vivante = false; // La cellule meurt
        }
    }
}