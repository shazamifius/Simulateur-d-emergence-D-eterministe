#ifndef CHUNK_H
#define CHUNK_H

#include "SimulationData.h"
#include <array>
#include <vector>

// Dimensions standard d'un Chunk (16x16x16 = 4096 cellules)
constexpr int CHUNK_SIZE = 16;
constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

class Chunk {
public:
  Chunk(int cx, int cy, int cz);
  ~Chunk() = default;

  // Accès local (0..15)
  Cellule &GetCell(int local_x, int local_y, int local_z);
  const Cellule &GetCell(int local_x, int local_y, int local_z) const;

  // Coordonnées du Chunk dans le monde (ex: 0, 0, 0 ou -1, 2, 5)
  int cx, cy, cz;

  // Données (Flat vector pour compatibilité cache)
  std::vector<Cellule> cells;

  // État
  bool is_active = false;       // Si faux, peut être désalloué ou ignoré
  bool has_alive_cells = false; // Optimization flag

  // Helpers
  static int GetIndex(int lx, int ly, int lz) {
    return lx + (ly * CHUNK_SIZE) + (lz * CHUNK_SIZE * CHUNK_SIZE);
  }
};

#endif // CHUNK_H
