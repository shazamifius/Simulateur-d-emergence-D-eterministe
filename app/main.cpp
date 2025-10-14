// --- Fichiers d'en-tête ---
#include "imgui.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "rlImGui.h"
#include "MondeSED.h"
#include <iostream>
#include <memory>
#include <algorithm> // Pour std::max
#include <cmath>     // Pour fmod
#include <vector>

// --- Variables globales ---
std::unique_ptr<MondeSED> monde;
bool simulation_running = false;
std::vector<float> cell_count_history; // Pour le graphique de l'historique

// --- Paramètres de simulation configurables via l'UI ---
static int sim_size[3] = {16, 16, 16};
static float sim_density = 0.1f;
static int sim_cycles_per_frame = 1;
static int sim_seed = 12345; // Graine de simulation
static bool use_random_seed = true;
static int selected_cell_coords[3] = {-1, -1, -1}; // Coordonnées de la cellule sélectionnée
static bool cell_is_selected = false;

// --- Fonctions ---
void ResetSimulation();
void DrawUI();
void Draw3DVisualization(Camera& camera);

int main() {
    int screenWidth = 1280;
    int screenHeight = 800;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "SED-Lab | C++ Edition");
    SetTargetFPS(60);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // ImGui::StyleColorsDark(); // Remplacé par un thème personnalisé ci-dessous

    // Thème personnalisé pour une meilleure lisibilité (gris, blanc, noir)
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.18f, 0.18f, 0.18f, 0.94f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.18f, 0.18f, 0.18f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.30f, 0.30f, 0.30f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.40f, 0.40f, 0.40f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.45f, 0.45f, 0.45f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.44f, 0.44f, 0.44f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.46f, 0.47f, 0.48f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.35f, 0.35f, 0.35f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.45f, 0.45f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.75f, 0.75f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.30f, 0.30f, 0.30f, 0.86f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.40f, 0.40f, 0.40f, 0.80f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.15f, 0.15f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    rlImGuiSetup(true);

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 30.0f, 30.0f, 30.0f };
    camera.target = (Vector3){ (float)sim_size[0]/2, (float)sim_size[1]/2, (float)sim_size[2]/2 };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose()) {
        if (simulation_running && monde) {
            for(int i = 0; i < sim_cycles_per_frame; ++i) {
                monde->AvancerTemps();
            }
            // --- Enregistrement de l'historique pour le graphique ---
            cell_count_history.push_back(static_cast<float>(monde->getNombreCellulesVivantes()));
            if (cell_count_history.size() > 500) {
                cell_count_history.erase(cell_count_history.begin());
            }
        }

        // --- Logique de Sélection de Cellule ---
        if (monde && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Ray ray = GetMouseRay(GetMousePosition(), camera);
            float closest_collision_dist = FLT_MAX;
            bool hit_found = false;

            const auto& grille = monde->getGrille();
            int size_x = monde->getTailleX();
            int size_y = monde->getTailleY();

            // Normalisation du rayon (identique à Draw3DVisualization)
            float max_c = 0.0f;
            for (const auto& cell : grille) {
                if (cell.is_alive && cell.C > max_c) max_c = cell.C;
            }
            if (max_c == 0.0f) max_c = 1.0f;

            for (int i = 0; i < grille.size(); ++i) {
                const auto& cell = grille[i];
                if (cell.is_alive) {
                    int x = i % size_x;
                    int y = (i / size_x) % size_y;
                    int z = i / (size_x * size_y);
                    Vector3 position = {(float)x, (float)y, (float)z};
                    float radius = 0.1f + (cell.C / max_c) * 0.5f;

                    RayCollision collision = GetRayCollisionSphere(ray, position, radius);
                    if (collision.hit && collision.distance < closest_collision_dist) {
                        closest_collision_dist = collision.distance;
                        selected_cell_coords[0] = x;
                        selected_cell_coords[1] = y;
                        selected_cell_coords[2] = z;
                        hit_found = true;
                    }
                }
            }
            cell_is_selected = hit_found;
        }

        // --- Contrôles de Caméra Personnalisés ---
        if (!ImGui::GetIO().WantCaptureMouse)
        {
            // Zoom avec la molette de la souris
            float wheel = GetMouseWheelMove();
            if (wheel != 0)
            {
                Vector3 toTarget = Vector3Subtract(camera.target, camera.position);
                float distance = Vector3Length(toTarget);
                distance -= wheel * 2.0f; // Vitesse de zoom
                if (distance < 2.0f) distance = 2.0f; // Empêche de traverser la cible
                if (distance > 100.0f) distance = 100.0f; // Distance maximale
                camera.position = Vector3Add(camera.target, Vector3Scale(Vector3Normalize(Vector3Negate(toTarget)), distance));
            }

            // Panoramique (déplacement latéral) avec Maj + Clic Molette
            if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) && IsKeyDown(KEY_LEFT_SHIFT))
            {
                Vector2 delta = GetMouseDelta();
                float panSpeed = 0.05f;

                Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
                Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
                Vector3 up = Vector3CrossProduct(right, forward);

                Vector3 panVector = Vector3Add(Vector3Scale(right, -delta.x * panSpeed), Vector3Scale(up, delta.y * panSpeed));
                camera.target = Vector3Add(camera.target, panVector);
                camera.position = Vector3Add(camera.position, panVector);
            }
            // Orbite (rotation) avec Clic Molette
            else if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
            {
                Vector2 delta = GetMouseDelta();
                float rotateSpeed = 0.01f;

                Vector3 targetToCam = Vector3Subtract(camera.position, camera.target);

                // Rotation Yaw (autour de l'axe Y du monde)
                Matrix yawRotation = MatrixRotateY(-delta.x * rotateSpeed);
                targetToCam = Vector3Transform(targetToCam, yawRotation);

                // Rotation Pitch (autour de l'axe droit de la caméra)
                Vector3 forward = Vector3Normalize(Vector3Negate(targetToCam));
                Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
                Matrix pitchRotation = MatrixRotate(right, -delta.y * rotateSpeed);
                targetToCam = Vector3Transform(targetToCam, pitchRotation);

                // Appliquer la nouvelle position si elle ne cause pas de "Gimbal Lock"
                Vector3 newForward = Vector3Normalize(Vector3Negate(targetToCam));
                float dot = Vector3DotProduct(newForward, camera.up);
                if (fabs(dot) < 0.995)
                {
                    camera.position = Vector3Add(camera.target, targetToCam);
                }
            }
        }

        BeginDrawing();
        ClearBackground(Color{20, 20, 20, 255});

        BeginMode3D(camera);
        if (monde) {
            Draw3DVisualization(camera);
        }
        DrawGrid(40, 1.0f);
        EndMode3D();

        rlImGuiBegin();
        DrawUI();
        rlImGuiEnd();

        EndDrawing();
    }

    rlImGuiShutdown();
    ImGui::DestroyContext();
    CloseWindow();

    return 0;
}

void ResetSimulation() {
    // Si l'utilisateur a choisi d'utiliser une graine aléatoire, en générer une nouvelle.
    if (use_random_seed) {
        sim_seed = time(NULL);
    }
    monde = std::make_unique<MondeSED>(sim_size[0], sim_size[1], sim_size[2]);
    // Initialise le monde avec la graine et la densité configurées.
    monde->InitialiserMonde(static_cast<unsigned int>(sim_seed), sim_density);
    simulation_running = false;
    cell_count_history.clear();
}

void DrawUI() {
    ImGui::Begin("Panneau de Contrôle", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::BeginTabBar("MainTabBar")) {

        if (ImGui::BeginTabItem("Contrôle")) {
            ImGui::SeparatorText("Actions");
            if (ImGui::Button("Initialiser/Réinitialiser", ImVec2(-1, 0))) {
                ResetSimulation();
            }
            if (monde) {
                if (simulation_running) {
                    if (ImGui::Button("Pause", ImVec2(-1, 0))) simulation_running = false;
                } else {
                    float button_width = ImGui::GetContentRegionAvail().x * 0.5f - ImGui::GetStyle().ItemSpacing.x * 0.5f;
                    if (ImGui::Button("Démarrer", ImVec2(button_width, 0))) simulation_running = true;
                    ImGui::SameLine();
                    if (ImGui::Button("Step", ImVec2(button_width, 0))) {
                        monde->AvancerTemps();
                        cell_count_history.push_back(static_cast<float>(monde->getNombreCellulesVivantes()));
                        if (cell_count_history.size() > 500) {
                            cell_count_history.erase(cell_count_history.begin());
                        }
                    }
                }
            }

            ImGui::SeparatorText("Gestion de l'État");
            if (monde) {
                float button_width = ImGui::GetContentRegionAvail().x * 0.5f - ImGui::GetStyle().ItemSpacing.x * 0.5f;
                if (ImGui::Button("Sauvegarder", ImVec2(button_width, 0))) {
                    monde->SauvegarderEtat("simulation_state.sed");
                }
                ImGui::SameLine();
                if (ImGui::Button("Charger", ImVec2(button_width, 0))) {
                    simulation_running = false;
                    monde->ChargerEtat("simulation_state.sed");
                    cell_count_history.clear();
                }
            }

            ImGui::SeparatorText("Statistiques");
             if (monde) {
                ImGui::Text("Cycle: %d", monde->getCycleActuel());
                ImGui::Text("Graine (Seed): %u", monde->getSeed());
                ImGui::Text("Cellules vivantes: %d", monde->getNombreCellulesVivantes());
                if (!cell_count_history.empty()) {
                    ImGui::Text("Historique du nombre de cellules :");
                    ImGui::PlotLines("##cell_history", cell_count_history.data(), cell_count_history.size(), 0, nullptr, 0.0f, FLT_MAX, ImVec2(0, 80));
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Configuration")) {
            ImGui::SeparatorText("Taille du Monde");
             if (ImGui::InputInt3("Taille Grille", sim_size)) {
                sim_size[0] = std::max(1, sim_size[0]);
                sim_size[1] = std::max(1, sim_size[1]);
                sim_size[2] = std::max(1, sim_size[2]);
            }
            ImGui::SeparatorText("Conditions Initiales");
            ImGui::SliderFloat("Densité Initiale", &sim_density, 0.0f, 1.0f, "%.2f");

            ImGui::SeparatorText("Graine de Génération");
            ImGui::Checkbox("Graine Aléatoire", &use_random_seed);
            ImGui::BeginDisabled(use_random_seed);
            ImGui::InputInt("Graine", &sim_seed);
            ImGui::EndDisabled();
            if (ImGui::Button("Nouvelle Graine Aléatoire", ImVec2(-1, 0))) {
                sim_seed = time(NULL);
            }


            ImGui::SeparatorText("Performance");
            ImGui::SliderInt("Cycles par Frame", &sim_cycles_per_frame, 1, 100);
            ImGui::EndTabItem();
        }

        if (monde && ImGui::BeginTabItem("Paramètres")) {
             auto& params = monde->params;
            ImGui::SeparatorText("Loi 1: Mouvement");
            ImGui::SliderFloat("K_E (Attraction Énergie)", &params.K_E, 0.0f, 5.0f);
            ImGui::SliderFloat("K_D (Motivation Faim)", &params.K_D, 0.0f, 5.0f);
            ImGui::SliderFloat("K_C (Aversion Stress)", &params.K_C, 0.0f, 5.0f);
            ImGui::SliderFloat("K_M (Influence Mémoire)", &params.K_M, 0.0f, 5.0f);

            ImGui::SeparatorText("Loi 2: Division");
            ImGui::SliderFloat("Seuil Énergie Division", &params.SEUIL_ENERGIE_DIVISION, 0.1f, 5.0f);

            ImGui::SeparatorText("Loi 4: Échange Énergétique");
            ImGui::SliderFloat("Facteur Échange Énergie", &params.FACTEUR_ECHANGE_ENERGIE, 0.0f, 0.5f, "%.3f");
            ImGui::SliderFloat("Seuil Diff. Énergie", &params.SEUIL_DIFFERENCE_ENERGIE, 0.0f, 1.0f);
            ImGui::SliderFloat("Seuil Similarité R", &params.SEUIL_SIMILARITE_R, 0.0f, 1.0f);

            ImGui::SeparatorText("Loi 5: Interaction Psychique");
            ImGui::SliderFloat("Taux Augmentation Ennui", &params.TAUX_AUGMENTATION_ENNUI, 0.0f, 0.01f, "%.4f");
            ImGui::SliderFloat("Facteur Échange Psychique", &params.FACTEUR_ECHANGE_PSYCHIQUE, 0.0f, 0.5f);

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(10, GetScreenHeight() - 110), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(250, 100), ImGuiCond_Always);
    ImGui::Begin("Légende", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Couleur = Énergie (Bleu -> Rouge)");
    ImGui::Text("Taille = Charge (Petit -> Grand)");
    ImGui::End();

    // --- Fenêtre de l'Inspecteur de Cellule ---
    if (cell_is_selected) {
        ImGui::SetNextWindowSize(ImVec2(280, 250), ImGuiCond_FirstUseEver);
        ImGui::Begin("Inspecteur de Cellule", &cell_is_selected);
        if (monde) {
            int x = selected_cell_coords[0];
            int y = selected_cell_coords[1];
            int z = selected_cell_coords[2];
            // Crée une copie constante pour garantir la sécurité des threads.
            const Cellule cell = monde->getGrille()[x + y * monde->getTailleX() + z * monde->getTailleX() * monde->getTailleY()];

            ImGui::Text("Position: (%d, %d, %d)", x, y, z);
            ImGui::Separator();
            if (cell.is_alive) {
                ImGui::Text("État: Vivante");
                ImGui::ProgressBar(cell.E / 2.0f, ImVec2(-1, 0), "Énergie (E)");
                ImGui::ProgressBar(cell.D, ImVec2(-1, 0), "Dette Besoin (D)");
                ImGui::ProgressBar(cell.C / cell.Sc, ImVec2(-1, 0), "Charge Émo. (C)");
                ImGui::ProgressBar(cell.L, ImVec2(-1, 0), "Dette Stimulus (L)");
                ImGui::Separator();
                ImGui::Text("Âge (A): %d", cell.A);
                ImGui::Text("Mémoire (M): %.3f", cell.M);
                ImGui::Separator();
                ImGui::Text("Génétique");
                ImGui::Text("Résistance (R): %.3f", cell.R);
                ImGui::Text("Seuil Critique (Sc): %.3f", cell.Sc);
            } else {
                ImGui::Text("État: Morte");
            }
        }
        ImGui::End();
    }
}

#include <algorithm> // Pour std::max
#include <cmath>     // Pour fmod

void Draw3DVisualization(Camera& camera) {
    // --- Pre-calcul pour la normalisation ---
    float max_e = 0.0f;
    float max_c = 0.0f;
    const auto& grille = monde->getGrille();

    for (const auto& cellule : grille) {
        if (cellule.is_alive) {
            if (cellule.E > max_e) max_e = cellule.E;
            if (cellule.C > max_c) max_c = cellule.C;
        }
    }
    // Pour éviter la division par zéro
    if (max_e == 0.0f) max_e = 1.0f;
    if (max_c == 0.0f) max_c = 1.0f;


    // --- Rendu des cellules ---
    int size_x = monde->getTailleX();
    int size_y = monde->getTailleY();
    int size_z = monde->getTailleZ();

    for (int i = 0; i < grille.size(); ++i) {
        const auto& cellule = grille[i];
        if (cellule.is_alive) {
            int x = i % size_x;
            int y = (i / size_x) % size_y;
            int z = i / (size_x * size_y);
            Vector3 position = {(float)x, (float)y, (float)z};
            float radius = 0.1f + (cellule.C / max_c) * 0.5f;
            float hue = 240.0f - (cellule.E / max_e) * 240.0f;
            Color color = ColorFromHSV(hue, 0.85f, 0.95f);

            DrawSphere(position, radius, color);

            // --- Indicateur de Sélection ---
            if (cell_is_selected && selected_cell_coords[0] == x && selected_cell_coords[1] == y && selected_cell_coords[2] == z) {
                DrawSphereWires(position, radius + 0.05f, 8, 8, Color{255, 255, 255, 180});
            }
        }
    }
}
