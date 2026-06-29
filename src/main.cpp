#include "graphics.h"


std::vector<Object> objects;


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Only trigger on the initial press event, ignoring GLFW_REPEAT and GLFW_RELEASE
    if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
        objects.pop_back();
    }
}


int main()
{
    initWindow();
    buildShaderProgram();
    glfwSetKeyCallback(window, key_callback);

    // --- init objects --- //
    objects.push_back(makeObject("models/gun/gun.obj", "models/gun/gun.png",
                                std::vector<float>{0, 0, 0,  90, 45, 1.0f}));
    objects.push_back(makeObject("models/skull/skull.obj", "models/skull/skull.png",
                                 std::vector<float>{1, 1, 1,  0, 0, 0.01f}));

    // A Doom-style billboard sprite. yaw/pitch are ignored for sprites; only
    // position (first 3) and scale (last) are used. Use an RGBA PNG with a
    // transparent background so the cutout looks right.
    objects.push_back(makeSprite("models/dog/dog.png",
                                 std::vector<float>{2, 0, 0,  0, 0, 1.0f}));

    for (Object& obj : objects)
        uploadObject(obj);
        
    // --- render loop --- //
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float speed = 2.5f * deltaTime; // deltaTime keeps speed steady regardless of FPS
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += speed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= speed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)        cameraPos += speed * cameraUp;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)   cameraPos -= speed * cameraUp;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        clearBG(0.10f, 0.12f, 0.15f, 1.0f); // r, g, b, a

        for (Object& obj : objects)
        {
            drawObj(obj);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}   