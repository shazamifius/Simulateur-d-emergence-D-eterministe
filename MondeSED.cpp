#include "MondeSED.h"
#include <vector>
#include <random>
#include <ctime>
#include <algorithm>
#include <map>
#include <limits>
MondeSED::MondeSED(int sx, int sy, int sz) : size_x(sx), size_y(sy), size_z(sz) {
    grille.resize(size_x * size_y * size_z);
}

int MondeSED::getIndex(int x, int y, int z) {
    if (x < 0 || x >= size_x || y < 0 || y >= size_y || z < 0 || z >= size_z) {
        return -1; // Out of bounds
    }
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
                    cell.charge_emotionnelle = 0.0f;
                    cell.horloge_interne = 0;
                    cell.age = 0;
                    cell.est_vivante = true;
                } else {
                    // Voxel vide
                    cell.reserve_energie = 0.0f;
                    cell.dette_besoin = 0.0f;
                    cell.resistance_stress = 0.0f;
                    cell.score_survie = 0.0f;
                    cell.charge_emotionnelle = 0.0f;
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
    if (cell.est_vivante) { // More robust check
        cell.reserve_energie -= 0.001f;
        cell.dette_besoin += 0.002f;

        if (cell.reserve_energie <= 0) {
            cell.reserve_energie = 0; // S'assurer qu'elle ne devienne pas négative
            cell.est_vivante = false; // La cellule meurt
        }
    }
}

std::vector<std::tuple<int, int, int>> MondeSED::GetCoordsVoisins(int x, int y, int z) {
    std::vector<std::tuple<int, int, int>> voisins;
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0 && dz == 0) continue; // Skip center cell
                int nx = x + dx;
                int ny = y + dy;
                int nz = z + dz;
                if (getIndex(nx, ny, nz) != -1) {
                    voisins.emplace_back(nx, ny, nz);
                }
            }
        }
    }
    // Sort by 1D index for determinism
    std::sort(voisins.begin(), voisins.end(), [&](const std::tuple<int, int, int>& a, const std::tuple<int, int, int>& b) {
        return getIndex(std::get<0>(a), std::get<1>(a), std::get<2>(a)) < getIndex(std::get<0>(b), std::get<1>(b), std::get<2>(b));
    });
    return voisins;
}

void MondeSED::AppliquerLoiMouvement(int x, int y, int z) {
    Cellule& source_cell = getCellule(x, y, z);
    if (!source_cell.est_vivante) {
        return;
    }

    std::vector<std::tuple<int, int, int>> voisins = GetCoordsVoisins(x, y, z);
    std::tuple<int, int, int> meilleure_cible;
    float max_score = -std::numeric_limits<float>::infinity();
    bool cible_trouvee = false;

    for (const auto& coords_voisin : voisins) {
        int vx, vy, vz;
        std::tie(vx, vy, vz) = coords_voisin;
        Cellule& voisin_cell = getCellule(vx, vy, vz);

        if (voisin_cell.reserve_energie <= 0) { // Cible est vide
            float score = K_E * voisin_cell.reserve_energie + K_D * source_cell.dette_besoin - K_C * source_cell.charge_emotionnelle;

            if (score > max_score) {
                max_score = score;
                meilleure_cible = coords_voisin;
                cible_trouvee = true;
            }
            // Tie-breaking is handled by the deterministic order of iteration
        }
    }

    if (cible_trouvee) {
        mouvements_souhaites.push_back({std::make_tuple(x, y, z), meilleure_cible, source_cell.dette_besoin});
    }
}

void MondeSED::AppliquerMouvements() {
    // Map to track which cell wins the claim for each destination
    std::map<std::tuple<int, int, int>, MouvementSouhaite> mouvements_gagnants;

    for (const auto& mouvement : mouvements_souhaites) {
        auto it = mouvements_gagnants.find(mouvement.destination);
        if (it == mouvements_gagnants.end()) {
            // No one else claimed this spot yet
            mouvements_gagnants[mouvement.destination] = mouvement;
        } else {
            // Conflict: another cell wants the same spot
            if (mouvement.dette_besoin_source > it->second.dette_besoin_source) {
                // The new claimant is more "hungry", it wins
                mouvements_gagnants[mouvement.destination] = mouvement;
            }
            // If debts are equal, the first one to claim it (due to deterministic iteration) keeps it.
        }
    }

    // Apply the winning moves
    for (const auto& pair : mouvements_gagnants) {
        const auto& mouvement = pair.second;
        int sx, sy, sz, dx, dy, dz;
        std::tie(sx, sy, sz) = mouvement.source;
        std::tie(dx, dy, dz) = mouvement.destination;

        Cellule& source_cell = getCellule(sx, sy, sz);
        Cellule& dest_cell = getCellule(dx, dy, dz);

        // Move the cell's data
        dest_cell = source_cell;

        // Empty the source cell
        source_cell = {}; // Default initialize to an empty/dead state
        source_cell.est_vivante = false;
        source_cell.reserve_energie = 0.0f;
    }

    // Clear the list for the next cycle
    mouvements_souhaites.clear();
}