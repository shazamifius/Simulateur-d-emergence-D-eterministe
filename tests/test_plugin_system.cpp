#include "laws/ILocalLaw.h"
#include "plugins/PluginManager.h"
#include <cassert>
#include <filesystem>
#include <iostream>

// Simple mock for testing without full simulation
int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: test_plugin_system <path_to_plugin_dll>" << std::endl;
    return 1;
  }

  std::string plugin_path = argv[1];
  std::cout << "Testing Plugin Loading: " << plugin_path << std::endl;

  PluginManager mgr;
  if (!mgr.LoadPlugin(plugin_path)) {
    std::cerr << "Failed to load plugin." << std::endl;
    return 1;
  }

  const auto &laws = mgr.GetLoadedLaws();
  if (laws.empty()) {
    std::cerr << "No laws found in plugin." << std::endl;
    return 1;
  }

  ILaw *law = laws[0];
  std::cout << "Loaded Law Name: " << law->GetName() << std::endl;

  // Verify it is a Healing Law
  if (law->GetName().find("Healing") == std::string::npos) {
    std::cerr << "Unexpected law name." << std::endl;
    return 1;
  }

  // Test execution (cast to LocalLaw)
  ILocalLaw *local_law = dynamic_cast<ILocalLaw *>(law);
  if (local_law) {
    Cellule c;
    c.is_alive = true;
    c.E = 0.1f;
    ParametresGlobaux p;

    local_law->Apply(0, 0, 0, c, p);

    // Healing Law adds 0.005
    if (c.E > 0.104f) {
      std::cout << "Law execution verified. Energy: " << c.E << std::endl;
    } else {
      std::cerr << "Law did not modify cell as expected. E=" << c.E
                << std::endl;
      return 1;
    }
  } else {
    std::cout << "Law is not an ILocalLaw, skipping execution test."
              << std::endl;
  }

  std::cout << "Plugin System Test PASSED" << std::endl;
  return 0;
}
