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
#include <utility>


struct Object
{
    std::vector<float> transform;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int texture = 0;

    GLsizei indexCount = 0;

    bool billboard = false;  // if true, the quad is rotated to face the camera each frame

    Object() = default;

    // The GL handles above name resources owned by the driver, not by this struct.
    // Free them here so destroying an Object (e.g. objects.pop_back()) releases its
    // VAO/VBO/EBO/texture. glDelete* treat a 0 handle as a no-op, so default-zeroed
    // or moved-from objects clean up safely.
    ~Object()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteTextures(1, &texture);
    }

    // No copying: two Objects must never share the same GL handles, or the first
    // destroyed would invalidate the other.
    Object(const Object&)            = delete;
    Object& operator=(const Object&) = delete;

    // Moving is fine: steal the other's handles and zero them out so its destructor
    // deletes nothing. noexcept lets std::vector move (not copy) on reallocation.
    Object(Object&& o) noexcept
        : transform(std::move(o.transform)),
          vertices(std::move(o.vertices)),
          indices(std::move(o.indices)),
          VAO(o.VAO), VBO(o.VBO), EBO(o.EBO),
          texture(o.texture),
          indexCount(o.indexCount),
          billboard(o.billboard)
    {
        o.VAO = o.VBO = o.EBO = 0;
        o.texture = 0;
    }

    Object& operator=(Object&& o) noexcept
    {
        if (this != &o)
        {
            // Free our own handles before overwriting them, or we'd leak.
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
            glDeleteTextures(1, &texture);

            transform  = std::move(o.transform);
            vertices   = std::move(o.vertices);
            indices    = std::move(o.indices);
            VAO        = o.VAO;
            VBO        = o.VBO;
            EBO        = o.EBO;
            texture    = o.texture;
            indexCount = o.indexCount;
            billboard  = o.billboard;

            o.VAO = o.VBO = o.EBO = 0;
            o.texture = 0;
        }
        return *this;
    }
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

extern std::vector<Object> objects;

extern GLFWwindow* window;

extern unsigned int vs;
extern unsigned int fs;
extern unsigned int shaderProgram;
extern int modelLoc, viewLoc, projectionLoc;

unsigned int compileShader(GLenum type, const char* src);

void framebufferSizeCallback(GLFWwindow*, int width, int height);

unsigned int loadTexture(const char* path);

void mouseCallback(GLFWwindow*, double xpos, double ypos);

bool loadOBJ(const char* path,
                    std::vector<float>& outVerts,
                    std::vector<unsigned int>& outIndices);

Object makeObject(const char* objPath, const char* texPath,
                  std::vector<float> transform);

Object makeSprite(const char* texPath, std::vector<float> transform);

glm::mat4 billboardModel(const glm::vec3& pos, float scale, const glm::vec3& camPos);

void uploadObject(Object &obj);

void buildShaderProgram();

int initWindow();

void drawObj(Object& obj);

void clearBG(float r, float g, float b, float a);