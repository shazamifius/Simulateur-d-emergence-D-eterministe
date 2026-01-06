#include "WorldMap.h"
#include <cmath>

WorldMap::WorldMap() {}

WorldMap::~WorldMap() { Clear(); }

void WorldMap::WorldToChunkCoords(int wx, int wy, int wz, int &cx, int &cy,
                                  int &cz, int &lx, int &ly, int &lz) {
  // Gestion des coordonnées négatives correcte (floor)
  // Ex ChunkSize=16.  x=15 -> cx=0, lx=15. x=-1 -> cx=-1, lx=15.

  // Méthode rapide avec floor division
  cx = (wx >= 0) ? (wx / CHUNK_SIZE) : ((wx - (CHUNK_SIZE - 1)) / CHUNK_SIZE);
  cy = (wy >= 0) ? (wy / CHUNK_SIZE) : ((wy - (CHUNK_SIZE - 1)) / CHUNK_SIZE);
  cz = (wz >= 0) ? (wz / CHUNK_SIZE) : ((wz - (CHUNK_SIZE - 1)) / CHUNK_SIZE);

  lx = wx - (cx * CHUNK_SIZE);
  ly = wy - (cy * CHUNK_SIZE);
  lz = wz - (cz * CHUNK_SIZE);
}

Chunk *WorldMap::GetChunk(int wc_x, int wc_y, int wc_z) {
  // Note: wc_x here assumes Chunk Coordinates input?
  // Wait, the header said "Accès Global" but taking 3 ints.
  // Let's assume input is CHUNK COORDINATES for this specific method
  // seeing GetOrCreateChunk likely takes World Coords?
  // No, strictly following signature:
  // If I pass World Coords to GetChunk, I expect it to find the chunk
  // CONTAINING that world coord.

  int cx, cy, cz, lx, ly, lz;
  WorldToChunkCoords(wc_x, wc_y, wc_z, cx, cy, cz, lx, ly, lz);

  auto it = chunks.find({cx, cy, cz});
  if (it != chunks.end()) {
    return it->second.get();
  }
  return nullptr;
}

Chunk *WorldMap::GetOrCreateChunk(int wc_x, int wc_y, int wc_z) {
  int cx, cy, cz, lx, ly, lz;
  WorldToChunkCoords(wc_x, wc_y, wc_z, cx, cy, cz, lx, ly, lz);

  ChunkKey key = {cx, cy, cz};
  auto it = chunks.find(key);

  if (it != chunks.end()) {
    return it->second.get();
  }

  // Create new
  auto new_chunk = std::make_unique<Chunk>(cx, cy, cz);
  Chunk *ptr = new_chunk.get();
  chunks[key] = std::move(new_chunk);
  return ptr;
}

Cellule *WorldMap::GetCellGlobal(int x, int y, int z) {
  Chunk *chk = GetChunk(x, y, z);
  if (!chk)
    return nullptr;

  int cx, cy, cz, lx, ly, lz;
  WorldToChunkCoords(x, y, z, cx, cy, cz, lx, ly, lz);

  return &chk->GetCell(lx, ly, lz);
}

const std::unordered_map<ChunkKey, std::unique_ptr<Chunk>, ChunkKeyHash> &
WorldMap::GetAllChunks() const {
  return chunks;
}

void WorldMap::Clear() { chunks.clear(); }
