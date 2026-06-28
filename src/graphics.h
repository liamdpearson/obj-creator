#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>                  // vec3, mat4, basic types
#include <glm/gtc/matrix_transform.hpp> // perspective, lookAt, rotate, radians
#include <glm/gtc/type_ptr.hpp>       // value_ptr (hand a matrix to OpenGL)

#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>


struct Object
{
    std::vector<float> transform;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int texture = 0;

    GLsizei indexCount = 0;
};

extern int SW;
extern int SH;

extern glm::vec3 cameraPos;
extern glm::vec3 cameraFront;
extern glm::vec3 cameraUp;

extern float yaw;
extern float pitch;
extern float lastX, lastY;   // last mouse pos (start at screen center)
extern bool  firstMouse;

extern const float sensitivity;
extern float deltaTime, lastFrame;

extern const char* vertexShaderSrc;
extern const char* fragmentShaderSrc;

unsigned int compileShader(GLenum type, const char* src);

void framebufferSizeCallback(GLFWwindow*, int width, int height);

unsigned int loadTexture(const char* path);

void mouseCallback(GLFWwindow*, double xpos, double ypos);

bool loadOBJ(const char* path,
                    std::vector<float>& outVerts,
                    std::vector<unsigned int>& outIndices);

Object makeObject(const char* objPath, const char* texPath,
                  std::vector<float> transform);

void uploadObject(Object &obj);

void buildShaderProgram(unsigned int& vs, unsigned int& fs, unsigned int& shaderProgram,
                        int& modelLoc, int&viewLoc, int& projectionLoc);

int initWindow(GLFWwindow*& window);