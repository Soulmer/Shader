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

struct vertex4f {
	GLfloat x, y, z, w;
};


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

Shader::Shader()
{
	srand(time(NULL));
	color[0] = 0.0;
	color[1] = 0.0;
	color[2] = 0.0;
}

GLuint Shader::loadBaseProgram(const char* vertexShaderFile, const char* fragmentShaderFile)
{
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

	string vertShaderStr = readFile(vertexShaderFile);
	string fragShaderStr = readFile(fragmentShaderFile);
	const char* vertShaderSrc = vertShaderStr.c_str();
	const char* fragShaderSrc = fragShaderStr.c_str();

	glShaderSource(vertShader, 1, &vertShaderSrc, NULL);
	glCompileShader(vertShader);

	glShaderSource(fragShader, 1, &fragShaderSrc, NULL);
	glCompileShader(fragShader);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return program;
}

GLuint Shader::loadComputeProgram(const char* computeShaderFile)
{
	GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);

	string computeShaderStr = readFile(computeShaderFile);
	const char* computeShaderSrc = computeShaderStr.c_str();

	GLint result = GL_FALSE;

	glShaderSource(computeShader, 1, &computeShaderSrc, NULL);
	glCompileShader(computeShader);

	GLuint program = glCreateProgram();
	glAttachShader(program, computeShader);
	glLinkProgram(program);

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

	struct vertex4f* verticesPos = (struct vertex4f*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, particleCount * sizeof(vertex4f), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	for (int i = 0; i < particleCount; i++) {
		unsigned seed = chrono::system_clock::now().time_since_epoch().count();
		mt19937 generator(seed);
		uniform_real_distribution<float> uniform_1(0.9, 1.1);
		verticesPos[i].x = startX;
		verticesPos[i].y = startY;
		verticesPos[i].z = startZ;
		verticesPos[i].w = float(uniform_1(generator));
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void Shader::resetVelocitySSBO()
{
	struct vertex4f* verticesVel = (struct vertex4f*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, particleCount * sizeof(vertex4f), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	for (int i = 0; i < particleCount; i++) {
		unsigned seed = chrono::system_clock::now().time_since_epoch().count();
		mt19937 generator(seed);
		uniform_real_distribution<float> uniform_1(0.0, 1.0);
		uniform_real_distribution<float> uniform_2(0.995, 1.005);
		float theta = 2 * 3.14 * uniform_1(generator);
		float phi = acos(1 - 2 * uniform_1(generator));
		float radius = random(0.095f, 0.1f);
		verticesVel[i].x = sin(phi) * cos(theta) * radius;
		verticesVel[i].y = sin(phi) * sin(theta) * radius;
		verticesVel[i].z = cos(phi) * radius;
		verticesVel[i].w = float(uniform_2(generator));
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
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	if (glIsBuffer(SSBOPos)) {
		glDeleteBuffers(1, &SSBOPos);
	};
	glGenBuffers(1, &SSBOPos);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBOPos);
	glBufferData(GL_SHADER_STORAGE_BUFFER, particleCount * sizeof(vertex4f), NULL, GL_STATIC_DRAW);
	resetPositionSSBO();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBOPos);

	if (glIsBuffer(SSBOVel)) {
		glDeleteBuffers(1, &SSBOVel);
	};
	glGenBuffers(1, &SSBOVel);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBOVel);
	glBufferData(GL_SHADER_STORAGE_BUFFER, particleCount * sizeof(vertex4f), NULL, GL_STATIC_DRAW);
	resetVelocitySSBO();
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


	double frameTimeStart = glfwGetTime();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glUseProgram(computeProgram);
	glUniform1f(glGetUniformLocation(computeProgram, "dt"), frameDelta * 0.15f);

	int workingGroups = particleCount / 16;

	glDispatchCompute(workingGroups, 1, 1);

	glUseProgram(0);

	glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

	glUseProgram(baseProgram);
	glUniform4f(glGetUniformLocation(baseProgram, "inColor"), color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f, 1.0f);

	glGetError();

	glBindTexture(GL_TEXTURE_2D, particleTex);

	GLuint posAttrib = glGetAttribLocation(baseProgram, "pos");

	glBindBuffer(GL_ARRAY_BUFFER, SSBOPos);
	glVertexAttribPointer(posAttrib, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);
	glPointSize(16);
	glDrawArrays(GL_POINTS, 0, particleCount);

	glfwSwapBuffers(window);

	frameDelta = (float)(glfwGetTime() - frameTimeStart) * 100.0f;
}