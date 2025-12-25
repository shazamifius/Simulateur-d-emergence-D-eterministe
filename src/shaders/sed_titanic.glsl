#version 430

// --- SED V8.0 TITANIC ENGINE ---
// "Double Clock" Architecture 
// Deterministic 3D Cellular Automata on GPU

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

// --- CONSTANTS (Must match CPU) ---
const float K_THERMO = 0.001;
const float D_PER_TICK = 0.002;
const float L_PER_TICK = 0.001;
const float K_D = 1.0;
const float K_C = 0.5;
const float K_M = 0.5;
const float K_ADH = 0.5;
const float COUT_MOUVEMENT = 0.01;
const float SEUIL_FIRE = 0.85;
const float COUT_SPIKE = 0.005;
const int PERIODE_REFRACTAIRE = 2;
const float SEUIL_ENERGIE_DIVISION = 1.8;
const float RAYON_DIFFUSION = 2.0;

// Pass Identifiers
const int PASS_NEURAL     = 0;
const int PASS_PHYSICS    = 1;
const int PASS_RESOLUTION = 2; // If needed, or merged

uniform int u_Pass; // controlled by CPU
uniform ivec3 u_Size;
uniform float u_Time;
uniform int u_Tick;

// --- DATA STRUCTURES ---

// Aligned to 16 bytes for std430 stability
struct Cell {
    uint Type_Alive; // [0-2]=Type, [3]=Alive
    float E;         // Energie
    float D;         // Dette
    float C;         // Stress
    
    float L;         // Ennui
    float M;         // Memoire
    float R;         // Genetic R
    float Sc;        // Seuil Critique
    
    float P;         // Potentiel
    float G;         // Gradient
    int Ref;         // Refractaire
    float E_cost;    // Cout accumulé
    
    float Weights[27]; // Synapses (Linearized)
    uint History;      // 32-bit history
    int Age;           // Age
    float Padding1;    // Pad to align if needed
    float Padding2;
};

// --- BUFFERS ---
layout(std430, binding = 0) readonly buffer GridIn {
    Cell cells_in[];
};

layout(std430, binding = 1) buffer GridOut {
    Cell cells_out[];
};

// --- HELPERS ---

uint getIndex(ivec3 p) {
    return p.x + p.y * u_Size.x + p.z * u_Size.x * u_Size.y;
}

bool isValid(ivec3 p) {
    return p.x >= 0 && p.x < u_Size.x && p.y >= 0 && p.y < u_Size.y && p.z >= 0 && p.z < u_Size.z;
}

// Deterministic RNG (xxHash style variant for float)
float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float random(ivec3 p, int seed_offset) {
    return hash(vec3(p) + float(seed_offset) * 0.123);
}

// --- LOGIC: NEURAL (Time Fast) ---
void Kernel_Neural(ivec3 pos, uint idx) {
    Cell c = cells_in[idx];
    
    // Only process Neurons (Type=2)
    uint type = c.Type_Alive & 7;
    bool alive = bool((c.Type_Alive >> 3) & 1);
    
    if (!alive || type != 2) {
        // Pass-through non-neurons
        // But wait, if it's dead, P should be 0?
        if(!alive) c.P = 0.0;
        cells_out[idx] = c; 
        return;
    }

    // Loi 3: Integration
    if (c.Ref > 0) {
        c.Ref--;
        c.P = -0.1; // Inhibited
        c.History = (c.History << 1); // 0
    } else {
        float sum_input = 0.0;
        float sum_w = 0.0;
        
        // Scan 3x3 neighbors
        int w_idx = 0;
        for(int dz=-1; dz<=1; ++dz) {
            for(int dy=-1; dy<=1; ++dy) {
                for(int dx=-1; dx<=1; ++dx) {
                    w_idx++; // 1-27
                    if(dx==0 && dy==0 && dz==0) continue;
                    
                    ivec3 n_pos = pos + ivec3(dx, dy, dz);
                    if(isValid(n_pos)) {
                        Cell n_c = cells_in[getIndex(n_pos)];
                        bool n_alive = bool((n_c.Type_Alive >> 3) & 1);
                        // Access linearized weight (w_idx-1 because self is skipped in loop logic or just simplistic mapping)
                        // Mapping: Standard 3x3x3 order
                        int flat_w = (dz+1)*9 + (dy+1)*3 + (dx+1); 
                        float w = c.Weights[flat_w];
                        
                        if(n_alive && w > 0.0) {
                            sum_input += n_c.P * w;
                            sum_w += w;
                        }
                    }
                }
            }
        }
        
        float I = (sum_w > 0.0) ? (sum_input / sum_w) : 0.0;
        float noise = (random(pos, u_Tick) - 0.5) * 0.01;
        
        c.P = (c.P * 0.9) + I + noise;
        
        // Spike?
        if (c.P > SEUIL_FIRE) {
            c.P = 1.0;
            c.Ref = PERIODE_REFRACTAIRE;
            c.E_cost += COUT_SPIKE; // Accumulate cost
            c.History = (c.History << 1) | 1;
        } else {
            c.History = (c.History << 1);
        }
    }
    
    // Pass 1 Ignition Broadcast is skipped for simplicity or done in next pass?
    // Doing it here requires READ from P_out which is impossible in same pass.
    // So Ignition usually requires a second sub-pass or assumed negligible for huge grids.
    // Spec says: "If P is Spike, neighbors get +0.1". 
    // This implies immediate effect. 
    // We will stick to local integration for now.
    
    cells_out[idx] = c;
}

// --- LOGIC: PHYSICS (Time Slow) ---

// Eval Move Score for Source -> Dest
float GetMoveScore(Cell src, ivec3 src_p, ivec3 dst_p, Cell dst_placeholder) {
    // Basic scoring
    // Field scan at DST
    float bonus_field = 0.0; // Simplify field for perf
    
    // Gravity: Pulls towards lower Debt? No, D is "Need". Higher D = Higher Priority.
    // Movement logic: Score = K_D * D ...
    // We simplify: A cell wants to move to a location that maximizes its survival.
    // Here we just use a "Desire to Move" based on Gradient ??
    
    // Adhesion
    // ...
    
    // For Titanic: simplified "Random Walk + Gradient"
    // Gravity (D) -> pushes slightly Randomly
    // Gradient (G) -> pushes towards/away center
    
    return src.D * 10.0; // Dummy implementation TODO: Full Field logic
}

// The "Reciprocal Pull" Function
// Returns the index of the best neighbor that wants to move into 'target_pos'
// Returns -1 if no one wants to enter/can enter
int GetBestIncoming(ivec3 target_pos) {
    int best_idx = -1;
    float best_val = -1.0;
    
    // Check all neighbors
    for(int dz=-1; dz<=1; ++dz) {
        for(int dy=-1; dy<=1; ++dy) {
            for(int dx=-1; dx<=1; ++dx) {
                if(dx==0 && dy==0 && dz==0) continue;
                ivec3 n_pos = target_pos + ivec3(dx, dy, dz);
                if(isValid(n_pos)) {
                    uint n_idx = getIndex(n_pos);
                    Cell n_c = cells_in[n_idx];
                    bool n_alive = bool((n_c.Type_Alive >> 3) & 1);
                    
                    if(n_alive) {
                         // Does 'n_c' want to come here?
                         // "Want" is defined as: `Score(n -> target) > Score(n -> any_other)`
                         // This is expensive to compute fully.
                         
                         // TITANIC OPTIMIZATION: 
                         // "Greedy Local": I compute my score to go to 'target_pos'.
                         // If it's high enough, I propose.
                         
                         // Simple check: Is target empty? Yes (assumed by caller context usually, but we check all)
                         // Prioritize D (Dette/Faim)
                         if(n_c.D > best_val) {
                             best_val = n_c.D;
                             best_idx = int(n_idx);
                         }
                    }
                }
            }
        }
    }
    return best_idx;
}

void Kernel_Physics(ivec3 pos, uint idx) {
    Cell c = cells_in[idx];
    
    // 1. Metabolism (Always happens)
    if (bool((c.Type_Alive >> 3) & 1)) {
        c.E -= c.E_cost + K_THERMO;
        c.E_cost = 0.0;
        c.D += D_PER_TICK;
        c.L += L_PER_TICK;
        c.Age++;
        
        // Death check
        if (c.E <= 0.0 || c.C > c.Sc) {
            c.Type_Alive = 0;
            c.E = 0.0;
            c.P = 0.0;
        }
    }
    
    // 2. Movement (Pull Model)
    // Am I empty?
    bool alive = bool((c.Type_Alive >> 3) & 1);
    
    if (!alive) {
        // Try to pull a neighbor
        int best_in = GetBestIncoming(pos);
        if (best_in != -1) {
            Cell mover = cells_in[best_in];
            // Verify mover is actually wanting to move? 
            // For now, if D is high, they move.
            // Move happens!
            c = mover;
            c.E -= COUT_MOUVEMENT;
        }
    } else {
        // I am alive. Do I move out?
        // Check if I am the "Best Incoming" for any of MY empty neighbors
        // If I am the best for neighbor X, then I move to X.
        // BUT, what if I am best for X and Y? We need a deterministic choice.
        // AND, what if I move to X, I must leave my spot Empty.
        
        // Check my neighbors for openings
        ivec3 best_target = ivec3(-999);
        float best_t_score = -1.0;
        
        // Scan for empty spots
        for(int dz=-1; dz<=1; ++dz) {
            for(int dy=-1; dy<=1; ++dy) {
                for(int dx=-1; dx<=1; ++dx) {
                     if(dx==0 && dy==0 && dz==0) continue;
                     ivec3 n_pos = pos + ivec3(dx, dy, dz);
                     if(isValid(n_pos)) {
                         Cell n_c = cells_in[getIndex(n_pos)];
                         if(!bool((n_c.Type_Alive >> 3) & 1)) {
                             // Empty spot.
                             // Would I be the winner there? 
                             // Optimized: Just check if I beat the current 'best_val' of that spot?
                             // Re-run 'GetBestIncoming' logic for that spot partially?
                             int winner_idx = GetBestIncoming(n_pos);
                             if (winner_idx == int(idx)) {
                                 // I won that spot!
                                 // Mark as moving out.
                                 // If multiple spots won? Deterministic ordering (loop order).
                                 // So I take the first one I win.
                                 best_target = n_pos;
                                 goto decided_move;
                             }
                         }
                     }
                }
            }
        }
        decided_move:
        
        if(best_target.x != -999) {
            // I moved out.
            c.Type_Alive = 0; // Become empty
            c.E = 0.0;
        }
    }
    
    // 3. Gradient
    vec3 center = vec3(u_Size) / 2.0;
    c.G = exp(-0.1 * distance(vec3(pos), center));
    
    cells_out[idx] = c;
}

void main() {
    ivec3 pos = ivec3(gl_GlobalInvocationID.xyz);
    if (!isValid(pos)) return;
    uint idx = getIndex(pos);
    
    if (u_Pass == PASS_NEURAL) {
        Kernel_Neural(pos, idx);
    } else if (u_Pass == PASS_PHYSICS) {
        Kernel_Physics(pos, idx);
    }
}
