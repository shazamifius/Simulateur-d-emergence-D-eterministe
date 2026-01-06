#pragma once
#include "MondeSED.h"
#include "raylib.h"

struct RenderSettings {
  bool show_cells = true;
  bool show_network = false;  // Synapses
  bool show_fields = false;   // Force Fields
  bool show_gizmos = true;    // Grid/Bounds
  float field_density = 0.5f; // Optimization for fields
  int cell_color_mode = 0;    // 0=Default, 1=Energy, 2=Stress, 3=Gradient
};

class Renderer {
public:
  static void Init();
  static void Unload();
  static void Render(MondeSED *monde, Camera3D &camera, RenderTexture2D &target,
                     int selectedCoords[3], const RenderSettings &settings);

private:
  static void DrawTechGrid(int slices, float spacing);
  static void DrawCells(MondeSED *monde, int selectedCoords[3], int colorMode,
                        Vector3 camPos);
  static void DrawNetwork(MondeSED *monde);
  static void DrawFieldVectors(MondeSED *monde, float density);
};
