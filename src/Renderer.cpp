#include "Renderer.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>

// Static resources
static bool initialized = false;

void Renderer::Init() { initialized = true; }

void Renderer::Unload() { initialized = false; }

void Renderer::Render(MondeSED *monde, Camera3D &camera,
                      RenderTexture2D &target, int selectedCoords[3],
                      const RenderSettings &settings) {

  if (target.id == 0)
    return;

  BeginTextureMode(target);
  ClearBackground(Color{13, 17, 23, 255}); // #0d1117 GitHub Dark BG

  BeginMode3D(camera);

  // 1. Tech Grid (Gizmos)
  if (settings.show_gizmos) {
    DrawTechGrid(64, 1.0f);
    if (monde) {
      Vector3 dims = {(float)monde->getTailleX(), (float)monde->getTailleY(),
                      (float)monde->getTailleZ()};
      Vector3 center = Vector3Scale(dims, 0.5f);
      DrawCubeWiresV(center, dims, Color{48, 54, 61, 255});
    }
  }

  if (monde) {
    // 2. Force Fields (Invisible Math)
    if (settings.show_fields) {
      DrawFieldVectors(monde, settings.field_density);
    }

    // 3. Synaptic Web (Network)
    if (settings.show_network) {
      DrawNetwork(monde);
    }

    // 4. Bio-Data Particles (Cells)
    if (settings.show_cells) {
      DrawCells(monde, selectedCoords);
    }
  } else {
    DrawCube({0, 0, 0}, 2.0f, 2.0f, 2.0f, RED);
  }

  EndMode3D();
  EndTextureMode();
}

void Renderer::DrawTechGrid(int slices, float spacing) {
  rlBegin(RL_LINES);
  rlColor4ub(48, 54, 61, 255);

  int halfSlices = slices / 2;
  for (int i = -halfSlices; i <= halfSlices; i++) {
    if (i == 0)
      continue;
    float pos = (float)i * spacing;
    rlVertex3f(pos, 0.0f, (float)-halfSlices * spacing);
    rlVertex3f(pos, 0.0f, (float)halfSlices * spacing);
    rlVertex3f((float)-halfSlices * spacing, 0.0f, pos);
    rlVertex3f((float)halfSlices * spacing, 0.0f, pos);
  }
  rlEnd();
}

// --- Helper for Chromadynamique Visuals ---
Color GetCellColor(const Cellule &c, float timeVal) {
  // 1. BASE IDENTITAIRE (Teinte / Hue)
  float hue = 0.0f;
  float saturation = 0.0f;
  float value = 1.0f;

  if (c.T == 1) { // SOMA (Structure) -> Cyan / Blue
    // Base 190 (Cyan), Variation +R*40 (Towards Blue)
    hue = 190.0f + (c.R * 40.0f);
  } else if (c.T == 2) { // NEURONE (Calcul) -> Gold / Amber
    // Base 45 (Gold), Variation +R*30 (Towards Orange)
    hue = 45.0f + (c.R * 30.0f);
  } else { // SOUCHE (Potentiel) -> White / Pale Green
    // Base 120 (Green), low saturation usually
    hue = 120.0f;
    saturation = 0.2f; // Low sat for stem cells
  }

  // 2. ETAT PSYCHIQUE (Saturation / Memory)
  // M = Profondeur. High M = High Saturation.
  // Formula: Sat = 0.5 + (0.5 * tanh(M))
  // We assume M is roughly 0..5 or so. tanh(5) ~ 1.
  if (c.T != 0) {
    float memFactor = std::tanh(c.M);
    saturation = 0.5f + (0.5f * memFactor);
  }

  // 3. ENNUI (Oscillation / L)
  // L creates agitation (flicker).
  // Freq increases with L.
  if (c.L > 0.1f) {
    float freq = 5.0f + (c.L * 20.0f); // 5Hz to 25Hz
    float osc = std::sin(timeVal * freq);
    // Modulate value slightly
    value = 0.8f + (0.2f * osc);
  }

  // 4. DETTE (Dark Core indicator - approximated as darkening)
  // Darken if Debt is high.
  if (c.D > 0.1f) {
    // D ranges 0..1 typically, maybe higher.
    float darken = std::max(0.0f, 1.0f - (c.D * 0.5f));
    value *= darken;
  }

  // 5. STRESS (Glitch / C)
  // If C is high, shift Hue or turn White.
  // We treat severe stress as White Strobe.
  if (c.C > (c.Sc * 0.9f) && c.Sc > 0.0f) {
    // Critical Stress -> Strobe White
    float strobe = std::sin(timeVal * 50.0f);
    if (strobe > 0.0f)
      saturation = 0.0f; // White
  }

  return ColorFromHSV(hue, saturation, value);
}

void Renderer::DrawCells(MondeSED *monde, int selectedCoords[3]) {
  const auto &grille = monde->getGrille();
  int sx = monde->getTailleX();
  int sy = monde->getTailleY();
  float timeVal = (float)GetTime(); // Raylib time

  for (size_t i = 0; i < grille.size(); ++i) {
    const auto &c = grille[i];
    if (!c.is_alive)
      continue;

    int x = (int)i % sx;
    int y = ((int)i / sx) % sy;
    int z = (int)i / (sx * sy);
    Vector3 pos = {(float)x, (float)y, (float)z};

    // --- VITALITE (Taille / Size) ---
    // Size = Base * min(1, E/Seuil) * (1 - D/MaxD)
    float baseSize = 0.7f; // "Sphere pulsante" - cube for now

    // Breathing effect (small pulse) regardless of state, creates "life"
    float breath =
        1.0f + (0.05f * std::sin(timeVal * 2.0f + (x * 0.1f + y * 0.1f)));

    float energyFactor =
        std::min(1.0f, c.E / 1.8f); // 1.8 is typically division threshold
    float debtFactor =
        std::max(0.0f, 1.0f - (c.D / 5.0f)); // Assume MaxD around 5

    float finalSize = baseSize * energyFactor * debtFactor * breath;
    finalSize = std::clamp(finalSize, 0.1f, 0.95f);

    // --- COLOR CALCULATION ---
    Color finalCol = GetCellColor(c, timeVal);

    // --- URGENCE (Glitch / Vibration) ---
    // Jitter position if stressed
    if (c.C > 0.5f) {
      float jitterAmt = (c.C - 0.5f) * 0.2f;
      pos.x += ((float)GetRandomValue(-100, 100) / 1000.0f) * jitterAmt;
      pos.y += ((float)GetRandomValue(-100, 100) / 1000.0f) * jitterAmt;
      pos.z += ((float)GetRandomValue(-100, 100) / 1000.0f) * jitterAmt;
    }

    // --- RENDERING ---
    // Standard Draw
    if (x == selectedCoords[0] && y == selectedCoords[1] &&
        z == selectedCoords[2]) {
      // Selected
      DrawCube(pos, finalSize, finalSize, finalSize, WHITE);
      DrawCubeWires(pos, 1.1f, 1.1f, 1.1f, GREEN);
    } else {
      DrawCube(pos, finalSize, finalSize, finalSize, finalCol);
    }

    // --- ACTIVITE NEURALE (Emission / Bloom) ---
    // T=2 only. P determines emission.
    if (c.T == 2 && c.P > 0.05f) {
      // "Emission" = Additive drawing or larger halo
      float emission = std::min(c.P, 2.0f); // Cap at 2.0

      // Spike (Flash)
      if (emission > 0.9f) {
        // Massive Bloom
        Color bloomCol = {255, 255, 200, 150}; // Whitish Gold
        if (emission > 1.2f)
          bloomCol = WHITE; // Blinding
        float boomSize = finalSize * (1.0f + (emission * 0.5f));
        // Transparent shell implementation of bloom
        DrawCube(pos, boomSize, boomSize, boomSize, ColorAlpha(bloomCol, 0.4f));
      } else {
        // Low glow
        Color glowCol = {255, 215, 0, 50}; // Gold weak
        DrawCubeWires(pos, finalSize * 1.2f, finalSize * 1.2f, finalSize * 1.2f,
                      glowCol);
      }
    }
  }
}

void Renderer::DrawNetwork(MondeSED *monde) {
  const auto &grille = monde->getGrille();
  int sx = monde->getTailleX();
  int sy = monde->getTailleY();
  int sz = monde->getTailleZ();

  rlBegin(RL_LINES);
  // Optimization: Skip every N cells or limit distance?
  // Using crude loop for now as requested "High Perf Cost OK"

  for (size_t i = 0; i < grille.size(); ++i) {
    const auto &c = grille[i];
    if (!c.is_alive || c.T != 2)
      continue; // Only Neurons

    int x = (int)i % sx;
    int y = ((int)i / sx) % sy;
    int z = (int)i / (sx * sy);
    Vector3 posA = {(float)x, (float)y, (float)z};

    // Check neighbors for high weights
    // Neighbors logic matches MondeSED::GetCoordsVoisins (26 neighbors)
    // Simplified: Check standard 3x3x3 block
    for (int dz = -1; dz <= 1; dz++) {
      for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
          if (dx == 0 && dy == 0 && dz == 0)
            continue;

          int nx = x + dx;
          int ny = y + dy;
          int nz = z + dz;
          // Toroidal wrap (assuming periodic boundary)
          if (nx < 0)
            nx += sx;
          else if (nx >= sx)
            nx -= sx;
          if (ny < 0)
            ny += sy;
          else if (ny >= sy)
            ny -= sy;
          if (nz < 0)
            nz += sz;
          else if (nz >= sz)
            nz -= sz;

          int nIdx = monde->getIndex(nx, ny, nz);
          if (nIdx != -1) {
            // Find weight index (approximate, since W is flat array in struct)
            // Real implementation would need precise neighbor mapping.
            // For viz, we just check if neighbor is a connected neuron.
            const auto &nC = grille[nIdx];
            if (nC.is_alive && nC.T == 2) {
              // Check synapse strength (hypothetical accessor or average W)
              // We'll trust Hebbian result: if both active recently, draw line.
              float w = 0.5f; // Placeholder as W mapping is complex

              // Draw Line
              // Cyan for excitatory
              rlColor4ub(0, 255, 255, 40);
              rlVertex3f(posA.x, posA.y, posA.z);
              rlVertex3f((float)nx, (float)ny,
                         (float)nz); // Note: Wrapping artifacts might occur
            }
          }
        }
      }
    }
  }
  rlEnd();
}

void Renderer::DrawFieldVectors(MondeSED *monde, float density) {
  // Law 8: Action at a Distance.
  // We visualize the gradient of "Desire" at EMPTY spots.
  // Vector V = Attraction to Energy (High E) - Repulsion from Stress (High C)

  int sx = monde->getTailleX();
  int sy = monde->getTailleY();
  int sz = monde->getTailleZ();

  int step = (int)(1.0f / density);
  if (step < 1)
    step = 1;

  rlBegin(RL_LINES);

  for (int z = 0; z < sz; z += step) {
    for (int y = 0; y < sy; y += step) {
      for (int x = 0; x < sx; x += step) {
        int idx = monde->getIndex(x, y, z);
        if (monde->getGrille()[idx].is_alive)
          continue; // Skip occupied

        // Sample Fields (Simplified Physics)
        float attractE = 0.0f;
        float repelC = 0.0f;
        Vector3 dir = {0, 0, 0};

        // Scan local area (Radius 5)
        int rad = 4; // Spec says R_diff = 2, we look a bit further for visuals
        for (int dz = -rad; dz <= rad; dz++) {
          for (int dy = -rad; dy <= rad; dy++) {
            for (int dx = -rad; dx <= rad; dx++) {
              if (dx == 0 && dy == 0 && dz == 0)
                continue;

              int nx = (x + dx + sx) % sx;
              int ny = (y + dy + sy) % sy;
              int nz = (z + dz + sz) % sz;

              const auto &other =
                  monde->getGrille()[monde->getIndex(nx, ny, nz)];
              if (other.is_alive) {
                float distSq = (float)(dx * dx + dy * dy + dz * dz);
                float dist = sqrtf(distSq);

                // Influence falls off
                float influence = exp(-1.0f * dist); // Alpha=1.0 from spec

                Vector3 toCell = {(float)dx, (float)dy,
                                  (float)dz};             // Vector TO the cell
                Vector3 fromCell = Vector3Negate(toCell); // Vector AWAY

                // Attraction to Energy
                dir = Vector3Add(dir,
                                 Vector3Scale(toCell, (other.E * influence)));

                // Repulsion from Stress
                dir = Vector3Add(
                    dir, Vector3Scale(fromCell, (other.C * 5.0f * influence)));
              }
            }
          }
        }

        // Draw Vector
        float mag = Vector3Length(dir);
        if (mag > 0.1f) {
          Vector3 norm =
              Vector3Scale(Vector3Normalize(dir), 0.4f); // Length 0.4
          Vector3 start = {(float)x, (float)y, (float)z};
          Vector3 end = Vector3Add(start, norm);

          // Color by type of force
          // Pull (Energy) = Green/White
          // Push (Stress) = Red
          // We approximate:
          rlColor4ub(100, 255, 100, 50); // Faint line
          rlVertex3f(start.x, start.y, start.z);
          rlVertex3f(end.x, end.y, end.z);
        }
      }
    }
  }
  rlEnd();
  rlEnd();
}

static int titanic_vis_loc_size = -1;
static int titanic_vis_loc_time = -1;
static int titanic_vis_loc_model = -1;
static int titanic_vis_loc_mvp = -1;
static unsigned int titanic_vis_shader = 0;
static Mesh cubeMesh = {0};
static Material cubeMat = {0};

void Renderer::RenderTitanic(MondeSED *monde, Camera3D &camera,
                             RenderTexture2D &target,
                             const RenderSettings &settings) {
  if (target.id == 0 || !monde)
    return;

  // 1. Initialize Shader & Mesh (Lazy Initialization)
  // This ensures we only load GPU resources when actively trying to render
  if (titanic_vis_shader == 0) {
    if (!initialized)
      return;

    std::string vsCode = LoadFileText("src/shaders/sed_vis.glsl");
    std::string vs = "#version 430\n#define VERTEX\n" + vsCode;
    std::string fs = "#version 430\n#define FRAGMENT\n" + vsCode;

    Shader shd = LoadShaderFromMemory(vs.c_str(), fs.c_str());
    titanic_vis_shader = shd.id;

    titanic_vis_loc_size = GetShaderLocation(shd, "u_Size");
    titanic_vis_loc_time = GetShaderLocation(shd, "u_Time");
    titanic_vis_loc_model = GetShaderLocation(shd, "matModel");
    titanic_vis_loc_mvp = GetShaderLocation(shd, "mvp");

    cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    UploadMesh(&cubeMesh, false); // Important: Upload to VRAM to generate VAO

    cubeMat = LoadMaterialDefault();
    cubeMat.shader = shd;
  }

  // 2. Setup Frame
  BeginTextureMode(target);
  ClearBackground(Color{13, 17, 23, 255}); // Dark BG
  BeginMode3D(camera);

  // Tech Grid (Still needed?)
  if (settings.show_gizmos) {
    DrawTechGrid(64, 1.0f);
    Vector3 dims = {(float)monde->getTailleX(), (float)monde->getTailleY(),
                    (float)monde->getTailleZ()};
    Vector3 center = Vector3Scale(dims, 0.5f);
    DrawCubeWiresV(center, dims, Color{48, 54, 61, 255});
  }

  // 3. Draw Instanced
  // Lazy Init: Ensure GPU data is ready before rendering
  // InitTitanic checks gpu_initialized internally so it's safe to call.
  monde->InitTitanic();

  unsigned int ssbo = monde->GetSSBO();
  int count = monde->GetCellCount();

  if (ssbo > 0 && count > 0 && settings.show_cells) {
    rlEnableShader(titanic_vis_shader);

    // Bind SSBO to Binding 0
    rlBindShaderBuffer(ssbo, 0);

    // Uniforms
    int sizes[3] = {monde->getTailleX(), monde->getTailleY(),
                    monde->getTailleZ()};
    rlSetUniform(titanic_vis_loc_size, sizes, RL_SHADER_UNIFORM_IVEC3, 1);

    float t = (float)GetTime();
    rlSetUniform(titanic_vis_loc_time, &t, RL_SHADER_UNIFORM_FLOAT, 1);

    Matrix matModel = MatrixIdentity();
    Matrix matView = rlGetMatrixModelview();
    Matrix matProj = rlGetMatrixProjection();
    Matrix matMVP = MatrixMultiply(
        matView, matProj); // simplified, rlgl handles it typically

    // Send matrices? Raylib VS usually expects "mvp" or
    // "matModelViewProjection" Our custom shader asks for "mvp".
    rlSetUniformMatrix(titanic_vis_loc_mvp, matMVP);

    // Draw Mesh Instanced
    // Raylib doesn't expose DrawMeshInstanced easily for custom raw shader
    // pipeline? DrawMeshInstanced() exists in Raylib 5.0! But we want to use
    // OUR shader. DrawMeshInstanced uses the material's shader.

    // Let's use Material approach?
    // CubeMesh + Material(titanic_vis_shader).
    // However, we need to bind SSBO *before* drawing. DrawMeshInstanced does
    // not know about SSBO. So we MUST use rlEnableShader above, then
    // DrawMeshInstanced? DrawMeshInstanced sets the shader from material.

    // Workaround:
    // 1. Set Material Shader
    cubeMat.shader.id = titanic_vis_shader;
    cubeMat.shader.locs[SHADER_LOC_MATRIX_MVP] = titanic_vis_loc_mvp;
    cubeMat.shader.locs[SHADER_LOC_MATRIX_MODEL] = titanic_vis_loc_model;

    // 2. Bind SSBO globally
    rlBindShaderBuffer(ssbo, 0);

    // 3. Draw
    // Use rlgl wrappers where possible
    rlEnableVertexArray(cubeMesh.vaoId);

// glDrawElementsInstanced might not be exposed directly.
// We can use rlDrawRenderBatch if we were batching.
// Try raw call, but we need definitions if headers are missing.
// Raylib's rlgl.h should define standard GL headers.
// If undefined, we can define it:
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif
#ifndef GL_UNSIGNED_SHORT
#define GL_UNSIGNED_SHORT 0x1403
#endif

    // Use rlgl wrapper for instanced drawing which handles the GL call
    // Offset 0, Count = indices count, Buffer = 0 (bound in VAO), Instances =
    // cell count
    rlDrawVertexArrayElementsInstanced(0, cubeMesh.triangleCount * 3, 0, count);

    rlDisableVertexArray();
    rlBindShaderBuffer(0, 0); // Unbind
    rlDisableShader();
  }

  EndMode3D();
  EndTextureMode();
}
