#ifndef WORLDMAP_H
#define WORLDMAP_H

#include "Chunk.h"
#include <memory>
#include <tuple>
#include <unordered_map>


// Clé de hachage pour les coordonnées de Chunk (x, y, z)
struct ChunkKey {
  int x, y, z;

  bool operator==(const ChunkKey &other) const {
    return x == other.x && y == other.y && z == other.z;
  }
};

// Fonction de hachage custom pour unordered_map
struct ChunkKeyHash {
  std::size_t operator()(const ChunkKey &k) const {
    // Hachage simple avec nombres premiers pour éviter les collisions
    return ((std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1)) >> 1) ^
           (std::hash<int>()(k.z) << 1);
  }
};

class WorldMap {
public:
  WorldMap();
  ~WorldMap();

  // Accès Global (World Coordinates)
  Chunk *GetChunk(int wc_x, int wc_y, int wc_z);

  // Création / Récupération (Auto-create if missing)
  Chunk *GetOrCreateChunk(int wc_x, int wc_y, int wc_z);

  // Accès Cellule Global (Helper majeur pour la transition)
  // Retourne nullptr si le chunk n'existe pas
  Cellule *GetCellGlobal(int x, int y, int z);

  // Accès à toutes les données (pour le Rendu / Itération)
  const std::unordered_map<ChunkKey, std::unique_ptr<Chunk>, ChunkKeyHash> &
  GetAllChunks() const;

  // Gestion
  void Clear();
  size_t GetChunkCount() const { return chunks.size(); }

private:
  std::unordered_map<ChunkKey, std::unique_ptr<Chunk>, ChunkKeyHash> chunks;

  // Conversion coord global -> coord chunk
  // Ex: x=17 -> chunk_x=1, local_x=1
  static void WorldToChunkCoords(int wx, int wy, int wz, int &cx, int &cy,
                                 int &cz, int &lx, int &ly, int &lz);
};

#endif // WORLDMAP_H
