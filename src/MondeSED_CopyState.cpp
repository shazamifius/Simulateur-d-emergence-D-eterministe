
void MondeSED::CopyStateFrom(const MondeSED &other) {
  // Copy minimal state required for rendering
  if (size_x != other.size_x || size_y != other.size_y ||
      size_z != other.size_z) {
    size_x = other.size_x;
    size_y = other.size_y;
    size_z = other.size_z;
    grille.resize(size_x * size_y * size_z);
  }

  // Fast Copy
  params = other.params;
  cycle_actuel = other.cycle_actuel;
  current_seed = other.current_seed;
  barycentre_x = other.barycentre_x;
  barycentre_y = other.barycentre_y;
  barycentre_z = other.barycentre_z;

  // Vector Copy (Most Expensive part)
  grille = other.grille;
}
