#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <time.h> 

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
#include <chrono>

#include "shader.h"

using namespace std;

//	STRUCT ----------
struct vertex4f {
	GLfloat x, y, z, w;
};


// HELPER FUNCTIONS ----------
string readFile(const char* fileName)
{
	string fileContent;
	ifstream fileStream(fileName, ios::in);
	if (!fileStream.is_open()) {
		printf("File %s not found\n", fileName);
		return "";
	}
	string line = "";
	while (!fileStream.eof()) {
		getline(fileStream, line);
		fileContent.append(line + "\n");
	}
	fileStream.close();
	return fileContent;
}

GLuint loadBMPTexture(const char* fileName)
{
	FILE* bmpFile;
	fopen_s(&bmpFile, fileName, "rb");
	if (!bmpFile)
	{
		cout << "Count not load bitmap\n";
		return 0;
	}

	unsigned char* bmpHeader = new unsigned char[54];
	if (fread(bmpHeader, 1, 54, bmpFile) != 54) {
		cout << "Headersize does not fit BMP\n";
		return 0;
	}

	unsigned int width = *(int*)&(bmpHeader[0x12]);
	unsigned int height = *(int*)&(bmpHeader[0x16]);
	unsigned int dataPos = *(int*)&(bmpHeader[0x0A]) != 0 ? *(int*)&(bmpHeader[0x0A]) : 54;
	unsigned int imageSize = *(int*)&(bmpHeader[0x22]) != 0 ? *(int*)&(bmpHeader[0x22]) : width * height * 3;

	unsigned char* bmpData = new unsigned char[imageSize];
	fread(bmpData, 1, imageSize, bmpFile);
	fclose(bmpFile);

	// Generate texture from bitmap
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, bmpData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	return textureID;
}

float random(float fMin, float fMax)
{
	float fRandNum = (float)rand() / RAND_MAX;
	return fMin + (fMax - fMin) * fRandNum;
}


// SHADER METHODS ----------
Shader::Shader()
{
	srand(time(NULL));
	color[0] = 0.0;
	color[1] = 0.0;
	color[2] = 0.0;
}

GLuint Shader::loadBaseProgram(const char* vertexShaderFile, const char* fragmentShaderFile)
{
	// Create vertex shader & fragment shader
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Read data from respective shader file
	string vertShaderStr = readFile(vertexShaderFile);
	string fragShaderStr = readFile(fragmentShaderFile);
	const char* vertShaderSrc = vertShaderStr.c_str();
	const char* fragShaderSrc = fragShaderStr.c_str();

	// Compile shaders
	glShaderSource(vertShader, 1, &vertShaderSrc, NULL);
	glCompileShader(vertShader);

	glShaderSource(fragShader, 1, &fragShaderSrc, NULL);
	glCompileShader(fragShader);

	// Create shader program & attach vertex shader and fragment shader to program & link program
	GLuint program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);

	// Delete redundant data (once shaders are compiled & attached to program there is no need to keep them in memory)
	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return program;
}

GLuint Shader::loadComputeProgram(const char* computeShaderFile)
{
	// Create compute shader
	GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);

	// Read data from compute shader file
	string computeShaderStr = readFile(computeShaderFile);
	const char* computeShaderSrc = computeShaderStr.c_str();

	// Compile shader
	glShaderSource(computeShader, 1, &computeShaderSrc, NULL);
	glCompileShader(computeShader);

	// Create shader program & attach compute shader to program & link program
	GLuint program = glCreateProgram();
	glAttachShader(program, computeShader);
	glLinkProgram(program);

	// Delete redundant data (once shader is compiled & attached to program there is no need to keep it in memory)
	glDeleteShader(computeShader);

	return program;
}

void Shader::generateShaders()
{
	baseProgram = loadBaseProgram("vertex.glsl", "fragment.glsl");
	computeProgram = loadComputeProgram("compute.glsl");
}

void Shader::resetPositionSSBO()
{
	float startX = 0.0f;
	float startY = 0.26f;
	float startZ = 0.0f;

	// Reset particles position to starting point (0.0f, 0.26f, 0.0f)
	struct vertex4f* particlesPos = (struct vertex4f*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, particleCount * sizeof(vertex4f), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	for (int i = 0; i < particleCount; i++) {
		particlesPos[i].x = startX;
		particlesPos[i].y = startY;
		particlesPos[i].z = startZ;
		// Reset texture transparency
		particlesPos[i].w = 1.0f;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void Shader::resetVelocitySSBO()
{
	// Reset particles velocity
	struct vertex4f* particlesVel = (struct vertex4f*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, particleCount * sizeof(vertex4f), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	for (int i = 0; i < particleCount; i++) {
		// Generate new starting velocities, based on Mersenne Twister algorithm
		unsigned seed = chrono::system_clock::now().time_since_epoch().count();
		mt19937 generator(seed);
		// Generate random numbers, distributed according to the propability density function
		uniform_real_distribution<float> distribution_angle(0.0f, 1.0f);
		uniform_real_distribution<float> distribution_mass(0.2f, 0.3f);
		normal_distribution<float> distribution_radius(1.75f, 0.05f);
		// Azimuthal angle & polar angle & explosion radius
		float theta = 2 * 3.14 * distribution_angle(generator);
		float phi = acos(1 - 2 * distribution_angle(generator));
		float radius = distribution_radius(generator);
		particlesVel[i].x = sin(phi) * cos(theta) * radius;
		particlesVel[i].y = sin(phi) * sin(theta) * radius;
		particlesVel[i].z = cos(phi) * radius;
		// Reset mass
		particlesVel[i].w = distribution_mass(generator);
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void Shader::resetBuffers()
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBOPos);
	resetPositionSSBO();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBOVel);
	resetVelocitySSBO();
}

void Shader::generateBuffers()
{
	// Generate VAO (Vertex Array Object)
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Generate SSBOs (Shader Storage Buffer Object)
	// Positions storage
	if (glIsBuffer(SSBOPos)) {
		glDeleteBuffers(1, &SSBOPos);
	};
	// Generate buffer & bind buffer & generate empty storage & fill storage with data
	glGenBuffers(1, &SSBOPos);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBOPos);
	glBufferData(GL_SHADER_STORAGE_BUFFER, particleCount * sizeof(vertex4f), NULL, GL_STATIC_DRAW);
	resetPositionSSBO();
	// Bind buffer to index 0
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBOPos);

	// Velocities storage
	if (glIsBuffer(SSBOVel)) {
		glDeleteBuffers(1, &SSBOVel);
	};
	// Generate buffer & bind buffer & generate empty storage & fill storage with data
	glGenBuffers(1, &SSBOVel);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBOVel);
	glBufferData(GL_SHADER_STORAGE_BUFFER, particleCount * sizeof(vertex4f), NULL, GL_STATIC_DRAW);
	resetVelocitySSBO();
	// Bind buffer to index 1
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, SSBOVel);
}

void Shader::generateTextures()
{
	particleTex = loadBMPTexture("textures/particle.bmp");
}

void Shader::renderScene()
{
	color[0] = 255.0f;
	color[1] = 64.0f;
	color[2] = 0.0f;


	float frameTimeStart = glfwGetTime();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	// Install compute program & set time uniform
	glUseProgram(computeProgram);
	glUniform1f(glGetUniformLocation(computeProgram, "dt"), frameDelta);

	int workingGroups = particleCount / 16;
	glDispatchCompute(workingGroups, 1, 1);

	// Install empty program
	glUseProgram(0);

	// Define a barrier that orders memory transactions
	glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

	// Install base program & set color uniform
	glUseProgram(baseProgram);
	glUniform4f(glGetUniformLocation(baseProgram, "inColor"), color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f, 1.0f);

	glGetError();

	glBindTexture(GL_TEXTURE_2D, particleTex);

	// Bind position SSBO to base program
	GLuint posAttrib = glGetAttribLocation(baseProgram, "pos");
	glBindBuffer(GL_ARRAY_BUFFER, SSBOPos);
	glVertexAttribPointer(posAttrib, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);

	glPointSize(16);
	glDrawArrays(GL_POINTS, 0, particleCount);

	glfwSwapBuffers(window);

	frameDelta = (float)(glfwGetTime() - frameTimeStart);
}