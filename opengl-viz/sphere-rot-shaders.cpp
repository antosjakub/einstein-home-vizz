// g++ sphere-rot.cpp -o sphere-rot.out $(pkg-config --cflags --libs glfw3) -lGL

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

struct Point {
    float x, y, z;
};

struct TextVertex {
    float x, y;
};

static glm::quat viewRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
static float distanceFromSphere = 4.0f;

static bool dragging = false;
static double lastMouseX = 0.0;
static double lastMouseY = 0.0;

using GLchar = char;
using GLsizeiptr = ptrdiff_t;

using GLCreateShaderProc = GLuint (*)(GLenum);
using GLShaderSourceProc = void (*)(GLuint, GLsizei, const GLchar* const*, const GLint*);
using GLCompileShaderProc = void (*)(GLuint);
using GLGetShaderivProc = void (*)(GLuint, GLenum, GLint*);
using GLGetShaderInfoLogProc = void (*)(GLuint, GLsizei, GLsizei*, GLchar*);
using GLDeleteShaderProc = void (*)(GLuint);
using GLCreateProgramProc = GLuint (*)();
using GLAttachShaderProc = void (*)(GLuint, GLuint);
using GLLinkProgramProc = void (*)(GLuint);
using GLGetProgramivProc = void (*)(GLuint, GLenum, GLint*);
using GLGetProgramInfoLogProc = void (*)(GLuint, GLsizei, GLsizei*, GLchar*);
using GLDeleteProgramProc = void (*)(GLuint);
using GLUseProgramProc = void (*)(GLuint);
using GLGenVertexArraysProc = void (*)(GLsizei, GLuint*);
using GLBindVertexArrayProc = void (*)(GLuint);
using GLDeleteVertexArraysProc = void (*)(GLsizei, const GLuint*);
using GLGenBuffersProc = void (*)(GLsizei, GLuint*);
using GLBindBufferProc = void (*)(GLenum, GLuint);
using GLBufferDataProc = void (*)(GLenum, GLsizeiptr, const void*, GLenum);
using GLDeleteBuffersProc = void (*)(GLsizei, const GLuint*);
using GLVertexAttribPointerProc = void (*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
using GLEnableVertexAttribArrayProc = void (*)(GLuint);
using GLGetUniformLocationProc = GLint (*)(GLuint, const GLchar*);
using GLUniformMatrix4fvProc = void (*)(GLint, GLsizei, GLboolean, const GLfloat*);
using GLUniform1fProc = void (*)(GLint, GLfloat);
using GLUniform3fProc = void (*)(GLint, GLfloat, GLfloat, GLfloat);

static GLCreateShaderProc glCreateShaderLoaded = nullptr;
static GLShaderSourceProc glShaderSourceLoaded = nullptr;
static GLCompileShaderProc glCompileShaderLoaded = nullptr;
static GLGetShaderivProc glGetShaderivLoaded = nullptr;
static GLGetShaderInfoLogProc glGetShaderInfoLogLoaded = nullptr;
static GLDeleteShaderProc glDeleteShaderLoaded = nullptr;
static GLCreateProgramProc glCreateProgramLoaded = nullptr;
static GLAttachShaderProc glAttachShaderLoaded = nullptr;
static GLLinkProgramProc glLinkProgramLoaded = nullptr;
static GLGetProgramivProc glGetProgramivLoaded = nullptr;
static GLGetProgramInfoLogProc glGetProgramInfoLogLoaded = nullptr;
static GLDeleteProgramProc glDeleteProgramLoaded = nullptr;
static GLUseProgramProc glUseProgramLoaded = nullptr;
static GLGenVertexArraysProc glGenVertexArraysLoaded = nullptr;
static GLBindVertexArrayProc glBindVertexArrayLoaded = nullptr;
static GLDeleteVertexArraysProc glDeleteVertexArraysLoaded = nullptr;
static GLGenBuffersProc glGenBuffersLoaded = nullptr;
static GLBindBufferProc glBindBufferLoaded = nullptr;
static GLBufferDataProc glBufferDataLoaded = nullptr;
static GLDeleteBuffersProc glDeleteBuffersLoaded = nullptr;
static GLVertexAttribPointerProc glVertexAttribPointerLoaded = nullptr;
static GLEnableVertexAttribArrayProc glEnableVertexAttribArrayLoaded = nullptr;
static GLGetUniformLocationProc glGetUniformLocationLoaded = nullptr;
static GLUniformMatrix4fvProc glUniformMatrix4fvLoaded = nullptr;
static GLUniform1fProc glUniform1fLoaded = nullptr;
static GLUniform3fProc glUniform3fLoaded = nullptr;

template <typename Proc>
bool loadGlProc(Proc& proc, const char* name) {
    proc = reinterpret_cast<Proc>(glfwGetProcAddress(name));
    if (!proc) {
        std::cerr << "Failed to load OpenGL function: " << name << "\n";
        return false;
    }
    return true;
}

bool loadModernOpenGL() {
    return loadGlProc(glCreateShaderLoaded, "glCreateShader") &&
           loadGlProc(glShaderSourceLoaded, "glShaderSource") &&
           loadGlProc(glCompileShaderLoaded, "glCompileShader") &&
           loadGlProc(glGetShaderivLoaded, "glGetShaderiv") &&
           loadGlProc(glGetShaderInfoLogLoaded, "glGetShaderInfoLog") &&
           loadGlProc(glDeleteShaderLoaded, "glDeleteShader") &&
           loadGlProc(glCreateProgramLoaded, "glCreateProgram") &&
           loadGlProc(glAttachShaderLoaded, "glAttachShader") &&
           loadGlProc(glLinkProgramLoaded, "glLinkProgram") &&
           loadGlProc(glGetProgramivLoaded, "glGetProgramiv") &&
           loadGlProc(glGetProgramInfoLogLoaded, "glGetProgramInfoLog") &&
           loadGlProc(glDeleteProgramLoaded, "glDeleteProgram") &&
           loadGlProc(glUseProgramLoaded, "glUseProgram") &&
           loadGlProc(glGenVertexArraysLoaded, "glGenVertexArrays") &&
           loadGlProc(glBindVertexArrayLoaded, "glBindVertexArray") &&
           loadGlProc(glDeleteVertexArraysLoaded, "glDeleteVertexArrays") &&
           loadGlProc(glGenBuffersLoaded, "glGenBuffers") &&
           loadGlProc(glBindBufferLoaded, "glBindBuffer") &&
           loadGlProc(glBufferDataLoaded, "glBufferData") &&
           loadGlProc(glDeleteBuffersLoaded, "glDeleteBuffers") &&
           loadGlProc(glVertexAttribPointerLoaded, "glVertexAttribPointer") &&
           loadGlProc(glEnableVertexAttribArrayLoaded, "glEnableVertexAttribArray") &&
           loadGlProc(glGetUniformLocationLoaded, "glGetUniformLocation") &&
           loadGlProc(glUniformMatrix4fvLoaded, "glUniformMatrix4fv") &&
           loadGlProc(glUniform1fLoaded, "glUniform1f") &&
           loadGlProc(glUniform3fLoaded, "glUniform3f");
}

std::vector<Point> makeRandomPointSphere(int pointCount, float radius) {
    std::vector<Point> points;
    points.reserve(pointCount);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> yDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());

    for (int i = 0; i < pointCount; ++i) {
        float y = yDist(rng);
        float angle = angleDist(rng);
        float ringRadius = std::sqrt(1.0f - y * y);

        points.push_back({
            radius * ringRadius * std::cos(angle),
            radius * y,
            radius * ringRadius * std::sin(angle)
        });
    }

    return points;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    (void)mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        dragging = action == GLFW_PRESS;
        glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
    }
}

void cursorCallback(GLFWwindow* window, double x, double y) {
    (void)window;
    if (!dragging) return;

    double dx = x - lastMouseX;
    double dy = y - lastMouseY;

    float sensitivity = 0.005f;
    glm::quat yawRotation = glm::angleAxis(float(dx) * sensitivity, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat pitchRotation = glm::angleAxis(float(dy) * sensitivity, glm::vec3(1.0f, 0.0f, 0.0f));

    viewRotation = glm::normalize(yawRotation * viewRotation * pitchRotation);

    lastMouseX = x;
    lastMouseY = y;
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)window;
    (void)xoffset;
    distanceFromSphere *= std::pow(0.9f, float(yoffset));
    distanceFromSphere = glm::clamp(distanceFromSphere, 1.5f, 20.0f);
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShaderLoaded(type);
    glShaderSourceLoaded(shader, 1, &source, nullptr);
    glCompileShaderLoaded(shader);

    GLint ok = 0;
    glGetShaderivLoaded(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        GLsizei length = 0;
        glGetShaderInfoLogLoaded(shader, sizeof(log), &length, log);
        std::cerr << "Shader compilation failed:\n" << std::string(log, length) << "\n";
        glDeleteShaderLoaded(shader);
        return 0;
    }

    return shader;
}

GLuint createProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!vertexShader || !fragmentShader) {
        glDeleteShaderLoaded(vertexShader);
        glDeleteShaderLoaded(fragmentShader);
        return 0;
    }

    GLuint program = glCreateProgramLoaded();
    glAttachShaderLoaded(program, vertexShader);
    glAttachShaderLoaded(program, fragmentShader);
    glLinkProgramLoaded(program);

    glDeleteShaderLoaded(vertexShader);
    glDeleteShaderLoaded(fragmentShader);

    GLint ok = 0;
    glGetProgramivLoaded(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        GLsizei length = 0;
        glGetProgramInfoLogLoaded(program, sizeof(log), &length, log);
        std::cerr << "Program link failed:\n" << std::string(log, length) << "\n";
        glDeleteProgramLoaded(program);
        return 0;
    }

    return program;
}

std::vector<TextVertex> makeTextVertices(float x, float y, const char* text, float scale) {
    char buffer[99999];
    int quadCount = stb_easy_font_print(0.0f, 0.0f, const_cast<char*>(text), nullptr, buffer, sizeof(buffer));

    std::vector<TextVertex> vertices;
    vertices.reserve(size_t(quadCount) * 6);

    for (int i = 0; i < quadCount; ++i) {
        const char* quad = buffer + i * 4 * 16;
        TextVertex q[4];

        for (int j = 0; j < 4; ++j) {
            const float* position = reinterpret_cast<const float*>(quad + j * 16);
            q[j] = {x + position[0] * scale, y + position[1] * scale};
        }

        vertices.push_back(q[0]);
        vertices.push_back(q[1]);
        vertices.push_back(q[2]);
        vertices.push_back(q[0]);
        vertices.push_back(q[2]);
        vertices.push_back(q[3]);
    }

    return vertices;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(900, 700, "Einstein@Home Point Sphere", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!loadModernOpenGL()) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorCallback);
    glfwSetScrollCallback(window, scrollCallback);

    const char* pointVertexShader = R"GLSL(
        #version 330 core

        layout (location = 0) in vec3 inPosition;

        uniform mat4 uMvp;
        uniform float uTime;

        out vec3 vColor;

        float hashPoint(int id) {
            return fract(sin(float(id) * 12.9898) * 43758.5453);
        }

        void main() {
            float latitude = inPosition.y * 0.5 + 0.5;
            float pointNoise = hashPoint(gl_VertexID);
            float shimmer = 0.5 + 0.5 * sin(uTime * 1.6 + pointNoise * 6.2831853);
            float brightness = 0.66 + 0.24 * latitude + 0.10 * shimmer;

            vColor = vec3(0.20, 0.74, 1.0) * brightness;
            gl_Position = uMvp * vec4(inPosition, 1.0);
            gl_PointSize = 2.7 + 1.4 * pointNoise + 0.5 * shimmer;
        }
    )GLSL";

    const char* pointFragmentShader = R"GLSL(
        #version 330 core

        in vec3 vColor;
        out vec4 outColor;

        void main() {
            vec2 centered = gl_PointCoord * 2.0 - 1.0;
            float radius2 = dot(centered, centered);
            if (radius2 > 1.0) discard;

            float alpha = smoothstep(1.0, 0.35, radius2);
            outColor = vec4(vColor, alpha);
        }
    )GLSL";

    const char* textVertexShader = R"GLSL(
        #version 330 core

        layout (location = 0) in vec2 inPosition;

        uniform mat4 uProjection;

        void main() {
            gl_Position = uProjection * vec4(inPosition, 0.0, 1.0);
        }
    )GLSL";

    const char* textFragmentShader = R"GLSL(
        #version 330 core

        uniform vec3 uColor;
        out vec4 outColor;

        void main() {
            outColor = vec4(uColor, 1.0);
        }
    )GLSL";

    GLuint pointProgram = createProgram(pointVertexShader, pointFragmentShader);
    GLuint textProgram = createProgram(textVertexShader, textFragmentShader);
    if (!pointProgram || !textProgram) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::vector<Point> spherePoints = makeRandomPointSphere(10000, 1.0f);
    std::vector<TextVertex> titleVertices = makeTextVertices(16.0f, 18.0f, "Einstein@Home", 4.0f);

    GLuint sphereVao = 0;
    GLuint sphereVbo = 0;
    glGenVertexArraysLoaded(1, &sphereVao);
    glGenBuffersLoaded(1, &sphereVbo);
    glBindVertexArrayLoaded(sphereVao);
    glBindBufferLoaded(GL_ARRAY_BUFFER, sphereVbo);
    glBufferDataLoaded(GL_ARRAY_BUFFER, GLsizeiptr(spherePoints.size() * sizeof(Point)), spherePoints.data(), GL_STATIC_DRAW);
    glVertexAttribPointerLoaded(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), reinterpret_cast<void*>(offsetof(Point, x)));
    glEnableVertexAttribArrayLoaded(0);

    GLuint textVao = 0;
    GLuint textVbo = 0;
    glGenVertexArraysLoaded(1, &textVao);
    glGenBuffersLoaded(1, &textVbo);
    glBindVertexArrayLoaded(textVao);
    glBindBufferLoaded(GL_ARRAY_BUFFER, textVbo);
    glBufferDataLoaded(GL_ARRAY_BUFFER, GLsizeiptr(titleVertices.size() * sizeof(TextVertex)), titleVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointerLoaded(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), reinterpret_cast<void*>(offsetof(TextVertex, x)));
    glEnableVertexAttribArrayLoaded(0);

    glBindBufferLoaded(GL_ARRAY_BUFFER, 0);
    glBindVertexArrayLoaded(0);

    GLint pointMvpLocation = glGetUniformLocationLoaded(pointProgram, "uMvp");
    GLint pointTimeLocation = glGetUniformLocationLoaded(pointProgram, "uTime");
    GLint textProjectionLocation = glGetUniformLocationLoaded(textProgram, "uProjection");
    GLint textColorLocation = glGetUniformLocationLoaded(textProgram, "uColor");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    while (!glfwWindowShouldClose(window)) {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (height == 0) height = 1;

        float aspect = float(width) / float(height);
        glViewport(0, 0, width, height);
        glClearColor(0.01f, 0.01f, 0.025f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glm::vec3 cameraPosition = viewRotation * glm::vec3(0.0f, 0.0f, distanceFromSphere);
        glm::mat4 view = glm::lookAt(
            cameraPosition,
            glm::vec3(0.0f, 0.0f, 0.0f),
            viewRotation * glm::vec3(0.0f, 1.0f, 0.0f)
        );

        float time = float(glfwGetTime());
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * 0.15f, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 mvp = projection * view * model;

        glUseProgramLoaded(pointProgram);
        glUniformMatrix4fvLoaded(pointMvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform1fLoaded(pointTimeLocation, time);
        glBindVertexArrayLoaded(sphereVao);
        glDrawArrays(GL_POINTS, 0, GLsizei(spherePoints.size()));

        glDisable(GL_DEPTH_TEST);
        glm::mat4 textProjection = glm::ortho(0.0f, float(width), float(height), 0.0f);
        glUseProgramLoaded(textProgram);
        glUniformMatrix4fvLoaded(textProjectionLocation, 1, GL_FALSE, glm::value_ptr(textProjection));
        glUniform3fLoaded(textColorLocation, 0.70f, 0.90f, 1.0f);
        glBindVertexArrayLoaded(textVao);
        glDrawArrays(GL_TRIANGLES, 0, GLsizei(titleVertices.size()));
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffersLoaded(1, &textVbo);
    glDeleteVertexArraysLoaded(1, &textVao);
    glDeleteBuffersLoaded(1, &sphereVbo);
    glDeleteVertexArraysLoaded(1, &sphereVao);
    glDeleteProgramLoaded(textProgram);
    glDeleteProgramLoaded(pointProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
