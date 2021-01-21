#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <string>
#include <iostream>

#include "shader.h"
Shader shader;

using namespace std;

// FUNCTIONS ----------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		shader.resetBuffers();
}

const unsigned int SCREEN_WIDTH = 1000;
const unsigned int SCREEN_HEIGHT = 1000;

// MAIN ----------
int main()
{
	glfwInit();
	if (!glfwInit())
	{
		cout << "Failed to initialize GLFW\n";
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window;

	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Shader", NULL, NULL);

	if (!window)
	{
		cout << "Failed to create GLFW window\n";
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();

	if (err != GLEW_OK)
	{
		cout << "Failed to initialize GLEW\n";
		return -1;
	}

	const GLubyte* glvendor = glGetString(GL_VENDOR);
	const GLubyte* glrenderer = glGetString(GL_RENDERER);
	const GLubyte* glversion = glGetString(GL_VERSION);

	cout << "Vendor: " << glvendor << "\n";
	cout << "Renderer: " << glrenderer << "\n";
	cout << "Version: " << glversion << "\n";

	shader = Shader();
	shader.window = window;
	shader.generateTextures();
	shader.generateBuffers();
	shader.generateShaders();

	cout << "\nKeys:\n" << "Space : Restart fireworks\n" << "Esc : Exit presentation\n";

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while (!glfwWindowShouldClose(window))
	{
		shader.renderScene();
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
