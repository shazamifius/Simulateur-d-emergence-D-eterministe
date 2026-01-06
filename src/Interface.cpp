#include "Interface.h"
#include "Renderer.h"
#include "Validator.h"
#include "raymath.h"
#include "rlImGui.h"
#include <algorithm>
#include <cstdio>
#include <string>

// External state
extern int selected_cell_coords[3];
extern int brush_mode;
extern bool cell_is_selected;
extern std::vector<float> cell_count_history;

InputSystem *Interface::inputSystem = nullptr;

int brush_mode = 0; // 0=View, 1=Soma, 2=Stem, 3=Delete

static bool isViewportHovered = false;
static Rectangle viewRect = {0};

// Static Render Settings (Persist across frames)
// Static Render Settings (Persist across frames)
static RenderSettings renderSettings;
static bool camera_follow_selection = false;

void Interface::SetupStyle() {
  // ... (No change)
  ImGuiStyle &style = ImGui::GetStyle();

  // --- Geometry (GitHub Style) ---
  style.WindowPadding = ImVec2(12, 12);
  style.FramePadding = ImVec2(8, 6);
  style.ItemSpacing = ImVec2(8, 8);
  style.IndentSpacing = 20.0f;
  style.ScrollbarSize = 14.0f;
  style.WindowRounding = 6.0f; // GitHub uses rounded corners
  style.FrameRounding = 6.0f;
  style.PopupRounding = 6.0f;
  style.ScrollbarRounding = 9.0f;
  style.GrabRounding = 6.0f;
  style.TabRounding = 6.0f;
  style.ChildRounding = 6.0f;
  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;

  // --- Colors (GitHub Dark) ---
  ImVec4 bg_canvas = ImVec4(0.01f, 0.04f, 0.09f, 1.00f);  // #0d1117 (Main BG)
  ImVec4 bg_panel = ImVec4(0.09f, 0.11f, 0.13f, 1.00f);   // #161b22 (Panel BG)
  ImVec4 border = ImVec4(0.19f, 0.21f, 0.24f, 1.00f);     // #30363d (Borders)
  ImVec4 text_main = ImVec4(0.90f, 0.90f, 0.93f, 1.00f);  // #c9d1d9
  ImVec4 text_muted = ImVec4(0.54f, 0.58f, 0.62f, 1.00f); // #8b949e
  ImVec4 accent_blue =
      ImVec4(0.11f, 0.40f, 0.70f, 1.00f); // #1f6feb (Button primary)
  ImVec4 accent_hover = ImVec4(0.23f, 0.51f, 0.96f, 1.00f); // #388bfd
  ImVec4 header = ImVec4(0.09f, 0.11f, 0.13f, 1.00f);       // Same as panel

  style.Colors[ImGuiCol_Text] = text_main;
  style.Colors[ImGuiCol_TextDisabled] = text_muted;
  style.Colors[ImGuiCol_WindowBg] = bg_canvas;
  style.Colors[ImGuiCol_ChildBg] = bg_panel;
  style.Colors[ImGuiCol_PopupBg] = bg_panel;
  style.Colors[ImGuiCol_Border] = border;
  style.Colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
  style.Colors[ImGuiCol_FrameBg] = bg_canvas; // Input fields darker
  style.Colors[ImGuiCol_FrameBgHovered] = border;
  style.Colors[ImGuiCol_FrameBgActive] = border;
  style.Colors[ImGuiCol_TitleBg] = bg_panel;
  style.Colors[ImGuiCol_TitleBgActive] = bg_panel;
  style.Colors[ImGuiCol_TitleBgCollapsed] = bg_panel;
  style.Colors[ImGuiCol_MenuBarBg] = bg_panel;
  style.Colors[ImGuiCol_ScrollbarBg] = bg_canvas;
  style.Colors[ImGuiCol_ScrollbarGrab] = border;
  style.Colors[ImGuiCol_ScrollbarGrabHovered] = text_muted;
  style.Colors[ImGuiCol_ScrollbarGrabActive] = text_muted;
  style.Colors[ImGuiCol_CheckMark] = accent_hover;
  style.Colors[ImGuiCol_SliderGrab] = text_muted;
  style.Colors[ImGuiCol_SliderGrabActive] = accent_hover;
  style.Colors[ImGuiCol_Button] =
      ImVec4(0.13f, 0.16f, 0.20f, 1.00f); // #21262d (Btn Secondary)
  style.Colors[ImGuiCol_ButtonHovered] = border;
  style.Colors[ImGuiCol_ButtonActive] = border;
  style.Colors[ImGuiCol_Header] = bg_canvas;
  style.Colors[ImGuiCol_HeaderHovered] = border;
  style.Colors[ImGuiCol_HeaderActive] = border;
  style.Colors[ImGuiCol_Separator] = border;
  style.Colors[ImGuiCol_SeparatorHovered] = border;
  style.Colors[ImGuiCol_SeparatorActive] = border;
  style.Colors[ImGuiCol_ResizeGrip] = border;
  style.Colors[ImGuiCol_ResizeGripHovered] = text_muted;
  style.Colors[ImGuiCol_ResizeGripActive] = text_main;
  style.Colors[ImGuiCol_Tab] = bg_panel;
  style.Colors[ImGuiCol_TabHovered] = bg_canvas;
  style.Colors[ImGuiCol_TabActive] = bg_canvas; // Active tab mimics canvas
  style.Colors[ImGuiCol_TabUnfocused] = bg_panel;
  style.Colors[ImGuiCol_TabUnfocusedActive] = bg_canvas;
  style.Colors[ImGuiCol_PlotLines] =
      ImVec4(0.17f, 0.84f, 0.45f, 1.00f); // #2da44e (Green stats)
  style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.27f, 0.94f, 0.55f, 1.00f);
  style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.17f, 0.84f, 0.45f, 1.00f);
  style.Colors[ImGuiCol_PlotHistogramHovered] =
      ImVec4(0.27f, 0.94f, 0.55f, 1.00f);
  style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.11f, 0.40f, 0.70f, 0.30f);
}

void Interface::DrawLayout(MondeSED *monde, Camera3D &camera,
                           RenderTexture2D &viewTexture,
                           bool &simulation_running, int &sim_cycles_per_frame,
                           int sim_size[3], float &sim_density,
                           bool &use_random_seed, int &sim_seed,
                           bool &reset_requested, bool &enable_max_speed,
                           int &target_tps) {
  // Robust Layout using IO DisplaySize instead of GetScreenHeight (better sync)
  float width = ImGui::GetIO().DisplaySize.x;
  float height = ImGui::GetIO().DisplaySize.y;

  // --- Layout Configuration ---
  float menuHeight = 26.0f;
  float sideLeftWidth = 320.0f;
  float sideRightWidth = 340.0f;
  float bottomHeight = 240.0f;

  // Small screen adapt
  if (width < 1200) {
    sideLeftWidth = 260.0f;
    sideRightWidth = 280.0f;
  }
  if (height < 768) {
    bottomHeight = 180.0f;
  }

  float viewportW = width - sideLeftWidth - sideRightWidth;
  float viewportH = height - bottomHeight - menuHeight;

  // Constraints
  if (viewportW < 100)
    viewportW = 100;

  // --- Draw Panels ---
  DrawMainMenuBar(monde, simulation_running, reset_requested);

  // Common Flags for Static Layout
  ImGuiWindowFlags staticFlags =
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoBringToFrontOnFocus;

  // Left Panel (Control)
  ImGui::SetNextWindowPos(ImVec2(0, menuHeight));
  ImGui::SetNextWindowSize(ImVec2(sideLeftWidth, height - menuHeight));
  DrawPropertiesPanel(monde, simulation_running, sim_cycles_per_frame, sim_size,
                      sim_density, use_random_seed, sim_seed, reset_requested,
                      enable_max_speed, target_tps);

  // Right Panel (Inspector)
  ImGui::SetNextWindowPos(ImVec2(width - sideRightWidth, menuHeight));
  ImGui::SetNextWindowSize(ImVec2(sideRightWidth, height - menuHeight));
  DrawInspectorPanel(monde);

  // Bottom Panel (Stats)
  ImGui::SetNextWindowPos(ImVec2(sideLeftWidth, height - bottomHeight));
  ImGui::SetNextWindowSize(ImVec2(viewportW, bottomHeight));
  DrawStatsPanel(monde);

  // Center Viewport
  ImGui::SetNextWindowPos(ImVec2(sideLeftWidth, menuHeight));
  ImGui::SetNextWindowSize(ImVec2(viewportW, viewportH));
  DrawViewport(camera, viewTexture, monde);

  // Camera Follow Logic
  if (camera_follow_selection && cell_is_selected && monde) {
    // Find Cell Position
    int sx = monde->getTailleX();
    int sy = monde->getTailleY();
    int idx = monde->getIndex(selected_cell_coords[0], selected_cell_coords[1],
                              selected_cell_coords[2]);
    if (idx != -1) {
      // Smooth Lerp Target
      Vector3 targetPos = {(float)selected_cell_coords[0],
                           (float)selected_cell_coords[1],
                           (float)selected_cell_coords[2]};
      camera.target.x += (targetPos.x - camera.target.x) * 0.1f;
      camera.target.y += (targetPos.y - camera.target.y) * 0.1f;
      camera.target.z += (targetPos.z - camera.target.z) * 0.1f;
    }
  }
}

void Interface::DrawMainMenuBar(MondeSED *monde, bool &simulation_running,
                                bool &reset_requested) {
  if (ImGui::BeginMainMenuBar()) {
    ImGui::TextDisabled("SED LAB v8.0");
    ImGui::Separator();
    if (ImGui::BeginMenu("FILE")) {
      if (ImGui::MenuItem("New / Reset")) {
        reset_requested = true;
      }
      if (ImGui::MenuItem("Save State", "Ctrl+S")) {
        if (monde)
          monde->SauvegarderEtat("save.sed");
      }
      if (ImGui::MenuItem("Load State", "Ctrl+O")) {
        if (monde)
          monde->ChargerEtat("save.sed");
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Exit"))
        CloseWindow();
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("VIEW")) {
      if (ImGui::MenuItem("Reset Camera")) { /* ... */
      }
      ImGui::EndMenu();
    }

    // Center info
    float w = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + w / 2 - 50);
    if (simulation_running)
      ImGui::TextColored(ImVec4(0, 1, 0, 1), "RUNNING");
    else
      ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "PAUSED");

    ImGui::EndMainMenuBar();
  }
}

void Interface::DrawPropertiesPanel(MondeSED *monde, bool &simulation_running,
                                    int &sim_cycles, int sim_size[3],
                                    float &density, bool &use_rnd, int &seed,
                                    bool &reset_requested,
                                    bool &enable_max_speed, int &target_tps) {
  ImGui::Begin("CONTROLS", nullptr,
               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

  DrawFuturisticHeader("SIMULATION CONTROL");

  float gw = ImGui::GetContentRegionAvail().x;
  if (simulation_running) {
    if (ImGui::Button("PAUSE ||", ImVec2(gw, 40)))
      simulation_running = false;
  } else {
    if (ImGui::Button("PLAY >", ImVec2(gw, 40)))
      simulation_running = true;

    ImGui::BeginDisabled(); // Spacing hack or style
    ImGui::EndDisabled();

    if (ImGui::Button("STEP >>", ImVec2(gw, 25))) {
      if (monde)
        monde->AvancerTemps();
    }
  }

  ImGui::Dummy(ImVec2(0, 10));
  ImGui::Dummy(ImVec2(0, 10));
  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "SPEED CONTROL");
  ImGui::Checkbox("MAX TURBO SPEED", &enable_max_speed);
  ImGui::SameLine();
  if (ImGui::Button("BENCHMARK")) {
    enable_max_speed = true;
    if (monde)
      Validator::LogEvent(*monde, EventType::INFO,
                          "Benchmark Started (Max Speed ON)");
  }

  if (!enable_max_speed) {
    ImGui::Text("Target Speed (TPS)");
    ImGui::SetNextItemWidth(gw);
    ImGui::SliderInt("##tps", &target_tps, 1, 120);
  } else {
    ImGui::TextDisabled("Speed Limit: OFF (CPU Max)");
  }

  ImGui::Dummy(ImVec2(0, 5));
  ImGui::Text("Cycles / Frame");
  ImGui::SetNextItemWidth(gw);
  ImGui::SliderInt("##speed", &sim_cycles, 1, 50);

  ImGui::Dummy(ImVec2(0, 20));
  DrawFuturisticHeader("WORLD GEN");

  ImGui::Text("Limit");
  ImGui::InputInt3("##dims", sim_size);
  ImGui::Text("Density");
  ImGui::SliderFloat("##dens", &density, 0.01f, 1.0f);

  ImGui::Checkbox("Random Seed", &use_rnd);
  if (!use_rnd) {
    ImGui::InputInt("Seed Value", &seed);
  }

  ImGui::Dummy(ImVec2(0, 10));
  if (ImGui::Button("REGENERATE WORLD", ImVec2(gw, 30))) {
    reset_requested = true;
  }

  // --- MANUAL PLACEMENT & SCENARIOS ---
  if (monde) {
    ImGui::Dummy(ImVec2(0, 20));
    DrawFuturisticHeader("TOOLS");
    const char *tools[] = {"VIEW", "BRUSH (SOMA)", "BRUSH (STEM)", "DELETE"};
    static int current_tool = 0;
    ImGui::Combo("Tool Mode", &current_tool, tools, IM_ARRAYSIZE(tools));

    ImGui::Dummy(ImVec2(0, 10));
    DrawFuturisticHeader("SCENARIOS");

    if (ImGui::Button("Load Default", ImVec2(gw, 25))) {
      if (monde->ChargerScenario("scenarios/default.json")) {
        Validator::LogEvent(*monde, EventType::INFO,
                            "Loaded scenarios/default.json");
        reset_requested = false; // Just updated data, no need full reset
      } else {
        Validator::LogEvent(*monde, EventType::ERROR,
                            "Failed to load scenarios/default.json");
      }
    }

    // Manual interaction logic needs to be handled in DrawViewport really,
    // but we can set a flag/state here or handle click there.
    // Let's use a static state for the brush.
    extern int brush_mode;
    brush_mode = current_tool;
  }

  if (monde) {
    ImGui::Dummy(ImVec2(0, 20));
    DrawFuturisticHeader("PHYSICS (LAWS)");
    auto &p = monde->params;
    int cycle = monde->getCycleActuel();

    ImGui::Text("Gravity (K_D)");
    ImGui::SetNextItemWidth(gw);
    if (ImGui::SliderFloat("##kd", &p.K_D, 0.0f, 5.0f)) {
      if (inputSystem && !inputSystem->IsReplaying())
        inputSystem->RecordAction(
            {cycle, ActionType::PARAM_CHANGE_FLOAT, "K_D", p.K_D});
    }
    ImGui::Text("Repulsion (K_C)");
    ImGui::SetNextItemWidth(gw);
    if (ImGui::SliderFloat("##kc", &p.K_C, 0.0f, 5.0f)) {
      if (inputSystem && !inputSystem->IsReplaying())
        inputSystem->RecordAction(
            {cycle, ActionType::PARAM_CHANGE_FLOAT, "K_C", p.K_C});
    }
    ImGui::Text("Inertia (K_M)");
    ImGui::SetNextItemWidth(gw);
    if (ImGui::SliderFloat("##km", &p.K_M, 0.0f, 5.0f)) {
      if (inputSystem && !inputSystem->IsReplaying())
        inputSystem->RecordAction(
            {cycle, ActionType::PARAM_CHANGE_FLOAT, "K_M", p.K_M});
    }
    ImGui::Text("Cohesion (K_ADH)");
    ImGui::SetNextItemWidth(gw);
    if (ImGui::SliderFloat("##kadh", &p.K_ADH, 0.0f, 5.0f)) {
      if (inputSystem && !inputSystem->IsReplaying())
        inputSystem->RecordAction(
            {cycle, ActionType::PARAM_CHANGE_FLOAT, "K_ADH", p.K_ADH});
    }

    ImGui::Text("Field Radius");
    if (ImGui::SliderFloat("##fd", &p.RAYON_DIFFUSION, 1.0f, 10.0f)) {
      if (inputSystem && !inputSystem->IsReplaying())
        inputSystem->RecordAction({cycle, ActionType::PARAM_CHANGE_FLOAT,
                                   "RAYON_DIFFUSION", p.RAYON_DIFFUSION});
    }
    ImGui::Text("Field Alpha");
    if (ImGui::SliderFloat("##fa", &p.ALPHA_ATTENUATION, 0.1f, 5.0f)) {
      if (inputSystem && !inputSystem->IsReplaying())
        inputSystem->RecordAction({cycle, ActionType::PARAM_CHANGE_FLOAT,
                                   "ALPHA_ATTENUATION", p.ALPHA_ATTENUATION});
    }
    ImGui::Text("K_CHAMP_E");
    if (ImGui::SliderFloat("##kce", &p.K_CHAMP_E, 0.0f, 5.0f)) {
      if (inputSystem && !inputSystem->IsReplaying())
        inputSystem->RecordAction(
            {cycle, ActionType::PARAM_CHANGE_FLOAT, "K_CHAMP_E", p.K_CHAMP_E});
    }
    ImGui::Text("K_CHAMP_C");
    if (ImGui::SliderFloat("##kcc", &p.K_CHAMP_C, 0.0f, 5.0f)) {
      if (inputSystem && !inputSystem->IsReplaying())
        inputSystem->RecordAction(
            {cycle, ActionType::PARAM_CHANGE_FLOAT, "K_CHAMP_C", p.K_CHAMP_C});
    }

    ImGui::Dummy(ImVec2(0, 10));
    DrawFuturisticHeader("BIO-LOGIC");
    ImGui::Text("Div Threshold");
    if (ImGui::SliderFloat("##sdiv", &p.SEUIL_ENERGIE_DIVISION, 1.0f, 5.0f)) {
      if (inputSystem && !inputSystem->IsReplaying())
        inputSystem->RecordAction({cycle, ActionType::PARAM_CHANGE_FLOAT,
                                   "SEUIL_ENERGIE_DIVISION",
                                   p.SEUIL_ENERGIE_DIVISION});
    }
    ImGui::Text("Soma Threshold");
    if (ImGui::SliderFloat("##ssoma", &p.SEUIL_SOMA, 0.0f, 1.0f)) {
      if (inputSystem && !inputSystem->IsReplaying())
        inputSystem->RecordAction({cycle, ActionType::PARAM_CHANGE_FLOAT,
                                   "SEUIL_SOMA", p.SEUIL_SOMA});
    }
    ImGui::Text("Diff. Rate");
    if (ImGui::SliderFloat("##kxc", &p.FACTEUR_ECHANGE_ENERGIE, 0.0f, 1.0f)) {
      if (inputSystem && !inputSystem->IsReplaying())
        inputSystem->RecordAction({cycle, ActionType::PARAM_CHANGE_FLOAT,
                                   "FACTEUR_ECHANGE_ENERGIE",
                                   p.FACTEUR_ECHANGE_ENERGIE});
    }

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "PRESETS");
    // TODO: Record presets as multiple param changes or specialized Event
    if (ImGui::Button("Balance")) {
      p.K_THERMO = 0.001f;
      p.K_ADH = 0.5f;
      p.SEUIL_ENERGIE_DIVISION = 1.8f;
      // Partial recording manual for now...
    }
    ImGui::SameLine();
    if (ImGui::Button("Expansion")) {
      p.K_THERMO = 0.0005f;
      p.K_ADH = 0.1f;
      p.SEUIL_ENERGIE_DIVISION = 1.2f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Brain")) {
      p.SEUIL_SOMA = 0.2f;
      p.SEUIL_NEURO = 0.4f;
      p.LEARN_RATE = 0.1f;
    }
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 10));

    ImGui::Dummy(ImVec2(0, 10));
    DrawFuturisticHeader("METABOLISM");
    if (ImGui::InputFloat("Life Cost", &p.K_THERMO)) {
      if (inputSystem && !inputSystem->IsReplaying())
        inputSystem->RecordAction(
            {cycle, ActionType::PARAM_CHANGE_FLOAT, "K_THERMO", p.K_THERMO});
    }
    if (ImGui::InputFloat("Debt/Tick", &p.D_PER_TICK)) {
      if (inputSystem && !inputSystem->IsReplaying())
        inputSystem->RecordAction({cycle, ActionType::PARAM_CHANGE_FLOAT,
                                   "D_PER_TICK", p.D_PER_TICK});
    }

    ImGui::Dummy(ImVec2(0, 10));
    DrawFuturisticHeader("VISUALIZATION (V5)");
    ImGui::Checkbox("Show Cells (Bio/Data)", &renderSettings.show_cells);
    ImGui::Checkbox("Show Network (Synapses)", &renderSettings.show_network);
    ImGui::Checkbox("Show Force Fields (Math)", &renderSettings.show_fields);
    if (renderSettings.show_fields) {
      ImGui::SliderFloat("Field Density", &renderSettings.field_density, 0.1f,
                         1.0f);
    }
    ImGui::Checkbox("Show Grid/Gizmos", &renderSettings.show_gizmos);

    ImGui::Separator();
    const char *modes[] = {"DEFAULT (Type)", "ENERGY (Heat)", "STRESS (Heat)",
                           "GRADIENT (Heat)"};
    ImGui::Combo("Color Mode", &renderSettings.cell_color_mode, modes,
                 IM_ARRAYSIZE(modes));
  }

  ImGui::End();
}

void Interface::DrawInspectorPanel(MondeSED *monde) {
  ImGui::Begin("INSPECTOR", nullptr,
               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
  DrawFuturisticHeader("INSPECTOR");

  if (!cell_is_selected || !monde) {
    ImGui::TextDisabled("No Selection\nClick on a cell in viewport.");
  } else {
    int idx = monde->getIndex(selected_cell_coords[0], selected_cell_coords[1],
                              selected_cell_coords[2]);
    // Note: getIndex is now fake in infinite world, but returns valid if in
    // bounds. However, we should use getCellule directly.
    const auto &c =
        monde->getCellule(selected_cell_coords[0], selected_cell_coords[1],
                          selected_cell_coords[2]);

    // Big ID Card
    ImGui::BeginGroup();
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "CELL ID: %d",
                       idx); // idx is less meaningful now
    ImGui::Text("XYZ: [%d, %d, %d]", selected_cell_coords[0],
                selected_cell_coords[1], selected_cell_coords[2]);
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::Checkbox("CAMERA FOLLOW", &camera_follow_selection);

    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 10));

    if (c.is_alive) {
      ImVec4 typeCol = ImVec4(0.8f, 0.8f, 0.8f, 1);
      const char *typeStr = "UNKNOWN";
      if (c.T == 0) {
        typeStr = "STEM (Souche)";
        typeCol = ImVec4(0.7f, 0.7f, 0.7f, 1);
      } else if (c.T == 1) {
        typeStr = "SOMA";
        typeCol = ImVec4(1.0f, 0.4f, 0.4f, 1);
      } else if (c.T == 2) {
        typeStr = "NEURON";
        typeCol = ImVec4(0.2f, 0.8f, 1.0f, 1);
      }

      ImGui::TextColored(typeCol, "TYPE: %s", typeStr);

      ImGui::Dummy(ImVec2(0, 10));
      ImGui::Text("State Variables");
      ImGui::Separator();

      char buf[32];
      snprintf(buf, sizeof(buf), "%.2f", c.E);
      DrawStatCard("Energy", buf, Color{0, 255, 255, 255});

      snprintf(buf, sizeof(buf), "%.2f", c.D);
      DrawStatCard("Debt", buf, Color{255, 100, 100, 255});

      snprintf(buf, sizeof(buf), "%.2f", c.P);
      DrawStatCard("Potential", buf, Color{255, 255, 0, 255});

      ImGui::Dummy(ImVec2(0, 10));
      ImGui::ProgressBar(c.E / 10.0f, ImVec2(-1, 10), "");
      ImGui::SameLine();
      ImGui::Text("E");
    } else {
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "STATUS: DEAD / VOID");
    }
  }
  ImGui::End();
}

void Interface::DrawStatsPanel(MondeSED *monde) {
  ImGui::Begin("DATA_CENTER", nullptr,
               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

  // Split into 2 columns: Text Stats & Graphs
  ImGui::Columns(2, "stats_cols", false);
  ImGui::SetColumnWidth(0, 250);

  DrawFuturisticHeader("SYSTEM STATUS");
  if (monde) {
    ImGui::Text("Population:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "%d",
                       monde->getNombreCellulesVivantes());

    ImGui::Text("Cycle:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "%d", monde->getCycleActuel());

    ImGui::Text("Frame Time:");
    ImGui::SameLine();
    ImGui::Text("%.2f ms", GetFrameTime() * 1000.0f);

    ImGui::Text("FPS:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d", GetFPS());

    ImGui::Text("Hash:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 1, 1), "%zu",
                       monde->CalculateStateHash());

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::Separator();
    if (!Validator::GetLogHistory().empty()) {
      const auto &last = Validator::GetLogHistory().back();
      ImVec4 col = ImVec4(0.7f, 0.7f, 0.7f, 1);
      if (last.type == EventType::ERROR)
        col = ImVec4(1, 0, 0, 1);
      else if (last.type == EventType::DEATH)
        col = ImVec4(1, 0.4f, 0.4f, 1);
      else if (last.type == EventType::MITOSIS)
        col = ImVec4(0, 1, 0, 1);
      ImGui::TextColored(col, "[%d] %s", last.cycle, last.message.c_str());
    }
  }

  ImGui::NextColumn();

  DrawFuturisticHeader("EMERGENCE GRAPH");
  if (!cell_count_history.empty()) {
    float maxVal = 0;
    for (float v : cell_count_history)
      if (v > maxVal)
        maxVal = v;

    ImGui::PlotLines("##pop", cell_count_history.data(),
                     (int)cell_count_history.size(), 0, nullptr, 0.0f,
                     maxVal * 1.2f, ImVec2(-1, -1));
  }

  ImGui::Columns(1);
  ImGui::End();
}

void Interface::DrawViewport(Camera3D &camera, RenderTexture2D &viewTexture,
                             MondeSED *monde) {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1));
  ImGui::Begin("VIEWPORT", nullptr,
               ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDecoration |
                   ImGuiWindowFlags_NoMove);

  ImVec2 vSize = ImGui::GetContentRegionAvail();
  // Sanity check size
  if (vSize.x < 1)
    vSize.x = 1;
  if (vSize.y < 1)
    vSize.y = 1;

  // 1. Manage Texture Resize / Init
  if (viewTexture.id == 0 || viewTexture.texture.width != (int)vSize.x ||
      viewTexture.texture.height != (int)vSize.y) {
    if (viewTexture.id != 0)
      UnloadRenderTexture(viewTexture);

    viewTexture = LoadRenderTexture((int)vSize.x, (int)vSize.y);
  }

  // 2. Render Scene to this Texture (Immediate Update)
  // We need to pass the selected coords. We can grab them from externs for now.
  extern int selected_cell_coords[3];
  Renderer::Render(monde, camera, viewTexture, selected_cell_coords,
                   renderSettings);

  // 3. Display Texture
  if (viewTexture.id != 0) {
    // Raylib RenderTextures are flipped Y by default in OpenGL
    rlImGuiImageRect(&viewTexture.texture, (int)vSize.x, (int)vSize.y,
                     Rectangle{0, 0, (float)viewTexture.texture.width,
                               -(float)viewTexture.texture.height});
  }

  // Overlay info
  ImGui::SetCursorPos(ImVec2(10, 10));
  ImGui::TextColored(ImVec4(1, 1, 1, 0.5f), "CAM: %.1f %.1f %.1f",
                     camera.position.x, camera.position.y, camera.position.z);

  // Store state for interactions
  isViewportHovered = ImGui::IsWindowHovered();
  ImVec2 winPos = ImGui::GetWindowPos();
  viewRect = {winPos.x, winPos.y, vSize.x, vSize.y};

  // Logic: Raycast from Mouse
  if (isViewportHovered && monde) {
    ImVec2 mousePos = ImGui::GetMousePos();
    Vector2 mouseRel = {mousePos.x - winPos.x, mousePos.y - winPos.y};

    // Raylib Raycast needs window-relative, but here we render to texture.
    // We must map mouse from UI space to Texture space (if identical, easy).

    Ray ray = GetMouseRay(mouseRel, camera);

    // Voxel Traversal (Fast Voxel Traversal Algorithm or Simple Step)
    // Since grid is small (32-128), simple step is OK.

    RayCollision hit = {0};
    hit.distance = FLT_MAX;
    hit.hit = false;
    int hitX = -1, hitY = -1, hitZ = -1;

    // Optim: Bounding Box check first
    BoundingBox worldBox = {Vector3{0, 0, 0},
                            Vector3{(float)monde->getTailleX(),
                                    (float)monde->getTailleY(),
                                    (float)monde->getTailleZ()}};

    RayCollision boxHit = GetRayCollisionBox(ray, worldBox);

    if (boxHit.hit) {
      // Raymarch inside box
      Vector3 p = boxHit.point;
      Vector3 dir = Vector3Normalize(ray.direction);
      float step = 0.1f;

      for (float d = 0; d < monde->getTailleX() * 1.7f; d += step) { // Max diag
        Vector3 cur = Vector3Add(p, Vector3Scale(dir, d));

        int gx = (int)floor(cur.x);
        int gy = (int)floor(cur.y);
        int gz = (int)floor(cur.z);

        if (gx >= 0 && gx < monde->getTailleX() && gy >= 0 &&
            gy < monde->getTailleY() && gz >= 0 && gz < monde->getTailleZ()) {

          // If View Mode: Stop at first ALIVE cell
          // If Build Mode: We need to see where we point.
          // Simplification: We intersect the "Grid" regardless of empty or not?
          // No, we want to click on visible cells OR empty space?
          // "Minecraft" style: click on face to add.

          // For now: Simple "Paint 3D" -> Click = Add at cursor depth?
          // Easier: Raycast against logic.

          // Let's implement "Click on existing cell selects it" (View)
          if (brush_mode == 0) {
            const Cellule &c = monde->getCellule(gx, gy, gz);
            if (c.is_alive) {
              hit.hit = true;
              hitX = gx;
              hitY = gy;
              hitZ = gz;
              break;
            }
          } else {
            // Build Mode: We need a "Plane" or "Voxel" interaction.
            // Let's stick to "Draw on Surface" or "Fill Empty".
            // Complex. Let's start with: Click = fill cell at int coords.
            // Issue: empty cells are not rendered, so we click through them. :p

            // Solution: Check if we are close to integer center?
            // Hack: Just select the first ALIVE cell for VIEW.
            // For BUILD, we might need a visible "Cursor" or "Grid Plane".
            // Let's rely on BoxHit entry for now + Step?
            // Better: Just highlight the voxel under cursor based on box entry
            // + mouse depth?

            // Let's stick to VIEW logic for selection first.
          }
        } else {
          // Exit box
          // break;
        }
      }
    }

    // Handle Click (VIEW MODE)
    if (ImGui::IsMouseClicked(0) && hit.hit && brush_mode == 0) {
      selected_cell_coords[0] = hitX;
      selected_cell_coords[1] = hitY;
      selected_cell_coords[2] = hitZ;
      cell_is_selected = true;
    }

    // Handle Click (BUILD MODE - SOMA/STEM)
    // Infinite World Build Logic - Ray Hit
    // For now we use the same hit detection
    if (ImGui::IsMouseClicked(0) && hit.hit && brush_mode > 0 && inputSystem &&
        !inputSystem->IsReplaying()) {
      // Apply change logic?
      // We need to actually MODIFY the world here, usually Interface didn't do
      // it before? Wait, the original code had no implementation for Brush in
      // DrawViewport. Let's implementing it via recording.

      // Action: Brush
      // Val1=X, Val2=Y, Val3=Z, Val_Int=Type (1=Soma, 2=Stem, 3=Delete)
      // Note: Brush types: 1=Soma(T=1), 2=Stem(T=0), 3=Delete
      // We record it. Main loop or MondeSED must process it.
      // But Interface is allowed to modify MondeSED if passed pointer.

      // Direct Modify + Record
      // 1. Record
      float typeVal = 0;
      if (brush_mode == 1)
        typeVal = 1.0f; // SOMA
      if (brush_mode == 2)
        typeVal = 0.0f; // STEM
      // Delete?

      ActionType atT = ActionType::BRUSH_ACTION;
      // Encode Brush Mode in Target string or extra val?
      // Let's use Val4 or target string
      inputSystem->RecordAction({monde->getCycleActuel(), atT, "BRUSH",
                                 (float)hitX, (float)hitY, (float)hitZ,
                                 (float)brush_mode});

      // 2. Modify Immediately (for responsiveness in Record Mode)
      // Actually, let's defer to Main Loop to keep determinism?
      // If we modify NOW, it happens at 'Frame Time', which might be middle of
      // Sim Update if threaded. SAFE WAY: Interface only Records. Main Loop
      // Pops and Applies. BUT user lag? Acceptable for this project.
    }
  }

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
}

void Interface::DrawFuturisticHeader(const char *label) {
  ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Default font for now
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 1.0f, 1.0f));
  ImGui::Text("[%s]", label);
  ImGui::PopStyleColor();
  ImGui::PopFont();
  ImGui::Separator();
  ImGui::Dummy(ImVec2(0, 5));
}

void Interface::DrawStatCard(const char *label, const char *value, Color c) {
  ImGui::BeginGroup();
  ImGui::TextDisabled("%s", label);
  ImGui::TextColored(ImVec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f),
                     "%s", value);
  ImGui::EndGroup();
  ImGui::SameLine(0, 20);
}
