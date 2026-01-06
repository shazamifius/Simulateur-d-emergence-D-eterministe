#ifndef ILOCALLAW_H
#define ILOCALLAW_H

#include "../SimulationData.h"
#include "ILaw.h"


/**
 * @interface ILocalLaw
 * @brief Interface for laws that modify a cell's state based on local
 * parameters and global constants. These laws are typically applied during the
 * update phase.
 */
class ILocalLaw : public ILaw {
public:
  /**
   * @brief Apply properties to the cell.
   * @param x X coordinate (context)
   * @param y Y coordinate (context)
   * @param z Z coordinate (context)
   * @param cell The cell to modify
   * @param params Global parameters
   */
  virtual void Apply(int x, int y, int z, Cellule &cell,
                     const ParametresGlobaux &params) = 0;
};

#endif // ILOCALLAW_H
