// g++ 1-sphere-rot-shaders.cpp -o 1-sphere-rot-shaders.out $(pkg-config --cflags --libs glfw3) -lGL

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

struct Point {
    float x, y, z;
};

struct TextVertex {
    float x, y;
    float u, v;
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
using GLUniform1iProc = void (*)(GLint, GLint);
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
static GLUniform1iProc glUniform1iLoaded = nullptr;
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
           loadGlProc(glUniform1iLoaded, "glUniform1i") &&
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

std::vector<unsigned char> loadBinaryFile(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};

    std::streamsize size = file.tellg();
    if (size <= 0) return {};

    std::vector<unsigned char> data(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) return {};

    return data;
}

std::vector<TextVertex> makeTextVertices(
    float x,
    float baselineY,
    const char* text,
    const stbtt_bakedchar* glyphs,
    int atlasWidth,
    int atlasHeight
) {
    float penX = x;
    float penY = baselineY;

    std::vector<TextVertex> vertices;
    vertices.reserve(std::string(text).size() * 6);

    for (const char* c = text; *c; ++c) {
        if (*c < 32 || *c > 126) continue;

        stbtt_aligned_quad quad;
        stbtt_GetBakedQuad(glyphs, atlasWidth, atlasHeight, *c - 32, &penX, &penY, &quad, 1);

        vertices.push_back({quad.x0, quad.y0, quad.s0, quad.t0});
        vertices.push_back({quad.x1, quad.y0, quad.s1, quad.t0});
        vertices.push_back({quad.x1, quad.y1, quad.s1, quad.t1});
        vertices.push_back({quad.x0, quad.y0, quad.s0, quad.t0});
        vertices.push_back({quad.x1, quad.y1, quad.s1, quad.t1});
        vertices.push_back({quad.x0, quad.y1, quad.s0, quad.t1});
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
        layout (location = 1) in vec2 inTexCoord;

        uniform mat4 uProjection;

        out vec2 vTexCoord;

        void main() {
            vTexCoord = inTexCoord;
            gl_Position = uProjection * vec4(inPosition, 0.0, 1.0);
        }
    )GLSL";

    const char* textFragmentShader = R"GLSL(
        #version 330 core

        uniform vec3 uColor;
        uniform sampler2D uFontTexture;

        in vec2 vTexCoord;
        out vec4 outColor;

        void main() {
            float alpha = texture(uFontTexture, vTexCoord).r;
            outColor = vec4(uColor, alpha);
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

    constexpr int fontAtlasWidth = 512;
    constexpr int fontAtlasHeight = 512;
    const char* fontPath = "/usr/share/fonts/google-noto-vf/NotoSans[wght].ttf";

    std::vector<unsigned char> fontData = loadBinaryFile(fontPath);
    if (fontData.empty()) {
        std::cerr << "Failed to load font: " << fontPath << "\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }


    std::vector<unsigned char> fontAtlas(fontAtlasWidth * fontAtlasHeight);
    stbtt_bakedchar glyphs[96];
    int bakeResult = stbtt_BakeFontBitmap(
        fontData.data(),
        0,
        50.0f, // font size
        fontAtlas.data(),
        fontAtlasWidth,
        fontAtlasHeight,
        32,
        96,
        glyphs
    );

    if (bakeResult <= 0) {
        std::cerr << "Failed to bake font atlas\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::vector<TextVertex> titleVertices = makeTextVertices(
        50.0f, // x coord of text
        1800.0f, // y coord of text
        "Einstein@Home",
        glyphs,
        fontAtlasWidth,
        fontAtlasHeight
    );

    GLuint fontTexture = 0;
    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        fontAtlasWidth,
        fontAtlasHeight,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        fontAtlas.data()
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
    glVertexAttribPointerLoaded(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), reinterpret_cast<void*>(offsetof(TextVertex, u)));
    glEnableVertexAttribArrayLoaded(1);

    glBindBufferLoaded(GL_ARRAY_BUFFER, 0);
    glBindVertexArrayLoaded(0);

    GLint pointMvpLocation = glGetUniformLocationLoaded(pointProgram, "uMvp");
    GLint pointTimeLocation = glGetUniformLocationLoaded(pointProgram, "uTime");
    GLint textProjectionLocation = glGetUniformLocationLoaded(textProgram, "uProjection");
    GLint textColorLocation = glGetUniformLocationLoaded(textProgram, "uColor");
    GLint textTextureLocation = glGetUniformLocationLoaded(textProgram, "uFontTexture");

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
        glUniform3fLoaded(textColorLocation, 0.74f, 0.88f, 0.96f);
        glUniform1iLoaded(textTextureLocation, 0);
        glBindTexture(GL_TEXTURE_2D, fontTexture);
        glBindVertexArrayLoaded(textVao);
        glDrawArrays(GL_TRIANGLES, 0, GLsizei(titleVertices.size()));
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffersLoaded(1, &textVbo);
    glDeleteVertexArraysLoaded(1, &textVao);
    glDeleteTextures(1, &fontTexture);
    glDeleteBuffersLoaded(1, &sphereVbo);
    glDeleteVertexArraysLoaded(1, &sphereVao);
    glDeleteProgramLoaded(textProgram);
    glDeleteProgramLoaded(pointProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
