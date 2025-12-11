#version 430

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

// --- Constants (Matches C++ ParametresGlobaux) ---
const float K_THERMO = 0.001;
const float D_PER_TICK = 0.002;
const float L_PER_TICK = 0.001;

const float K_D = 1.0;
const float K_C = 0.5;
const float K_M = 0.5;
const float K_ADH = 0.5;
const float COUT_MOUVEMENT = 0.01;

const float K_D = 1.0;
const float K_C = 0.5;
const float K_M = 0.5;
const float K_ADH = 0.5;
const float COUT_MOUVEMENT = 0.01;
const float K_CHAMP_E = 1.0;
const float K_CHAMP_C = 1.0;
const float ALPHA_ATTENUATION = 1.0;

const float SEUIL_SOMA = 0.3;
const float SEUIL_NEURO = 0.7;

const float SEUIL_FIRE = 0.8;
const float COUT_SPIKE = 0.005;
const int PERIODE_REFRACTAIRE = 2;

const float SEUIL_ENERGIE_DIVISION = 2.0; // E must be > this to divide
const float COUT_DIVISION = 1.0;


// --- Structures ---

struct Cellule {
    uint Type_Alive; // Bits: [0-2] Type, [3] Alive, [4..31] Reserved
    float E;
    float D;
    float C;
    float L;
    float M;
    float R;
    float Sc;
    float P;      // Current Potential
    float P_next; // Double buffer for logic (Not strictly needed if ping-ponging buffers, but useful for internal state)
    float Weights[27]; 
    uint History;
    float Gradient;
    int Ref;
    float E_cost;
    int Age;
    float Padding; // Alignment
};

// SSBOs
layout(std430, binding = 0) buffer GridIn {
    Cellule cells_in[];
};

layout(std430, binding = 1) buffer GridOut {
    Cellule cells_out[];
};

layout(std430, binding = 2) buffer Params {
    float time;
    float seed;
    // Dynamic params can be added here
};

uniform ivec3 u_Size;

// --- Helpers ---

uint getIndex(ivec3 p) {
    return p.x + p.y * u_Size.x + p.z * u_Size.x * u_Size.y;
}

bool isValid(ivec3 p) {
    return p.x >= 0 && p.x < u_Size.x && p.y >= 0 && p.y < u_Size.y && p.z >= 0 && p.z < u_Size.z;
}

float random(vec3 st) {
    return fract(sin(dot(st.xyz, vec3(12.9898,78.233,45.5432))) * 43758.5453123);
}

// Deterministic Pseudo-Random based on inputs
float deterministic_hash(ivec3 p, int seed_offset) {
    vec3 p3 = fract(vec3(p) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

// Find Best Free Neighbor (Deterministic scan order)
// Returns index relative to 0 (0..26) or -1 if none
// Criteria: Deterministic "Preference" (e.g. density gradient or just deterministic hash)
ivec3 find_best_child_pos(ivec3 pos, float parent_E) {
    if (parent_E <= SEUIL_ENERGIE_DIVISION) return ivec3(-999);
    
    // We want a deterministic target. 
    // Scan all neighbors, pick the first EMPTY one that satisfies "Projected" constraints
    // Or closer to barycenter? Let's use Hash for variety but stability.
    
    ivec3 best_pos = ivec3(-999);
    float best_score = -1.0;
    
    for (int dz = -1; dz <= 1; dz++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                
                ivec3 n_pos = pos + ivec3(dx, dy, dz);
                if (!isValid(n_pos)) continue;
                
                // Check if empty in READ buffer
                uint n_idx = getIndex(n_pos);
                Cellule n_c = cells_in[n_idx];
                bool n_alive = bool((n_c.Type_Alive >> 3) & 1);
                
                if (!n_alive) {
                    // Candidate. Score it.
                    float score = deterministic_hash(n_pos, int(pos.x)); // Simple hash
                    if(score > best_score) {
                        best_score = score;
                        best_pos = n_pos;
                    }
                }
            }
        }
    }
    return best_pos;
}

// --- Movement Logic ---

// Score for moving FROM src TO dst
// Based on CPU 'AppliquerLoiMouvement'
float calculate_move_score(ivec3 src_pos, Cellule src, ivec3 dst_pos) {
    // 1. Gravity (Dette)
    float gravity = K_D * src.D; // Direction? Ideally D implies vector. Here D is scalar. 
    // CPU logic uses D as scalar weighting for... ? Actually CPU Logic was complex. 
    // Simplified GPU Logic: Move towards lower Gradient? Or Random?
    // Let's rely on Field Attraction.
    
    // Field Calculation at DST
    float bonus_champ_E = 0.0;
    float bonus_champ_C = 0.0;
    int count_adh = 0;
    
    int r_field = 2; // Fixed radius
    for (int dz = -r_field; dz <= r_field; dz++) {
        for (int dy = -r_field; dy <= r_field; dy++) {
             for (int dx = -r_field; dx <= r_field; dx++) {
                  if(dx==0 && dy==0 && dz==0) continue;
                  ivec3 n_pos = dst_pos + ivec3(dx, dy, dz);
                  if(!isValid(n_pos)) continue;
                  
                  // Read Field from neighbors
                  uint n_idx = getIndex(n_pos);
                  Cellule n_c = cells_in[n_idx];
                  if(bool((n_c.Type_Alive >> 3) & 1)) {
                       float dist = length(vec3(dx, dy, dz));
                       float infl = exp(-ALPHA_ATTENUATION * dist);
                       bonus_champ_E += n_c.E * infl;
                       bonus_champ_C += n_c.C * infl;
                       
                       // Adhesion (Radius 1)
                       if(dist < 1.5 && (n_c.Type_Alive & 7) == (src.Type_Alive & 7)) {
                           count_adh++;
                       }
                  }
             }
        }
    }
    float field_score = (K_CHAMP_E * bonus_champ_E) - (K_CHAMP_C * bonus_champ_C);
    float adh_score = K_ADH * float(count_adh);
    
    // Deterministic Randomness based on Coord Pair to break symmetries
    float rand_tie = deterministic_hash(dst_pos, int(src.E*100)); 
    
    return field_score + adh_score + rand_tie;
}

// Find best target to move TO (Push)
ivec3 find_best_move_target(ivec3 pos, Cellule c) {
    ivec3 best_pos = ivec3(-999);
    float best_score = -1e20; // Corrected from -99999.0
    
    for (int dz = -1; dz <= 1; dz++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                ivec3 n_pos = pos + ivec3(dx, dy, dz);
                if (!isValid(n_pos)) continue;
                
                // Target must be empty
                 uint n_idx = getIndex(n_pos);
                 Cellule n_c = cells_in[n_idx];
                 if(!bool((n_c.Type_Alive >> 3) & 1)) {
                     float score = calculate_move_score(pos, c, n_pos);
                     if(score > best_score) {
                         best_score = score;
                         best_pos = n_pos;
                     }
                 }
            }
        }
    }
    return best_pos;
}

// Find best mover who wants to enter ME (Pull)
int find_best_incoming_mover(ivec3 my_pos) {
    int best_idx = -1;
    // We need to know who wants to move HERE.
    // Check all neighbors. If neighbor is Alive, calculate WHERE it wants to go.
    // If it wants to go HERE, consider it. 
    // If multiple want to go here, Resolve (e.g. highest E or random).
    
    float best_priority = -1.0;
    
    for (int dz = -1; dz <= 1; dz++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0 && dz == 0) continue;
                ivec3 n_pos = my_pos + ivec3(dx, dy, dz);
                if (!isValid(n_pos)) continue;
                
                uint n_idx = getIndex(n_pos);
                Cellule n_c = cells_in[n_idx];
                if(bool((n_c.Type_Alive >> 3) & 1)) {
                    // It is alive. Where does it want to go?
                     ivec3 target = find_best_move_target(n_pos, n_c);
                     if(target == my_pos) {
                         // It chose me!
                         // Tie-breaker: Highest Energy wins
                         if(n_c.E > best_priority) {
                             best_priority = n_c.E;
                             best_idx = int(n_idx);
                         }
                     }
                }
            }
        }
    }
    return best_idx;
}

// --- Main Logic ---

void main() {
    ivec3 pos = ivec3(gl_GlobalInvocationID.xyz);
    if (!isValid(pos)) return;
    
    uint idx = getIndex(pos);
    Cellule c = cells_in[idx]; // Read State
    
    // Unpack Type/Alive
    bool is_alive = bool((c.Type_Alive >> 3) & 1);
    uint type = c.Type_Alive & 7;
    
    if (!is_alive) {
        // I am empty. Do I become alive? (Division Reception)
        int parent_idx = find_parent_that_chose_me(pos);
        if (parent_idx != -1) {
            // Born!
            Cellule parent = cells_in[parent_idx];
            c.Type_Alive = parent.Type_Alive; // Inherit Type/Alive
            c.E = parent.E / 2.0;
            c.Age = 0;
            // c.R = parent.R + mutation... (Skip mutation for speed)
        } else {
             // 2. Incoming Movement check
             int mover_idx = find_best_incoming_mover(pos);
             if(mover_idx != -1) {
                 // I am being invaded (Move)
                 Cellule mover = cells_in[mover_idx];
                 c = mover; // Copy state
                 c.E -= COUT_MOUVEMENT; // Pay cost
             }
        }
        cells_out[idx] = c; // Write result
        return;
    }
    
    // --- Loi 1: Gradient & Morphogenese ---
    vec3 center = vec3(u_Size) / 2.0; // Simplification: Center is grid center
    float dist = distance(vec3(pos), center);
    c.Gradient = exp(-0.1 * dist);
    
    if (type == 0) { // Souche
        if (c.Gradient < SEUIL_SOMA) type = 1; // Soma
        else if (c.Gradient >= SEUIL_NEURO) type = 2; // Neuro
    }
    
    // --- Loi 7 Part 2: Energy Source ---
    // Simple Central Source + Ambient
    float dist_center = distance(vec3(pos), vec3(u_Size)/2.0);
    float source = 0.01 * exp(-dist_center / (float(u_Size.x)*0.25));
    c.E = min(c.E + source, 2.0); // Cap at 2.0
    
    // --- Loi 7: Metabolisme ---
    c.D += D_PER_TICK;
    c.L += L_PER_TICK;
    c.E -= K_THERMO;
    c.Age++;
    
    // --- Loi 3: Neural Dynamics (If Neuro) ---
    if (type == 2) {
        float sum_input = 0.0;
        float sum_w = 0.0;
        
        // Neighbor Loop (3x3x3)
        for (int dz = -1; dz <= 1; dz++) {
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0 && dz == 0) continue;
                    
                    ivec3 n_pos = pos + ivec3(dx, dy, dz);
                    if (isValid(n_pos)) {
                        uint n_idx = getIndex(n_pos);
                        Cellule n_cell = cells_in[n_idx];
                        
                        // Weight Index map (simpler flat mapping)
                        int w_idx = (dz+1)*9 + (dy+1)*3 + (dx+1);
                        float w = c.Weights[w_idx];
                        
                        if (w > 0.0) {
                            sum_input += n_cell.P * w;
                            sum_w += w;
                        }
                    }
                }
            }
        }
        
        float I = (sum_w > 0.0) ? (sum_input / max(1.0, sum_w)) : 0.0;
        
        if (c.Ref > 0) {
            c.Ref--;
            c.P = 0.0;
        } else {
            c.P = (c.P * 0.9) + I + (random(vec3(pos) + time) * 0.1 - 0.05); // Noise
            if (c.P > SEUIL_FIRE) {
                c.P = 1.0;
                c.Ref = PERIODE_REFRACTAIRE;
                c.E -= COUT_SPIKE;
                c.History = (c.History << 1) | 1;
            } else {
                 c.History = (c.History << 1);
            }
        }
    }
    
    // --- Loi 6: Division (Emission) ---
    // If I selected a child, I must pay the cost (halve energy)
    ivec3 child_pos = find_best_child_pos(pos, c.E);
    if (child_pos.x != -999) {
        // I have a valid child target.
        // I assume it will adhere to the contract and become alive.
        // So I pay.
        c.E /= 2.0;
    }
    
    // --- Loi 8: Mouvement (Emission) ---
    // If I moved, I must leave emptiness behind.
    // Check if I successfully moved.
    ivec3 move_target = find_best_move_target(pos, c);
    if(move_target.x != -999) {
         // I want to move.
         // Does the target accept me? (i.e. am I the best mover for it?)
         int winner_idx = find_best_incoming_mover(move_target);
         if(winner_idx == int(idx)) {
             // Valid move. I leave this cell.
             // Becomes Dead
             c.Type_Alive = 0;
             c.E = 0;
             // ...
         }
    }
    
    // --- Check Death ---
    if (c.E <= 0.0 || c.C > c.Sc) {
        c.Type_Alive = 0; // Dead
        c.E = 0;
        c.P = 0;
    } else {
        // Repack Type/Alive
        c.Type_Alive = (type & 7) | (1 << 3);
    }
    
    // Write Back
    cells_out[idx] = c;
}
