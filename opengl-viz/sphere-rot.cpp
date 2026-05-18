// g++ sphere-rot.cpp -o sphere-rot.out $(pkg-config --cflags --libs glfw3) -lGL



#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <iostream>
#include <vector>
#include <random> 

#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

struct Point {
    float x, y, z;
};

static glm::quat viewRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
static float distanceFromSphere = 4.0f;

static bool dragging = false;
static double lastMouseX = 0.0;
static double lastMouseY = 0.0;

std::vector<Point> makePointSphere(int latSteps, int lonSteps, float radius) {
    std::vector<Point> points;

    for (int i = 0; i <= latSteps; ++i) {
        float theta = glm::pi<float>() * float(i) / float(latSteps);
        float y = std::cos(theta);
        float ringRadius = std::sin(theta);

        for (int j = 0; j < lonSteps; ++j) {
            float phi = glm::two_pi<float>() * float(j) / float(lonSteps);

            float x = ringRadius * std::cos(phi);
            float z = ringRadius * std::sin(phi);

            points.push_back({
                radius * x,
                radius * y,
                radius * z
            });
        }
    }

    return points;
}
std::vector<Point> makeRandomPointSphere(int pointCount, float radius) {
    std::vector<Point> points;
    points.reserve(pointCount);

    std::random_device rd;
    std::mt19937 rng(rd());

    std::uniform_real_distribution<float> yDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());

    for (int i = 0; i < pointCount; ++i) {
        float y = yDist(rng);
        float angle = angleDist(rng);

        float ringRadius = std::sqrt(1.0f - y * y);

        float x = ringRadius * std::cos(angle);
        float z = ringRadius * std::sin(angle);

        points.push_back({
            radius * x,
            radius * y,
            radius * z
        });
    }

    return points;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        dragging = action == GLFW_PRESS;
        glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
    }
}

void cursorCallback(GLFWwindow* window, double x, double y) {
    if (!dragging) return;

    double dx = x - lastMouseX;
    double dy = y - lastMouseY;

    float sensitivity = 0.005f;

    float angleX = float(dx) * sensitivity;
    float angleY = float(dy) * sensitivity;

    glm::quat yawRotation = glm::angleAxis(
        angleX,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::quat pitchRotation = glm::angleAxis(
        angleY,
        glm::vec3(1.0f, 0.0f, 0.0f)
    );

    viewRotation = yawRotation * viewRotation * pitchRotation;
    viewRotation = glm::normalize(viewRotation);

    lastMouseX = x;
    lastMouseY = y;
}



void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    distanceFromSphere *= std::pow(0.9f, float(yoffset));
    distanceFromSphere = glm::clamp(distanceFromSphere, 1.5f, 20.0f);
}



void drawText2D(float x, float y, const char* text, int width, int height, float scale) {
    char buffer[99999];

    int num_quads = stb_easy_font_print(
        x,
        y,
        const_cast<char*>(text),
        nullptr,
        buffer,
        sizeof(buffer)
    );

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);

    glDisable(GL_DEPTH_TEST);

    glColor3f(0.7f, 0.9f, 1.0f);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}



int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(900, 700, "Point Sphere", nullptr, nullptr);

    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorCallback);
    glfwSetScrollCallback(window, scrollCallback);

    //std::vector<Point> spherePoints = makePointSphere(
    //    12,   // latitude rings
    //    24,   // points per ring
    //    1.0f  // radius
    //);
    std::vector<Point> spherePoints = makeRandomPointSphere(
        50000,  // n points
        1.0f  // radius
    );

    glEnable(GL_DEPTH_TEST);
    glPointSize(4.0f);

    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        float aspect = float(width) / float(height);

        glViewport(0, 0, width, height);
        glClearColor(0.01f, 0.01f, 0.025f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            aspect,
            0.1f,
            100.0f
        );

        glm::vec3 baseCameraPosition = glm::vec3(0.0f, 0.0f, distanceFromSphere);
        glm::vec3 cameraPosition = viewRotation * baseCameraPosition;

        glm::mat4 view = glm::lookAt(
            cameraPosition,
            glm::vec3(0.0f, 0.0f, 0.0f),
            viewRotation * glm::vec3(0.0f, 1.0f, 0.0f)
        );


        float spinAngle = float(glfwGetTime()) * 0.15f;

        glm::mat4 model = glm::rotate(
            glm::mat4(1.0f),
            spinAngle,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 mvp = projection * view * model;

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(&projection[0][0]);

        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(&(view * model)[0][0]);

        glBegin(GL_POINTS);

        for (const Point& p : spherePoints) {
            float brightness = 0.6f + 0.4f * ((p.y + 1.0f) * 0.5f);
            glColor3f(0.2f * brightness, 0.75f * brightness, 1.0f * brightness);
            glVertex3f(p.x, p.y, p.z);
        }

        glEnd();
        
        drawText2D(
            16.0f,
            16.0f,
            "Einstein@Home",
            width,
            height,
            5
        );

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}