#include "laws/ILocalLaw.h"
#include "plugins/PluginAPI.h"
#include <iostream>

class HealingLaw : public ILocalLaw {
public:
  std::string GetName() const override { return "Plugin: Healing Law"; }

  void Apply(int x, int y, int z, Cellule &cell,
             const ParametresGlobaux &params) override {
    // Simple mechanic: If cell is alive and low on energy, small regen
    // (photosynthesis?)
    if (cell.is_alive && cell.E < 0.5f) {
      cell.E += 0.005f;
    }
  }
};

extern "C" {
PLUGIN_API ILaw *CreateLaw() { return new HealingLaw(); }

PLUGIN_API void DestroyLaw(ILaw *law) { delete law; }
}
