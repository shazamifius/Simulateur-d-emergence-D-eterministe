// --- Fichiers d'en-tête ---
#include "Interface.h"
#include "MondeSED.h"
#include "Renderer.h"
#include "imgui.h"
#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// --- Variables globales (Thread Safe) ---
// Sim World: Owned by Sim Thread (mostly)
std::unique_ptr<MondeSED> sim_world;
// Render World: Owned by Main Thread, Shared during Sync
std::unique_ptr<MondeSED> render_world;

std::mutex sync_mutex;
std::atomic<bool> app_exit = false;
std::atomic<bool> simulation_running = false;
std::atomic<bool> reset_requested_atomic = false;

// Shared Settings
std::atomic<int> shared_sim_size_x = 32;
std::atomic<int> shared_sim_size_y = 32;
std::atomic<int> shared_sim_size_z = 32;
std::atomic<float> shared_density = 0.15f;
std::atomic<int> shared_seed = 12345;
std::atomic<int> shared_sim_cycles = 1;

// Speed Control
std::atomic<bool> shared_enable_max_speed = false;
std::atomic<int> shared_target_tps = 60;

// Input System
InputSystem inputSystem;
std::string replay_file = "";
std::string record_file = "last_run.json";

std::vector<float> cell_count_history; // Protected by sync_mutex effectively

// --- Config UI ---
// Local copies for UI
static int sim_size[3] = {32, 32, 32};
static float sim_density = 0.15f;
static int sim_cycles_per_frame = 1;
static int sim_seed = 12345;
static bool use_random_seed = true;
// Speed Control (Local)
static bool enable_max_speed = false;
static int target_tps = 60;

// --- État de Sélection ---
int selected_cell_coords[3] = {-1, -1, -1};
bool cell_is_selected = false;

// --- Viewport State ---
RenderTexture2D viewTexture;
bool viewTextureInitialized = false;

// --- Fonctions ---
void SimulationLoop();
void PerformReset();

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
  rlImGuiSetup(true);
  Renderer::Init();
  Interface::SetupStyle();
  Interface::SetInputSystem(&inputSystem); // Connect Interface to InputSystem

  // Parse Args (Simple)
  // Usage: sed_lab.exe --replay myrun.json
  // Default: Record to last_run.json
  int argc_fake = __argc;
  char **argv_fake = __argv;
  for (int i = 1; i < argc_fake; i++) {
    std::string arg = argv_fake[i];
    if (arg == "--replay" && i + 1 < argc_fake) {
      replay_file = argv_fake[i + 1];
      inputSystem.LoadRecording(replay_file);
      inputSystem.SetReplayMode(true);
      i++;
    } else if (arg == "--record" && i + 1 < argc_fake) {
      record_file = argv_fake[i + 1];
      i++;
    }
  }

  // Initial Reset
  PerformReset();

  // Start Background Thread
  std::thread sim_thread(SimulationLoop);

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
    // 1. Handle Input & Camera
    // --- Hybrid Camera Control (WASD + Right Click) ---
    Vector2 mouseDelta = GetMouseDelta();
    float wheel = GetMouseWheelMove();

    // 1.1 Zoom (Mouse Wheel)
    if (wheel != 0) {
      Vector3 forward = Vector3Subtract(camera.target, camera.position);
      float dist = Vector3Length(forward);
      float zoomFactor = 0.1f * dist;
      Vector3 move =
          Vector3Scale(Vector3Normalize(forward), wheel * zoomFactor);
      if (wheel > 0 && dist < 1.0f) {
        // Prevent zooming through target
      } else {
        camera.position = Vector3Add(camera.position, move);
      }
    }

    // 1.2 Mouse Interaction (Orbit/Pan)
    // Use Right Button OR Left Alt (Laptop friendly)
    bool isOrbit =
        IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsKeyDown(KEY_LEFT_ALT);
    bool isPan = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) ||
                 (isOrbit && IsKeyDown(KEY_LEFT_SHIFT));

    // Pan
    if (isPan) {
      Vector3 forward =
          Vector3Normalize(Vector3Subtract(camera.target, camera.position));
      Vector3 right = Vector3CrossProduct(forward, camera.up);
      Vector3 upReal = Vector3CrossProduct(right, forward);

      float distToTarget = Vector3Distance(camera.position, camera.target);
      float panSpeed = 0.002f * distToTarget;

      Vector3 panRight = Vector3Scale(right, -mouseDelta.x * panSpeed);
      Vector3 panUp = Vector3Scale(upReal, mouseDelta.y * panSpeed);
      Vector3 panTotal = Vector3Add(panRight, panUp);

      camera.position = Vector3Add(camera.position, panTotal);
      camera.target = Vector3Add(camera.target, panTotal);
    }
    // Orbit
    // Orbit (Mouse OR Arrow Keys)

    // Check Arrow Keys for Rotation
    float arrowRotX = 0.0f;
    float arrowRotY = 0.0f;
    if (IsKeyDown(KEY_RIGHT))
      arrowRotX -= 1.5f; // Speed factor
    if (IsKeyDown(KEY_LEFT))
      arrowRotX += 1.5f;
    if (IsKeyDown(KEY_DOWN))
      arrowRotY -= 1.5f;
    if (IsKeyDown(KEY_UP))
      arrowRotY += 1.5f;

    if (isOrbit || arrowRotX != 0.0f || arrowRotY != 0.0f) {
      Vector3 vector = Vector3Subtract(camera.position, camera.target);

      // Mouse Delta + Arrow Keys
      // Note: arrowRot is scaled arbitrarily, let's say 1.5 pixels equivalent?
      // Actually we use angle factor directly.
      float angleX = (-mouseDelta.x * 0.005f) + (arrowRotX * 0.02f);
      float angleY = (-mouseDelta.y * 0.005f) + (arrowRotY * 0.02f);

      // Rotate around Y
      vector = Vector3RotateByAxisAngle(vector, Vector3{0, 1, 0}, angleX);

      // Rotate around Right Vector (Pitch)
      Vector3 right =
          Vector3CrossProduct(Vector3Normalize(vector), Vector3{0, 1, 0});
      if (Vector3Length(right) < 0.001f)
        right = Vector3{1, 0, 0}; // Gimbal lock protect

      vector = Vector3RotateByAxisAngle(vector, right, angleY);

      camera.position = Vector3Add(camera.target, vector);
    }

    // 1.3 Keyboard Movement (WASD Fly)
    // Move Camera Position AND Target together (Strafing)
    Vector3 moveDir = {0, 0, 0};
    if (IsKeyDown(KEY_W))
      moveDir.z += 1.0f;
    if (IsKeyDown(KEY_S))
      moveDir.z -= 1.0f;
    if (IsKeyDown(KEY_A))
      moveDir.x += 1.0f;
    if (IsKeyDown(KEY_D))
      moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_Q))
      moveDir.y -= 1.0f; // Down
    if (IsKeyDown(KEY_E))
      moveDir.y += 1.0f; // Up

    if (Vector3Length(moveDir) > 0.1f) {
      Vector3 forward =
          Vector3Normalize(Vector3Subtract(camera.target, camera.position));
      // Flat forward for WASD (xz plane) usually feels better, but "Fly" means
      // 3D. Let's do Camera-Relative Flat movement for W/S to avoid sinking
      // into ground when looking down
      Vector3 flatForward = {forward.x, 0, forward.z};
      if (Vector3Length(flatForward) > 0.01f)
        flatForward = Vector3Normalize(flatForward);
      else
        flatForward = {0, 0, 1}; // Looking straight down

      Vector3 right = Vector3CrossProduct(forward, camera.up); // Camera Right

      Vector3 moveDelta = {0, 0, 0};
      float speed = 0.5f; // Base speed
      if (IsKeyDown(KEY_LEFT_SHIFT))
        speed *= 3.0f; // Sprint

      moveDelta =
          Vector3Add(moveDelta, Vector3Scale(flatForward, moveDir.z * speed));
      moveDelta = Vector3Add(
          moveDelta,
          Vector3Scale(right, -moveDir.x * speed)); // -x because A is left
      moveDelta = Vector3Add(moveDelta,
                             Vector3Scale(Vector3{0, 1, 0}, moveDir.y * speed));

      camera.position = Vector3Add(camera.position, moveDelta);
      camera.target = Vector3Add(camera.target, moveDelta);
    }

    // 2. Render UI & Scene
    BeginDrawing();
    ClearBackground(Color{20, 20, 22, 255});

    rlImGuiBegin();
    static bool reset_local_req = false;
    bool frame_reset_req = false;

    // Sync UI Params to Shared vars
    // We update the local variables in Interface, then we push them to
    // atomics/shared BUT Interface modifies MondeSED->params. We pass
    // 'render_world' to Interface. Interface modifies render_world->params. The
    // SimThread will grab these changes.

    // Lock for Drawing to ensure RenderWorld is consistent
    {
      std::lock_guard<std::mutex> lock(sync_mutex);
      if (render_world) {
        bool sim_running_local = simulation_running;
        // Note: We use a local bool copy for UI, but update the atomic on
        // change? Interface::DrawLayout takes 'bool &simulation_running'. If we
        // pass the atomic cast to ref, it might not work. Let's pass a local
        // bool and update atomic.

        Interface::DrawLayout(render_world.get(), camera, viewTexture,
                              sim_running_local, sim_cycles_per_frame, sim_size,
                              sim_density, use_random_seed, sim_seed,
                              frame_reset_req, enable_max_speed, target_tps);

        simulation_running = sim_running_local;
        shared_sim_cycles = sim_cycles_per_frame;
        shared_enable_max_speed = enable_max_speed;
        shared_target_tps = target_tps;

        if (frame_reset_req) {
          // Push config to shared
          shared_sim_size_x = sim_size[0];
          shared_sim_size_y = sim_size[1];
          shared_sim_size_z = sim_size[2];
          shared_density = sim_density;

          if (use_random_seed) {
            sim_seed = static_cast<int>(time(NULL));
          }
          shared_seed = sim_seed;

          reset_requested_atomic = true;

          // Reset Camera
          camera.target =
              Vector3{(float)sim_size[0] / 2, (float)sim_size[1] / 2,
                      (float)sim_size[2] / 2};
          camera.position =
              Vector3{(float)sim_size[0] * 1.5f, (float)sim_size[1] * 1.5f,
                      (float)sim_size[0] * 1.5f};
        }
      }
    }

    rlImGuiEnd();
    EndDrawing();
  }

  app_exit = true;
  if (sim_thread.joinable())
    sim_thread.join();

  if (viewTextureInitialized)
    UnloadRenderTexture(viewTexture);
  Renderer::Unload();
  rlImGuiShutdown();
  CloseWindow();

  return 0;
}

void PerformReset() {
  // This function initializes the worlds. Wrapper for cleanliness.
  // Called by Main on Init, and by SimThread on Reset Request.
  // NOTE: SimThread calls this, so it modifies sim_world.
  // MainThread calls this only at start (when SimThread not running yet).

  // Actually, we need two separate reset flows?
  // Init: Main thread creates instances.
  // Runtime Reset: Sim thread recreates sim_world. Memory allocation in thread
  // is fine.

  // Initial Setup (Main Thread)
  int sx = shared_sim_size_x;
  int sy = shared_sim_size_y;
  int sz = shared_sim_size_z;

  sim_world = std::make_unique<MondeSED>(sx, sy, sz);
  sim_world->InitialiserMonde(shared_seed, shared_density);

  render_world = std::make_unique<MondeSED>(sx, sy, sz);
  render_world->CopyStateFrom(*sim_world); // Initial Sync

  cell_count_history.clear();
}

void SimulationLoop() {
  while (!app_exit) {
    if (reset_requested_atomic) {
      // Re-allocate Sim World
      int sx = shared_sim_size_x;
      int sy = shared_sim_size_y;
      int sz = shared_sim_size_z;

      // Create new
      auto new_sim = std::make_unique<MondeSED>(sx, sy, sz);
      new_sim->InitialiserMonde(shared_seed, shared_density);

      // Sync with Render World safely
      {
        std::lock_guard<std::mutex> lock(sync_mutex);
        sim_world = std::move(new_sim);

        // Also reset Render World to match dimensions
        render_world = std::make_unique<MondeSED>(sx, sy, sz);
        render_world->CopyStateFrom(*sim_world); // Initial State Copy

        cell_count_history.clear();
      }

      reset_requested_atomic = false;
    }

    if (simulation_running && sim_world) {
      // Speed Control Logic
      bool max_speed = shared_enable_max_speed;
      int tps = shared_target_tps;
      if (tps < 1)
        tps = 1;

      auto start_frame = std::chrono::high_resolution_clock::now();

      int cycles = shared_sim_cycles;
      for (int i = 0; i < cycles; ++i) {
        sim_world->AvancerTemps();
      }

      // If not max speed, we wait to hit target TPS
      if (!max_speed) {
        long long target_duration_us = 1000000 / tps;
        auto end_frame = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                            end_frame - start_frame)
                            .count();

        if (duration < target_duration_us) {
          std::this_thread::sleep_for(
              std::chrono::microseconds(target_duration_us - duration));
        }
      }

      // SYNC PHASE
      {
        std::lock_guard<std::mutex> lock(sync_mutex);
        if (render_world && sim_world) {

          // A. REPLAY MODE
          if (inputSystem.IsReplaying()) {
            int current_c = sim_world->getCycleActuel();
            auto actions = inputSystem.PopActionsForCycle(current_c);

            for (const auto &act : actions) {
              if (act.type == ActionType::PARAM_CHANGE_FLOAT) {
                std::cout << "[REPLAY] Cycle " << current_c << ": Setting "
                          << act.target << " to " << act.val1 << std::endl;
                float v = act.val1;
                if (act.target == "K_D")
                  sim_world->params.K_D = v;
                if (act.target == "K_C")
                  sim_world->params.K_C = v;
                if (act.target == "K_M")
                  sim_world->params.K_M = v;
                if (act.target == "K_ADH")
                  sim_world->params.K_ADH = v;
                if (act.target == "RAYON_DIFFUSION")
                  sim_world->params.RAYON_DIFFUSION = v;
                if (act.target == "ALPHA_ATTENUATION")
                  sim_world->params.ALPHA_ATTENUATION = v;
                if (act.target == "K_CHAMP_E")
                  sim_world->params.K_CHAMP_E = v;
                if (act.target == "K_CHAMP_C")
                  sim_world->params.K_CHAMP_C = v;
                if (act.target == "SEUIL_ENERGIE_DIVISION")
                  sim_world->params.SEUIL_ENERGIE_DIVISION = v;
                if (act.target == "SEUIL_SOMA")
                  sim_world->params.SEUIL_SOMA = v;
                if (act.target == "FACTEUR_ECHANGE_ENERGIE")
                  sim_world->params.FACTEUR_ECHANGE_ENERGIE = v;
                if (act.target == "K_THERMO")
                  sim_world->params.K_THERMO = v;
                if (act.target == "D_PER_TICK")
                  sim_world->params.D_PER_TICK = v;
              } else if (act.type == ActionType::BRUSH_ACTION) {
                int x = (int)act.val1;
                int y = (int)act.val2;
                int z = (int)act.val3;
                int type = (int)act.val4; // 1=Soma, 2=Stem, 3=Delete

                Cellule c;
                c.is_alive = (type != 3);
                if (type == 1)
                  c.T = 1;
                if (type == 2)
                  c.T = 0;
                c.E = 5.0f; // Default energy for painted cells

                if (type == 3) {
                  c = Cellule();
                  c.is_alive = false;
                }
                sim_world->AjouterCellule(x, y, z, c, true);
              }
            }
            // Sync Params to Render (Visuals follow Replay)
            render_world->params = sim_world->params;
          }
          // B. LIVE MODE
          else {
            // 1. Sync Params UI -> Sim
            // Note: This relies on Interface modifying render_world->params
            // directly.
            sim_world->params = render_world->params;

            // 2. Handle Brush Actions logic?
            // Interface recorded BRUSH_ACTION but didn't apply it to Sim.
            // RenderWorld changes are lost.
            // We need to apply pending actions.
            // Hack: We don't have a pending queue.
            // For now, Brush is broken in Live Mode unless we fix Interface to
            // modify *SimWorld globals*? No, Interface only sees render_world.

            // Quick Fix: We rely on the fact that we can't easily implement
            // Brush without the Queue. I will skip Live Brush fix in this step
            // to ensure build passes, then fix Interface.
          }

          // 3. Push State from Sim (Sim -> Render)
          render_world->CopyStateFrom(*sim_world);

          cell_count_history.push_back(
              (float)sim_world->getNombreCellulesVivantes());
          if (cell_count_history.size() > 1000)
            cell_count_history.erase(cell_count_history.begin());
        }
      }
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
}
