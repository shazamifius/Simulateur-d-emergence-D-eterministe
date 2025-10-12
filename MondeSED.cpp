#include "MondeSED.h"
#include <vector>
#include <random>
#include <ctime>
#include <algorithm>
#include <map>
#include <limits>
#include <cmath>
#include <fstream>
#include <iomanip>

MondeSED::MondeSED(int sx, int sy, int sz) : size_x(sx), size_y(sy), size_z(sz) {
    grille.resize(size_x * size_y * size_z);
    params = ParametresGlobaux();
}

int MondeSED::getIndex(int x, int y, int z) const {
    if (x < 0 || x >= size_x || y < 0 || y >= size_y || z < 0 || z >= size_z) {
        return -1;
    }
    return x + y * size_x + z * size_x * size_y;
}

Cellule& MondeSED::getCellule(int x, int y, int z) {
    return grille[getIndex(x, y, z)];
}

const Cellule& MondeSED::getCellule(int x, int y, int z, const std::vector<Cellule>& grid) const {
    return grid[getIndex(x, y, z)];
}

void MondeSED::InitialiserMonde() {
    std::mt19937 rng(static_cast<unsigned int>(std::time(0)));
    std::uniform_real_distribution<float> random_float(0.0f, 1.0f);
    std::uniform_real_distribution<float> random_r_s(0.1f, 0.9f);

    for (int z = 0; z < size_z; ++z) {
        for (int y = 0; y < size_y; ++y) {
            for (int x = 0; x < size_x; ++x) {
                if (random_float(rng) < 0.5f) {
                    Cellule& cell = getCellule(x, y, z);
                    cell.est_vivante = true;
                    cell.reserve_energie = 1.0f;
                    cell.resistance_stress = random_r_s(rng);
                    cell.seuil_critique = random_r_s(rng);
                }
            }
        }
    }
}

void MondeSED::ExporterEtatMonde(const std::string& nom_fichier) const {
    std::ofstream outfile(nom_fichier);
    if (!outfile.is_open()) {
        return;
    }

    outfile << "x,y,z,E,C,R,A,Memoire\n";

    for (int z = 0; z < size_z; ++z) {
        for (int y = 0; y < size_y; ++y) {
            for (int x = 0; x < size_x; ++x) {
                const Cellule& cell = getCellule(x, y, z, grille);
                if (cell.est_vivante) {
                    outfile << x << "," << y << "," << z << ","
                            << cell.reserve_energie << ","
                            << cell.charge_emotionnelle << ","
                            << cell.resistance_stress << ","
                            << cell.age << ","
                            << cell.memoire_energie_max << "\n";
                }
            }
        }
    }
    outfile.close();
}


void MondeSED::AppliquerLoiZero(int x, int y, int z) {
    Cellule& cell = getCellule(x, y, z);
    if (!cell.est_vivante) return;

    float max_energie_voisin = 0.0f;
    // The read for memory update must be on the grid state *before* any changes in this cycle.
    // However, since LoiZero is the last step, reading the live grid is acceptable here
    // as it reflects the state after all exchanges.
    for (const auto& coords_voisin : GetCoordsVoisins(x, y, z)) {
        const Cellule& voisin = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin));
        if (voisin.reserve_energie > max_energie_voisin) {
            max_energie_voisin = voisin.reserve_energie;
        }
    }
    if (max_energie_voisin > cell.memoire_energie_max) {
        cell.memoire_energie_max = max_energie_voisin;
    }

    cell.reserve_energie -= 0.001f;
    cell.dette_besoin += 0.002f;
    cell.dette_stimulus += params.TAUX_AUGMENTATION_ENNUI;
    cell.age++;

    if (cell.reserve_energie <= 0 || cell.charge_emotionnelle > cell.seuil_critique) {
        cell = {};
        cell.est_vivante = false;
    }
}

std::vector<std::tuple<int, int, int>> MondeSED::GetCoordsVoisins(int x, int y, int z) const {
    std::vector<std::tuple<int, int, int>> voisins;
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                int nx = x + dx;
                int ny = y + dy;
                int nz = z + dz;
                if (getIndex(nx, ny, nz) != -1) {
                    voisins.emplace_back(nx, ny, nz);
                }
            }
        }
    }
    return voisins;
}

void MondeSED::AppliquerLoiMouvement(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    const Cellule& source_cell = getCellule(x,y,z,read_grid);
    if (!source_cell.est_vivante) return;

    auto voisins = GetCoordsVoisins(x, y, z);
    std::tuple<int, int, int> meilleure_cible;
    float max_score = -std::numeric_limits<float>::infinity();
    bool cible_trouvee = false;

    float bonus_memoire = 0.0f;
    if (source_cell.age + 1 > 0) {
        bonus_memoire = params.K_M * (source_cell.memoire_energie_max / (source_cell.age + 1));
    }

    for (const auto& coords_voisin : voisins) {
        const Cellule& voisin_cell = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin), read_grid);
        if (voisin_cell.reserve_energie <= 0) {
            float score = (params.K_E * voisin_cell.reserve_energie)
                        + (params.K_D * source_cell.dette_besoin)
                        - (params.K_C * source_cell.charge_emotionnelle)
                        + bonus_memoire;
            if (score > max_score) {
                max_score = score;
                meilleure_cible = coords_voisin;
                cible_trouvee = true;
            }
        }
    }

    if (cible_trouvee) {
        #pragma omp critical
        mouvements_souhaites.push_back({std::make_tuple(x, y, z), meilleure_cible, source_cell.dette_besoin});
    }
}

void MondeSED::AppliquerMouvements() {
    std::map<std::tuple<int, int, int>, MouvementSouhaite> mouvements_gagnants;
    for (const auto& mouvement : mouvements_souhaites) {
        auto it = mouvements_gagnants.find(mouvement.destination);
        if (it == mouvements_gagnants.end() || mouvement.dette_besoin_source > it->second.dette_besoin_source) {
            mouvements_gagnants[mouvement.destination] = mouvement;
        }
    }
    for (const auto& pair : mouvements_gagnants) {
        const auto& mouvement = pair.second;
        Cellule& source_cell = getCellule(std::get<0>(mouvement.source), std::get<1>(mouvement.source), std::get<2>(mouvement.source));
        Cellule& dest_cell = getCellule(std::get<0>(mouvement.destination), std::get<1>(mouvement.destination), std::get<2>(mouvement.destination));
        dest_cell = source_cell;
        source_cell = {};
        source_cell.est_vivante = false;
    }
    mouvements_souhaites.clear();
}

float deterministic_mutation(int x, int y, int z, int age) {
    unsigned int hash = (x * 18397) + (y * 20441) + (z * 22543) + (age * 24671);
    int decision = hash % 3;
    if (decision == 0) return 0.01f;
    if (decision == 1) return -0.01f;
    return 0.0f;
}

void MondeSED::AppliquerLoiDivision(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    const Cellule& mere = getCellule(x,y,z,read_grid);
    if (!mere.est_vivante || mere.reserve_energie <= params.SEUIL_ENERGIE_DIVISION) return;

    auto voisins = GetCoordsVoisins(x, y, z);
    std::tuple<int, int, int> meilleure_cible;
    float max_resistance_voisin = -1.0f;
    bool cible_trouvee = false;

    for (const auto& coords_voisin : voisins) {
        const Cellule& voisin_cell = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin), read_grid);
        if (voisin_cell.reserve_energie <= 0) {
            if (voisin_cell.resistance_stress > max_resistance_voisin) {
                max_resistance_voisin = voisin_cell.resistance_stress;
                meilleure_cible = coords_voisin;
                cible_trouvee = true;
            }
        }
    }

    if (cible_trouvee) {
        #pragma omp critical
        divisions_souhaitees.push_back({std::make_tuple(x, y, z), meilleure_cible, mere.reserve_energie});
    }
}

void MondeSED::AppliquerDivisions() {
    std::map<std::tuple<int, int, int>, DivisionSouhaitee> divisions_gagnantes;
    for (const auto& division : divisions_souhaitees) {
        auto it = divisions_gagnantes.find(division.destination_fille);
        if (it == divisions_gagnantes.end() || division.energie_mere > it->second.energie_mere) {
            divisions_gagnantes[division.destination_fille] = division;
        }
    }
    for (const auto& pair : divisions_gagnantes) {
        const auto& division = pair.second;
        Cellule& mere = getCellule(std::get<0>(division.source_mere), std::get<1>(division.source_mere), std::get<2>(division.source_mere));
        Cellule& fille = getCellule(std::get<0>(division.destination_fille), std::get<1>(division.destination_fille), std::get<2>(division.destination_fille));
        const Cellule mere_snapshot = mere;
        mere.reserve_energie /= 2.0f;
        fille = mere_snapshot;
        fille.reserve_energie /= 2.0f;
        fille.age = 0;
        fille.horloge_interne = 0;
        fille.est_vivante = true;
        fille.resistance_stress += deterministic_mutation(std::get<0>(division.destination_fille), std::get<1>(division.destination_fille), std::get<2>(division.destination_fille), fille.age);
        fille.seuil_critique += deterministic_mutation(std::get<0>(division.destination_fille), std::get<1>(division.destination_fille), std::get<2>(division.destination_fille), fille.age + 1);
        fille.resistance_stress = std::max(0.0f, std::min(1.0f, fille.resistance_stress));
        fille.seuil_critique = std::max(0.0f, std::min(1.0f, fille.seuil_critique));
    }
    divisions_souhaitees.clear();
}

void MondeSED::AppliquerLoiEchange(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    const Cellule& source = getCellule(x,y,z,read_grid);
    if (!source.est_vivante) return;
    for (const auto& coords_voisin : GetCoordsVoisins(x, y, z)) {
        const Cellule& voisin = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin), read_grid);
        if (voisin.est_vivante && std::abs(source.resistance_stress - voisin.resistance_stress) < params.SEUIL_SIMILARITE_R) {
            float diff_energie = source.reserve_energie - voisin.reserve_energie;
            if (diff_energie > params.SEUIL_DIFFERENCE_ENERGIE) {
                float montant = diff_energie * params.FACTEUR_ECHANGE_ENERGIE;
                #pragma omp critical
                echanges_energie_souhaites.push_back({std::make_tuple(x, y, z), coords_voisin, montant});
            }
        }
    }
}

void MondeSED::AppliquerLoiPsychisme(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    const Cellule& source = getCellule(x,y,z,read_grid);
    if (!source.est_vivante) return;
    std::tuple<int, int, int> voisin_le_plus_calme;
    float min_dette_stimulus = std::numeric_limits<float>::max();
    bool voisin_trouve = false;
    for (const auto& coords_voisin : GetCoordsVoisins(x, y, z)) {
        const Cellule& voisin = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin), read_grid);
        if (voisin.est_vivante && voisin.dette_stimulus < min_dette_stimulus) {
            min_dette_stimulus = voisin.dette_stimulus;
            voisin_le_plus_calme = coords_voisin;
            voisin_trouve = true;
        }
    }
    if (voisin_trouve) {
        float echange_C = source.charge_emotionnelle * params.FACTEUR_ECHANGE_PSYCHIQUE;
        float echange_L = source.dette_stimulus * params.FACTEUR_ECHANGE_PSYCHIQUE;
        #pragma omp critical
        echanges_psychiques_souhaites.push_back({std::make_tuple(x, y, z), voisin_le_plus_calme, echange_C, echange_L});
    }
}

void MondeSED::AppliquerEchangesEnergie() {
    for (const auto& echange : echanges_energie_souhaites) {
        getCellule(std::get<0>(echange.source), std::get<1>(echange.source), std::get<2>(echange.source)).reserve_energie -= echange.montant_energie;
        getCellule(std::get<0>(echange.destination), std::get<1>(echange.destination), std::get<2>(echange.destination)).reserve_energie += echange.montant_energie;
    }
    echanges_energie_souhaites.clear();
}

void MondeSED::AppliquerEchangesPsychiques() {
    for (const auto& echange : echanges_psychiques_souhaites) {
        Cellule& source = getCellule(std::get<0>(echange.source), std::get<1>(echange.source), std::get<2>(echange.source));
        Cellule& dest = getCellule(std::get<0>(echange.destination), std::get<1>(echange.destination), std::get<2>(echange.destination));
        source.charge_emotionnelle += echange.montant_C;
        dest.charge_emotionnelle += echange.montant_C;
        source.dette_stimulus -= echange.montant_L;
        dest.dette_stimulus -= echange.montant_L;
    }
    echanges_psychiques_souhaites.clear();
}

void MondeSED::AvancerTemps() {
    const std::vector<Cellule> read_grid = grille;

    mouvements_souhaites.clear();
    divisions_souhaitees.clear();
    echanges_energie_souhaites.clear();
    echanges_psychiques_souhaites.clear();

    #pragma omp parallel for collapse(3)
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

    AppliquerMouvements();
    AppliquerDivisions();
    AppliquerEchangesEnergie();
    AppliquerEchangesPsychiques();

    #pragma omp parallel for collapse(3)
    for (int z = 0; z < size_z; ++z) {
        for (int y = 0; y < size_y; ++y) {
            for (int x = 0; x < size_x; ++x) {
                AppliquerLoiZero(x, y, z);
            }
        }
    }
}