#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "graphics.h"

int SW = 0;   // overwritten from the monitor's video mode in main()
int SH = 0;

glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw   = -90.0f;   // degrees. -90 so we start looking down -Z, not +X
float pitch = 0.0f;
float lastX, lastY;   // last mouse pos (start at screen center)
bool  firstMouse = true;

const float sensitivity = 0.1f;
float deltaTime = 0.0f, lastFrame = 0.0f;  // for frame-rate-independent speed

// Runs once per vertex. Its only job: set gl_Position, the vertex's final
// position in "clip space". For now we pass our coordinates straight through.
// `layout (location = 0)` ties aPos to attribute slot 0 (we configure slot 0 below).
const char* vertexShaderSrc = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)glsl";


// Runs once per fragment (≈ per pixel the triangle covers). Outputs the color.
// Here every fragment is the same orange.
const char* fragmentShaderSrc = R"glsl(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D tex0;

void main()
{
    FragColor = texture(tex0, TexCoord);
}
)glsl";


// ----------------------------------------------------------------------------
// Helper: compile one shader and report any GLSL compile errors.
// (Shaders compile on the GPU driver — if you get a typo, you NEED this log.)
// ----------------------------------------------------------------------------
unsigned int compileShader(GLenum type, const char* src)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Shader compile error:\n%s\n", log);
    }
    return shader;
}


void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}


// Load a PNG/JPG from `path` into a new GL texture and return its id.
unsigned int loadTexture(const char* path)
{
    stbi_set_flip_vertically_on_load(true);
    int tw, th, tch;
    unsigned char* data = stbi_load(path, &tw, &th, &tch, 0);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (data) {
        GLenum fmt = (tch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, tw, th, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::fprintf(stderr, "Failed to load texture: %s\n", path);
    }
    stbi_image_free(data);
    return texture;
}


void mouseCallback(GLFWwindow*, double xpos, double ypos)
{
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }

    float xoffset = (float)(xpos - lastX);
    float yoffset = (float)(lastY - ypos);
    lastX = float(xpos);
    lastY = float(ypos);

    yaw += xoffset * sensitivity;
    pitch += yoffset * sensitivity;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}


bool loadOBJ(const char* path,
                    std::vector<float>& outVerts,
                    std::vector<unsigned int>& outIndices)
{
    std::ifstream file(path);
    if (!file) { std::fprintf(stderr, "Cannot open %s\n", path); return false; }

    //unsigned int num_verts = outVerts.size() / 3;

    std::vector<glm::vec3> pos;
    std::vector<glm::vec2> uv;

    std::map<std::pair<int,int>, unsigned int> uniqueMap;

    std::string line;
    while(std::getline(file, line))
    {
        std::istringstream ss(line);
        std::string tag;
        ss >> tag;

        if (tag == "v") {
            float x, y, z;
            ss >> x >> y >> z;
            glm::vec3 temp = glm::vec3(x, y, z);
            pos.push_back(temp);
        }
        else if (tag == "vt") {
            float u, v;
            ss >> u >> v;
            glm::vec2 temp = glm::vec2(u, v);
            uv.push_back(temp);
        }
        else if (tag == "f") {
            std::string vert;
            std::vector<unsigned int> face;
            while (ss >> vert) {
                std::istringstream vs(vert);
                std::string p, t;
                std::getline(vs, p, '/');
                std::getline(vs, t, '/');
                int posIdx = std::stoi(p) - 1;
                int uvIdx = t.empty() ? -1 : std::stoi(t) - 1;

                std::pair<int, int> key(posIdx, uvIdx);
                auto found = uniqueMap.find(key);
                if (found != uniqueMap.end()) {
                    face.push_back(found->second);
                } else {
                    unsigned int newIndex = (unsigned int)(outVerts.size() / 5);
                    outVerts.push_back(pos[posIdx].x);
                    outVerts.push_back(pos[posIdx].y);
                    outVerts.push_back(pos[posIdx].z);

                    if (uvIdx >= 0) {
                        outVerts.push_back(uv[uvIdx].x);
                        outVerts.push_back(uv[uvIdx].y);
                    } else {
                        outVerts.push_back(0.0f);
                        outVerts.push_back(0.0f);
                    }

                    uniqueMap[key] = newIndex;
                    face.push_back(newIndex);
                }

            }
            // triangulate as a fan — indices are already global, so no +num_verts here
            for (size_t i = 1; i + 1 < face.size(); i++) {
                outIndices.push_back(face[0]);
                outIndices.push_back(face[i]);
                outIndices.push_back(face[i+1]);
            }
        }
        // ignore vt, vn, #, o, g, s, mtllib, usemtl for now
    }
    return true;
}


Object makeObject(const char* objPath, const char* texPath,
                  std::vector<float> transform)
{
    Object obj;
    loadOBJ(objPath, obj.vertices, obj.indices);
    obj.transform = transform;
    obj.texture = loadTexture(texPath);
    obj.indexCount = (GLsizei)obj.indices.size();
    return obj;
}


void uploadObject(Object &obj)
{
    glGenVertexArrays(1, &obj.VAO);
    glGenBuffers(1, &obj.VBO);
    glGenBuffers(1, &obj.EBO);

    glBindVertexArray(obj.VAO); // start recording into this VAO

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
             obj.indices.size() * sizeof(unsigned int),
             obj.indices.data(), GL_STATIC_DRAW);

    // VBO: upload the vertex bytes to GPU memory.
    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
    glBufferData(GL_ARRAY_BUFFER,
             obj.vertices.size() * sizeof(float),
             obj.vertices.data(), GL_STATIC_DRAW); // change GL_STATIC_DRAW to GL_DYNAMIC_DRAW if tri will warp in 3d space

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // stop recording (optional tidy-up)
}


void buildShaderProgram(unsigned int& vs, unsigned int& fs, unsigned int& shaderProgram,
                        int& modelLoc, int&viewLoc, int& projectionLoc) {
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    int linked = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        char log[512];
        glGetProgramInfoLog(shaderProgram, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Program link error:\n%s\n", log);
    }

    modelLoc      = glGetUniformLocation(shaderProgram, "model");
    viewLoc       = glGetUniformLocation(shaderProgram, "view");
    projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    // The individual shaders are now baked into the program; we can delete them.
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    // Tell the "tex0" sampler to read from texture unit 0 (set once).
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "tex0"), 0);
}


int initWindow(GLFWwindow*& window)
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SW, SH, "OpenGL", nullptr, nullptr);
    if (!window) { std::fprintf(stderr, "Failed to create window\n"); glfwTerminate(); return -1; }
    glfwSetWindowPos(window, 0, 0);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    { std::fprintf(stderr, "Failed to init GLAD\n"); glfwTerminate(); return -1; }

    glEnable(GL_DEPTH_TEST);
    return 0;
}