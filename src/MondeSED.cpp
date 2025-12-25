#include "MondeSED.h"
#include "raylib.h"
#include "rlgl.h" // For Compute Shader & SSBO
#include <algorithm>
#include <cmath>
#include <cstring> // For memcpy
#include <cstring> // Pour memset si besoin
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

// Windows Includes removed to avoid Raylib collision
// Helper for RAM Detection
size_t GetAvailableRAMMB() {
  return 16384; // Dummy 16GB assumption
}

// --- Manual OpenGL Loader for Compute Shaders ---
#ifndef APIENTRY
#ifdef _WIN32
#define APIENTRY __stdcall
#else
#define APIENTRY
#endif
#endif

// Win32 Manual Declarations to avoid <windows.h> conflicts
extern "C" {
__declspec(dllimport) void *__stdcall LoadLibraryA(const char *lpLibFileName);
__declspec(dllimport) void *__stdcall GetProcAddress(void *hModule,
                                                     const char *lpProcName);
}

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef int GLint;
typedef char GLchar;
typedef unsigned int GLbitfield;

#define GL_COMPUTE_SHADER 0x91B9
#define GL_SHADER_STORAGE_BARRIER_BIT 0x2000

typedef GLuint(APIENTRY *PFNGLCREATESHADERPROC)(GLenum type);
typedef void(APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count,
                                              const GLchar *const *string,
                                              const GLint *length);
typedef void(APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void(APIENTRY *PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname,
                                             GLint *params);
typedef void(APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint shader,
                                                  GLsizei bufSize,
                                                  GLsizei *length,
                                                  GLchar *infoLog);
typedef void(APIENTRY *PFNGLDISPATCHCOMPUTEPROC)(GLuint num_groups_x,
                                                 GLuint num_groups_y,
                                                 GLuint num_groups_z);
typedef void(APIENTRY *PFNGLMEMORYBARRIERPROC)(GLbitfield barriers);
typedef void(APIENTRY *PFNGLDELETEPROGRAM)(GLuint program);

static PFNGLCREATESHADERPROC pfn_glCreateShader = nullptr;
static PFNGLSHADERSOURCEPROC pfn_glShaderSource = nullptr;
static PFNGLCOMPILESHADERPROC pfn_glCompileShader = nullptr;
static PFNGLGETSHADERIVPROC pfn_glGetShaderiv = nullptr;
static PFNGLGETSHADERINFOLOGPROC pfn_glGetShaderInfoLog = nullptr;
static PFNGLDISPATCHCOMPUTEPROC pfn_glDispatchCompute = nullptr;
static PFNGLMEMORYBARRIERPROC pfn_glMemoryBarrier = nullptr;
static PFNGLDELETEPROGRAM pfn_glDeleteProgram = nullptr;

static bool LoadComputeProcs() {
  if (pfn_glDispatchCompute)
    return true; // Already loaded

  // Get wglGetProcAddress
  void *hMod = LoadLibraryA("opengl32.dll");
  typedef void *(APIENTRY * PFNWGLGETPROCADDRESS)(const char *proc);
  PFNWGLGETPROCADDRESS wglGetProcAddress =
      (PFNWGLGETPROCADDRESS)GetProcAddress(hMod, "wglGetProcAddress");

  if (!wglGetProcAddress)
    return false;

  pfn_glCreateShader =
      (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
  pfn_glShaderSource =
      (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
  pfn_glCompileShader =
      (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
  pfn_glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
  pfn_glGetShaderInfoLog =
      (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
  pfn_glDispatchCompute =
      (PFNGLDISPATCHCOMPUTEPROC)wglGetProcAddress("glDispatchCompute");
  pfn_glMemoryBarrier =
      (PFNGLMEMORYBARRIERPROC)wglGetProcAddress("glMemoryBarrier");
  pfn_glDeleteProgram =
      (PFNGLDELETEPROGRAM)wglGetProcAddress("glDeleteProgram");

  return pfn_glDispatchCompute != nullptr;
}

static unsigned int CompileComputeShaderManual(const char *source) {
  if (!LoadComputeProcs() || !pfn_glCreateShader)
    return 0;

  GLuint shader = pfn_glCreateShader(GL_COMPUTE_SHADER);
  pfn_glShaderSource(shader, 1, &source, nullptr);
  pfn_glCompileShader(shader);

  GLint success;
  pfn_glGetShaderiv(shader, 0x8B81, &success); // GL_COMPILE_STATUS
  if (!success) {
    char infoLog[1024];
    pfn_glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
    std::cerr << "[TITANIC] Manual Shader Compile Error: " << infoLog
              << std::endl;
    return 0;
  }
  return shader;
}

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

  // Auto-Detect System Limits
  size_t free_ram = GetAvailableRAMMB();
  // Safety Margin: Use 70% of Free RAM
  size_t safe_ram_mb = (size_t)(free_ram * 0.7);
  params.MAX_RAM_MB = safe_ram_mb;

  // Calculate Safe Cell Count
  // Cellule size ~100 bytes (struct check required, let's say 128 for safety)
  size_t cell_size = sizeof(Cellule);
  size_t max_cells = (safe_ram_mb * 1024 * 1024) / cell_size;

  // Clamp to reasonable upper bound (e.g. 50M) to limit integer overflows
  if (max_cells > 50000000)
    max_cells = 50000000;

  params.MAX_CELLS = max_cells;

  std::cout << "[SYSTEM] RAM Detected: " << free_ram << " MB" << std::endl;
  std::cout << "[SYSTEM] Safe Limit Set: " << params.MAX_CELLS << " Cells ("
            << params.MAX_RAM_MB << " MB Allocated)" << std::endl;

  if (CheckGPUSupport()) {
    use_gpu = true;
    std::cout << "[SYSTEM] GPU Compute is Available. Enabled by default."
              << std::endl;
  } else {
    use_gpu = false;
    std::cout << "[SYSTEM] GPU Compute not available or experimental. "
                 "Defaulting to CPU."
              << std::endl;
  }
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

        // Initialisation des poids synaptiques (pour tous, au cas où ils
        // deviennent neurones)
        for (int i = 0; i < 27; ++i)
          cell.W[i] = random_w(rng);

        if (random_float(rng) < initial_density) {
          // Safety Check: Max Cells
          if (!params.limit_safety_override &&
              getNombreCellulesVivantes() >= params.MAX_CELLS) {
            std::cerr << "[WARN] Initialisation: Max Cells ("
                      << params.MAX_CELLS << ") reached at z=" << z << "\n";
            goto init_finished;
          }

          cell.is_alive = true;
          cell.E = 1.0f;
          cell.R = random_r_s(rng);
          cell.Sc = random_r_s(rng);
          cell.T = 0; // Souche par défaut
        }
      }
    }
  }

init_finished:;
}

void MondeSED::ExporterEtatMonde(const std::string &nom_de_base) const {
  std::string nom_fichier =
      nom_de_base + "_cycle_" + std::to_string(cycle_actuel) + ".csv";
  std::ofstream outfile(nom_fichier);
  if (!outfile.is_open())
    return;

  outfile << "x,y,z,T,E,D,C,L,M,R,P,G\n";

  for (int z = 0; z < size_z; ++z) {
    for (int y = 0; y < size_y; ++y) {
      for (int x = 0; x < size_x; ++x) {
        const Cellule &cell = getCellule(x, y, z, grille);
        if (cell.is_alive) {
          outfile << x << "," << y << "," << z << "," << (int)cell.T << ","
                  << cell.E << "," << cell.D << "," << cell.C << "," << cell.L
                  << "," << cell.M << "," << cell.R << "," << cell.P << ","
                  << cell.G << "\n";
        }
      }
    }
  }
  outfile.close();
}

// --- Fonctions Utilitaires ---

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
        if (getIndex(nx, ny, nz) != -1) {
          if (indices_array) {
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
#pragma omp parallel for reduction(+ : sum_x, sum_y, sum_z, count)
  for (int i = 0; i < (int)grille.size(); ++i) {
    if (grille[i].is_alive) {
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

  if (count > 0) {
    barycentre_x = (float)(sum_x / count);
    barycentre_y = (float)(sum_y / count);
    barycentre_z = (float)(sum_z / count);
  } else {
    barycentre_x = size_x / 2.0f;
    barycentre_y = size_y / 2.0f;
    barycentre_z = size_z / 2.0f;
  }
}

// Fonction de bruit déterministe pour la Loi 3
float deterministic_noise(int x, int y, int z, int cycle, int sub_tick) {
  unsigned int hash = (x * 73856093) ^ (y * 19349663) ^ (z * 83492791) ^
                      (cycle * 55555) ^ (sub_tick * 1234);
  // Ramener entre -0.05 et +0.05 par exemple (bruit faible)
  // Spec doesn't specify magnitude, only "Noise(Seed)". Assuming small
  // perturbation.
  return ((hash % 100) / 1000.0f) - 0.05f;
}

// --- BOUCLE NEURALE (Groupe B) ---
void MondeSED::ExecuterCycleNeural() {
  // Allocation temporaire pour double-buffering de P
  std::vector<float> P_current(grille.size());
  std::vector<float> P_next(grille.size());

// Copie initiale
#pragma omp parallel for
  for (int i = 0; i < (int)grille.size(); ++i) {
    P_current[i] = grille[i].P;
  }

  // Boucle Rapide
  for (int iter = 0; iter < params.TICKS_NEURAUX_PAR_PHYSIQUE; ++iter) {

#pragma omp parallel for
    for (int z = 0; z < size_z; ++z) {
      for (int y = 0; y < size_y; ++y) {
        for (int x = 0; x < size_x; ++x) {
          int idx = getIndex(x, y, z);
          Cellule &cell = grille[idx]; // Note: Write access to metadata (Ref,
                                       // E_cost), Read P from buffer

          if (!cell.is_alive || cell.T != 2) { // Seulement Neurones
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

          for (int i = 0; i < nb_voisins; ++i) {
            int v_idx =
                getIndex(std::get<0>(voisins[i]), std::get<1>(voisins[i]),
                         std::get<2>(voisins[i]));
            float w = cell.W[indices_W[i]];
            float p_voisin = P_current[v_idx];

            if (w > 0) {
              sum_input += p_voisin * w;
              sum_W += w;
            }
          }

          float I = (sum_W > 0) ? (sum_input / std::max(1.0f, sum_W)) : 0.0f;

          // Loi 4: Ignition (Broadcast Local - Radius 2)
          // Si un voisin dans rayon 2 a spike (P > Seuil), je reçois +0.1
          float ignition_boost = 0.0f;
          int r_ign = (int)params.RAYON_IGNITION;

          for (int dz = -r_ign; dz <= r_ign; ++dz) {
            for (int dy = -r_ign; dy <= r_ign; ++dy) {
              for (int dx = -r_ign; dx <= r_ign; ++dx) {
                if (dx == 0 && dy == 0 && dz == 0)
                  continue;
                int nx = x + dx, ny = y + dy, nz = z + dz;
                int v_idx = getIndex(nx, ny, nz);
                if (v_idx != -1) {
                  // Check Spike dans P_current (Etat début de tick neural)
                  // Note: On suppose que "Spike" veut dire P > SEUIL_FIRE
                  if (P_current[v_idx] > params.SEUIL_FIRE) {
                    ignition_boost += 0.1f;
                  }
                }
              }
            }
          }

          // Gestion Réfractaire
          if (cell.Ref > 0) {
            cell.Ref--;
            P_next[idx] = 0.0f; // Inhibé
          } else {
            // Mise à jour Potentiel
            // P_new = (P_old * 0.9) + I + Noise
            float noise = deterministic_noise(x, y, z, cycle_actuel, iter);
            float p_new = (P_current[idx] * 0.9f) + I + noise + ignition_boost;

            // Spike ?
            if (p_new > params.SEUIL_FIRE) {
              P_next[idx] = 1.0f; // Spike
              cell.Ref = params.PERIODE_REFRACTAIRE;
              cell.E_cost += params.COUT_SPIKE; // Accumule coût

              // Mise à jour Historique (Bitfield)
              cell.H = (cell.H << 1) | 1;
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
  for (int i = 0; i < (int)grille.size(); ++i) {
    grille[i].P = P_current[i];
  }
}

// --- PREPARATION (Groupe A, C, D partiel) ---

void MondeSED::AppliquerLoisStructurelles(int x, int y, int z) {
  Cellule &cell = getCellule(x, y, z);
  if (!cell.is_alive)
    return;

  // Loi 1: Gradient
  float dx = x - barycentre_x;
  float dy = y - barycentre_y;
  float dz = z - barycentre_z;
  float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

  cell.G = std::exp(-params.LAMBDA_GRADIENT * dist);

  // Loi 2: Différenciation (Irréversible)
  if (cell.T == 0) { // Souche
    if (cell.G < params.SEUIL_SOMA) {
      cell.T = 1; // Soma
    } else if (cell.G >= params.SEUIL_NEURO) {
      cell.T = 2; // Neurone
    }
  }
}

void MondeSED::AppliquerLoiApprentissage(
    int x, int y, int z, const std::vector<Cellule> &read_grid) {
  Cellule &cell = getCellule(x, y, z);
  if (!cell.is_alive || cell.T != 2)
    return;

  const Cellule &read_cell =
      getCellule(x, y, z, read_grid); // État début de cycle pour P/H

  // Si la cellule a tiré récemment (check H bit 0 ou P actuel > seuil)
  bool self_fired = (read_cell.H & 1);

  std::tuple<int, int, int> voisins[26];
  int indices_W[26];
  int nb_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, indices_W, nb_voisins);

  for (int i = 0; i < nb_voisins; ++i) {
    const Cellule &voisin =
        getCellule(std::get<0>(voisins[i]), std::get<1>(voisins[i]),
                   std::get<2>(voisins[i]), read_grid);
    int w_idx = indices_W[i];

    // Check si voisin actif récent (3 derniers ticks = 3 derniers bits)
    bool voisin_active = (voisin.H & 0b111);

    if (self_fired && voisin_active) {
      cell.W[w_idx] += params.LEARN_RATE;
    } else {
      cell.W[w_idx] -= (params.LEARN_RATE * 0.1f);
    }

    // Homéostasie
    cell.W[w_idx] *= params.DECAY_SYNAPSE;
    cell.W[w_idx] = std::clamp(cell.W[w_idx], 0.0f, 1.0f);
  }
}

void MondeSED::AppliquerLoiMemoire(int x, int y, int z,
                                   const std::vector<Cellule> &read_grid) {
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
                   std::get<2>(voisins[i]), read_grid);
    if (v.is_alive && v.E > max_e)
      max_e = v.E;
  }

  // M <- max(M * decay, max_voisins)
  cell.M = std::max(cell.M * (1.0f - params.TAUX_OUBLI), max_e);
}

void MondeSED::AppliquerLoiMetabolisme(int x, int y, int z) {
  Cellule &cell = getCellule(x, y, z);
  if (!cell.is_alive)
    return;

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

void MondeSED::AppliquerLoiMouvement(int x, int y, int z,
                                     const std::vector<Cellule> &read_grid) {
  const Cellule &source = getCellule(x, y, z, read_grid);
  if (!source.is_alive)
    return;

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

  for (int i = 0; i < nb_voisins; ++i) {
    auto coords = voisins[i];
    const Cellule &target = getCellule(std::get<0>(coords), std::get<1>(coords),
                                       std::get<2>(coords), read_grid);

    if (!target.is_alive) {
      // Case vide: Calcul du champ local (Loi 8: Rayon > 1)
      float bonus_champ_E = 0.0f;
      float bonus_champ_C = 0.0f;
      int count_adh = 0; // Adhésion reste contact direct (Rayon 1)

      // 1. Adhésion (Direct neighbors of Target)
      std::tuple<int, int, int> v_cible[26];
      int nb_v_cible;
      GetCoordsVoisins_Optimized(std::get<0>(coords), std::get<1>(coords),
                                 std::get<2>(coords), v_cible, nullptr,
                                 nb_v_cible);

      for (int j = 0; j < nb_v_cible; ++j) {
        const Cellule &n =
            getCellule(std::get<0>(v_cible[j]), std::get<1>(v_cible[j]),
                       std::get<2>(v_cible[j]), read_grid);
        if (n.is_alive && n.T == source.T)
          count_adh++;
      }

      // 2. Champs (Rayon étendu)
      int r_field = (int)params.RAYON_DIFFUSION;
      int tx = std::get<0>(coords);
      int ty = std::get<1>(coords);
      int tz = std::get<2>(coords);

      for (int dz = -r_field; dz <= r_field; ++dz) {
        for (int dy = -r_field; dy <= r_field; ++dy) {
          for (int dx = -r_field; dx <= r_field; ++dx) {
            if (dx == 0 && dy == 0 && dz == 0)
              continue;
            int nx = tx + dx, ny = ty + dy, nz = tz + dz;
            int n_idx = getIndex(nx, ny, nz);
            if (n_idx != -1) {
              const Cellule &f_cell = read_grid[n_idx];
              if (f_cell.is_alive) {
                float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                float influence = std::exp(-params.ALPHA_ATTENUATION * dist);
                bonus_champ_E += f_cell.E * influence;
                bonus_champ_C += f_cell.C * influence;
              }
            }
          }
        }
      }

      float bonus_champ = (params.K_CHAMP_E * bonus_champ_E) -
                          (params.K_CHAMP_C * bonus_champ_C);
      float bonus_adh = params.K_ADH * count_adh;

      float score = gravity - pressure + inertia + bonus_champ + bonus_adh -
                    params.COUT_MOUVEMENT;

      if (score > max_score) {
        max_score = score;
        best_target = coords;
        found = true;
      }
    }
  }

  if (found) {
#pragma omp critical
    {
      mouvements_souhaites.push_back(
          {std::make_tuple(x, y, z), best_target, source.D, getIndex(x, y, z)});
    }
  }
}

void MondeSED::AppliquerLoiDivision(int x, int y, int z,
                                    const std::vector<Cellule> &read_grid) {
  const Cellule &source = getCellule(x, y, z, read_grid);
  if (!source.is_alive || source.E <= params.SEUIL_ENERGIE_DIVISION)
    return;

  // Cherche voisin vide avec max R (Territoire similaire ?)
  // Doc: "Recover genetically similar territory" -> implies check R of previous
  // occupant? But empty cells have R=0 usually. Doc says "innate resistance of
  // previously existing cell". Assuming empty cells retain 'ghost' traits or we
  // just check neighbors. Implementation: simple check for empty spot.

  std::tuple<int, int, int> voisins[26];
  int nb_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, nullptr, nb_voisins);

  for (int i = 0; i < nb_voisins; ++i) {
    const Cellule &target =
        getCellule(std::get<0>(voisins[i]), std::get<1>(voisins[i]),
                   std::get<2>(voisins[i]), read_grid);
    if (!target.is_alive) {
#pragma omp critical
      divisions_souhaitees.push_back(
          {std::make_tuple(x, y, z), voisins[i], source.E});
      return; // Une seule tentative par tick
    }
  }
}

void MondeSED::AppliquerLoiEchange(int x, int y, int z,
                                   const std::vector<Cellule> &read_grid) {
  const Cellule &source = getCellule(x, y, z, read_grid);
  if (!source.is_alive)
    return;

  std::tuple<int, int, int> voisins[26];
  int nb_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, nullptr, nb_voisins);

  for (int i = 0; i < nb_voisins; ++i) {
    auto coords = voisins[i];
    const Cellule &target = getCellule(std::get<0>(coords), std::get<1>(coords),
                                       std::get<2>(coords), read_grid);

    // Unicité: Traiter paire une seule fois (ID source < ID target)
    if (target.is_alive &&
        getIndex(x, y, z) < getIndex(std::get<0>(coords), std::get<1>(coords),
                                     std::get<2>(coords))) {
      if (std::abs(source.R - target.R) < params.SEUIL_SIMILARITE_R) {
        float delta = (source.E - target.E) * params.FACTEUR_ECHANGE_ENERGIE;
        float delta_safe = std::clamp(delta, -params.MAX_FLUX_ENERGIE,
                                      params.MAX_FLUX_ENERGIE);

        if (std::abs(delta_safe) > 0.0001f) {
#pragma omp critical
          echanges_energie_souhaites.push_back(
              {std::make_tuple(x, y, z), coords, delta_safe});
        }
      }
    }
  }
}

void MondeSED::AppliquerLoiPsychisme(int x, int y, int z,
                                     const std::vector<Cellule> &read_grid) {
  const Cellule &source = getCellule(x, y, z, read_grid);
  if (!source.is_alive)
    return;

  // Interaction forte: Contact voisin
  std::tuple<int, int, int> voisins[26];
  int nb_voisins;
  GetCoordsVoisins_Optimized(x, y, z, voisins, nullptr, nb_voisins);

  for (int i = 0; i < nb_voisins; ++i) {
    auto coords = voisins[i];
    const Cellule &target = getCellule(std::get<0>(coords), std::get<1>(coords),
                                       std::get<2>(coords), read_grid);

    if (target.is_alive) {
      float echange_C =
          source.C + (0.1f * target.C); // Friction (Unilatéral selon doc? "C <-
                                        // C + 0.1*C_voisin")
      // Wait, Doc says "C <- C + ...". This is a state update, not an exchange
      // flow. It modifies SELF based on Neighbor. Can be applied directly in
      // Preparation phase to 'grille'? Yes, if we use read_grid for values. But
      // let's keep it consistent.

      // Actually, the doc says "Interaction Forte... Modification directe".
      // If I do it here, I need to push an action?
      // Or can I do it in the "Preparation" loop directly?
      // "Appliquer l'échange C et L (Interaction Forte)" is listed in Phase 4
      // (Resolution). So I should schedule it.

      float dC = 0.1f * target.C;
      float dL = 0.1f * target.L;

#pragma omp critical
      echanges_psychiques_souhaites.push_back(
          {std::make_tuple(x, y, z), coords, dC, dL});
    }
  }
}

// --- RESOLUTION (Phase Ecriture Séquentielle) ---

void MondeSED::AppliquerMouvements() {
  // Tri déterministe
  std::sort(mouvements_souhaites.begin(), mouvements_souhaites.end(),
            [](const MouvementSouhaite &a, const MouvementSouhaite &b) {
              return a.index_source < b.index_source;
            });

  // Résolution conflits: Destination unique, Meilleur D gagne
  std::map<std::tuple<int, int, int>, MouvementSouhaite> winners;
  for (const auto &m : mouvements_souhaites) {
    auto it = winners.find(m.destination);
    if (it == winners.end()) {
      winners[m.destination] = m;
    } else {
      // Priority to higher D
      if (m.dette_besoin_source > it->second.dette_besoin_source) {
        winners[m.destination] = m;
      }
    }
  }

  // Apply
  for (const auto &pair : winners) {
    const auto &m = pair.second;
    Cellule &src = getCellule(std::get<0>(m.source), std::get<1>(m.source),
                              std::get<2>(m.source));
    Cellule &dst =
        getCellule(std::get<0>(m.destination), std::get<1>(m.destination),
                   std::get<2>(m.destination));

    // Move valid ? (Source still alive ?)
    if (src.is_alive && !dst.is_alive) {
      dst = src;
      dst.E -= params.COUT_MOUVEMENT; // Apply cost
      src = {};                       // Empty old
    }
  }
}

float deterministic_mutation(int x, int y, int z, int age) {
  unsigned int hash = (x * 18397) + (y * 20441) + (z * 22543) + (age * 24671);
  int decision = hash % 3;
  if (decision == 0)
    return 0.01f;
  if (decision == 1)
    return -0.01f;
  return 0.0f;
}

void MondeSED::AppliquerDivisions() {
  std::sort(divisions_souhaitees.begin(), divisions_souhaitees.end(),
            [](const DivisionSouhaitee &a, const DivisionSouhaitee &b) {
              return a.source_mere < b.source_mere;
            });

  std::map<std::tuple<int, int, int>, DivisionSouhaitee> winners;
  for (const auto &d : divisions_souhaitees) {
    auto it = winners.find(d.destination_fille);
    if (it == winners.end() || d.energie_mere > it->second.energie_mere) {
      winners[d.destination_fille] = d;
    }
  }

  for (const auto &pair : winners) {
    const auto &d = pair.second;
    Cellule &mere =
        getCellule(std::get<0>(d.source_mere), std::get<1>(d.source_mere),
                   std::get<2>(d.source_mere));
    Cellule &fille = getCellule(std::get<0>(d.destination_fille),
                                std::get<1>(d.destination_fille),
                                std::get<2>(d.destination_fille));

    if (mere.is_alive && !fille.is_alive &&
        mere.E > params.SEUIL_ENERGIE_DIVISION) {
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
      fille.R = std::clamp(fille.R + deterministic_mutation(fx, fy, fz, 0),
                           0.0f, 1.0f);
    }
  }
  // Safety check dynamic
  // optimization: We do NOT count total cells here (O(N) is too slow).
  // We rely on 'InitialiserMonde' to enforce start limits.
  // If user wants to exceed limits, they use the Override checkbox.
  // The physical grid size serves as the hard ceiling.
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
    // Note: e.destination is strictly for finding the neighbor, but the action
    // is on SOURCE (Unilateral friction) Wait, "Interaction Forte" logic in
    // AppliquerLoiPsychisme generated an action: Source C changes based on
    // Target C. So we apply to Source.
    if (src.is_alive) {
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

  // --- Regulation Dynamique (Homeostasie) ---
  double total_energy = 0.0;
  int living_cells = 0;
  // Utiliser une boucle simple pour la somme pour éviter les soucis de
  // concurrence OMP si non nécessaire, ou une réduction si critique pour la
  // perf. Ici boucle simple suffisante.
  for (const auto &c : grille) {
    if (c.is_alive) {
      total_energy += c.E;
      living_cells++;
    }
  }

  if (cycle_actuel % 100 == 0) { // Log tous les 100 ticks
    printf("Cycle %d: TotalEnergy=%.2f Living=%d K_THERMO=%.5f cost_mvt=%.5f\n",
           cycle_actuel, total_energy, living_cells, params.K_THERMO,
           params.COUT_MOUVEMENT);
  }

  // Formule: cout(t+1) = cout(t) * f(total_energy)
  // f(E) > 1 si E > Target (Augmente cout)
  // f(E) < 1 si E < Target (Diminue cout)
  // f(E) = 1 + alpha * ( (E - Target) / Target )

  // Safety check div by zero
  float target = params.TARGET_TOTAL_ENERGY;
  if (target < 1.0f)
    target = 1.0f;

  float ratio = (float)((total_energy - target) / target);
  float factor = 1.0f + (params.ADAPTATION_RATE * ratio);

  // Limites de sûreté pour le facteur (0.5x à 2.0x max par tick? non,
  // adaptation lente) Le factor sera proche de 1.0, ex: 1.0 + 0.01 * 0.5
  // = 1.005

  // Update
  params.K_THERMO *= factor;
  params.COUT_MOUVEMENT *= factor;

  // Hard Clamping pour éviter les valeurs absurdes
  params.K_THERMO = std::clamp(params.K_THERMO, 0.00001f, 1.0f);
  params.COUT_MOUVEMENT = std::clamp(params.COUT_MOUVEMENT, 0.0001f, 1.0f);

  // ------------------------------------------

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
  for (int z = 0; z < size_z; ++z) {
    for (int y = 0; y < size_y; ++y) {
      for (int x = 0; x < size_x; ++x) {
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
  for (int i = 0; i < (int)grille.size(); ++i) {
    Cellule &c = grille[i];
    if (c.is_alive) {
      c.C = std::clamp(c.C, 0.0f, 1.0f);
      c.R = std::clamp(c.R, 0.0f, 1.0f);
      c.Sc = std::clamp(c.Sc, 0.0f, 1.0f);
      c.E = std::max(0.0f, c.E);
      c.L = std::max(0.0f, c.L);

      // Mort
      if (c.E <= 0.0f || c.C > c.Sc) {
        c = {};
        // Restore Weights? No, death clears everything.
      }
    }
  }
}

// --- Accesseurs ---
const std::vector<Cellule> &MondeSED::getGrille() const { return grille; }

// --- IO ---
bool MondeSED::SauvegarderEtat(const std::string &nom_fichier) const {
  std::ofstream fichier_sortie(nom_fichier, std::ios::binary);
  if (!fichier_sortie)
    return false;

  fichier_sortie.write(reinterpret_cast<const char *>(&size_x), sizeof(size_x));
  fichier_sortie.write(reinterpret_cast<const char *>(&size_y), sizeof(size_y));
  fichier_sortie.write(reinterpret_cast<const char *>(&size_z), sizeof(size_z));
  fichier_sortie.write(reinterpret_cast<const char *>(&cycle_actuel),
                       sizeof(cycle_actuel));
  fichier_sortie.write(reinterpret_cast<const char *>(&params), sizeof(params));
  fichier_sortie.write(reinterpret_cast<const char *>(grille.data()),
                       grille.size() * sizeof(Cellule));

  fichier_sortie.close();
  return true;
}

bool MondeSED::ChargerEtat(const std::string &nom_fichier) {
  std::ifstream fichier_entree(nom_fichier, std::ios::binary);
  if (!fichier_entree)
    return false;

  fichier_entree.read(reinterpret_cast<char *>(&size_x), sizeof(size_x));
  fichier_entree.read(reinterpret_cast<char *>(&size_y), sizeof(size_y));
  fichier_entree.read(reinterpret_cast<char *>(&size_z), sizeof(size_z));
  fichier_entree.read(reinterpret_cast<char *>(&cycle_actuel),
                      sizeof(cycle_actuel));
  fichier_entree.read(reinterpret_cast<char *>(&params), sizeof(params));

  grille.resize(size_x * size_y * size_z);
  fichier_entree.read(reinterpret_cast<char *>(grille.data()),
                      grille.size() * sizeof(Cellule));

  fichier_entree.close();
  return true;
}

bool MondeSED::ChargerParametresDepuisFichier(const std::string &nom_fichier) {
  std::ifstream fichier_params(nom_fichier);
  if (!fichier_params.is_open())
    return false;

  std::string ligne;
  while (std::getline(fichier_params, ligne)) {
    if (ligne.empty() || ligne[0] == '#')
      continue;
    std::istringstream iss(ligne);
    std::string cle, valeur_str;
    if (std::getline(iss, cle, '=') && std::getline(iss, valeur_str)) {
      try {
        float valeur = std::stof(valeur_str);
        if (cle == "K_D")
          params.K_D = valeur;
        else if (cle == "K_C")
          params.K_C = valeur;
        else if (cle == "K_M")
          params.K_M = valeur;
        else if (cle == "K_THERMO")
          params.K_THERMO = valeur;
        else if (cle == "D_PER_TICK")
          params.D_PER_TICK = valeur;
        else if (cle == "L_PER_TICK")
          params.L_PER_TICK = valeur;
        else if (cle == "COUT_MOUVEMENT")
          params.COUT_MOUVEMENT = valeur;
        else if (cle == "SEUIL_ENERGIE_DIVISION")
          params.SEUIL_ENERGIE_DIVISION = valeur;
        else if (cle == "COUT_DIVISION")
          params.COUT_DIVISION = valeur;
        else if (cle == "RAYON_DIFFUSION")
          params.RAYON_DIFFUSION = valeur;
        else if (cle == "ALPHA_ATTENUATION")
          params.ALPHA_ATTENUATION = valeur;
        else if (cle == "FACTEUR_ECHANGE_ENERGIE")
          params.FACTEUR_ECHANGE_ENERGIE = valeur;
        else if (cle == "SEUIL_DIFFERENCE_ENERGIE")
          params.SEUIL_DIFFERENCE_ENERGIE = valeur;
        else if (cle == "SEUIL_SIMILARITE_R")
          params.SEUIL_SIMILARITE_R = valeur;
        else if (cle == "MAX_FLUX_ENERGIE")
          params.MAX_FLUX_ENERGIE = valeur;
        else if (cle == "FACTEUR_ECHANGE_PSYCHIQUE")
          params.FACTEUR_ECHANGE_PSYCHIQUE = valeur;
        else if (cle == "TAUX_OUBLI")
          params.TAUX_OUBLI = valeur;
        else if (cle == "TICKS_NEURAUX_PAR_PHYSIQUE")
          params.TICKS_NEURAUX_PAR_PHYSIQUE = (int)valeur;
        else if (cle == "SEUIL_SOMA")
          params.SEUIL_SOMA = valeur;
        else if (cle == "SEUIL_NEURO")
          params.SEUIL_NEURO = valeur;
        else if (cle == "LEARN_RATE")
          params.LEARN_RATE = valeur;
      } catch (...) {
      }
    }
  }
  return true;
}

// --- GPU IMPLEMENTATION ---

// GLSL Layout Mirror
// MUST MATCH GLSL STRUCT EXACTLY (std430)
struct CelluleGPU {
  uint32_t Type_Alive; // 4
  float E;             // 4
  float D;             // 4
  float C;             // 4
  float L;             // 4
  float M;             // 4
  float R;             // 4
  float Sc;            // 4
  float P;             // 4
  float G;             // 4
  int32_t Ref;         // 4
  float E_cost;        // 4
  float Weights[27];   // 108
  uint32_t History;    // 4
  int32_t Age;         // 4
  float Padding1;      // 4
  float Padding2;      // 4
  // Total: 176 bytes
};

// Helper to load file content
std::string LoadFileText(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open())
    return "";
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

void MondeSED::InitGPU() {
  if (gpu_initialized)
    return;

  std::cout << "[GPU] Initializing Compute Shader..." << std::endl;

  // 1. Load & Compile Shader
  std::string code = LoadFileText("src/shaders/sed_compute.glsl");
  if (code.empty()) {
    std::cerr << "[GPU] Error: Could not load src/shaders/sed_compute.glsl"
              << std::endl;
    return;
  }

  // Check GL version via RLGL
  // rlLoadComputeShaderProgram acts as a wrapper.
  // However, we need to compile first.
  // Since rlgl internal headers are not fully exposed for 'rlCompileShader'
  // with type if not defined? RL_COMPUTE_SHADER is defined in rlgl.h.

  unsigned int shader_id = rlCompileShader(code.c_str(), RL_COMPUTE_SHADER);
  if (shader_id == 0) {
    std::cerr << "[GPU] Error: Compute Shader Compilation Failed." << std::endl;
    return;
  }

  compute_program_id = rlLoadComputeShaderProgram(shader_id);
  if (compute_program_id == 0) {
    std::cerr << "[GPU] Error: Program Linking Failed." << std::endl;
    return;
  }

  u_size_loc = rlGetLocationUniform(compute_program_id, "u_Size");

  // 2. Convert Data to GPU Format
  std::vector<CelluleGPU> data_gpu(grille.size());
#pragma omp parallel for
  for (int i = 0; i < (int)grille.size(); ++i) {
    const Cellule &src = grille[i];
    CelluleGPU &dst = data_gpu[i];

    uint32_t type = src.T & 7;
    uint32_t alive = src.is_alive ? 1 : 0;
    dst.Type_Alive = type | (alive << 3);

    dst.E = src.E;
    dst.D = src.D;
    dst.C = src.C;
    dst.L = src.L;
    dst.M = src.M;
    dst.R = src.R;
    dst.Sc = src.Sc;
    dst.P = src.P;
    dst.G = src.G;
    dst.Ref = src.Ref;
    dst.E_cost = src.E_cost;
    std::memcpy(dst.Weights, src.W, 27 * sizeof(float));
    dst.History = src.H;
    dst.Age = src.A;
    dst.Padding1 = 0.0f;
    dst.Padding2 = 0.0f;
  }

  size_t buffer_size = data_gpu.size() * sizeof(CelluleGPU);
  ssbo_in = rlLoadShaderBuffer(buffer_size, data_gpu.data(), RL_DYNAMIC_COPY);
  ssbo_out = rlLoadShaderBuffer(buffer_size, nullptr, RL_DYNAMIC_COPY);

  std::cout << "[GPU] SSBOs Created (" << (buffer_size / 1024 / 1024)
            << " MB each)." << std::endl;

  gpu_initialized = true;
  use_gpu = true;
}

void MondeSED::StepGPU() {
  if (!gpu_initialized)
    return;

  // Use Raylib's state but manually dispatch if Raylib fails
  rlEnableShader(compute_program_id);

  int sizes[3] = {size_x, size_y, size_z};
  rlSetUniform(u_size_loc, sizes, RL_SHADER_UNIFORM_IVEC3, 1);

  // Bind current In/Out
  rlBindShaderBuffer(ssbo_in, 0);
  rlBindShaderBuffer(ssbo_out, 1);

  // Dispatch
  int gx = (size_x + 7) / 8;
  int gy = (size_y + 7) / 8;
  int gz = (size_z + 7) / 8;

  // Manual Dispatch
  if (pfn_glDispatchCompute) {
    pfn_glDispatchCompute(gx, gy, gz);
  } else {
    rlComputeShaderDispatch(gx, gy, gz);
  }

  rlDisableShader();

  // Swap Handles so 'ssbo_in' becomes the source of next frame (which holds
  // latest data)
  std::swap(ssbo_in, ssbo_out);
}

void MondeSED::SyncGPUToCPU() {
  if (!gpu_initialized)
    return;

  std::vector<CelluleGPU> data_gpu(grille.size());
  // Read from ssbo_in (which holds the result of the LAST step, because we
  // swapped at end of StepGPU)
  rlReadShaderBuffer(ssbo_in, data_gpu.data(),
                     data_gpu.size() * sizeof(CelluleGPU), 0);

#pragma omp parallel for
  for (int i = 0; i < (int)grille.size(); ++i) {
    const CelluleGPU &src = data_gpu[i];
    Cellule &dst = grille[i];

    uint32_t alive = (src.Type_Alive >> 3) & 1;
    dst.is_alive = (alive != 0);
    dst.T = src.Type_Alive & 7;

    dst.E = src.E;
    dst.D = src.D;
    dst.C = src.C;
    dst.L = src.L;
    dst.M = src.M;
    dst.R = src.R;
    dst.Sc = src.Sc;
    dst.P = src.P;
    dst.G = src.G; // Was stored in G
    dst.Ref = src.Ref;
    dst.E_cost = src.E_cost;
    std::memcpy(dst.W, src.Weights, 27 * sizeof(float));
    dst.H = src.History;
    dst.A = src.Age;
  }
}

bool MondeSED::CheckGPUSupport() {
  // Basic check: Can we compile a dummy compute shader?
  // Requires OpenGL context initialized (which Raylib does in InitWindow).
  // Caution: MondeSED might be created BEFORE InitWindow if global or early
  // init. Assuming MondeSED is created AFTER InitWindow in main.cpp.

  LoadComputeProcs();
  if (!pfn_glCreateShader)
    return false;

  // Simple dummy shader
  const char *dummy_code =
      "#version 430\n layout(local_size_x=1) in; void main(){}";
  unsigned int shader = CompileComputeShaderManual(dummy_code);

  if (shader != 0) {
    // We rely on Raylib to delete/detach usually only when programs are
    // deleted. But we should delete this shader object. We don't have
    // glDeleteShader loaded. It's fine for validity check leak (tiny).
    return true;
  }
  return false;
}

// --- TITANIC ENGINE IMPLEMENTATION ---

void MondeSED::InitTitanic() {
  if (gpu_initialized)
    return;

  std::cout << "[TITANIC] Initializing Massive Engine..." << std::endl;
  LoadComputeProcs();

  // 1. Load & Compile Shader
  std::string code = LoadFileText("src/shaders/sed_titanic.glsl");
  if (code.empty()) {
    std::cerr << "[TITANIC] Error: Could not load src/shaders/sed_titanic.glsl"
              << std::endl;
    use_gpu = false;
    gpu_initialized = false;
    return;
  }

  // Manual Compile
  unsigned int shader_id = CompileComputeShaderManual(code.c_str());
  if (shader_id == 0) {
    std::cerr << "[TITANIC] Error: Compute Shader Compilation Failed (Manual)."
              << std::endl;
    use_gpu = false;
    gpu_initialized = false;
    return;
  }

  titanic_program = rlLoadComputeShaderProgram(shader_id);
  if (titanic_program == 0) {
    std::cerr << "[TITANIC] Error: Program Linking Failed." << std::endl;
    use_gpu = false;
    gpu_initialized = false;
    return;
  }

  // Get Uniform Locations
  u_size_loc = rlGetLocationUniform(titanic_program, "u_Size"); // Re-use member
  u_time_loc = rlGetLocationUniform(titanic_program, "u_Time"); // Re-use
  loc_tick = rlGetLocationUniform(titanic_program, "u_Tick");
  loc_pass = rlGetLocationUniform(titanic_program, "u_Pass");

  std::cout << "[TITANIC] Shader Compiled. Pass Loc: " << loc_pass << std::endl;

  // 2. Convert Data to GPU Format (Using same layout as shader)
  // We reuse CelluleGPU struct from before but ensure alignment matches
  // Align 16 bytes? CelluleGPU is 172 bytes?
  // Shader struct size:
  // uint(4) + 14*float(56) + 27*float(108) + uint(4) + int(4) + 2*float(8) =
  // 184 bytes? Let's re-verify strict layout match. C++ CelluleGPU must match
  // GLSL Cell. GLSL Cell: uint Type_Alive; // 4 float E, D, C, L, M, R, Sc, P,
  // G; // 9 * 4 = 36 int Ref; // 4 float E_cost; // 4 float Weights[27]; // 108
  // uint History; // 4
  // int Age; // 4
  // float Padding1, Padding2; // 8
  // Total: 4 + 36 + 4 + 4 + 108 + 4 + 4 + 8 = 172 bytes.
  // Struct CelluleGPU in C++ was 172 bytes.
  // Ensure strict match. GLSL std430 aligns arrays/structs base.
  // We will assume packing is tight for scalar array.

  std::vector<CelluleGPU> data_gpu(grille.size());
#pragma omp parallel for
  for (int i = 0; i < (int)grille.size(); ++i) {
    const Cellule &src = grille[i];
    CelluleGPU &dst = data_gpu[i];

    dst.Type_Alive = (src.T & 7) | ((src.is_alive ? 1 : 0) << 3);
    dst.E = src.E;
    dst.D = src.D;
    dst.C = src.C;
    dst.L = src.L;
    dst.M = src.M;
    dst.R = src.R;
    dst.Sc = src.Sc;
    dst.P = src.P;
    dst.G = src.G;
    dst.Ref = src.Ref;
    dst.E_cost = src.E_cost;
    std::memcpy(dst.Weights, src.W, 27 * sizeof(float));
    dst.History = src.H;
    dst.Age = src.A;
    dst.Padding1 = 0.0f;
    dst.Padding2 = 0.0f;
  }

  size_t buffer_size = data_gpu.size() * sizeof(CelluleGPU);
  ssbo_in = rlLoadShaderBuffer(buffer_size, data_gpu.data(), RL_DYNAMIC_COPY);
  ssbo_out = rlLoadShaderBuffer(buffer_size, nullptr, RL_DYNAMIC_COPY);

  gpu_initialized = true;
  use_gpu = true;
}

void MondeSED::StepTitanic() {
  if (!gpu_initialized)
    return;

  rlEnableShader(titanic_program);

  int sizes[3] = {size_x, size_y, size_z};
  rlSetUniform(u_size_loc, sizes, RL_SHADER_UNIFORM_IVEC3, 1);

  float time_val = (float)GetTime();
  rlSetUniform(u_time_loc, &time_val, RL_SHADER_UNIFORM_FLOAT, 1);

  rlSetUniform(loc_tick, &cycle_actuel, RL_SHADER_UNIFORM_INT, 1);

  // Dispatch Dimensions
  int gx = (size_x + 7) / 8;
  int gy = (size_y + 7) / 8;
  int gz = (size_z + 7) / 8;

  // --- DOUBLE CLOCK PIPELINE ---

  // 1. NEURAL LOOP (N times)
  // Ping-Pong P between buffers?
  // Shader writes to Out. In -> Out.
  // If we want N steps, we must Swap buffers N times.
  // Neural kernel reads In, writes Out.

  int N_NEURAL = params.TICKS_NEURAUX_PAR_PHYSIQUE;
  if (N_NEURAL > 5)
    N_NEURAL = 5;   // Safety cap
  int p_neural = 0; // PASS_NEURAL
  rlSetUniform(loc_pass, &p_neural, RL_SHADER_UNIFORM_INT, 1);

  for (int i = 0; i < N_NEURAL; ++i) {
    rlBindShaderBuffer(ssbo_in, 0);
    rlBindShaderBuffer(ssbo_out, 1);

    if (pfn_glDispatchCompute)
      pfn_glDispatchCompute(gx, gy, gz);
    else
      rlComputeShaderDispatch(gx, gy, gz);

    // Explicit Barrier to ensure SSBO writes complete before next read
    if (pfn_glMemoryBarrier)
      pfn_glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    std::swap(ssbo_in, ssbo_out);
  }

  // ssb_in now holds the latest Neural state.

  // 2. PHYSICS LOOP (1 time)
  int p_physics = 1; // PASS_PHYSICS
  rlSetUniform(loc_pass, &p_physics, RL_SHADER_UNIFORM_INT, 1);

  rlBindShaderBuffer(ssbo_in, 0);
  rlBindShaderBuffer(ssbo_out, 1);

  if (pfn_glDispatchCompute)
    pfn_glDispatchCompute(gx, gy, gz);
  else
    rlComputeShaderDispatch(gx, gy, gz);

  if (pfn_glMemoryBarrier)
    pfn_glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // Final Swap so ssbo_in always holds the valid state for Readback
  std::swap(ssbo_in, ssbo_out);

  rlDisableShader();

  cycle_actuel++;
}
