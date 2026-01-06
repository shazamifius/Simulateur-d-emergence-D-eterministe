#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#if defined(_WIN32)
#ifdef EXPORT_PLUGIN
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __declspec(dllimport)
#endif
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif

#include "../laws/ILaw.h"

// Factory function signature meant to be exported by the plugin
// It should return a pointer to a new instance of the law.
typedef ILaw *(*CreateLawFunc)();

// Function to clean up the law (since it was allocated inside the DLL's heap)
typedef void (*DestroyLawFunc)(ILaw *);

#endif // PLUGIN_API_H
