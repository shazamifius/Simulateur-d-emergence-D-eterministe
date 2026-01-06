#include "plugins/PluginManager.h"
#include "plugins/PluginAPI.h"
#include <iostream>

#if defined(_WIN32)
#include <windows.h>
std::string GetLastErrorString() {
  DWORD error = GetLastError();
  if (error == 0)
    return "No error";
  LPSTR messageBuffer = nullptr;
  size_t size = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPSTR)&messageBuffer, 0, NULL);
  std::string message(messageBuffer, size);
  LocalFree(messageBuffer);
  return message;
}
#else
#include <dlfcn.h>
#endif

PluginManager::PluginManager() {}

PluginManager::~PluginManager() { UnloadAll(); }

bool PluginManager::LoadPlugin(const std::string &path) {
  LibHandle handle = nullptr;

  std::cout << "[PluginManager] Loading: " << path << std::endl;

#if defined(_WIN32)
  handle = LoadLibraryA(path.c_str());
  if (!handle) {
    std::cerr << "[PluginManager] Failed to load DLL: " << path
              << " Error: " << GetLastErrorString() << std::endl;
    return false;
  }
#else
  handle = dlopen(path.c_str(), RTLD_NOW);
  if (!handle) {
    std::cerr << "[PluginManager] Failed to load SO: " << dlerror()
              << std::endl;
    return false;
  }
#endif

  // Load Symbols
  CreateLawFunc create_law = nullptr;
  DestroyLawFunc destroy_law = nullptr;

#if defined(_WIN32)
  create_law = (CreateLawFunc)GetProcAddress((HMODULE)handle, "CreateLaw");
  destroy_law = (DestroyLawFunc)GetProcAddress((HMODULE)handle, "DestroyLaw");
#else
  create_law = (CreateLawFunc)dlsym(handle, "CreateLaw");
  destroy_law = (DestroyLawFunc)dlsym(handle, "DestroyLaw");
#endif

  if (!create_law) {
    std::cerr << "[PluginManager] Missing 'CreateLaw' symbol in " << path
              << std::endl;
    // Cleanup
#if defined(_WIN32)
    FreeLibrary((HMODULE)handle);
#else
    dlclose(handle);
#endif
    return false;
  }

  // Create Instance
  ILaw *law = create_law();
  if (!law) {
    std::cerr << "[PluginManager] CreateLaw returned null." << std::endl;
    // Cleanup
#if defined(_WIN32)
    FreeLibrary((HMODULE)handle);
#else
    dlclose(handle);
#endif
    return false;
  }

  std::cout << "[PluginManager] Successfully loaded law: " << law->GetName()
            << std::endl;

  LoadedPlugin p;
  p.path = path;
  p.handle = handle;
  p.law_instance = law;
  p.destroy_func = destroy_law;

  plugins.push_back(p);
  loaded_laws.push_back(law);

  return true;
}

void PluginManager::UnloadAll() {
  for (auto &p : plugins) {
    if (p.law_instance && p.destroy_func) {
      p.destroy_func(p.law_instance);
    } else if (p.law_instance) {
      // If no destroy func provided, we can try delete, but it's dangerous
      // across DLL heaps usually. Ideally we always export a Destroy function.
      // For now, warning.
      std::cerr << "[WARN] Plugin " << p.path
                << " had no DestroyLaw symbol. Memory leak potential."
                << std::endl;
    }

    if (p.handle) {
#if defined(_WIN32)
      FreeLibrary((HMODULE)p.handle);
#else
      dlclose(p.handle);
#endif
    }
  }
  plugins.clear();
  loaded_laws.clear();
}

const std::vector<ILaw *> &PluginManager::GetLoadedLaws() const {
  return loaded_laws;
}
