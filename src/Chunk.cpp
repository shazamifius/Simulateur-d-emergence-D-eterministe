#include "Chunk.h"
#include <algorithm>

Chunk::Chunk(int x, int y, int z) : cx(x), cy(y), cz(z) {
  // Allocation mémoire
  cells.resize(CHUNK_VOLUME);
  // Initialisation par défaut (Case vide / morte)
  std::fill(cells.begin(), cells.end(), Cellule{});
}

Cellule &Chunk::GetCell(int local_x, int local_y, int local_z) {
  // Pas de boundary check ici pour la perf (supposé fait par l'appelant)
  return cells[GetIndex(local_x, local_y, local_z)];
}

const Cellule &Chunk::GetCell(int local_x, int local_y, int local_z) const {
  return cells[GetIndex(local_x, local_y, local_z)];
}
