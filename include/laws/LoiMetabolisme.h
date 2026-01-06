#ifndef LOI_METABOLISME_H
#define LOI_METABOLISME_H

#include "ILocalLaw.h"

class LoiMetabolisme : public ILocalLaw {
public:
  std::string GetName() const override {
    return "Loi Thermodynamique (Metabolisme)";
  }

  void Apply(int x, int y, int z, Cellule &cell,
             const ParametresGlobaux &params) override;
};

#endif // LOI_METABOLISME_H
