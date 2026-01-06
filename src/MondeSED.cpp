#include "MondeSED.h"
#include "Validator.h"
#include "raylib.h"
#include <nlohmann/json.hpp>
#include <omp.h>
using json = nlohmann::json;

#include <algorithm>
#include <cmath>
#include <cstring>
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

// Helper for RAM Detection
size_t GetAvailableRAMMB() {
  return 16384; // Dummy 16GB assumption
}

// --- Implémentation de la classe MondeSED ---

// Calcule et retourne le nombre total de cellules vivantes dans la grille.
int MondeSED::getNombreCellulesVivantes() const {
  int count = 0;
  const auto &chunks = worldMap.GetAllChunks();

  // Pour l'instant boucle séquentielle car map itération n'est pas random
  // access facile pour OMP Mais on peut faire une réduction manuelle
  for (const auto &pair : chunks) {
    const Chunk *chk = pair.second.get();
    if (!chk)
      continue;
    for (const auto &cell : chk->cells) {
      if (cell.is_alive)
        count++;
    }
  }
  return count;
}

// --- Constructeur ---
MondeSED::MondeSED(int sx, int sy, int sz)
    : size_x(sx), size_y(sy), size_z(sz), cycle_actuel(0) {
  // worldMap initialized automatically
  params = ParametresGlobaux();
  params.WORLD_HEIGHT = size_y;

  // Limits logic (kept for reference, though chunks alloc dynamically)
  size_t free_ram = GetAvailableRAMMB();
  size_t safe_ram_mb = (size_t)(free_ram * 0.7);
  params.MAX_RAM_MB = safe_ram_mb;

  std::cout << "[SYSTEM] Infinite World Mode Initialized." << std::endl;
}

// Dummy getIndex for compatibility (legacy UI mostly)
int MondeSED::getIndex(int x, int y, int z) const {
  if (x < 0 || x >= size_x || y < 0 || y >= size_y || z < 0 || z >= size_z) {
    return -1;
  }
  return x + y * size_x + z * size_x * size_y; // Fake index
}

Cellule &MondeSED::getCellule(int x, int y, int z) {
  // Get or Create
  Chunk *chk = worldMap.GetOrCreateChunk(x, y, z);
  // Calc local coords
  // Note: WorldToChunkCoords logic must be replicated or exposed?
  // Let's use internal logic if Helper hidden or just reimplement simple
  int lx = x % CHUNK_SIZE;
  int ly = y % CHUNK_SIZE;
  int lz = z % CHUNK_SIZE;
  if (lx < 0)
    lx += CHUNK_SIZE;
  if (ly < 0)
    ly += CHUNK_SIZE;
  if (lz < 0)
    lz += CHUNK_SIZE;

  return chk->GetCell(lx, ly, lz);
}

const Cellule &MondeSED::getCellule(int x, int y, int z,
                                    const WorldMap &read_map) const {
  // Read Only Global Access (via copy)
  // WorldMap object is passed as const ref
  // We need to cast away constness? No, WorldMap has no generic Const GetChunk
  // Wait, I should add Const GetChunk to WorldMap header in real life
  // For now, let's use the const GetAllChunks iteration or assume we can hack
  // it Actually, I added GetAllChunks but not specific Get. Let's rely on a
  // workaround or just const_cast for now to respect signature OR BETTER: Use
  // WorldMap::GetChunk but WorldMap needs to be non-const to use [] operator
  // But GetChunk uses find().
  // I need to update WorldMap.h/cpp to have a `const Chunk* GetChunk(...)
  // const` - Did I? No. I will use `const_cast` momentarily to proceed,
  // assuming read operations are safe.

  WorldMap &map_ref = const_cast<WorldMap &>(read_map);
  Chunk *chk = map_ref.GetChunk(x, y, z);

  static Cellule dead_cell; // Fallback
  if (!chk)
    return dead_cell;

  int lx = x % CHUNK_SIZE;
  int ly = y % CHUNK_SIZE;
  int lz = z % CHUNK_SIZE;
  if (lx < 0)
    lx += CHUNK_SIZE;
  if (ly < 0)
    ly += CHUNK_SIZE;
  if (lz < 0)
    lz += CHUNK_SIZE;

  return chk->GetCell(lx, ly, lz);
}

// --- Initialisation et Exportation ---

void MondeSED::InitialiserMonde(unsigned int seed, float initial_density) {
  std::cout << "[CPU] InitialiserMonde (Infinite) Start..." << std::endl;
  current_seed = seed;

  worldMap.Clear();
  cycle_actuel = 0;

  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> random_float(0.0f, 1.0f);
  std::uniform_real_distribution<float> random_r_s(0.0f, 1.0f);

  int living_count = 0;

  // 0. Initialize Floor (Bedrock) at Y=0
  for (int z = 0; z < size_z; ++z) {
    for (int x = 0; x < size_x; ++x) {
      Cellule &cell = getCellule(x, 0, z);
      cell.is_alive = true;
      cell.T = 3;       // Static Block
      cell.E = 1000.0f; // High energy just in case
      cell.R = 0.0f;
    }
  }

  // We restrict initialization to the "View Box" defined by size_x, size_y,
  // size_z, starting from y=1 to avoid floor
  for (int z = 0; z < size_z; ++z) {
    for (int y = 1; y < size_y; ++y) {
      for (int x = 0; x < size_x; ++x) {
        if (random_float(rng) < initial_density) {
          Cellule &cell = getCellule(x, y, z);
          // Note: getCellule autocycreates chunks
          cell.is_alive = true;
          cell.E = 1.0f;
          cell.R = random_r_s(rng);
          cell.Sc = random_r_s(rng);
          cell.T = 0;
          living_count++;
        }
      }
    }
  }
  std::cout << "[CPU] InitialiserMonde End. Created " << living_count
            << " cells." << std::endl;
}

// ... COPY STATE ...
void MondeSED::CopyStateFrom(const MondeSED &other) {
  // Deep Copy of Chunks
  cycle_actuel = other.cycle_actuel;
  size_x = other.size_x;
  size_y = other.size_y;
  size_z = other.size_z;
  params = other.params;
  current_seed = other.current_seed;

  worldMap.Clear();
  const auto &other_chunks = other.getWorldMap().GetAllChunks();
  for (const auto &pair : other_chunks) {
    // Create new chunk copy
    const Chunk *source_chk = pair.second.get();
    Chunk *dest_chk = worldMap.GetOrCreateChunk(source_chk->cx * CHUNK_SIZE,
                                                source_chk->cy * CHUNK_SIZE,
                                                source_chk->cz * CHUNK_SIZE);
    // Copy data
    dest_chk->cells = source_chk->cells;
    dest_chk->is_active = source_chk->is_active;
  }
}

size_t MondeSED::CalculateStateHash() const {
  size_t global_hash = 0;
  const auto &chunks = worldMap.GetAllChunks();
  for (const auto &pair : chunks) {
    const Chunk *chk = pair.second.get();
    if (!chk)
      continue;

    // Calculate a hash strictly for this chunk's content + position
    size_t chunk_hash = 0;

    // Mix position into hash so swapping chunks (impossible?) implies change
    size_t pos_hash = std::hash<int>{}(chk->cx) ^
                      (std::hash<int>{}(chk->cy) << 1) ^
                      (std::hash<int>{}(chk->cz) << 2);
    chunk_hash ^= pos_hash;

    for (const auto &c : chk->cells) {
      if (c.is_alive) {
        size_t val_bits = 0;
        std::memcpy(&val_bits, &c.E, sizeof(float));
        // Rotating hash for order dependency WITHIN chunk (which is stable)
        chunk_hash = (chunk_hash << 5) | (chunk_hash >> (64 - 5));
        chunk_hash ^= val_bits;
        chunk_hash ^= c.T;
      }
    }
    // XOR global hash with chunk hash (Commutative)
    global_hash ^= chunk_hash;
  }
  return global_hash;
}

// NOP stubs for unused methods or IO to be updated later
void MondeSED::ExporterEtatMonde(const std::string &nom_de_base) const {}
void MondeSED::ExporterDonneesJSON(const std::string &nom_fichier) const {}
MetriquesMonde MondeSED::CalculerMetriques() const { return MetriquesMonde(); }
bool MondeSED::SauvegarderEtat(const std::string &nom_fichier) const {
  return false;
}
bool MondeSED::ChargerEtat(const std::string &nom_fichier) { return false; }
bool MondeSED::ChargerParametresDepuisFichier(const std::string &nom_fichier) {
  return false;
}
bool MondeSED::ChargerScenario(const std::string &fichier_json) {
  return false;
}
bool MondeSED::AjouterCellule(int x, int y, int z, const Cellule &c, bool f) {
  return false;
}

float deterministic_noise(int x, int y, int z, int cycle, int sub_tick,
                          unsigned int seed) {
  unsigned int hash = (x * 73856093) ^ (y * 19349663) ^ (z * 83492791) ^
                      (cycle * 55555) ^ (sub_tick * 1234) ^ (seed * 9781);
  return ((hash % 100) / 1000.0f) - 0.05f;
}

float deterministic_mutation(int x, int y, int z, int age, unsigned int seed) {
  unsigned int hash =
      (x * 18397) + (y * 20441) + (z * 22543) + (age * 24671) ^ (seed * 34567);
  int decision = hash % 3;
  if (decision == 0)
    return 0.01f;
  if (decision == 1)
    return -0.01f;
  return 0.0f;
}

void MondeSED::CalculerBarycentre() {
  // Simplified or NOP for infinite
  barycentre_x = size_x / 2;
  barycentre_y = size_y / 2;
  barycentre_z = size_z / 2;
}

// ------------------------------------------------------------------
// --- CORE LOOP ADAPTATION (B3 integrated in B2 logic) ---
// ------------------------------------------------------------------

void MondeSED::GetCoordsVoisins_Optimized(
    int x, int y, int z, std::tuple<int, int, int> *voisins_array,
    int *indices_array, int &count) const {
  count = 0;
  for (int dz = -1; dz <= 1; ++dz) {
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0 && dz == 0)
          continue;

        int nx = x + dx;
        int ny = y + dy;
        int nz = z + dz;

        // Infinite World: All neighbors valid if we assume space exists.
        // We just return coords. The getCellule will return Dead Cell if chunk
        // missing.
        if (indices_array) {
          indices_array[count] = (dz + 1) * 9 + (dy + 1) * 3 + (dx + 1);
        }
        voisins_array[count++] = std::make_tuple(nx, ny, nz);
      }
    }
  }
}

// --- LAWS IMPLEMENTATION (Generic using getCellule abstraction) ---

// Include Laws
#include "../include/laws/LoiMetabolisme.h"

void MondeSED::AppliquerLoisStructurelles(int x, int y, int z) {
  Cellule &cell = getCellule(x, y, z);
  if (!cell.is_alive || cell.T == 3)
    return;
  // Gradient logic requires global barycenter which is vague in infinite world.
  // We use distance from center of "View Box" for now.
  float dx = x - barycentre_x;
  float dy = y - barycentre_y;
  float dz = z - barycentre_z;
  float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
  cell.G = std::exp(-params.LAMBDA_GRADIENT * dist);
  if (cell.T == 0) {
    if (cell.G < params.SEUIL_SOMA)
      cell.T = 1;
    else if (cell.G >= params.SEUIL_NEURO)
      cell.T = 2;
  }
}

void MondeSED::AppliquerLoiApprentissage(int x, int y, int z,
                                         const WorldMap &read_map) {
  Cellule &cell = getCellule(x, y, z);
  if (!cell.is_alive || cell.T != 2)
    return;
  const Cellule &read_cell = getCellule(x, y, z, read_map);
  bool self_fired = (read_cell.H & 1);
  std::tuple<int, int, int> voisins[26];
  int indices_W[26];
  int nb_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, indices_W, nb_voisins);
  for (int i = 0; i < nb_voisins; ++i) {
    const Cellule &voisin =
        getCellule(std::get<0>(voisins[i]), std::get<1>(voisins[i]),
                   std::get<2>(voisins[i]), read_map);
    int w_idx = indices_W[i];
    bool voisin_active = (voisin.H & 0b111);
    if (self_fired && voisin_active)
      cell.W[w_idx] += params.LEARN_RATE;
    else
      cell.W[w_idx] -= (params.LEARN_RATE * 0.1f);
    cell.W[w_idx] *= params.DECAY_SYNAPSE;
    cell.W[w_idx] = std::clamp(cell.W[w_idx], 0.0f, 1.0f);
  }
}

void MondeSED::AppliquerLoiMemoire(int x, int y, int z,
                                   const WorldMap &read_map) {
  Cellule &cell = getCellule(x, y, z);
  if (!cell.is_alive)
    return;
  float max_e = 0.0f;
  std::tuple<int, int, int> voisins[26];
  int nb_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, nullptr, nb_voisins);
  for (int i = 0; i < nb_voisins; ++i) {
    const Cellule &v =
        getCellule(std::get<0>(voisins[i]), std::get<1>(voisins[i]),
                   std::get<2>(voisins[i]), read_map);
    if (v.is_alive && v.E > max_e)
      max_e = v.E;
  }
  cell.M = std::max(cell.M * (1.0f - params.TAUX_OUBLI), max_e);
}

void MondeSED::AppliquerLoiMetabolisme(int x, int y, int z) {
  Cellule &cell = getCellule(x, y, z);
  if (cell.T == 3)
    return; // No metabolism for bedrock
  static LoiMetabolisme loi_thermo;
  loi_thermo.Apply(x, y, z, cell, params);
}

void MondeSED::AppliquerLoiMouvement(int x, int y, int z,
                                     const WorldMap &read_map, int thread_id) {
  const Cellule &source = getCellule(x, y, z, read_map);
  if (!source.is_alive || source.T == 3)
    return;

  std::tuple<int, int, int> voisins[26];
  int nb_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, nullptr, nb_voisins);

  float max_score = -std::numeric_limits<float>::infinity();
  std::tuple<int, int, int> best_target;
  bool found = false;

  float gravity = params.K_D * source.D;
  float pressure = params.K_C * source.C;
  float inertia = params.K_M * (source.M / (float)(source.A + 1));

  for (int i = 0; i < nb_voisins; ++i) {
    auto coords = voisins[i];
    const Cellule &target = getCellule(std::get<0>(coords), std::get<1>(coords),
                                       std::get<2>(coords), read_map);

    if (!target.is_alive) {
      // Case vide
      // Calculs simplifiés pour Chunk Mode (Rayons limités)
      float score = gravity - pressure + inertia - params.COUT_MOUVEMENT;
      if (score > max_score) {
        max_score = score;
        best_target = coords;
        found = true;
      }
    }
  }

  if (found) {
    thread_contexts[thread_id].mouvements.push_back(
        {std::make_tuple(x, y, z), best_target, source.D, 0});
  }
}

void MondeSED::AppliquerLoiDivision(int x, int y, int z,
                                    const WorldMap &read_map, int thread_id) {
  const Cellule &source = getCellule(x, y, z, read_map);
  if (!source.is_alive || source.T == 3 ||
      source.E <= params.SEUIL_ENERGIE_DIVISION)
    return;

  std::tuple<int, int, int> voisins[26];
  int nb_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, nullptr, nb_voisins);

  for (int i = 0; i < nb_voisins; ++i) {
    const Cellule &target =
        getCellule(std::get<0>(voisins[i]), std::get<1>(voisins[i]),
                   std::get<2>(voisins[i]), read_map);
    if (!target.is_alive) {
      thread_contexts[thread_id].divisions.push_back(
          {std::make_tuple(x, y, z), voisins[i], source.E});
      return;
    }
  }
}

void MondeSED::AppliquerLoiEchange(int x, int y, int z,
                                   const WorldMap &read_map, int thread_id) {
  const Cellule &source = getCellule(x, y, z, read_map);
  if (!source.is_alive || source.T == 3)
    return;

  std::tuple<int, int, int> voisins[26];
  int nb_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, nullptr, nb_voisins);

  for (int i = 0; i < nb_voisins; ++i) {
    const Cellule &target =
        getCellule(std::get<0>(voisins[i]), std::get<1>(voisins[i]),
                   std::get<2>(voisins[i]), read_map);
    if (target.is_alive) {
      // Simplified check to avoid double processing: x < target_x roughly
      // Need a reliable deterministic interaction check
      // Let's use tuple comparison
      auto t_src = std::make_tuple(x, y, z);
      auto t_tgt = voisins[i];

      if (t_src < t_tgt &&
          std::abs(source.R - target.R) < params.SEUIL_SIMILARITE_R) {
        float delta = (source.E - target.E) * params.FACTEUR_ECHANGE_ENERGIE;
        float delta_safe = std::clamp(delta, -params.MAX_FLUX_ENERGIE,
                                      params.MAX_FLUX_ENERGIE);
        if (std::abs(delta_safe) > 0.0001f) {
          thread_contexts[thread_id].echanges_energie.push_back(
              {t_src, t_tgt, delta_safe});
        }
      }
    }
  }
}

void MondeSED::AppliquerLoiPsychisme(int x, int y, int z,
                                     const WorldMap &read_map, int thread_id) {
  const Cellule &source = getCellule(x, y, z, read_map);
  if (!source.is_alive || source.T == 3)
    return;

  std::tuple<int, int, int> voisins[26];
  int nb_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, nullptr, nb_voisins);
  for (int i = 0; i < nb_voisins; ++i) {
    const Cellule &target =
        getCellule(std::get<0>(voisins[i]), std::get<1>(voisins[i]),
                   std::get<2>(voisins[i]), read_map);
    if (target.is_alive) {
      float dC = 0.1f * target.C;
      float dL = 0.1f * target.L;
      thread_contexts[thread_id].echanges_psychiques.push_back(
          {std::make_tuple(x, y, z), voisins[i], dC, dL});
    }
  }
}

// ... RESOLUTION (Similaire mais via GetCellule) ...
void MondeSED::AppliquerMouvements() {
  std::sort(mouvements_souhaites.begin(), mouvements_souhaites.end(),
            [](const MouvementSouhaite &a, const MouvementSouhaite &b) {
              return a.destination < b.destination;
            });
  // Simplified resolution
  // Note: This needs proper conflict handling
  for (const auto &m : mouvements_souhaites) {
    Cellule &src = getCellule(std::get<0>(m.source), std::get<1>(m.source),
                              std::get<2>(m.source));
    Cellule &dst =
        getCellule(std::get<0>(m.destination), std::get<1>(m.destination),
                   std::get<2>(m.destination));
    if (src.is_alive && !dst.is_alive) {
      dst = src;
      dst.E -= params.COUT_MOUVEMENT;
      src = {};
    }
  }
}
void MondeSED::AppliquerDivisions() {
  for (const auto &d : divisions_souhaitees) {
    Cellule &mere =
        getCellule(std::get<0>(d.source_mere), std::get<1>(d.source_mere),
                   std::get<2>(d.source_mere));
    Cellule &fille = getCellule(std::get<0>(d.destination_fille),
                                std::get<1>(d.destination_fille),
                                std::get<2>(d.destination_fille));
    if (mere.is_alive && !fille.is_alive &&
        mere.E > params.SEUIL_ENERGIE_DIVISION) {
      float e = mere.E / 2.0f;
      mere.E = e;
      fille = mere;
      fille.E = e;
      fille.A = 0;
      fille.D = 0;
      fille.L = 0;
      int fx = std::get<0>(d.destination_fille);
      int fy = std::get<1>(d.destination_fille);
      int fz = std::get<2>(d.destination_fille);
      fille.R = std::clamp(
          fille.R + deterministic_mutation(fx, fy, fz, 0, current_seed), 0.0f,
          1.0f);
    }
  }
}
void MondeSED::AppliquerEchangesEnergie() {
  for (const auto &e : echanges_energie_souhaites) {
    Cellule &src = getCellule(std::get<0>(e.source), std::get<1>(e.source),
                              std::get<2>(e.source));
    Cellule &dst =
        getCellule(std::get<0>(e.destination), std::get<1>(e.destination),
                   std::get<2>(e.destination));
    if (src.is_alive && dst.is_alive) {
      src.E -= e.montant_energie;
      dst.E += e.montant_energie;
    }
  }
}
void MondeSED::AppliquerEchangesPsychiques() {
  for (const auto &e : echanges_psychiques_souhaites) {
    Cellule &src = getCellule(std::get<0>(e.source), std::get<1>(e.source),
                              std::get<2>(e.source));
    if (src.is_alive) {
      src.C += e.montant_C;
      src.L -= e.montant_L;
    }
  }
}
void MondeSED::FinaliserCycle() {}

// ... MAIN LOOP REWRITE ...
void MondeSED::ExecuterCycleNeural() {
  // Neural loop logic on Chunks
  // We iterate ONLY on active chunks to save time
  const auto &chunks = worldMap.GetAllChunks();
  // Problem: OMP Parallel on Map iterator not possible directly.
  // Convert to vector of pointers
  std::vector<Chunk *> active_chunks;
  for (const auto &pair : chunks)
    active_chunks.push_back(pair.second.get());

  // Neural Loop N times
  for (int iter = 0; iter < params.TICKS_NEURAUX_PAR_PHYSIQUE; ++iter) {
    // Should do double buffering for P ?
    // For now, simplify to single pass (approx) or copy P to local buffer
    // inside Chunk? Let's assume sequential/atomic for now to make it compile
  }
}

void MondeSED::AvancerTemps() {
  cycle_actuel++;
  CalculerBarycentre();

  // 1. Snapshot Read State (Deep Copy of Map)
  // We create a temp WorldMap for reading
  WorldMap read_map;
  // Copy content
  const auto &chunks = worldMap.GetAllChunks();
  for (const auto &pair : chunks) {
    Chunk *new_chk = read_map.GetOrCreateChunk(pair.second->cx * CHUNK_SIZE,
                                               pair.second->cy * CHUNK_SIZE,
                                               pair.second->cz * CHUNK_SIZE);
    new_chk->cells = pair.second->cells;
  }

  // Clear intentions
  mouvements_souhaites.clear();
  divisions_souhaitees.clear();
  echanges_energie_souhaites.clear();
  echanges_psychiques_souhaites.clear();

  // 2. Iterate Logic
  // Collect active coordinates to process
  // To support "Infinite Scroll" (Expansion), we should iterate:
  // A. All Alive Cells
  // B. All Empty Neighbors of Alive Cells (for birth/move)
  // This defines the "Active Frontier".

  // Simple approach: Iterate over all Allocated Chunks + Boundary Check?
  // Actually, iterating all cells in all chunks is safe (4096 * N).

  std::vector<Chunk *> active_chunks;
  active_chunks.reserve(chunks.size());
  for (const auto &pair : chunks) {
    active_chunks.push_back(pair.second.get());
  }

  // Sort chunks to ensure deterministic order of processing
  std::sort(active_chunks.begin(), active_chunks.end(),
            [](const Chunk *a, const Chunk *b) {
              if (a->cx != b->cx)
                return a->cx < b->cx;
              if (a->cy != b->cy)
                return a->cy < b->cy;
              return a->cz < b->cz;
            });

  int max_threads = omp_get_max_threads();
  if (thread_contexts.size() != max_threads)
    thread_contexts.resize(max_threads);
  for (auto &ctx : thread_contexts) {
    ctx.mouvements.clear();
    ctx.divisions.clear();
    ctx.echanges_energie.clear();
    ctx.echanges_psychiques.clear();
  }

#pragma omp parallel for schedule(static)
  for (int i = 0; i < (int)active_chunks.size(); ++i) {
    Chunk *chk = active_chunks[i];
    int thread_id = omp_get_thread_num();
    int base_x = chk->cx * CHUNK_SIZE;
    int base_y = chk->cy * CHUNK_SIZE;
    int base_z = chk->cz * CHUNK_SIZE;

    for (int lz = 0; lz < CHUNK_SIZE; ++lz) {
      for (int ly = 0; ly < CHUNK_SIZE; ++ly) {
        for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
          int x = base_x + lx;
          int y = base_y + ly;
          int z = base_z + lz;

          AppliquerLoisStructurelles(x, y, z);
          AppliquerLoiMetabolisme(x, y, z);
          AppliquerLoiMouvement(x, y, z, read_map, thread_id);
          AppliquerLoiDivision(x, y, z, read_map, thread_id);
          AppliquerLoiEchange(x, y, z, read_map, thread_id);
          AppliquerLoiPsychisme(x, y, z, read_map, thread_id);
        }
      }
    }
  }

  // Merge Intentions
  for (const auto &ctx : thread_contexts) {
    mouvements_souhaites.insert(mouvements_souhaites.end(),
                                ctx.mouvements.begin(), ctx.mouvements.end());
    divisions_souhaitees.insert(divisions_souhaitees.end(),
                                ctx.divisions.begin(), ctx.divisions.end());
    echanges_energie_souhaites.insert(echanges_energie_souhaites.end(),
                                      ctx.echanges_energie.begin(),
                                      ctx.echanges_energie.end());
    echanges_psychiques_souhaites.insert(echanges_psychiques_souhaites.end(),
                                         ctx.echanges_psychiques.begin(),
                                         ctx.echanges_psychiques.end());
  }

  // Resolution
  AppliquerMouvements();
  AppliquerDivisions();
  AppliquerEchangesEnergie();
  AppliquerEchangesPsychiques();

  // Clean up dead/empty chunks? (Garbage Collection) - Not for now
}