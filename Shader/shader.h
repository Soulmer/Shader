#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
private:
    GLuint baseProgram;
    GLuint computeProgram;
    GLuint SSBOPos;
    GLuint SSBOVel;
    GLuint particleTex;
    float frameDelta = 0.0f;
    float color[3];
    void resetPositionSSBO();
    void resetVelocitySSBO();
    GLuint loadBaseProgram(const char* vertexShaderFile, const char* fragmentShaderFile);
    GLuint loadComputeProgram(const char* computeShaderFile);
public:
    GLFWwindow* window;
    int particleCount = 4096;
    Shader();
    void generateShaders();
    void generateBuffers();
    void resetBuffers();
    void generateTextures();
    void renderScene();
};