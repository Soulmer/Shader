#version 460

// LAYOUTS ----------
// Layout 0 - Positions
layout(std140, binding = 0) buffer Pos {
   vec4 Positions[ ];
};

// Layout 1 - Velocities
layout(std140, binding = 1) buffer Vel {
    vec4 Velocities[ ];
};

// Input
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// GLOBAL VARIABLES ----------
const vec3 gravity = vec3(0.0f, -0.26f, 0.0f);
const float dragCoefficient = 0.47f;
const float particleDensity = 5.0f;
const float PI = 3.141592653589;

// FUNCTIONS ----------
vec3 enableGravity (vec3 pVel, float dt) {
    return pVel += gravity * dt;
};

vec3 enableDrag (vec3 pVel, float pMass, float dt) {
    return pVel -= (dragCoefficient / pMass) * pVel * dt;
};

float enableBurningOut (float pMass, float pTex, float dt) {
    return pTex -= PI * pMass * pTex * dt;
};


// UNIFORM ----------
// Frame delta for calculations
uniform float dt;

// MAIN ----------
void main() {

    // Current SSBO index
    uint index = gl_GlobalInvocationID.x;

    // Read data
    vec3 pPos = Positions[index].xyz;
    float pTex = Positions[index].w;

    vec3 pVel = Velocities[index].xyz;
    float pMass = Velocities[index].w;

    // Calculate new velocity
    pVel = enableDrag(pVel, pMass, dt); 
    pVel = enableGravity(pVel, dt);

    // Enable transparency change
    pTex = enableBurningOut(pMass, pTex, dt);

    // Move particle by velocity
    pPos += pVel * dt;

    // Write data back
    // Positions.xyz -> vector with x, y, z coordinates of a single particle
    Positions[index].xyz = pPos;
    // Positions.w -> particle transparency
    Positions[index].w = pTex;
    // Velocities.xyz -> velocity vector of a single particle
    Velocities[index].xyz = pVel;
    // Velocities.w -> particle mass
    Velocities[index].w = pMass;
}