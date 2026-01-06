#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "../include/laws/ILaw.h"
#include <map>
#include <memory>
#include <string>
#include <vector>


// Opaque handle for OS specific library handle
using LibHandle = void *;

struct LoadedPlugin {
  std::string path;
  LibHandle handle;
  ILaw *law_instance;
  // Function pointers
  void (*destroy_func)(ILaw *);
};

class PluginManager {
public:
  PluginManager();
  ~PluginManager();

  // Load a single plugin by path
  bool LoadPlugin(const std::string &path);

  // Unload all plugins
  void UnloadAll();

  // Get all loaded laws (to give to MondeSED)
  const std::vector<ILaw *> &GetLoadedLaws() const;

private:
  std::vector<LoadedPlugin> plugins;
  std::vector<ILaw *> loaded_laws; // Helper cache
};

#endif // PLUGIN_MANAGER_H
