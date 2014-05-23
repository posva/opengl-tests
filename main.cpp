#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
//Win: mingw32-g++.exe main.cpp -std=c++11 -DGLEW_STATIC -lglew32 -lglfw3 -lglu32 -lopengl32 -lglut32 -lgdi32
// OS X: g++ main.cpp -std=c++11 -DGLEW_STATIC -lGLEW -lglfw3 -framework OpenGL

const GLchar *vxShaderSrc = R"(
#version 150 core

in vec2 position;
in vec4 color;
uniform vec2 offset;

out vec4 Color;

void main()
{
    Color = sin(4.f*vec4(offset, 1.f, 1.f))*0.5f+0.5f;
    gl_Position = vec4(position+offset, 0.0, 1.0);
}
)";

const GLchar *fragShaderSrc = R"(
#version 150 core
in vec4 Color;
out vec4 outColor;

void main()
{
    outColor = 1.f-Color;
}
)";

int main()
{
    if (!glfwInit()) {
        std::cerr<<"Error initializing glfw...\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL", nullptr, nullptr); // Windowed
    if (!window) {
        std::cerr<<"Error creating Window...\n";
        return 2;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_NO_ERROR) {
        std::cerr<<"Error initializing GLEW...\n";
        return 1;
    }

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao); // save the calls of vertexattribpointer and others, MUST BE before the EBO

    GLuint vbo;
    glGenBuffers(1, &vbo); // Generate 1 buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLfloat vertices[] = {
        0.0f,  0.5f, 1.0f, 0.0f, 0.0f, 1.f, // Vertex 1: Red
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.f, // Vertex 2: Green
        -0.5f, -0.5f, 0.0f, 0.30f, 1.0f, 1.f, // Vertex 3: Blue
        // some extra for elements testing
        0.0f,  0.5f, 1.0f, 0.0f, 0.0f, 1.f, // Vertex 1: Red
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.f, // Vertex 2: Green
        -0.25f, -0.5f, 0.0f, 0.f, 1.0f, 1.f // Vertex 3: Blue
    };

    // size is actually sizeof(float)*vertices.length
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // elemets buffer object allows to use the same vertex multiple times
    GLuint elements[] = {
        3, 4, 5
    };
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

    // create shaders
    GLint status; // for error checking
    char buffer[512];
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vxShaderSrc, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
        std::cerr<<"Error in shader: "<<buffer<<"\n";
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragShaderSrc, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
        std::cerr<<"Error in shader: "<<buffer<<"\n";
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    // bind the output
    // this is the actual output by default
    glBindFragDataLocation(shaderProgram, 0, "outColor");

    glLinkProgram(shaderProgram); // called everytime somethign change for the shader
    glUseProgram(shaderProgram);

    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), 0);
    GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE,
                          6*sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));

    GLint uniPos = glGetUniformLocation(shaderProgram, "offset");

    while(!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);

        float time =  (float)glfwGetTime();
        glUniform2f(uniPos, -.5f, sin(time*4.f)*0.5f);

        // Clear the screen to black
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES, 0, 3); // arrays need to specify  every vertex
        glUniform2f(uniPos, cos(time*4.f)*0.5f+.5f, sin(time*4.f)*0.5f);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0); // we can share vertex and we specify indexes

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // clean up
    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);

    glDeleteVertexArrays(1, &vao);

    glfwTerminate();

    return 0;
}
