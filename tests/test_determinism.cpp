#include "../include/MondeSED.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>

// Helper to compute a simple hash of the grid state
size_t compute_grid_hash(const std::vector<Cellule> &grid) {
  size_t hash = 0;
  for (const auto &cell : grid) {
    if (cell.is_alive) {
      // Combine fields into hash
      size_t cell_hash = 0;
      auto hash_combine = [&](float v) {
        // primitive hash combine
        std::hash<float> hasher;
        cell_hash ^=
            hasher(v) + 0x9e3779b9 + (cell_hash << 6) + (cell_hash >> 2);
      };
      hash_combine(cell.E);
      hash_combine(cell.C);
      hash_combine(cell.R);
      hash_combine(cell.D);
      hash_combine((float)cell.A);

      hash ^= cell_hash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
  }
  return hash;
}

int main() {
  std::cout << "Starting Determinism Verification..." << std::endl;

  int cycles = 50;
  unsigned int seed = 42;
  int size = 20; // Small world for speed

  // --- Run 1 ---
  std::cout << "Run 1: Init with seed " << seed << "..." << std::endl;
  MondeSED monde1(size, size, size);
  monde1.InitialiserMonde(seed, 0.5f);

  for (int i = 0; i < cycles; ++i) {
    monde1.AvancerTemps();
  }
  size_t hash1 = monde1.CalculateStateHash();
  std::cout << "Run 1 Finished. Hash: " << hash1 << std::endl;

  // --- Run 2 ---
  std::cout << "Run 2: Init with seed " << seed << "..." << std::endl;
  MondeSED monde2(size, size, size);
  monde2.InitialiserMonde(seed, 0.5f);

  for (int i = 0; i < cycles; ++i) {
    monde2.AvancerTemps();
  }
  size_t hash2 = monde2.CalculateStateHash();
  std::cout << "Run 2 Finished. Hash: " << hash2 << std::endl;

  // --- Comparison ---
  if (hash1 == hash2) {
    std::cout << "SUCCESS: Hashes match. Determinism verified." << std::endl;
    return 0;
  } else {
    std::cerr << "FAILURE: Hashes do not match. Determinism broken."
              << std::endl;
    return 1;
  }
}
