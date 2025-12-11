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
#include <sstream>
#include <vector>
#include <cstring> // Pour memset si besoin

// --- Implémentation de la classe MondeSED ---

// Calcule et retourne le nombre total de cellules vivantes dans la grille.
int MondeSED::getNombreCellulesVivantes() const {
  return std::accumulate(grille.begin(), grille.end(), 0,
                         [](int count, const Cellule &cell) {
                           return count + (cell.is_alive ? 1 : 0);
                         });
}

// --- Constructeur ---
MondeSED::MondeSED(int sx, int sy, int sz)
    : size_x(sx), size_y(sy), size_z(sz), cycle_actuel(0) {
  grille.resize(size_x * size_y * size_z);
  params = ParametresGlobaux();
}

// Convertit les coordonnées 3D en un index 1D
int MondeSED::getIndex(int x, int y, int z) const {
  if (x < 0 || x >= size_x || y < 0 || y >= size_y || z < 0 || z >= size_z) {
    return -1;
  }
  return x + y * size_x + z * size_x * size_y;
}

Cellule &MondeSED::getCellule(int x, int y, int z) {
  return grille[getIndex(x, y, z)];
}

const Cellule &MondeSED::getCellule(int x, int y, int z,
                                    const std::vector<Cellule> &grid) const {
  return grid[getIndex(x, y, z)];
}

// --- Initialisation et Exportation ---

void MondeSED::InitialiserMonde(unsigned int seed, float initial_density) {
  current_seed = seed;
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> random_float(0.0f, 1.0f);
  std::uniform_real_distribution<float> random_r_s(0.1f, 0.9f);
  std::uniform_real_distribution<float> random_w(0.0f, 0.5f);

  for (int z = 0; z < size_z; ++z) {
    for (int y = 0; y < size_y; ++y) {
      for (int x = 0; x < size_x; ++x) {
        Cellule &cell = getCellule(x, y, z);
        cell = {}; // Reset

        // Initialisation des poids synaptiques (pour tous, au cas où ils deviennent neurones)
        for(int i=0; i<27; ++i) cell.W[i] = random_w(rng);

        if (random_float(rng) < initial_density) {
          cell.is_alive = true;
          cell.E = 1.0f;
          cell.R = random_r_s(rng);
          cell.Sc = random_r_s(rng);
          cell.T = 0; // Souche par défaut
        }
      }
    }
  }
}

void MondeSED::ExporterEtatMonde(const std::string &nom_de_base) const {
  std::string nom_fichier =
      nom_de_base + "_cycle_" + std::to_string(cycle_actuel) + ".csv";
  std::ofstream outfile(nom_fichier);
  if (!outfile.is_open()) return;

  outfile << "x,y,z,T,E,D,C,L,M,R,P,G\n";

  for (int z = 0; z < size_z; ++z) {
    for (int y = 0; y < size_y; ++y) {
      for (int x = 0; x < size_x; ++x) {
        const Cellule &cell = getCellule(x, y, z, grille);
        if (cell.is_alive) {
          outfile << x << "," << y << "," << z << ","
                  << (int)cell.T << "," << cell.E << "," << cell.D << ","
                  << cell.C << "," << cell.L << "," << cell.M << ","
                  << cell.R << "," << cell.P << "," << cell.G << "\n";
        }
      }
    }
  }
  outfile.close();
}

// --- Fonctions Utilitaires ---

void MondeSED::GetCoordsVoisins_Optimized(
    int x, int y, int z, std::tuple<int, int, int> *voisins_array,
    int* indices_array, int &count) const {
  count = 0;
  for (int dz = -1; dz <= 1; ++dz) {
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0 && dz == 0)
          continue;
        int nx = x + dx;
        int ny = y + dy;
        int nz = z + dz;
        if (getIndex(nx, ny, nz) != -1) {
          if(indices_array) {
             // Index 0-26 pour le tableau W.
             // (dz+1)*9 + (dy+1)*3 + (dx+1). Centre = 13.
             indices_array[count] = (dz + 1) * 9 + (dy + 1) * 3 + (dx + 1);
          }
          voisins_array[count++] = std::make_tuple(nx, ny, nz);
        }
      }
    }
  }
}

void MondeSED::CalculerBarycentre() {
    double sum_x = 0, sum_y = 0, sum_z = 0;
    double count = 0;

    // Pour éviter overflow si le monde est immense, on utilise double
    // Parallélisme pour la somme (reduction)
    #pragma omp parallel for reduction(+:sum_x, sum_y, sum_z, count)
    for(int i = 0; i < (int)grille.size(); ++i) {
        if(grille[i].is_alive) {
            // Retrouver x,y,z depuis index
            int idx = i;
            int z = idx / (size_x * size_y);
            idx -= (z * size_x * size_y);
            int y = idx / size_x;
            int x = idx % size_x;

            sum_x += x;
            sum_y += y;
            sum_z += z;
            count += 1.0;
        }
    }

    if(count > 0) {
        barycentre_x = (float)(sum_x / count);
        barycentre_y = (float)(sum_y / count);
        barycentre_z = (float)(sum_z / count);
    } else {
        barycentre_x = size_x / 2.0f;
        barycentre_y = size_y / 2.0f;
        barycentre_z = size_z / 2.0f;
    }
}

// --- BOUCLE NEURALE (Groupe B) ---
void MondeSED::ExecuterCycleNeural() {
    // Allocation temporaire pour double-buffering de P
    std::vector<float> P_current(grille.size());
    std::vector<float> P_next(grille.size());

    // Copie initiale
    #pragma omp parallel for
    for(int i=0; i<(int)grille.size(); ++i) {
        P_current[i] = grille[i].P;
    }

    // Boucle Rapide
    for(int iter = 0; iter < params.TICKS_NEURAUX_PAR_PHYSIQUE; ++iter) {

        #pragma omp parallel for
        for(int z = 0; z < size_z; ++z) {
            for(int y = 0; y < size_y; ++y) {
                for(int x = 0; x < size_x; ++x) {
                    int idx = getIndex(x, y, z);
                    Cellule& cell = grille[idx]; // Note: Write access to metadata (Ref, E_cost), Read P from buffer

                    if(!cell.is_alive || cell.T != 2) { // Seulement Neurones
                        P_next[idx] = 0.0f;
                        continue;
                    }

                    // Loi 3: Intégration
                    float sum_input = 0.0f;
                    float sum_W = 0.0f;

                    std::tuple<int, int, int> voisins[26];
                    int indices_W[26];
                    int nb_voisins;
                    GetCoordsVoisins_Optimized(x, y, z, voisins, indices_W, nb_voisins);

                    for(int i=0; i<nb_voisins; ++i) {
                         int v_idx = getIndex(std::get<0>(voisins[i]), std::get<1>(voisins[i]), std::get<2>(voisins[i]));
                         float w = cell.W[indices_W[i]];
                         float p_voisin = P_current[v_idx];

                         if(w > 0) {
                             sum_input += p_voisin * w;
                             sum_W += w;
                         }
                    }

                    float I = (sum_W > 0) ? (sum_input / std::max(1.0f, sum_W)) : 0.0f;

                    // Gestion Réfractaire
                    if(cell.Ref > 0) {
                        cell.Ref--;
                        P_next[idx] = 0.0f; // Inhibé
                    } else {
                        // Mise à jour Potentiel
                        // P_new = (P_old * 0.9) + I
                        // Note: Doc mentionne Noise(Seed), on omet pour perf/determinisme simple pour l'instant
                        float p_new = (P_current[idx] * 0.9f) + I;

                        // Spike ?
                        if(p_new > params.SEUIL_FIRE) {
                            P_next[idx] = 1.0f; // Spike
                            cell.Ref = params.PERIODE_REFRACTAIRE;
                            cell.E_cost += params.COUT_SPIKE; // Accumule coût

                            // Mise à jour Historique (Bitfield)
                            cell.H = (cell.H << 1) | 1;

                            // Loi 4: Ignition (Broadcast local immédiat pour le prochain tick ?)
                            // La doc dit "Si P est Spike ... P_voisins += 0.1".
                            // Dans un automate, cela influence le voisin au prochain tick.
                            // Ici, notre modèle "Somme pondérée" couvre déjà la transmission.
                            // La Loi 4 semble être une règle supplémentaire de "Boost" non synaptique.
                            // On l'ajoute directement ici ? Non, P_next est écrasé.
                            // On va supposer que l'Intégration synaptique couvre la transmission principale.
                            // Si Ignition est un boost EXTRA, il faudrait le faire en 2 passes.
                            // Simplification: On s'en tient à l'intégration synaptique qui est plus standard.
                        } else {
                            P_next[idx] = p_new;
                            cell.H = (cell.H << 1); // Pas de spike
                        }
                    }
                    // Clamping P
                    P_next[idx] = std::clamp(P_next[idx], -1.0f, 1.0f);
                }
            }
        }
        // Swap buffers
        std::swap(P_current, P_next);
    }

    // Sauvegarde état final P
    #pragma omp parallel for
    for(int i=0; i<(int)grille.size(); ++i) {
        grille[i].P = P_current[i];
    }
}

// --- PREPARATION (Groupe A, C, D partiel) ---

void MondeSED::AppliquerLoisStructurelles(int x, int y, int z) {
    Cellule& cell = getCellule(x, y, z);
    if(!cell.is_alive) return;

    // Loi 1: Gradient
    float dx = x - barycentre_x;
    float dy = y - barycentre_y;
    float dz = z - barycentre_z;
    float dist = std::sqrt(dx*dx + dy*dy + dz*dz);

    cell.G = std::exp(-params.LAMBDA_GRADIENT * dist);

    // Loi 2: Différenciation (Irréversible)
    if(cell.T == 0) { // Souche
        if(cell.G < params.SEUIL_SOMA) {
            cell.T = 1; // Soma
        } else if (cell.G >= params.SEUIL_NEURO) {
            cell.T = 2; // Neurone
        }
    }
}

void MondeSED::AppliquerLoiApprentissage(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    Cellule& cell = getCellule(x, y, z);
    if(!cell.is_alive || cell.T != 2) return;

    const Cellule& read_cell = getCellule(x, y, z, read_grid); // État début de cycle pour P/H

    // Si la cellule a tiré récemment (check H bit 0 ou P actuel > seuil)
    bool self_fired = (read_cell.H & 1);

    std::tuple<int, int, int> voisins[26];
    int indices_W[26];
    int nb_voisins;
    GetCoordsVoisins_Optimized(x, y, z, voisins, indices_W, nb_voisins);

    for(int i=0; i<nb_voisins; ++i) {
        const Cellule& voisin = getCellule(std::get<0>(voisins[i]), std::get<1>(voisins[i]), std::get<2>(voisins[i]), read_grid);
        int w_idx = indices_W[i];

        // Check si voisin actif récent (3 derniers ticks = 3 derniers bits)
        bool voisin_active = (voisin.H & 0b111);

        if(self_fired && voisin_active) {
            cell.W[w_idx] += params.LEARN_RATE;
        } else {
            cell.W[w_idx] -= (params.LEARN_RATE * 0.1f);
        }

        // Homéostasie
        cell.W[w_idx] *= params.DECAY_SYNAPSE;
        cell.W[w_idx] = std::clamp(cell.W[w_idx], 0.0f, 1.0f);
    }
}

void MondeSED::AppliquerLoiMemoire(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    Cellule& cell = getCellule(x, y, z);
    if(!cell.is_alive) return;

    float max_e = 0.0f;
    std::tuple<int, int, int> voisins[26];
    int nb_voisins;
    GetCoordsVoisins_Optimized(x, y, z, voisins, nullptr, nb_voisins);

    for(int i=0; i<nb_voisins; ++i) {
        const Cellule& v = getCellule(std::get<0>(voisins[i]), std::get<1>(voisins[i]), std::get<2>(voisins[i]), read_grid);
        if(v.is_alive && v.E > max_e) max_e = v.E;
    }

    // M <- max(M * decay, max_voisins)
    cell.M = std::max(cell.M * (1.0f - params.TAUX_OUBLI), max_e);
}

void MondeSED::AppliquerLoiMetabolisme(int x, int y, int z) {
    Cellule& cell = getCellule(x, y, z);
    if(!cell.is_alive) return;

    // Loi 7
    cell.D += params.D_PER_TICK;
    cell.L += params.L_PER_TICK;

    // Coût existence + Coût neuronal accumulé
    float total_cost = params.K_THERMO + cell.E_cost;
    cell.E -= total_cost;
    cell.E_cost = 0.0f; // Reset accumulateur

    cell.A++; // Vieillissement
}

// --- INTENTIONS (Phase Décision sur Read Grid) ---

void MondeSED::AppliquerLoiMouvement(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    const Cellule& source = getCellule(x, y, z, read_grid);
    if(!source.is_alive) return;

    // Calcul score vers chaque voisin VIDE
    std::tuple<int, int, int> voisins[26];
    int nb_voisins;
    GetCoordsVoisins_Optimized(x, y, z, voisins, nullptr, nb_voisins);

    float max_score = -std::numeric_limits<float>::infinity();
    std::tuple<int, int, int> best_target;
    bool found = false;

    // Pré-calculs constants pour la cellule
    float gravity = params.K_D * source.D;
    float pressure = params.K_C * source.C;
    float inertia = params.K_M * (source.M / (float)(source.A + 1));

    for(int i=0; i<nb_voisins; ++i) {
        auto coords = voisins[i];
        const Cellule& target = getCellule(std::get<0>(coords), std::get<1>(coords), std::get<2>(coords), read_grid);

        if(!target.is_alive) {
            // Case vide: Calcul du champ local (simulé)
            float champ_E = 0.0f;
            float champ_C = 0.0f;
            int count_adh = 0;

            // On regarde les voisins de la CIBLE pour estimer le champ et l'adhésion
            std::tuple<int, int, int> v_cible[26];
            int nb_v_cible;
            GetCoordsVoisins_Optimized(std::get<0>(coords), std::get<1>(coords), std::get<2>(coords), v_cible, nullptr, nb_v_cible);

            for(int j=0; j<nb_v_cible; ++j) {
                const Cellule& n = getCellule(std::get<0>(v_cible[j]), std::get<1>(v_cible[j]), std::get<2>(v_cible[j]), read_grid);
                if(n.is_alive) {
                    champ_E += n.E; // Simplification champ local immédiat (rayon 1)
                    champ_C += n.C;
                    if(n.T == source.T) count_adh++;
                }
            }

            float bonus_champ = (params.K_CHAMP_E * champ_E) - (params.K_CHAMP_C * champ_C); // Simplification sans exp decay complet pour perf
            float bonus_adh = params.K_ADH * count_adh;

            float score = gravity - pressure + inertia + bonus_champ + bonus_adh - params.COUT_MOUVEMENT;

            if(score > max_score) {
                max_score = score;
                best_target = coords;
                found = true;
            }
        }
    }

    if(found) {
        #pragma omp critical
        mouvements_souhaites.push_back({std::make_tuple(x,y,z), best_target, source.D, getIndex(x,y,z)});
    }
}

void MondeSED::AppliquerLoiDivision(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    const Cellule& source = getCellule(x, y, z, read_grid);
    if(!source.is_alive || source.E <= params.SEUIL_ENERGIE_DIVISION) return;

    // Cherche voisin vide avec max R (Territoire similaire ?)
    // Doc: "Recover genetically similar territory" -> implies check R of previous occupant?
    // But empty cells have R=0 usually. Doc says "innate resistance of previously existing cell".
    // Assuming empty cells retain 'ghost' traits or we just check neighbors.
    // Implementation: simple check for empty spot.

    std::tuple<int, int, int> voisins[26];
    int nb_voisins;
    GetCoordsVoisins_Optimized(x, y, z, voisins, nullptr, nb_voisins);

    for(int i=0; i<nb_voisins; ++i) {
        const Cellule& target = getCellule(std::get<0>(voisins[i]), std::get<1>(voisins[i]), std::get<2>(voisins[i]), read_grid);
        if(!target.is_alive) {
             #pragma omp critical
             divisions_souhaitees.push_back({std::make_tuple(x,y,z), voisins[i], source.E});
             return; // Une seule tentative par tick
        }
    }
}

void MondeSED::AppliquerLoiEchange(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    const Cellule& source = getCellule(x, y, z, read_grid);
    if(!source.is_alive) return;

    std::tuple<int, int, int> voisins[26];
    int nb_voisins;
    GetCoordsVoisins_Optimized(x, y, z, voisins, nullptr, nb_voisins);

    for(int i=0; i<nb_voisins; ++i) {
        auto coords = voisins[i];
        const Cellule& target = getCellule(std::get<0>(coords), std::get<1>(coords), std::get<2>(coords), read_grid);

        // Unicité: Traiter paire une seule fois (ID source < ID target)
        if(target.is_alive && getIndex(x,y,z) < getIndex(std::get<0>(coords), std::get<1>(coords), std::get<2>(coords))) {
            if(std::abs(source.R - target.R) < params.SEUIL_SIMILARITE_R) {
                float delta = (source.E - target.E) * params.FACTEUR_ECHANGE_ENERGIE;
                float delta_safe = std::clamp(delta, -params.MAX_FLUX_ENERGIE, params.MAX_FLUX_ENERGIE);

                if(std::abs(delta_safe) > 0.0001f) {
                    #pragma omp critical
                    echanges_energie_souhaites.push_back({std::make_tuple(x,y,z), coords, delta_safe});
                }
            }
        }
    }
}

void MondeSED::AppliquerLoiPsychisme(int x, int y, int z, const std::vector<Cellule>& read_grid) {
    const Cellule& source = getCellule(x, y, z, read_grid);
    if(!source.is_alive) return;

    // Interaction forte: Contact voisin
    std::tuple<int, int, int> voisins[26];
    int nb_voisins;
    GetCoordsVoisins_Optimized(x, y, z, voisins, nullptr, nb_voisins);

    for(int i=0; i<nb_voisins; ++i) {
        auto coords = voisins[i];
        const Cellule& target = getCellule(std::get<0>(coords), std::get<1>(coords), std::get<2>(coords), read_grid);

        if(target.is_alive) {
             float echange_C = source.C + (0.1f * target.C); // Friction (Unilatéral selon doc? "C <- C + 0.1*C_voisin")
             // Wait, Doc says "C <- C + ...". This is a state update, not an exchange flow.
             // It modifies SELF based on Neighbor.
             // Can be applied directly in Preparation phase to 'grille'?
             // Yes, if we use read_grid for values.
             // But let's keep it consistent.

             // Actually, the doc says "Interaction Forte... Modification directe".
             // If I do it here, I need to push an action?
             // Or can I do it in the "Preparation" loop directly?
             // "Appliquer l'échange C et L (Interaction Forte)" is listed in Phase 4 (Resolution).
             // So I should schedule it.

             float dC = 0.1f * target.C;
             float dL = 0.1f * target.L;

             #pragma omp critical
             echanges_psychiques_souhaites.push_back({std::make_tuple(x,y,z), coords, dC, dL});
        }
    }
}

// --- RESOLUTION (Phase Ecriture Séquentielle) ---

void MondeSED::AppliquerMouvements() {
    // Tri déterministe
    std::sort(mouvements_souhaites.begin(), mouvements_souhaites.end(),
        [](const MouvementSouhaite& a, const MouvementSouhaite& b) {
             return a.index_source < b.index_source;
        });

    // Résolution conflits: Destination unique, Meilleur D gagne
    std::map<std::tuple<int,int,int>, MouvementSouhaite> winners;
    for(const auto& m : mouvements_souhaites) {
        auto it = winners.find(m.destination);
        if(it == winners.end()) {
            winners[m.destination] = m;
        } else {
            // Priority to higher D
            if(m.dette_besoin_source > it->second.dette_besoin_source) {
                winners[m.destination] = m;
            }
        }
    }

    // Apply
    for(const auto& pair : winners) {
        const auto& m = pair.second;
        Cellule& src = getCellule(std::get<0>(m.source), std::get<1>(m.source), std::get<2>(m.source));
        Cellule& dst = getCellule(std::get<0>(m.destination), std::get<1>(m.destination), std::get<2>(m.destination));

        // Move valid ? (Source still alive ?)
        if(src.is_alive && !dst.is_alive) {
             dst = src;
             dst.E -= params.COUT_MOUVEMENT; // Apply cost
             src = {}; // Empty old
        }
    }
}

float deterministic_mutation(int x, int y, int z, int age) {
  unsigned int hash = (x * 18397) + (y * 20441) + (z * 22543) + (age * 24671);
  int decision = hash % 3;
  if (decision == 0) return 0.01f;
  if (decision == 1) return -0.01f;
  return 0.0f;
}

void MondeSED::AppliquerDivisions() {
    std::sort(divisions_souhaitees.begin(), divisions_souhaitees.end(),
        [](const DivisionSouhaitee& a, const DivisionSouhaitee& b) {
             return a.source_mere < b.source_mere;
        });

    std::map<std::tuple<int,int,int>, DivisionSouhaitee> winners;
    for(const auto& d : divisions_souhaitees) {
        auto it = winners.find(d.destination_fille);
        if(it == winners.end() || d.energie_mere > it->second.energie_mere) {
            winners[d.destination_fille] = d;
        }
    }

    for(const auto& pair : winners) {
        const auto& d = pair.second;
        Cellule& mere = getCellule(std::get<0>(d.source_mere), std::get<1>(d.source_mere), std::get<2>(d.source_mere));
        Cellule& fille = getCellule(std::get<0>(d.destination_fille), std::get<1>(d.destination_fille), std::get<2>(d.destination_fille));

        if(mere.is_alive && !fille.is_alive && mere.E > params.SEUIL_ENERGIE_DIVISION) {
            float e_total = mere.E;
            mere.E = e_total / 2.0f;
            fille = mere; // Copy genetics & props
            fille.E = e_total / 2.0f;
            fille.A = 0;
            fille.D = 0;
            fille.L = 0;

            // Mutation
            int fx = std::get<0>(d.destination_fille);
            int fy = std::get<1>(d.destination_fille);
            int fz = std::get<2>(d.destination_fille);
            fille.R = std::clamp(fille.R + deterministic_mutation(fx, fy, fz, 0), 0.0f, 1.0f);
            fille.Sc = std::clamp(fille.Sc + deterministic_mutation(fx, fy, fz, 1), 0.0f, 1.0f);
        }
    }
}

void MondeSED::AppliquerEchangesEnergie() {
    for(const auto& e : echanges_energie_souhaites) {
        Cellule& src = getCellule(std::get<0>(e.source), std::get<1>(e.source), std::get<2>(e.source));
        Cellule& dst = getCellule(std::get<0>(e.destination), std::get<1>(e.destination), std::get<2>(e.destination));

        if(src.is_alive && dst.is_alive) {
            src.E -= e.montant_energie;
            dst.E += e.montant_energie;
        }
    }
}

void MondeSED::AppliquerEchangesPsychiques() {
    for(const auto& e : echanges_psychiques_souhaites) {
        Cellule& src = getCellule(std::get<0>(e.source), std::get<1>(e.source), std::get<2>(e.source));
        // Note: e.destination is strictly for finding the neighbor, but the action is on SOURCE (Unilateral friction)
        // Wait, "Interaction Forte" logic in AppliquerLoiPsychisme generated an action:
        // Source C changes based on Target C.
        // So we apply to Source.
        if(src.is_alive) {
            src.C += e.montant_C;
            src.L -= e.montant_L;
        }
    }
}

void MondeSED::FinaliserCycle() {
     // Fait dans la boucle main pour parallelisme facile ou ici
}

// --- Main Loop ---

void MondeSED::AvancerTemps() {
    cycle_actuel++;

    // 1. Analyse Globale
    CalculerBarycentre();

    // 2. Boucle Neurale (Temps Rapide)
    ExecuterCycleNeural();

    // 3. Preparation (Lecture Read -> Ecriture Grille + Intentions)
    // On fait une copie pour la lecture cohérente
    std::vector<Cellule> read_grid = grille;

    mouvements_souhaites.clear();
    divisions_souhaitees.clear();
    echanges_energie_souhaites.clear();
    echanges_psychiques_souhaites.clear();

    #pragma omp parallel for
    for(int z = 0; z < size_z; ++z) {
        for(int y = 0; y < size_y; ++y) {
            for(int x = 0; x < size_x; ++x) {
                // Application directe des lois internes (A, C, D partiel) sur 'grille'
                // en lisant 'read_grid' pour les infos voisines
                AppliquerLoisStructurelles(x, y, z);
                AppliquerLoiApprentissage(x, y, z, read_grid);
                AppliquerLoiMemoire(x, y, z, read_grid);
                AppliquerLoiMetabolisme(x, y, z);

                // Génération des Intentions (Mvt, Div, etc.)
                AppliquerLoiMouvement(x, y, z, read_grid);
                AppliquerLoiDivision(x, y, z, read_grid);
                AppliquerLoiEchange(x, y, z, read_grid);
                AppliquerLoiPsychisme(x, y, z, read_grid);
            }
        }
    }

    // 4. Résolution
    AppliquerMouvements();
    AppliquerDivisions();
    AppliquerEchangesEnergie();
    AppliquerEchangesPsychiques();

    // 5. Finalisation (Clamping et Mort)
    #pragma omp parallel for
    for(int i=0; i<(int)grille.size(); ++i) {
        Cellule& c = grille[i];
        if(c.is_alive) {
            c.C = std::clamp(c.C, 0.0f, 1.0f);
            c.R = std::clamp(c.R, 0.0f, 1.0f);
            c.Sc = std::clamp(c.Sc, 0.0f, 1.0f);
            c.E = std::max(0.0f, c.E);
            c.L = std::max(0.0f, c.L);

            // Mort
            if(c.E <= 0.0f || c.C > c.Sc) {
                c = {};
                // Restore Weights? No, death clears everything.
            }
        }
    }
}

// --- Accesseurs ---
const std::vector<Cellule>& MondeSED::getGrille() const {
    return grille;
}

// --- IO ---
bool MondeSED::SauvegarderEtat(const std::string &nom_fichier) const {
  std::ofstream fichier_sortie(nom_fichier, std::ios::binary);
  if (!fichier_sortie) return false;

  fichier_sortie.write(reinterpret_cast<const char *>(&size_x), sizeof(size_x));
  fichier_sortie.write(reinterpret_cast<const char *>(&size_y), sizeof(size_y));
  fichier_sortie.write(reinterpret_cast<const char *>(&size_z), sizeof(size_z));
  fichier_sortie.write(reinterpret_cast<const char *>(&cycle_actuel), sizeof(cycle_actuel));
  fichier_sortie.write(reinterpret_cast<const char *>(&params), sizeof(params));
  fichier_sortie.write(reinterpret_cast<const char *>(grille.data()), grille.size() * sizeof(Cellule));

  fichier_sortie.close();
  return true;
}

bool MondeSED::ChargerEtat(const std::string &nom_fichier) {
  std::ifstream fichier_entree(nom_fichier, std::ios::binary);
  if (!fichier_entree) return false;

  fichier_entree.read(reinterpret_cast<char *>(&size_x), sizeof(size_x));
  fichier_entree.read(reinterpret_cast<char *>(&size_y), sizeof(size_y));
  fichier_entree.read(reinterpret_cast<char *>(&size_z), sizeof(size_z));
  fichier_entree.read(reinterpret_cast<char *>(&cycle_actuel), sizeof(cycle_actuel));
  fichier_entree.read(reinterpret_cast<char *>(&params), sizeof(params));

  grille.resize(size_x * size_y * size_z);
  fichier_entree.read(reinterpret_cast<char *>(grille.data()), grille.size() * sizeof(Cellule));

  fichier_entree.close();
  return true;
}

bool MondeSED::ChargerParametresDepuisFichier(const std::string &nom_fichier) {
  std::ifstream fichier_params(nom_fichier);
  if (!fichier_params.is_open()) return false;

  std::string ligne;
  while (std::getline(fichier_params, ligne)) {
    if (ligne.empty() || ligne[0] == '#') continue;
    std::istringstream iss(ligne);
    std::string cle, valeur_str;
    if (std::getline(iss, cle, '=') && std::getline(iss, valeur_str)) {
      try {
        float valeur = std::stof(valeur_str);
        if (cle == "K_D") params.K_D = valeur;
        else if (cle == "K_C") params.K_C = valeur;
        else if (cle == "K_M") params.K_M = valeur;
        else if (cle == "K_THERMO") params.K_THERMO = valeur;
        else if (cle == "D_PER_TICK") params.D_PER_TICK = valeur;
        else if (cle == "L_PER_TICK") params.L_PER_TICK = valeur;
        else if (cle == "COUT_MOUVEMENT") params.COUT_MOUVEMENT = valeur;
        else if (cle == "SEUIL_ENERGIE_DIVISION") params.SEUIL_ENERGIE_DIVISION = valeur;
        else if (cle == "COUT_DIVISION") params.COUT_DIVISION = valeur;
        else if (cle == "RAYON_DIFFUSION") params.RAYON_DIFFUSION = valeur;
        else if (cle == "ALPHA_ATTENUATION") params.ALPHA_ATTENUATION = valeur;
        else if (cle == "FACTEUR_ECHANGE_ENERGIE") params.FACTEUR_ECHANGE_ENERGIE = valeur;
        else if (cle == "SEUIL_DIFFERENCE_ENERGIE") params.SEUIL_DIFFERENCE_ENERGIE = valeur;
        else if (cle == "SEUIL_SIMILARITE_R") params.SEUIL_SIMILARITE_R = valeur;
        else if (cle == "MAX_FLUX_ENERGIE") params.MAX_FLUX_ENERGIE = valeur;
        else if (cle == "FACTEUR_ECHANGE_PSYCHIQUE") params.FACTEUR_ECHANGE_PSYCHIQUE = valeur;
        else if (cle == "TAUX_OUBLI") params.TAUX_OUBLI = valeur;
        else if (cle == "TICKS_NEURAUX_PAR_PHYSIQUE") params.TICKS_NEURAUX_PAR_PHYSIQUE = (int)valeur;
        else if (cle == "SEUIL_SOMA") params.SEUIL_SOMA = valeur;
        else if (cle == "SEUIL_NEURO") params.SEUIL_NEURO = valeur;
        else if (cle == "LEARN_RATE") params.LEARN_RATE = valeur;
      } catch (...) {}
    }
  }
  return true;
}
