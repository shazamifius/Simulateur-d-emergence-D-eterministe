#include "../../include/laws/LoiMetabolisme.h"

void LoiMetabolisme::Apply(int x, int y, int z, Cellule &cell,
                           const ParametresGlobaux &params) {
  if (!cell.is_alive)
    return;

  // Loi 7
  cell.D += params.D_PER_TICK;
  cell.L += params.L_PER_TICK;

  // --- PHOTOSYNTHESE (SOLEIL) ---
  // Si hauteur suffisante et pas Neurone (Soma ou Souche)
  if (cell.T != 2) {
    float height_threshold = params.WORLD_HEIGHT * params.HAUTEUR_SOLEIL;
    if (y >= height_threshold) {
      cell.E += params.SENSIBILITE_SOLEIL;
    }
  }

  // Coût existence + Coût neuronal accumulé
  float total_cost = params.K_THERMO + cell.E_cost;
  cell.E -= total_cost;
  cell.E_cost = 0.0f; // Reset accumulateur

  cell.A++; // Vieillissement
}
