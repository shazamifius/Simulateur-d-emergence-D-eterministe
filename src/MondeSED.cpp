#include "MondeSED.h"
#include <vector>
#include <random>
#include <ctime>
#include <algorithm>
#include <map>
#include <numeric>

int MondeSED::getNombreCellulesVivantes() const {
    return std::accumulate(grille.begin(), grille.end(), 0, [](int count, const Cellule& cell) {
        return count + (cell.is_alive ? 1 : 0);
    });
}
#include <limits>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

MondeSED::MondeSED(int sx, int sy, int sz) : size_x(sx), size_y(sy), size_z(sz), cycle_actuel(0) {
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

void MondeSED::InitialiserMonde(float initial_density) {
    std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_real_distribution<float> random_float(0.0f, 1.0f);
    std::uniform_real_distribution<float> random_r_s(0.1f, 0.9f);

    for (int z = 0; z < size_z; ++z) {
        for (int y = 0; y < size_y; ++y) {
            for (int x = 0; x < size_x; ++x) {
                if (random_float(rng) < initial_density) {
                    Cellule& cell = getCellule(x, y, z);
                    cell.is_alive = true;
                    cell.E = 1.0f;
                    cell.R = random_r_s(rng);
                    cell.Sc = random_r_s(rng);
                }
            }
        }
    }
}

void MondeSED::ExporterEtatMonde(const std::string& nom_de_base) const {
    std::string nom_fichier = nom_de_base + "_cycle_" + std::to_string(cycle_actuel) + ".csv";
    std::ofstream outfile(nom_fichier);
    if (!outfile.is_open()) {
        return;
    }

    outfile << "x,y,z,E,C,R,A,M\n";

    for (int z = 0; z < size_z; ++z) {
        for (int y = 0; y < size_y; ++y) {
            for (int x = 0; x < size_x; ++x) {
                const Cellule& cell = getCellule(x, y, z, grille);
                if (cell.is_alive) {
                    outfile << x << "," << y << "," << z << ","
                            << cell.E << ","
                            << cell.C << ","
                            << cell.R << ","
                            << cell.A << ","
                            << cell.M << "\n";
                }
            }
        }
    }
    outfile.close();
}


void MondeSED::AppliquerLoiZero(int x, int y, int z) {
    Cellule& cell = getCellule(x, y, z);
    if (!cell.is_alive) return;

    // Loi 6 (Mémorisation) est techniquement ici.
    float max_energie_voisin = 0.0f;
    for (const auto& coords_voisin : GetCoordsVoisins(x, y, z)) {
        const Cellule& voisin = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin));
        if (voisin.E > max_energie_voisin) {
            max_energie_voisin = voisin.E;
        }
    }
    if (max_energie_voisin > cell.M) {
        cell.M = max_energie_voisin;
    }

    // Loi 0 (Survie, Vieillissement)
    cell.E -= 0.001f;
    cell.D += 0.002f;
    cell.L += params.TAUX_AUGMENTATION_ENNUI;
    cell.A++;

    // Mort Énergétique ou Psychique
    if (cell.E <= 0 || cell.C > cell.Sc) {
        cell = {}; // Réinitialise à un état mort par défaut
        cell.is_alive = false;
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
    if (!source_cell.is_alive) return;

    auto voisins = GetCoordsVoisins(x, y, z);
    std::tuple<int, int, int> meilleure_cible;
    float max_score = -std::numeric_limits<float>::infinity();
    bool cible_trouvee = false;

    float bonus_memoire = 0.0f;
    if (source_cell.A + 1 > 0) {
        bonus_memoire = params.K_M * (source_cell.M / (source_cell.A + 1));
    }

    for (const auto& coords_voisin : voisins) {
        const Cellule& voisin_cell = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin), read_grid);
        if (voisin_cell.E <= 0) { // Cible une case vide (ou morte)
            float score = (params.K_E * voisin_cell.E) // L'énergie d'une case vide est 0, mais on garde pour la cohérence de la formule
                        + (params.K_D * source_cell.D)
                        - (params.K_C * source_cell.C)
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
        mouvements_souhaites.push_back({std::make_tuple(x, y, z), meilleure_cible, source_cell.D});
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
        source_cell.is_alive = false;
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
    if (!mere.is_alive || mere.E <= params.SEUIL_ENERGIE_DIVISION) return;

    auto voisins = GetCoordsVoisins(x, y, z);
    std::tuple<int, int, int> meilleure_cible;
    float max_resistance_voisin = -1.0f;
    bool cible_trouvee = false;

    for (const auto& coords_voisin : voisins) {
        const Cellule& voisin_cell = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin), read_grid);
        if (voisin_cell.E <= 0) { // Cible une case vide
            if (voisin_cell.R > max_resistance_voisin) { // La doc dit de cibler la case avec le plus haut R
                max_resistance_voisin = voisin_cell.R;
                meilleure_cible = coords_voisin;
                cible_trouvee = true;
            }
        }
    }

    if (cible_trouvee) {
        #pragma omp critical
        divisions_souhaitees.push_back({std::make_tuple(x, y, z), meilleure_cible, mere.E});
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
        mere.E /= 2.0f;

        fille = mere_snapshot;
        fille.E /= 2.0f;
        fille.A = 0;
        fille.is_alive = true;

        // Mutation déterministe basée sur les coordonnées et l'âge
        fille.R += deterministic_mutation(std::get<0>(division.destination_fille), std::get<1>(division.destination_fille), std::get<2>(division.destination_fille), fille.A);
        fille.Sc += deterministic_mutation(std::get<0>(division.destination_fille), std::get<1>(division.destination_fille), std::get<2>(division.destination_fille), fille.A + 1); // +1 pour varier le hash

        // Clamp values to a valid range [0, 1]
        fille.R = std::max(0.0f, std::min(1.0f, fille.R));
        fille.Sc = std::max(0.0f, std::min(1.0f, fille.Sc));
    }
    divisions_souhaitees.clear();
}

void MondeSED::AppliquerLoiEchange(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    const Cellule& source = getCellule(x,y,z,read_grid);
    if (!source.is_alive) return;
    for (const auto& coords_voisin : GetCoordsVoisins(x, y, z)) {
        const Cellule& voisin = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin), read_grid);
        if (voisin.is_alive && std::abs(source.R - voisin.R) < params.SEUIL_SIMILARITE_R) {
            float diff_energie = source.E - voisin.E;
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
    if (!source.is_alive) return;
    std::tuple<int, int, int> voisin_le_plus_calme;
    float min_dette_stimulus = std::numeric_limits<float>::max();
    bool voisin_trouve = false;
    for (const auto& coords_voisin : GetCoordsVoisins(x, y, z)) {
        const Cellule& voisin = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin), read_grid);
        if (voisin.is_alive && voisin.L < min_dette_stimulus) {
            min_dette_stimulus = voisin.L;
            voisin_le_plus_calme = coords_voisin;
            voisin_trouve = true;
        }
    }
    if (voisin_trouve) {
        float echange_C = source.C * params.FACTEUR_ECHANGE_PSYCHIQUE;
        float echange_L = source.L * params.FACTEUR_ECHANGE_PSYCHIQUE;
        #pragma omp critical
        echanges_psychiques_souhaites.push_back({std::make_tuple(x, y, z), voisin_le_plus_calme, echange_C, echange_L});
    }
}

void MondeSED::AppliquerEchangesEnergie() {
    for (const auto& echange : echanges_energie_souhaites) {
        getCellule(std::get<0>(echange.source), std::get<1>(echange.source), std::get<2>(echange.source)).E -= echange.montant_energie;
        getCellule(std::get<0>(echange.destination), std::get<1>(echange.destination), std::get<2>(echange.destination)).E += echange.montant_energie;
    }
    echanges_energie_souhaites.clear();
}

void MondeSED::AppliquerEchangesPsychiques() {
    for (const auto& echange : echanges_psychiques_souhaites) {
        Cellule& source = getCellule(std::get<0>(echange.source), std::get<1>(echange.source), std::get<2>(echange.source));
        Cellule& dest = getCellule(std::get<0>(echange.destination), std::get<1>(echange.destination), std::get<2>(echange.destination));
        source.C += echange.montant_C;
        dest.C += echange.montant_C;
        source.L -= echange.montant_L;
        dest.L -= echange.montant_L;
    }
    echanges_psychiques_souhaites.clear();
}

void MondeSED::AvancerTemps() {
    cycle_actuel++;

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

const std::vector<Cellule>& MondeSED::getGrille() const {
    return grille;
}

// --- Implementation for loading parameters from a file ---
bool MondeSED::ChargerParametresDepuisFichier(const std::string& nom_fichier) {
    std::ifstream fichier_params(nom_fichier);
    if (!fichier_params.is_open()) {
        std::cerr << "Avertissement: Impossible d'ouvrir le fichier de paramètres '" << nom_fichier << "'. Utilisation des valeurs par défaut." << std::endl;
        return false;
    }

    std::string ligne;
    std::cout << "Chargement des paramètres depuis '" << nom_fichier << "'..." << std::endl;
    while (std::getline(fichier_params, ligne)) {
        // Ignore empty lines or comments
        if (ligne.empty() || ligne[0] == '#') {
            continue;
        }

        std::istringstream iss(ligne);
        std::string cle;
        if (std::getline(iss, cle, '=')) {
            std::string valeur_str;
            if (std::getline(iss, valeur_str)) {
                try {
                    float valeur = std::stof(valeur_str);
                    if (cle == "K_E") params.K_E = valeur;
                    else if (cle == "K_D") params.K_D = valeur;
                    else if (cle == "K_C") params.K_C = valeur;
                    else if (cle == "SEUIL_ENERGIE_DIVISION") params.SEUIL_ENERGIE_DIVISION = valeur;
                    else if (cle == "FACTEUR_ECHANGE_ENERGIE") params.FACTEUR_ECHANGE_ENERGIE = valeur;
                    else if (cle == "SEUIL_DIFFERENCE_ENERGIE") params.SEUIL_DIFFERENCE_ENERGIE = valeur;
                    else if (cle == "SEUIL_SIMILARITE_R") params.SEUIL_SIMILARITE_R = valeur;
                    else if (cle == "TAUX_AUGMENTATION_ENNUI") params.TAUX_AUGMENTATION_ENNUI = valeur;
                    else if (cle == "FACTEUR_ECHANGE_PSYCHIQUE") params.FACTEUR_ECHANGE_PSYCHIQUE = valeur;
                    else if (cle == "K_M") params.K_M = valeur;
                    else if (cle == "intervalle_export") params.intervalle_export = static_cast<int>(valeur);
                } catch (const std::exception& e) {
                    std::cerr << "Avertissement: Ligne invalide dans le fichier de paramètres: " << ligne << std::endl;
                }
            }
        }
    }
    std::cout << "Paramètres chargés." << std::endl;
    return true;
}