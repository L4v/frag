#include <iostream>
#include <cstdint>
#include "include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <fstream>
#include <streambuf>

#define global static
#define internal static

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;
typedef u32 b32;

global i32 G_WWIDTH = 800;
global i32 G_WHEIGHT = 600;

internal void
_ErrorCallback(int error, const char* description) {
    std::cerr << "[Err] GLFW: " << description << std::endl;
}

internal void
_KeyCallback(GLFWwindow *window, i32 key, i32 scode, i32 action, i32 mods) {
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

internal u32
LoadAndCompileShader(std::string filename, GLuint shaderType) {
    u32 ShaderID = 0;
    std::ifstream In(filename);
    std::string Str;

    In.seekg(0, std::ios::end);
    Str.reserve(In.tellg());
    In.seekg(0, std::ios::beg);

    Str.assign((std::istreambuf_iterator<char>(In)), std::istreambuf_iterator<char>());
    const char *CharContent = Str.c_str();

    ShaderID = glCreateShader(shaderType);
    glShaderSource(ShaderID, 1, &CharContent, NULL);
    glCompileShader(ShaderID);

    int Success;
    char InfoLog[512];
    glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &Success);
    if (!Success) {
        glGetShaderInfoLog(ShaderID, 256, NULL, InfoLog);
        std::cout << "Error while compiling shader:" << std::endl << InfoLog << std::endl;
        return 0;
    }

    return ShaderID;
}

internal u32
CreateBasicProgram(u32 vShader, u32 fShader) {
    u32 ProgramID = 0;
    ProgramID = glCreateProgram();
    glAttachShader(ProgramID, vShader);
    glAttachShader(ProgramID, fShader);
    glLinkProgram(ProgramID);

    i32 Success;
    char InfoLog[512];
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Success);
    if(!Success) {
        glGetProgramInfoLog(ProgramID, 512, NULL, InfoLog);
        std::cerr << "[Err] Failed to link shader program:" << std::endl << InfoLog << std::endl;
        return 0;
    }

    return ProgramID;
}

i32
main() {

    std::cout << "Hello" << std::endl;
    if(!glfwInit()) {
        std::cout << "Failed to init GLFW" << std::endl;
        return -1;
    }
    glfwSetErrorCallback(_ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow *Window = glfwCreateWindow(G_WWIDTH, G_WHEIGHT, "Frag!", 0, 0);
    if(!Window) {
        std::cerr << "[Err] GLFW: Failed creating window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetKeyCallback(Window, _KeyCallback);
    glfwMakeContextCurrent(Window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    r32 Vertices[] = {
        -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f
    };

    u32 Indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    u32 VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(r32), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(r32), (void*)(3 * sizeof(r32)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    u32 BasicProgram, VShaderID, FShaderID;
    VShaderID = LoadAndCompileShader("../code/shaders/basic.vs", GL_VERTEX_SHADER);
    FShaderID = LoadAndCompileShader("../code/shaders/basic.fs", GL_FRAGMENT_SHADER);
    BasicProgram = CreateBasicProgram(VShaderID, FShaderID);
    glDetachShader(BasicProgram, VShaderID);
    glDetachShader(BasicProgram, FShaderID);
    glDeleteShader(VShaderID);
    glDeleteShader(FShaderID);


    while(!glfwWindowShouldClose(Window)) {
        glfwGetFramebufferSize(Window, &G_WWIDTH, &G_WHEIGHT);
        r32 AspectRatio = G_WWIDTH / (float) G_WHEIGHT;
        glViewport(0, 0, G_WWIDTH, G_WHEIGHT);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(BasicProgram); 
        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glDrawElements(GL_TRIANGLES, sizeof(Indices), GL_UNSIGNED_INT, 0);
       
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);


        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    glfwDestroyWindow(Window);
    glfwTerminate();
    return 0;
}
