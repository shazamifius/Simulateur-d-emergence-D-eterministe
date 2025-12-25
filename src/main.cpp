// --- Fichiers d'en-tête ---
#include "Interface.h"
#include "MondeSED.h"
#include "Renderer.h"
#include "imgui.h"
#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// --- Variables globales ---
std::unique_ptr<MondeSED> monde;
bool simulation_running = false;
std::vector<float> cell_count_history;

// --- Config UI ---
static int sim_size[3] = {32, 32, 32};
static float sim_density = 0.15f;
static int sim_cycles_per_frame = 1;
static int sim_seed = 12345;
static bool use_random_seed = true;

// --- État de Sélection ---
int selected_cell_coords[3] = {-1, -1, -1};
bool cell_is_selected = false;

// --- Viewport State ---
RenderTexture2D viewTexture;
bool viewTextureInitialized = false;

// --- Fonctions ---
void ResetSimulation();

int main() {
  int screenWidth = 1600;
  int screenHeight = 900;
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT |
                 FLAG_WINDOW_HIGHDPI);
  InitWindow(screenWidth, screenHeight,
             "SED V8.0 - DETERMINISTIC EMERGENCE SIMULATOR");
  SetTargetFPS(60);

  rlImGuiSetup(true);
  Renderer::Init();
  Interface::SetupStyle(); // Apply the new futuristic theme

  ResetSimulation();

  Camera3D camera = {0};
  camera.position =
      Vector3{(float)sim_size[0] * 1.5f, (float)sim_size[1] * 1.5f,
              (float)sim_size[0] * 1.5f};
  camera.target = Vector3{(float)sim_size[0] / 2, (float)sim_size[1] / 2,
                          (float)sim_size[2] / 2};
  camera.up = Vector3{0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  while (!WindowShouldClose()) {
    // --- Update Logic ---
    if (simulation_running && monde) {
      // Simulation Step
      for (int i = 0; i < sim_cycles_per_frame; ++i) {
        if (monde->use_gpu) {
          // GPU Mode: Init & Dispatch
          monde->InitTitanic();
          monde->StepTitanic();
        } else {
          // CPU Mode (Legacy/Fallback)
          monde->AvancerTemps();
        }

        // Stats
        cell_count_history.push_back(
            static_cast<float>(monde->getNombreCellulesVivantes()));
        if (cell_count_history.size() > 1000) {
          cell_count_history.erase(cell_count_history.begin());
        }
      }

      // --- Render Logic ---

      // Ensure Texture logic (Size check handled in Interface or here?
      // Best handled here to feed Renderer)
      int width = GetScreenWidth();
      int height = GetScreenHeight();
      // Approximate viewport size logic from Interface to prevent resizing lag?
      // Actually Interface::DrawLayout calculates viewport size.
      // We can just rely on the existing texture or resize it if too
      // small/different. For simplicity, let's keep the texture logic inside
      // Interface or Renderer? The previous main had logic to resize
      // viewTexture. Let's pass the texture to Interface, and Interface can
      // resize it if needed? Wait, Interface::DrawViewport has resizing logic.
      // But Renderer needs to draw TO it before Interface draws. So main loop:
      // 1. Check texture size (approximate or use last frame's size
      // requirement?)
      //    We'll set an arbitrary size for the first frame or use Screen size.
      //    Actually Interface can return the desired viewport size?
      //    Or we just resize if needed at the start of frame.
      //    Let's use a fixed size logic for now relative to screen to avoid
      //    1-frame lag.

      // Simplified: Just use full screen render target or large enough.
      // Efficiency: Only render to what we see.
      // Let's stick to the flow:
      // 1. Interface::DrawLayout calls DrawViewport which updates `viewRect`.
      //    Problem: ImGui is drawn AFTER Rendering usually.
      //    But we need to Render to texture first to display it in ImGui.
      //    So we use the texture from LAST frame (or invalid first frame).
      //    Then we Render for NEXT frame? No.
      //    Standard flow:
      //    Renderer::Render(texture)
      //    BeginDrawing
      //    rlImGuiBegin
      //    Interface::DrawLayout(texture) -> displays texture
      //    rlImGuiEnd
      //    EndDrawing
      //    This is fine. Texture might be 1 frame resized lagging, but that's
      //    standard for immediate GUIs.

      // Resize logic
      // Resize Board Logic (Responsive)
      int neededW = width - 620;  // Sidebar offset
      int neededH = height - 250; // Footer offset

      // Safety clamps
      if (neededW < 100)
        neededW = 100;
      if (neededH < 100)
        neededH = 100;

      if (!viewTextureInitialized || viewTexture.texture.width != neededW ||
          viewTexture.texture.height != neededH) {
        if (viewTextureInitialized)
          UnloadRenderTexture(viewTexture);
        viewTexture = LoadRenderTexture(neededW, neededH);
        viewTextureInitialized = true;
      }

      // --- Blender Style Camera Control ---
      // Orbit: Middle Mouse
      // Pan: Shift + Middle Mouse
      // Zoom: Scroll Wheel

      Vector2 mouseDelta = GetMouseDelta();
      float wheel = GetMouseWheelMove();

      // 1. Zoom (Scroll)
      if (wheel != 0) {
        // Move position towards target
        Vector3 forward = Vector3Subtract(camera.target, camera.position);
        float dist = Vector3Length(forward);
        float zoomFactor = 0.1f * dist; // Proportional zoom

        Vector3 move =
            Vector3Scale(Vector3Normalize(forward), wheel * zoomFactor);
        // Clamp min distance
        if (wheel > 0 && dist < 1.0f) { /* Limit zoom in */
        } else {
          camera.position = Vector3Add(camera.position, move);
        }
      }

      // 2. Pan (Shift + Middle)
      if ((IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || IsKeyDown(KEY_LEFT_ALT)) &&
          IsKeyDown(KEY_LEFT_SHIFT)) {
        // Move BOTH Position and Target relative to camera plane
        Vector3 forward =
            Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        Vector3 right = Vector3CrossProduct(forward, camera.up);
        Vector3 upReal = Vector3CrossProduct(right, forward);

        float panSpeed = 0.05f *
                         Vector3Distance(camera.position, camera.target) *
                         0.05f; // Scale with distance

        Vector3 panRight = Vector3Scale(right, -mouseDelta.x * panSpeed);
        Vector3 panUp = Vector3Scale(upReal, mouseDelta.y * panSpeed);

        Vector3 panTotal = Vector3Add(panRight, panUp);

        camera.position = Vector3Add(camera.position, panTotal);
        camera.target = Vector3Add(camera.target, panTotal);
      }
      // 3. Orbit (Middle only)
      else if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) ||
               IsKeyDown(KEY_LEFT_ALT)) {
        // Rotate Position around Target
        Vector3 vector = Vector3Subtract(camera.position, camera.target);

        // Yaw (Global Up)
        float angleX = -mouseDelta.x * 0.005f;
        vector = Vector3RotateByAxisAngle(vector, Vector3{0, 1, 0}, angleX);

        // Pitch (Local Right)
        Vector3 right =
            Vector3CrossProduct(Vector3Normalize(vector), Vector3{0, 1, 0});
        // Handle Gimbal Lock singularity if vector is roughly up
        if (Vector3Length(right) < 0.001f) {
          right = Vector3{1, 0, 0};
        }

        float angleY = -mouseDelta.y * 0.005f;
        vector = Vector3RotateByAxisAngle(vector, right, angleY);

        camera.position = Vector3Add(camera.target, vector);
      }

      // Casting / Selection
      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
          !ImGui::GetIO().WantCaptureMouse) {
        // Basic selection logic to be implemented
      }

      // RENDER 3D SCENE is now handled inside Interface::DrawLayout ->
      // DrawViewport to ensure synchronization with UI sizing.

      // DRAW UI
      BeginDrawing();
      ClearBackground(Color{20, 20, 22, 255}); // Match UI BG

      rlImGuiBegin();
      static bool reset_requested =
          false; // Static to persist if needed or just
                 // local? Local is fine per frame.
      // Actually needs to be local to the loop iteration, reset to false each
      // frame? No, DrawLayout sets it to true if clicked.
      bool frame_reset_req = false;

      // Pass the texture we just rendered
      Interface::DrawLayout(monde.get(), camera, viewTexture,
                            simulation_running, sim_cycles_per_frame, sim_size,
                            sim_density, use_random_seed, sim_seed,
                            frame_reset_req);

      if (frame_reset_req) {
        ResetSimulation();
        // Reset camera focus to center of new world?
        camera.target = Vector3{(float)sim_size[0] / 2, (float)sim_size[1] / 2,
                                (float)sim_size[2] / 2};
        camera.position =
            Vector3{(float)sim_size[0] * 1.5f, (float)sim_size[1] * 1.5f,
                    (float)sim_size[0] * 1.5f};
      }

      rlImGuiEnd();

      EndDrawing();
    } // Closes `if (simulation_running && monde)`
  } // Closes `while (!WindowShouldClose())`

  if (viewTextureInitialized)
    UnloadRenderTexture(viewTexture);
  Renderer::Unload();
  rlImGuiShutdown();
  CloseWindow();

  return 0;
} // Closes the main function

void ResetSimulation() {
  if (use_random_seed) {
    sim_seed = static_cast<int>(time(NULL));
  }

  // Safety: Clamp dimensions
  sim_size[0] = std::clamp(sim_size[0], 10, 500);
  sim_size[1] = std::clamp(sim_size[1], 10, 500);
  sim_size[2] = std::clamp(sim_size[2], 10, 500);

  monde = std::make_unique<MondeSED>(sim_size[0], sim_size[1], sim_size[2]);
  monde->InitialiserMonde(static_cast<unsigned int>(sim_seed), sim_density);
  simulation_running = false;
  cell_count_history.clear();
}
