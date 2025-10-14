// --- Fichiers d'en-tête ---
#include "MondeSED.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <vector>

// --- Implémentation de la classe MondeSED ---

// Calcule et retourne le nombre total de cellules vivantes dans la grille.
int MondeSED::getNombreCellulesVivantes() const {
    // Utilise std::accumulate pour sommer les cellules vivantes de manière efficace.
    return std::accumulate(grille.begin(), grille.end(), 0, [](int count, const Cellule& cell) {
        return count + (cell.is_alive ? 1 : 0);
    });
}

// --- Constructeur ---
// Initialise le monde avec les dimensions données et pré-alloue la grille.
MondeSED::MondeSED(int sx, int sy, int sz) : size_x(sx), size_y(sy), size_z(sz), cycle_actuel(0) {
    grille.resize(size_x * size_y * size_z);
    params = ParametresGlobaux(); // Initialise les paramètres avec les valeurs par défaut.
}

// Convertit les coordonnées 3D en un index 1D pour l'accès au vecteur `grille`.
// Retourne -1 si les coordonnées sont hors limites.
int MondeSED::getIndex(int x, int y, int z) const {
    if (x < 0 || x >= size_x || y < 0 || y >= size_y || z < 0 || z >= size_z) {
        return -1; // Coordonnées hors limites
    }
    // Formule pour un stockage "row-major"
    return x + y * size_x + z * size_x * size_y;
}

// Retourne une référence modifiable à une cellule à partir de ses coordonnées 3D.
Cellule& MondeSED::getCellule(int x, int y, int z) {
    return grille[getIndex(x, y, z)];
}

// Retourne une référence constante à une cellule d'une grille donnée (lecture seule).
const Cellule& MondeSED::getCellule(int x, int y, int z, const std::vector<Cellule>& grid) const {
    return grid[getIndex(x, y, z)];
}

// --- Initialisation et Exportation ---

// Remplit la grille avec une "soupe primordiale" de cellules de manière déterministe.
void MondeSED::InitialiserMonde(unsigned int seed, float initial_density) {
    current_seed = seed; // Sauvegarde la graine utilisée.
    // Le générateur est maintenant initialisé avec la graine fournie, garantissant la reproductibilité.
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> random_float(0.0f, 1.0f);
    std::uniform_real_distribution<float> random_r_s(0.1f, 0.9f);

    for (int z = 0; z < size_z; ++z) {
        for (int y = 0; y < size_y; ++y) {
            for (int x = 0; x < size_x; ++x) {
                Cellule& cell = getCellule(x, y, z);
                cell = {}; // Réinitialise la cellule à son état par défaut (morte)
                if (random_float(rng) < initial_density) {
                    // Crée une nouvelle cellule vivante avec des propriétés initiales.
                    cell.is_alive = true;
                    cell.E = 1.0f; // Énergie de départ
                    cell.R = random_r_s(rng); // Résistance innée aléatoire
                    cell.Sc = random_r_s(rng); // Seuil critique aléatoire
                }
            }
        }
    }
}

// Exporte l'état actuel des cellules vivantes dans un fichier CSV.
void MondeSED::ExporterEtatMonde(const std::string& nom_de_base) const {
    // Construit un nom de fichier unique basé sur le numéro de cycle.
    std::string nom_fichier = nom_de_base + "_cycle_" + std::to_string(cycle_actuel) + ".csv";
    std::ofstream outfile(nom_fichier);
    if (!outfile.is_open()) {
        std::cerr << "Erreur: Impossible d'ouvrir le fichier d'exportation '" << nom_fichier << "'" << std::endl;
        return;
    }

    // En-tête du fichier CSV
    outfile << "x,y,z,E,C,R,A,M\n";

    // Parcours de la grille et écriture des données pour chaque cellule vivante.
    for (int z = 0; z < size_z; ++z) {
        for (int y = 0; y < size_y; ++y) {
            for (int x = 0; x < size_x; ++x) {
                const Cellule& cell = getCellule(x, y, z, grille);
                if (cell.is_alive) {
                    outfile << x << "," << y << "," << z << ","
                            << cell.E << "," << cell.C << "," << cell.R << ","
                            << cell.A << "," << cell.M << "\n";
                }
            }
        }
    }
    outfile.close();
}


// --- Lois de Simulation (Phase de Mise à Jour d'État) ---

// Applique la Loi 0 (Survie, Vieillissement) et la Loi 6 (Mémorisation).
// Cette fonction est appliquée à la fin du cycle, après les décisions et actions.
void MondeSED::AppliquerLoiZero(int x, int y, int z) {
    Cellule& cell = getCellule(x, y, z);
    if (!cell.is_alive) return;

    // Loi 6 (Mémorisation) : La cellule mémorise la plus haute énergie de son voisinage.
    float max_energie_voisin = 0.0f;
    std::tuple<int, int, int> voisins[26];
    int nombre_voisins;
    GetCoordsVoisins_Optimized(x, y, z, voisins, nombre_voisins);

    for (int i = 0; i < nombre_voisins; ++i) {
        const auto& coords_voisin = voisins[i];
        // NOTE: Cette lecture se fait sur la grille mise à jour, ce qui est l'intention ici.
        const Cellule& voisin = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin));
        if (voisin.E > max_energie_voisin) {
            max_energie_voisin = voisin.E;
        }
    }
    if (max_energie_voisin > cell.M) {
        cell.M = max_energie_voisin;
    }

    // Loi 0 (Survie, Vieillissement) : Changements d'état passifs à chaque cycle.
    cell.E -= 0.001f; // Consommation d'énergie
    cell.D += 0.002f; // Augmentation de la faim/dette
    cell.L += params.TAUX_AUGMENTATION_ENNUI; // Augmentation de l'ennui
    cell.A++; // Vieillissement

    // Conditions de Mort
    if (cell.E <= 0 || cell.C > cell.Sc) {
        cell = {}; // Réinitialise à un état mort par défaut.
    }
}

// --- Fonctions Utilitaires ---

// Version optimisée de GetCoordsVoisins qui évite les allocations de vecteur.
void MondeSED::GetCoordsVoisins_Optimized(int x, int y, int z, std::tuple<int, int, int>* voisins_array, int& count) const {
    count = 0;
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                int nx = x + dx;
                int ny = y + dy;
                int nz = z + dz;
                if (getIndex(nx, ny, nz) != -1) {
                    voisins_array[count++] = std::make_tuple(nx, ny, nz);
                }
            }
        }
    }
}


// --- Lois de Simulation (Phase de Décision) ---

// Applique la Loi 1 (Mouvement) : Calcule la meilleure destination pour une cellule.
void MondeSED::AppliquerLoiMouvement(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    const Cellule& source_cell = getCellule(x, y, z, read_grid);
    if (!source_cell.is_alive) return;

    std::tuple<int, int, int> meilleure_cible;
    float max_score = -std::numeric_limits<float>::infinity();
    bool cible_trouvee = false;

    // Calcule le bonus de mémoire, qui diminue avec l'âge.
    float bonus_memoire = (source_cell.A > 0) ? (params.K_M * (source_cell.M / source_cell.A)) : 0.0f;

    // Alloue un tableau sur la pile pour les voisins afin d'éviter l'allocation dynamique.
    std::tuple<int, int, int> voisins[26];
    int nombre_voisins;
    GetCoordsVoisins_Optimized(x, y, z, voisins, nombre_voisins);

    // Évalue toutes les cases voisines vides comme cibles potentielles.
    for (int i = 0; i < nombre_voisins; ++i) {
        const auto& coords_voisin = voisins[i];
        const Cellule& voisin_cell = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin), read_grid);
        if (!voisin_cell.is_alive) { // Cible une case vide
            float score = (params.K_E * voisin_cell.E) // L'énergie d'une case vide est 0
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

    // Si une cible a été trouvée, enregistre le mouvement souhaité.
    if (cible_trouvee) {
        #pragma omp critical
        mouvements_souhaites.push_back({std::make_tuple(x, y, z), meilleure_cible, source_cell.D});
    }
}


// --- Fonctions d'Application des Actions (Phase d'Action) ---

// Résout les conflits de mouvement et applique les mouvements validés.
void MondeSED::AppliquerMouvements() {
    // Gère les conflits : si plusieurs cellules visent la même destination,
    // seule celle avec la "Dette de Besoin" (D) la plus élevée l'emporte.
    // Cette approche garantit le déterminisme.
    std::map<std::tuple<int, int, int>, MouvementSouhaite> mouvements_gagnants;
    for (const auto& mouvement : mouvements_souhaites) {
        auto it = mouvements_gagnants.find(mouvement.destination);
        if (it == mouvements_gagnants.end() || mouvement.dette_besoin_source > it->second.dette_besoin_source) {
            mouvements_gagnants[mouvement.destination] = mouvement;
        }
    }

    // Applique les mouvements qui ont gagné la résolution de conflit.
    for (const auto& pair : mouvements_gagnants) {
        const auto& mouvement = pair.second;
        Cellule& source_cell = getCellule(std::get<0>(mouvement.source), std::get<1>(mouvement.source), std::get<2>(mouvement.source));
        Cellule& dest_cell = getCellule(std::get<0>(mouvement.destination), std::get<1>(mouvement.destination), std::get<2>(mouvement.destination));

        dest_cell = source_cell; // Copie la cellule vers la destination
        source_cell = {};        // Réinitialise la cellule source
    }
    mouvements_souhaites.clear();
}

// Génère une mutation déterministe pour R et Sc lors de la division.
// La mutation dépend des coordonnées de la case cible et de l'âge de la nouvelle cellule,
// garantissant que le résultat est toujours le même pour une situation donnée.
// Ceci est crucial pour la reproductibilité de la simulation.
float deterministic_mutation(int x, int y, int z, int age) {
    // Utilise des nombres premiers pour améliorer la distribution du hash.
    unsigned int hash = (x * 18397) + (y * 20441) + (z * 22543) + (age * 24671);
    int decision = hash % 3; // Décide entre +0.01, -0.01, ou 0.0
    if (decision == 0) return 0.01f;
    if (decision == 1) return -0.01f;
    return 0.0f;
}

// Applique la Loi 2 (Division) : Détermine si une cellule doit se diviser et où.
void MondeSED::AppliquerLoiDivision(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    const Cellule& mere = getCellule(x, y, z, read_grid);
    if (!mere.is_alive || mere.E <= params.SEUIL_ENERGIE_DIVISION) return;

    std::tuple<int, int, int> meilleure_cible;
    float max_resistance_voisin = -1.0f;
    bool cible_trouvee = false;

    std::tuple<int, int, int> voisins[26];
    int nombre_voisins;
    GetCoordsVoisins_Optimized(x, y, z, voisins, nombre_voisins);

    // Cherche la case voisine vide avec la plus haute "Résistance Innée" (R).
    // Cibler une `R` élevée (même si la case est vide) favorise la cohésion génétique.
    for (int i = 0; i < nombre_voisins; ++i) {
        const auto& coords_voisin = voisins[i];
        const Cellule& voisin_cell = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin), read_grid);
        if (!voisin_cell.is_alive) { // Cible une case vide
            if (voisin_cell.R > max_resistance_voisin) {
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

// Résout les conflits de division et applique les divisions validées.
void MondeSED::AppliquerDivisions() {
    // Gère les conflits : si plusieurs cellules visent la même case,
    // seule celle avec l'Énergie (E) la plus élevée l'emporte.
    std::map<std::tuple<int, int, int>, DivisionSouhaitee> divisions_gagnantes;
    for (const auto& division : divisions_souhaitees) {
        auto it = divisions_gagnantes.find(division.destination_fille);
        if (it == divisions_gagnantes.end() || division.energie_mere > it->second.energie_mere) {
            divisions_gagnantes[division.destination_fille] = division;
        }
    }

    // Applique les divisions qui ont gagné la résolution de conflit.
    for (const auto& pair : divisions_gagnantes) {
        const auto& division = pair.second;
        Cellule& mere = getCellule(std::get<0>(division.source_mere), std::get<1>(division.source_mere), std::get<2>(division.source_mere));
        Cellule& fille = getCellule(std::get<0>(division.destination_fille), std::get<1>(division.destination_fille), std::get<2>(division.destination_fille));

        // Divise l'énergie et copie les propriétés.
        mere.E /= 2.0f;
        fille = mere; // La fille hérite de toutes les propriétés, y compris R et Sc.
        fille.A = 0;  // L'âge de la fille est réinitialisé.

        // Applique une mutation déterministe à la fille.
        int dest_x = std::get<0>(division.destination_fille);
        int dest_y = std::get<1>(division.destination_fille);
        int dest_z = std::get<2>(division.destination_fille);
        fille.R += deterministic_mutation(dest_x, dest_y, dest_z, fille.A);
        fille.Sc += deterministic_mutation(dest_x, dest_y, dest_z, fille.A + 1); // +1 pour varier le hash

        // S'assure que les valeurs restent dans un intervalle valide [0, 1].
        fille.R = std::max(0.0f, std::min(1.0f, fille.R));
        fille.Sc = std::max(0.0f, std::min(1.0f, fille.Sc));
    }
    divisions_souhaitees.clear();
}

// Applique la Loi 4 (Échange Énergétique) : Partage d'énergie entre cellules similaires.
void MondeSED::AppliquerLoiEchange(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    const Cellule& source = getCellule(x, y, z, read_grid);
    if (!source.is_alive) return;

    std::tuple<int, int, int> voisins[26];
    int nombre_voisins;
    GetCoordsVoisins_Optimized(x, y, z, voisins, nombre_voisins);

    for (int i = 0; i < nombre_voisins; ++i) {
        const auto& coords_voisin = voisins[i];
        const Cellule& voisin = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin), read_grid);
        // Condition: le voisin est vivant, génétiquement similaire, et a moins d'énergie.
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

// Applique la Loi 5 (Interaction Psychique) : Gestion de l'ennui et du stress.
void MondeSED::AppliquerLoiPsychisme(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    const Cellule& source = getCellule(x, y, z, read_grid);
    if (!source.is_alive) return;

    std::tuple<int, int, int> voisin_le_plus_calme;
    float min_dette_stimulus = std::numeric_limits<float>::max();
    bool voisin_trouve = false;

    std::tuple<int, int, int> voisins[26];
    int nombre_voisins;
    GetCoordsVoisins_Optimized(x, y, z, voisins, nombre_voisins);

    // Recherche le voisin vivant avec le plus faible niveau d'ennui (L).
    for (int i = 0; i < nombre_voisins; ++i) {
        const auto& coords_voisin = voisins[i];
        const Cellule& voisin = getCellule(std::get<0>(coords_voisin), std::get<1>(coords_voisin), std::get<2>(coords_voisin), read_grid);
        if (voisin.is_alive && voisin.L < min_dette_stimulus) {
            min_dette_stimulus = voisin.L;
            voisin_le_plus_calme = coords_voisin;
            voisin_trouve = true;
        }
    }

    // Si un voisin est trouvé, planifie une interaction psychique.
    if (voisin_trouve) {
        float echange_C = source.C * params.FACTEUR_ECHANGE_PSYCHIQUE; // Le stress augmente
        float echange_L = source.L * params.FACTEUR_ECHANGE_PSYCHIQUE; // L'ennui diminue
        #pragma omp critical
        echanges_psychiques_souhaites.push_back({std::make_tuple(x, y, z), voisin_le_plus_calme, echange_C, echange_L});
    }
}

// Applique les échanges d'énergie planifiés.
void MondeSED::AppliquerEchangesEnergie() {
    for (const auto& echange : echanges_energie_souhaites) {
        // La cellule source donne de l'énergie, la destination en reçoit.
        getCellule(std::get<0>(echange.source), std::get<1>(echange.source), std::get<2>(echange.source)).E -= echange.montant_energie;
        getCellule(std::get<0>(echange.destination), std::get<1>(echange.destination), std::get<2>(echange.destination)).E += echange.montant_energie;
    }
    echanges_energie_souhaites.clear();
}

// Applique les échanges psychiques planifiés.
void MondeSED::AppliquerEchangesPsychiques() {
    for (const auto& echange : echanges_psychiques_souhaites) {
        Cellule& source = getCellule(std::get<0>(echange.source), std::get<1>(echange.source), std::get<2>(echange.source));
        Cellule& dest = getCellule(std::get<0>(echange.destination), std::get<1>(echange.destination), std::get<2>(echange.destination));
        // Le stress (C) augmente pour les deux, l'ennui (L) diminue pour les deux.
        source.C += echange.montant_C;
        dest.C += echange.montant_C;
        source.L -= echange.montant_L;
        dest.L -= echange.montant_L;
    }
    echanges_psychiques_souhaites.clear();
}

// --- Boucle de Simulation Principale ---

// Fait avancer la simulation d'un cycle.
// L'ordre des opérations est crucial pour le déterminisme.
void MondeSED::AvancerTemps() {
    cycle_actuel++;

    // --- 1. Phase de Décision (Lecture seule) ---
    // Une copie de la grille est utilisée pour que toutes les décisions soient basées sur l'état du monde AU MÊME moment.
    // Cela garantit le déterminisme même en parallèle.
    const std::vector<Cellule> read_grid = grille;

    // Réinitialise les listes d'actions souhaitées.
    mouvements_souhaites.clear();
    divisions_souhaitees.clear();
    echanges_energie_souhaites.clear();
    echanges_psychiques_souhaites.clear();

    // Chaque cellule prend ses décisions en parallèle.
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

    // --- 2. Phase d'Action (Écriture séquentielle) ---
    // Les actions sont appliquées séquentiellement après la résolution des conflits.
    AppliquerMouvements();
    AppliquerDivisions();
    AppliquerEchangesEnergie();
    AppliquerEchangesPsychiques();

    // --- 3. Phase de Mise à Jour de l'État (Écriture parallèle) ---
    // Les lois de survie et de vieillissement sont appliquées en dernier.
    #pragma omp parallel for collapse(3)
    for (int z = 0; z < size_z; ++z) {
        for (int y = 0; y < size_y; ++y) {
            for (int x = 0; x < size_x; ++x) {
                AppliquerLoiZero(x, y, z);
            }
        }
    }
}

// --- Accesseurs ---

const std::vector<Cellule>& MondeSED::getGrille() const {
    return grille;
}

// --- Sauvegarde et Chargement ---

// Sauvegarde l'état complet de la simulation dans un fichier binaire.
bool MondeSED::SauvegarderEtat(const std::string& nom_fichier) const {
    std::ofstream fichier_sortie(nom_fichier, std::ios::binary);
    if (!fichier_sortie) {
        std::cerr << "Erreur: Impossible d'ouvrir le fichier de sauvegarde '" << nom_fichier << "'" << std::endl;
        return false;
    }

    // Écrit les métadonnées (dimensions, cycle, paramètres)
    fichier_sortie.write(reinterpret_cast<const char*>(&size_x), sizeof(size_x));
    fichier_sortie.write(reinterpret_cast<const char*>(&size_y), sizeof(size_y));
    fichier_sortie.write(reinterpret_cast<const char*>(&size_z), sizeof(size_z));
    fichier_sortie.write(reinterpret_cast<const char*>(&cycle_actuel), sizeof(cycle_actuel));
    fichier_sortie.write(reinterpret_cast<const char*>(&params), sizeof(params));

    // Écrit l'intégralité de la grille.
    fichier_sortie.write(reinterpret_cast<const char*>(grille.data()), grille.size() * sizeof(Cellule));

    fichier_sortie.close();
    std::cout << "État de la simulation sauvegardé dans '" << nom_fichier << "'." << std::endl;
    return true;
}

// Charge un état de simulation depuis un fichier binaire.
bool MondeSED::ChargerEtat(const std::string& nom_fichier) {
    std::ifstream fichier_entree(nom_fichier, std::ios::binary);
    if (!fichier_entree) {
        std::cerr << "Erreur: Impossible d'ouvrir le fichier de chargement '" << nom_fichier << "'" << std::endl;
        return false;
    }

    // Lit les métadonnées.
    fichier_entree.read(reinterpret_cast<char*>(&size_x), sizeof(size_x));
    fichier_entree.read(reinterpret_cast<char*>(&size_y), sizeof(size_y));
    fichier_entree.read(reinterpret_cast<char*>(&size_z), sizeof(size_z));
    fichier_entree.read(reinterpret_cast<char*>(&cycle_actuel), sizeof(cycle_actuel));
    fichier_entree.read(reinterpret_cast<char*>(&params), sizeof(params));

    // Redimensionne la grille si nécessaire et lit les données des cellules.
    grille.resize(size_x * size_y * size_z);
    fichier_entree.read(reinterpret_cast<char*>(grille.data()), grille.size() * sizeof(Cellule));

    fichier_entree.close();
    std::cout << "État de la simulation chargé depuis '" << nom_fichier << "'." << std::endl;
    return true;
}

// Charge les paramètres de simulation depuis un fichier texte de type clé=valeur.
bool MondeSED::ChargerParametresDepuisFichier(const std::string& nom_fichier) {
    std::ifstream fichier_params(nom_fichier);
    if (!fichier_params.is_open()) {
        std::cerr << "Avertissement: Fichier de paramètres '" << nom_fichier << "' introuvable. Utilisation des valeurs par défaut." << std::endl;
        return false;
    }

    std::string ligne;
    std::cout << "Chargement des paramètres depuis '" << nom_fichier << "'..." << std::endl;
    while (std::getline(fichier_params, ligne)) {
        // Ignore les lignes vides ou les commentaires.
        if (ligne.empty() || ligne[0] == '#') {
            continue;
        }

        std::istringstream iss(ligne);
        std::string cle, valeur_str;
        if (std::getline(iss, cle, '=') && std::getline(iss, valeur_str)) {
            try {
                // Associe la clé lue à la variable de paramètre correspondante.
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
            } catch (const std::invalid_argument& e) {
                std::cerr << "Avertissement: Ligne invalide dans le fichier de paramètres: " << ligne << std::endl;
            }
        }
    }
    std::cout << "Paramètres chargés." << std::endl;
    return true;
}