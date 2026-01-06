#ifndef MONDESED_H
#define MONDESED_H

#include "SimulationData.h"
#include "WorldMap.h"

#include <string>
#include <tuple>
#include <vector>
// Other includes are in SimulationData.h but good to keep if used here
// explicitly

/**
 * @class MondeSED
 * @brief Classe principale gérant la grille de simulation, les lois et l'état
 * global.
 */
class MondeSED {
public:
  // --- Configuration et Cycle de Vie ---
  MondeSED(int size_x, int size_y, int size_z);
  void InitialiserMonde(unsigned int seed, float initial_density = 0.5f);
  void AvancerTemps();
  void ExporterEtatMonde(const std::string &nom_de_base) const;
  void ExporterDonneesJSON(const std::string &nom_fichier) const;
  MetriquesMonde CalculerMetriques() const;

  // --- Sauvegarde et Chargement ---
  bool SauvegarderEtat(const std::string &nom_fichier) const;
  bool ChargerEtat(const std::string &nom_fichier);
  bool ChargerParametresDepuisFichier(const std::string &nom_fichier);
  // Scénarios
  bool ChargerScenario(const std::string &fichier_json);
  bool AjouterCellule(int x, int y, int z, const Cellule &c,
                      bool force_overwrite = false);

  // ...

  // --- Accesseurs (Getters) ---
  // const std::vector<Cellule> &getGrille() const; // DEPRECATED for Infinite
  // World
  const WorldMap &getWorldMap() const { return worldMap; }
  void CopyStateFrom(const MondeSED &other); // New method for Thread Sync
  int GetCellCount() const;

  // ...
  // Getters
  int getTailleX() const { return size_x; }
  int getTailleY() const { return size_y; }
  int getTailleZ() const { return size_z; }
  int getCycleActuel() const { return cycle_actuel; }
  unsigned int getSeed() const { return current_seed; }
  int getNombreCellulesVivantes() const;
  size_t CalculateStateHash() const;

  // --- Helpers Publics (UI) ---
  int getIndex(int x, int y, int z) const;
  Cellule &getCellule(int x, int y, int z);
  const Cellule &getCellule(int x, int y, int z,
                            const WorldMap &read_map) const; // Modified sig

  // --- Paramètres Publics ---
  ParametresGlobaux params;

private:
  // --- État Interne ---
  int size_x, size_y, size_z;
  int cycle_actuel = 0;
  unsigned int current_seed = 0;
  WorldMap worldMap; // REPLACED: std::vector<Cellule> grille;

  // Barycentre pour le Gradient (G)
  float barycentre_x = 0, barycentre_y = 0, barycentre_z = 0;

  // --- Vecteurs d'Actions Différées (Global) ---
  std::vector<MouvementSouhaite> mouvements_souhaites;
  std::vector<DivisionSouhaitee> divisions_souhaitees;
  std::vector<EchangeEnergieSouhaite> echanges_energie_souhaites;
  std::vector<EchangePsychiqueSouhaite> echanges_psychiques_souhaites;

  // --- Thread-Local Buffers (Pour éviter OMP Critical) ---
  // Structure holding all intentions for a single thread
  struct ThreadContext {
    std::vector<MouvementSouhaite> mouvements;
    std::vector<DivisionSouhaitee> divisions;
    std::vector<EchangeEnergieSouhaite> echanges_energie;
    std::vector<EchangePsychiqueSouhaite> echanges_psychiques;
  };
  std::vector<ThreadContext> thread_contexts;

  void CalculerBarycentre();
  void ExecuterCycleNeural(); // Groupe B (N fois)

  // Groupe A & C & D (Preparation Phase)
  void AppliquerLoisStructurelles(int x, int y,
                                  int z); // Gradient G, Différenciation T
  void AppliquerLoiApprentissage(int x, int y, int z,
                                 const WorldMap &read_map); // Hebb W
  void AppliquerLoiMemoire(int x, int y, int z,
                           const WorldMap &read_map); // M
  void AppliquerLoiMetabolisme(int x, int y,
                               int z); // E, D, L update (Loi 7 partie 1)

  // Décisions (Phase Lecture) - Modified to accept thread ID
  void AppliquerLoiMouvement(int x, int y, int z, const WorldMap &read_map,
                             int thread_id);
  void AppliquerLoiDivision(int x, int y, int z, const WorldMap &read_map,
                            int thread_id);
  void AppliquerLoiEchange(int x, int y, int z, const WorldMap &read_map,
                           int thread_id);
  void AppliquerLoiPsychisme(int x, int y, int z, const WorldMap &read_map,
                             int thread_id);

  // --- Fonctions de Résolution (Logique Interne - Phase Ecriture) ---
  void AppliquerMouvements();
  void AppliquerDivisions();
  void AppliquerEchangesEnergie();
  void AppliquerEchangesPsychiques();
  void FinaliserCycle(); // Clamping, Mort

  // --- Fonctions Utilitaires (Optimisation) ---
  // Nouvelle méthode optimisée qui écrit directement dans un tableau
  // pré-alloué. Retourne aussi les indices dans le tableau 'voisins_indices' si
  // fourni
  void GetCoordsVoisins_Optimized(int x, int y, int z,
                                  std::tuple<int, int, int> *voisins_array,
                                  int *indices_array, int &count) const;
};

#endif // MONDESED_H