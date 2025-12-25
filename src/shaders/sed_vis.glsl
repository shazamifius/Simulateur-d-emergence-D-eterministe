#version 430

// --- SED TITANIC VISUALIZATION SHADER ---
// Renders millions of cells directly from SSBO

// --- VERTEX SHADER ---
#if defined(VERTEX)

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal;

// Instancing: We don't use VertexAttribDivisor usually for pure SSBO approach
// We rely on gl_InstanceID

// Uniforms
uniform mat4 mvp;
uniform mat4 matModel; // Base model transform (if any)
uniform ivec3 u_Size;
uniform float u_Time;

// SSBO Layout (MUST MATCH EXACTLY)
struct Cell {
    uint Type_Alive; // 4
    float E;         // 4
    float D;         // 4
    float C;         // 4
    float L;         // 4
    float M;         // 4
    float R;         // 4
    float Sc;        // 4
    float P;         // 4
    float G;         // 4
    int Ref;         // 4
    float E_cost;    // 4
    float Weights[27]; // 108
    uint History;      // 4
    int Age;           // 4
    float Padding1;    // 4
    float Padding2;    // 4
};

layout(std430, binding = 0) readonly buffer GridIn {
    Cell cells[];
};

// Outputs to Fragment
out vec4 fragColor;
out vec3 fragNormal;
out vec3 fragPos;

// Utils
vec3 HSVtoRGB(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    int idx = gl_InstanceID;
    Cell c = cells[idx];
    
    // Decode Type/Alive
    uint type = c.Type_Alive & 7;
    bool alive = bool((c.Type_Alive >> 3) & 1);
    
    // Cull dead cells (degenerate triangle)
    if (!alive) {
        gl_Position = vec4(0.0/0.0); // NaNs usually cull? Or just clip.
        // Better: Put it behind camera
        // gl_Position = vec4(-99999, -99999, -99999, 1);
        // We will scale to 0.
        // This is inefficient VS execution but simplest without Compute Culling.
    }
    
    // Calculate Grid Position
    int x = idx % u_Size.x;
    int y = (idx / u_Size.x) % u_Size.y;
    int z = idx / (u_Size.x * u_Size.y);
    vec3 worldPos = vec3(x, y, z);
    
    // Colors & Size
    vec3 color = vec3(0.5); // Grey default
    float size = 0.0;
    
    if (alive) {
        size = 0.8;
        
        // Breathing
        float breath = 1.0 + 0.05 * sin(u_Time * 5.0 + x*0.1 + y*0.1);
        size *= breath;
        
        // --- COLOR LOGIC (Match CPU Renderer) ---
        float hue = 0.0;
        float sat = 0.0;
        float val = 1.0;
        
        if (type == 1) { // SOMA (Cyan)
            hue = 0.5 + (c.R * 0.1); // Cyan range 0.5
        } else if (type == 2) { // NEURON (Gold)
            hue = 0.12 + (c.R * 0.1); // Gold/Orange
            // Neuron Emission
             if (c.P > 0.1) {
                val += c.P * 2.0; // Glow
                size *= (1.0 + c.P * 0.5);
            }
        } else { // STEM
            hue = 0.3; // Green
            sat = 0.2;
        }
        
        // Stress -> White
        if (c.C > c.Sc * 0.9) {
            sat = 0.0;
            val = 2.0; // Bloom
        }

        if (type != 0) sat += 0.5;
        
        color = HSVtoRGB(vec3(hue, sat, min(val, 1.0)));
        if(val > 1.0) color += vec3(val - 1.0); // additive white
    }
    
    // Apply Transform
    // Model Matrix is Identity usually, we explicitly translate
    vec3 localPos = vertexPosition * size;
    vec3 finalPos = localPos + worldPos;
    
    fragNormal = vertexNormal;
    fragPos = finalPos; 
    fragColor = vec4(color, 1.0);
    
    // Cull dead logic application
    if (!alive) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0); // Clipped strictly? 
        // Just make size 0 effectively visually
        // Vertex shader still runs.
        // Ideally we use Indirect Drawing to skip dead cells.
        // For now, Scale 0 is fine.
        finalPos = worldPos; // No size
        // Force clip
        gl_Position = vec4(NAN); 
    } else {
        gl_Position = mvp * vec4(finalPos, 1.0);
    }
}

#endif

// --- FRAGMENT SHADER ---
#if defined(FRAGMENT)

in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragPos;

out vec4 finalColor;

void main() {
    // Simple Lighting
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(fragNormal), lightDir), 0.2); // ambient
    
    finalColor = vec4(fragColor.rgb * diff, fragColor.a);
}

#endif

