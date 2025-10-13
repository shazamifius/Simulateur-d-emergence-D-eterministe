// --- Fichiers d'en-tête ---
#include "imgui.h"
#include "raylib.h"
#include "rlgl.h"
#include "rlImGui.h"
#include "MondeSED.h"
#include <iostream>
#include <memory>
#include <algorithm> // Pour std::max
#include <cmath>     // Pour fmod

// --- Variables globales ---
std::unique_ptr<MondeSED> monde;
bool simulation_running = false;

// --- Paramètres de simulation configurables via l'UI ---
static int sim_size[3] = {16, 16, 16};
static float sim_density = 0.1f;
static int sim_cycles_per_frame = 1;

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
    ImGui::StyleColorsDark();
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
        }
        UpdateCamera(&camera, CAMERA_ORBITAL);

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
    monde = std::make_unique<MondeSED>(sim_size[0], sim_size[1], sim_size[2]);
    monde->InitialiserMonde(sim_density);
    simulation_running = false;
}

void DrawUI() {
    ImGui::Begin("Contrôles", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Configuration");
    ImGui::Separator();
    if (ImGui::InputInt3("Taille Grille", sim_size)) {
        // Clamp values to be at least 1
        sim_size[0] = std::max(1, sim_size[0]);
        sim_size[1] = std::max(1, sim_size[1]);
        sim_size[2] = std::max(1, sim_size[2]);
    }
    ImGui::SliderFloat("Densité Initiale", &sim_density, 0.0f, 1.0f, "%.2f");
    ImGui::SliderInt("Cycles par Frame", &sim_cycles_per_frame, 1, 100);

    ImGui::Separator();

    if (monde) {
        ImGui::Text("Cycle: %d", monde->getCycleActuel());
        ImGui::Text("Cellules vivantes: %d", monde->getNombreCellulesVivantes());
    }

    ImGui::Separator();

    if (ImGui::Button("Initialiser/Réinitialiser", ImVec2(-1, 0))) {
        ResetSimulation();
    }

    if (monde) {
        if (simulation_running) {
            if (ImGui::Button("Pause", ImVec2(-1, 0))) {
                simulation_running = false;
            }
        } else {
            if (ImGui::Button("Démarrer", ImVec2(-1, 0))) {
                simulation_running = true;
            }
        }
    }

    if (monde && ImGui::CollapsingHeader("Paramètres Avancés des Lois")) {
        auto& params = monde->params;
        ImGui::SliderFloat("K_E (Attraction Énergie)", &params.K_E, 0.0f, 5.0f);
        ImGui::SliderFloat("K_D (Motivation Faim)", &params.K_D, 0.0f, 5.0f);
        ImGui::SliderFloat("K_C (Aversion Stress)", &params.K_C, 0.0f, 5.0f);
        ImGui::SliderFloat("Seuil Énergie Division", &params.SEUIL_ENERGIE_DIVISION, 0.1f, 5.0f);
        ImGui::SliderFloat("Facteur Échange Énergie", &params.FACTEUR_ECHANGE_ENERGIE, 0.0f, 0.5f, "%.3f");
        ImGui::SliderFloat("Seuil Diff. Énergie", &params.SEUIL_DIFFERENCE_ENERGIE, 0.0f, 1.0f);
        ImGui::SliderFloat("Seuil Similarité R", &params.SEUIL_SIMILARITE_R, 0.0f, 1.0f);
        ImGui::SliderFloat("Taux Augmentation Ennui", &params.TAUX_AUGMENTATION_ENNUI, 0.0f, 0.01f, "%.4f");
        ImGui::SliderFloat("Facteur Échange Psychique", &params.FACTEUR_ECHANGE_PSYCHIQUE, 0.0f, 0.5f);
        ImGui::SliderFloat("K_M (Influence Mémoire)", &params.K_M, 0.0f, 5.0f);
    }

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(10, GetScreenHeight() - 110), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(250, 100), ImGuiCond_Always);
    ImGui::Begin("Légende", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Couleur = Énergie (Bleu -> Rouge)");
    ImGui::Text("Taille = Charge (Petit -> Grand)");
    ImGui::End();
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
            // Recalculer les coordonnées 3D à partir de l'index 1D
            int x = i % size_x;
            int y = (i / size_x) % size_y;
            int z = i / (size_x * size_y);

            // Position de la sphère
            Vector3 position = {(float)x, (float)y, (float)z};

            // La taille est basée sur la Charge (C)
            float radius = 0.1f + (cellule.C / max_c) * 0.5f;

            // La couleur est basée sur l'Énergie (E)
            // Utilise une transition de couleur HSV (Teinte) de bleu (240) à rouge (0)
            float hue = 240.0f - (cellule.E / max_e) * 240.0f;
            Color color = ColorFromHSV(hue, 0.85f, 0.95f);

            DrawSphere(position, radius, color);
        }
    }
}