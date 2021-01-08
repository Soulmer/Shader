#version 460

// LAYOUTS ----------
// Layout 0 - Vertex position
layout(std140, binding = 0) buffer Pos {
   vec4 Positions[ ];
};

// Layout 1 - Vertex velocity
layout(std140, binding = 1) buffer Vel {
    vec4 Velocities[ ];
};

/*
    TODO:
    noise function + particles burning out, based on mass (Velocities.w)
*/

// Input
layout (local_size_x = 16, local_size_y = 16) in;


// GLOBAL VARIABLES ----------
const vec3 gravity = vec3(0.0f, -0.0002f, 0.0f);
const float airResistance = 0.98f;

// FUNCTIONS ----------
vec3 enableGravity (vec3 pVel, float pArea) {
    return pVel += gravity * pArea;
};

vec3 enableAirResistance (vec3 pVel) {
    return pVel *= airResistance;
};

// UNIFORMS ----------
// Frame delta for calculations
uniform float dt;


// MAIN LOOP
void main() {

    // Current SSBO index
    uint index = gl_GlobalInvocationID.x;

    // Read data
    vec3 pPos = Positions[index].xyz;
    float pArea = Positions[index].w;
    vec3 pVel = Velocities[index].xyz;
    float pMass = Velocities[index].w;

    // Calculate new velocity
    pVel = enableAirResistance(pVel); 
    pVel = enableGravity(pVel, pArea);

    // Move by velocity
    pPos += (pVel) * dt;

    // Write data back
    Positions[index].xyz = pPos;
    Velocities[index].xyz = pVel;
}