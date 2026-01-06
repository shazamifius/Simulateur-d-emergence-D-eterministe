#pragma once

#include "InputSystem.h" // Add this include
#include "MondeSED.h"
#include "imgui.h"
#include "raylib.h"

class Interface {
public:
  static void SetupStyle();
  static void DrawLayout(MondeSED *monde, Camera3D &camera,
                         RenderTexture2D &viewTexture, bool &simulation_running,
                         int &sim_cycles_per_frame, int sim_size[3],
                         float &sim_density, bool &use_random_seed,
                         int &sim_seed, bool &reset_requested,
                         bool &enable_max_speed, int &target_tps);

  // Set the InputSystem to record to
  static void SetInputSystem(InputSystem *sys) { inputSystem = sys; }

private:
  static InputSystem *inputSystem; // Add static member

  static void DrawMainMenuBar(MondeSED *monde, bool &simulation_running,
                              bool &reset_requested);
  static void DrawPropertiesPanel(MondeSED *monde, bool &simulation_running,
                                  int &sim_cycles, int sim_size[3],
                                  float &density, bool &use_rnd, int &seed,
                                  bool &reset_requested, bool &enable_max_speed,
                                  int &target_tps);
  static void DrawInspectorPanel(MondeSED *monde);
  static void DrawStatsPanel(MondeSED *monde);
  static void DrawViewport(Camera3D &camera, RenderTexture2D &viewTexture,
                           MondeSED *monde);

  // Helpers
  static void DrawFuturisticHeader(const char *label);
  static void DrawStatCard(const char *label, const char *value, Color c);
};
