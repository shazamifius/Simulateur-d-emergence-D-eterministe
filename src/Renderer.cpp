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
      DrawCells(monde, selectedCoords, settings.cell_color_mode,
                camera.position);
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
Color GetCellColor(const Cellule &c, float timeVal, int mode) {
  if (mode == 1) { // ENERGY (Heat Map)
    // 0.0 (Blue) -> 2.0 (Red)
    float t = std::clamp(c.E / 2.0f, 0.0f, 1.0f);
    return ColorFromHSV(240.0f - (t * 240.0f), 0.8f, 0.9f);
  } else if (mode == 2) { // STRESS (Heat Map)
    // 0.0 (White) -> 1.0 (Red)
    float t = std::clamp(c.C, 0.0f, 1.0f);
    return ColorFromHSV(0.0f, t,
                        1.0f); // Red Hue, Saturation increases with stress
  } else if (mode == 3) {      // GRADIENT (Heat Map)
    // 0.0 (Black) -> 1.0 (Green)
    float t = std::clamp(c.G, 0.0f, 1.0f);
    return ColorFromHSV(120.0f, 1.0f, t);
  }

  // MODE 0: DEFAULT CHROMADYNAMIQUE
  // 1. BASE IDENTITAIRE (Teinte / Hue)
  float hue = 0.0f;
  float saturation = 0.0f;
  float value = 1.0f;

  if (c.T == 3) { // BEDROCK (Static Floor) -> Dark Gray
    hue = 0.0f;
    saturation = 0.0f;
    value = 0.3f;        // Dark gray
  } else if (c.T == 1) { // SOMA (Structure) -> Cyan / Blue
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

void Renderer::DrawCells(MondeSED *monde, int selectedCoords[3], int colorMode,
                         Vector3 camPos) {
  // const auto &grille = monde->getGrille(); // REMOVED
  int sx = monde->getTailleX(); // Used for index hacks previously, now
                                // irrelevant or kept for UI
  int sy = monde->getTailleY();
  float timeVal = (float)GetTime(); // Raylib time

  const auto &chunks = monde->getWorldMap().GetAllChunks();
  for (const auto &pair : chunks) {
    const Chunk *chk = pair.second.get();
    if (!chk)
      continue;

    int base_x = chk->cx * CHUNK_SIZE;
    int base_y = chk->cy * CHUNK_SIZE;
    int base_z = chk->cz * CHUNK_SIZE;

    for (int i = 0; i < CHUNK_VOLUME; ++i) {
      const auto &c = chk->cells[i];
      if (!c.is_alive)
        continue;

      // Reconstruct local coords from index i
      int lz = i / (CHUNK_SIZE * CHUNK_SIZE);
      int temp = i % (CHUNK_SIZE * CHUNK_SIZE);
      int ly = temp / CHUNK_SIZE;
      int lx = temp % CHUNK_SIZE;

      int x = base_x + lx;
      int y = base_y + ly;
      int z = base_z + lz;
      Vector3 pos = {(float)x, (float)y, (float)z};

      // Opti: Frustum Culling (Simple Dist Check for now)
      // We do LOD based on distance
      float distSq = Vector3DistanceSqr(pos, camPos);
      bool useLOD = (distSq > 5000.0f); // >70 units away approx

      // --- VITALITE (Taille / Size) ---
      // Size = Base * min(1, E/Seuil) * (1 - D/MaxD)
      float baseSize = 0.7f; // "Sphere pulsante" - cube for now

      // Breathing effect (small pulse) regardless of state, creates "life"
      // Optimization: Skip breath math for far objects
      float breath = 1.0f;
      if (!useLOD) {
        breath =
            1.0f + (0.05f * std::sin(timeVal * 2.0f + (x * 0.1f + y * 0.1f)));
      }

      float energyFactor =
          std::min(1.0f, c.E / 1.8f); // 1.8 is typically division threshold
      float debtFactor =
          std::max(0.0f, 1.0f - (c.D / 5.0f)); // Assume MaxD around 5

      float finalSize = baseSize * energyFactor * debtFactor * breath;
      finalSize = std::clamp(finalSize, 0.1f, 0.95f);

      // --- COLOR CALCULATION ---
      Color finalCol = GetCellColor(c, timeVal, colorMode);

      // --- URGENCE (Glitch / Vibration) ---
      // Jitter position if stressed (Close up only)
      if (!useLOD && c.C > 0.5f) {
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
        // LOD: Skip Wires for far objects
        if (!useLOD) {
          // Optional: Only draw wires if closer? Or simplify
          // Original code didn't draw wires for normal cells, only Selected or
          // Glow
        }
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
          DrawCube(pos, boomSize, boomSize, boomSize,
                   ColorAlpha(bloomCol, 0.4f));
        } else {
          // Low glow
          // LOD: Skip wire glow if far
          if (!useLOD) {
            Color glowCol = {255, 215, 0, 50}; // Gold weak
            DrawCubeWires(pos, finalSize * 1.2f, finalSize * 1.2f,
                          finalSize * 1.2f, glowCol);
          }
        }
      }
    } // End Cell Loop
  } // End Chunk Loop
}

void Renderer::DrawNetwork(MondeSED *monde) {
  // const auto &grille = monde->getGrille();
  int sx = monde->getTailleX(); // For wrap logic if needed, but Infinite World
                                // usually no wrap?
  int sy = monde->getTailleY();
  int sz = monde->getTailleZ();

  rlBegin(RL_LINES);

  const auto &chunks = monde->getWorldMap().GetAllChunks();
  for (const auto &pair : chunks) {
    const Chunk *chk = pair.second.get();
    if (!chk)
      continue;

    int base_x = chk->cx * CHUNK_SIZE;
    int base_y = chk->cy * CHUNK_SIZE;
    int base_z = chk->cz * CHUNK_SIZE;

    for (int i = 0; i < CHUNK_VOLUME; ++i) {
      const auto &c = chk->cells[i];
      if (!c.is_alive || c.T != 2)
        continue;

      int lz = i / (CHUNK_SIZE * CHUNK_SIZE);
      int temp = i % (CHUNK_SIZE * CHUNK_SIZE);
      int ly = temp / CHUNK_SIZE;
      int lx = temp % CHUNK_SIZE;

      int x = base_x + lx;
      int y = base_y + ly;
      int z = base_z + lz;
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

            // Infinite World: No wrap
            // Check neighbor existence via safe accessor
            const Cellule &nC = monde->getCellule(nx, ny, nz);

            if (nC.is_alive && nC.T == 2) {
              // Determine Weight Index logic matching
              // GetCoordsVoisins_Optimized
              int w_idx = (dz + 1) * 9 + (dy + 1) * 3 + (dx + 1);

              // Note: W is on the SOURCE cell (c), pointing TO neighbor (nC)
              if (c.W[w_idx] > 0.1f) {
                float weight = c.W[w_idx];
                float alpha = std::clamp(weight, 0.0f, 1.0f);

                rlColor4ub(100, 200, 255, (unsigned char)(alpha * 200));
                rlVertex3f(posA.x, posA.y, posA.z);
                rlVertex3f((float)nx, (float)ny, (float)nz);
              }
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
        if (monde->getCellule(x, y, z).is_alive)
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

              const auto &other = monde->getCellule(nx, ny, nz);
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
