#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#if __cplusplus <= 199711L
#define nullptr NULL
#endif
#include <cmath>
#include <stdlib.h>
#include <cassert>
#include <AntTweakBar.h>
#include "stb_image.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#define RAD2DEG(X) (float)(360.0*(X)/(2.0*M_PI))
#define DEG2RAD(X) (float)(2.0*M_PI*(X)/360.0)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const GLchar *vxShaderSrc = R"(
#version 150 core

in vec3 position;
in vec4 color;
in vec2 texcoord;
uniform mat4 model, view, proj;

out vec4 Color;
out vec2 Texcoord;

void main()
{
    Color = color;
    Texcoord = texcoord;
    gl_Position = proj * view * model * vec4(position, 1.0);
}
)";

const GLchar *fragShaderSrc = R"(
#version 150 core

in vec4 Color;
in vec2 Texcoord;
uniform float time, force, amp;
out vec4 outColor;
uniform sampler2D tex, tex2;

void main()
{
    vec4 texC1 = texture(tex, Texcoord),
         texC2 = texture(tex2, vec2(Texcoord.x + force*sin((Texcoord.y * 60.0 + time)*amp) / 3.0, Texcoord.y));
    outColor = vec4(mix(texC1, texC2, 1.0).rgb, Texcoord.y);

}
)";

unsigned char* loadImage(const char* file, int &width, int &height)
{
    int channels;
    unsigned char* tmp = stbi_load(file, &width, &height, &channels, STBI_rgb_alpha);

        if (tmp && width && height) {
            unsigned char* ptr = (unsigned char*)malloc(4*sizeof(unsigned char)*width*height);
            assert(ptr);
            // invert the image vertically
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                    for (int k = 0; k < 4; ++k)
                        ptr[(x+(height-1-y)*width)*4+k] = tmp[(x+y*width)*4+k];
            stbi_image_free(tmp);
            return ptr;
        }
        std::cerr << "Error loading image " << file << "\n";
        return nullptr;
}

inline void freeImage(unsigned char* ptr) { stbi_image_free(ptr); }

inline void TwEventMouseButtonGLFW3(GLFWwindow* /*window*/, int button, int action, int /*mods*/)
{TwEventMouseButtonGLFW(button, action);}
inline void TwEventMousePosGLFW3(GLFWwindow* /*window*/, double xpos, double ypos)
{TwMouseMotion(int(xpos), int(ypos));}
inline void TwEventMouseWheelGLFW3(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset)
{TwEventMouseWheelGLFW(yoffset);}
inline void TwEventKeyGLFW3(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/)
{TwEventKeyGLFW(key, action);}
inline void TwEventCharGLFW3(GLFWwindow* /*window*/, int codepoint)
{TwEventCharGLFW(codepoint, GLFW_PRESS);}
inline void TwWindowSizeGLFW3(GLFWwindow* /*window*/, int width, int height)
{TwWindowSize(width, height);}


int main()
{
    if (!glfwInit()) {
        std::cerr<<"Error initializing glfw...\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#ifdef __APPLE__ // TODO is it ok to use it on Windows and Linux?
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    TwBar *bar;
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL", nullptr, nullptr); // Windowed
    if (!window) {
        std::cerr<<"Error creating window...\n";
        glfwTerminate();
        return 2;
    }
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // can be GLFW_CURSOR_HIDDEN

     // Initialize AntTweakBar
    TwInit(TW_OPENGL_CORE, NULL);

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create a tweak bar
    bar = TwNewBar("TweakBar");
    TwWindowSize(800, 600);
    int wire = 0;
    float force = 0.05, amp = 1.f, size = 1.f;
    float bgColor[] = { 0.1f, 0.2f, 0.4f };
    glm::vec3 cameraPos(1.f, 1.f, 1.f);
    glm::quat quatRot;
    TwDefine(" GLOBAL help='This example shows how to integrate AntTweakBar with GLFW and OpenGL.' "); // Message added to the help bar.
    // Add 'wire' to 'bar': it is a modifable variable of type TW_TYPE_BOOL32 (32 bits boolean). Its key shortcut is [w].
    TwAddVarRW(bar, "wire", TW_TYPE_BOOL32, &wire, 
               " label='Wireframe mode' key=w help='Toggle wireframe display mode.' ");
    // Add 'bgColor' to 'bar': it is a modifable variable of type TW_TYPE_COLOR3F (3 floats color)
    TwAddVarRW(bar, "bgColor", TW_TYPE_COLOR3F, &bgColor, " label='Background color' ");
    // Add 'speed' to 'bar': it is a modifable (RW) variable of type TW_TYPE_DOUBLE. Its key shortcuts are [s] and [S].
    TwAddVarRW(bar, "force", TW_TYPE_FLOAT, &force, 
               " label='Rot speed' min=0.01 max=1 step=.01 keyIncr=f keyDecr=F help='Rotation speed (turns/second)' ");
    TwAddVarRW(bar, "amp", TW_TYPE_FLOAT, &amp, 
               " label='Amplitude' min=0.01 max=10 step=.01 keyIncr=a keyDecr=A help='sin amplitude' ");
    TwAddVarRW(bar, "size", TW_TYPE_FLOAT, &size, 
               " label='Size' min=0.1 max=5 step=.1 help='Size of scale' ");
    TwAddVarRW(bar, "Rotation", TW_TYPE_QUAT4F, glm::value_ptr(quatRot), "opened=true axisz=-z");

    TwStructMember vec3Members[] = {
        { "x", TW_TYPE_FLOAT, offsetof(glm::vec3, x), "step=0.1" },
        { "y", TW_TYPE_FLOAT, offsetof(glm::vec3, y), "step=0.1" },
        { "z", TW_TYPE_FLOAT, offsetof(glm::vec3, z), "step=0.1" }
    };
    TwType vec3Type;
    vec3Type = TwDefineStruct("vec3", vec3Members, 3, sizeof(float)*3, NULL, NULL);
    TwAddVarRW(bar, "cameraPos", vec3Type, glm::value_ptr(cameraPos), "label='Camera Position'");


    // Set GLFW event callbacks
    // - Redirect window size changes to the callback function WindowSizeCB
    glfwSetWindowSizeCallback(window, (GLFWwindowposfun)TwWindowSizeGLFW3);

    glfwSetMouseButtonCallback(window, (GLFWmousebuttonfun)TwEventMouseButtonGLFW3);
    glfwSetCursorPosCallback(window, (GLFWcursorposfun)TwEventMousePosGLFW3);
    glfwSetScrollCallback(window, (GLFWscrollfun)TwEventMouseWheelGLFW3);
    glfwSetKeyCallback(window, (GLFWkeyfun)TwEventKeyGLFW3);
    glfwSetCharCallback(window, (GLFWcharfun)TwEventCharGLFW3);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_NO_ERROR) {
        std::cerr<<"Error initializing GLEW...\n";
        glfwTerminate();
        return 1;
    }

    GLuint vao;
    glGenVertexArrays(1, &vao);
    std::cout << "Created vertex array with id " << vao << "\n";
    glBindVertexArray(vao); // save the calls of vertexattribpointer and others, MUST BE before the EBO

    GLuint vbo;
    glGenBuffers(1, &vbo); // Generate 1 buffer
    std::cout << "Created Arrray with id " << vbo << "\n";
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLfloat vertices[] = {
        -0.5f, -0.5f, -0.5f, 1.f, 1.f, 1.0f, 1.f, 0.f, 0.f,
        -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.f, 0.f, 1.f,
        0.5f, -0.5f, -0.5f, 0.0f, 0.f, 1.0f, 1.f, 1.f, 0.f,
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.f, 1.f, 1.f,

        -0.5f, -0.5f, 0.5f, 1.f, 1.f, 1.0f, 1.f, 0.f, 0.f,
        -0.5f,  0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.f, 0.f, 1.f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.f, 1.0f, 1.f, 1.f, 0.f,
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.f, 1.f, 1.f,
    };

    // size is actually sizeof(float)*vertices.length
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // elemets buffer object allows to use the same vertex multiple times
    GLuint elements[] = {
        0, 1, 2, 3
    };
    GLuint ebo;
    glGenBuffers(1, &ebo);
    std::cout << "Created elements with id " << ebo << "\n";
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

    //texture
    GLuint tex, tex2;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex);
    unsigned char* img;
    int imgWidth, imgHeight;
    img = loadImage("data/grumpy.jpg", imgWidth, imgHeight);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, img);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // LINEAR is usually better
    glGenerateMipmap(GL_TEXTURE_2D);
    freeImage(img);

    glGenTextures(1, &tex2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex2);
    img = loadImage("data/doge.png", imgWidth, imgHeight);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, img);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // LINEAR is usually better
    glGenerateMipmap(GL_TEXTURE_2D);
    freeImage(img);

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
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), 0);
    GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 4, GL_FLOAT, GL_FALSE,
                          9*sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));
    GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE,
                           9*sizeof(float), (void*)(7*sizeof(float)));

    GLint uniPos = glGetUniformLocation(shaderProgram, "offset");

    glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "tex2"), 1);

    GLint timePos = glGetUniformLocation(shaderProgram, "time"),
          forcePos = glGetUniformLocation(shaderProgram, "force"),
          uniAmp = glGetUniformLocation(shaderProgram, "amp");

    glm::mat4 trans;
    trans = glm::rotate(trans, DEG2RAD(160.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    GLint uniTrans = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(trans));

    glm::mat4 view = glm::lookAt(
        cameraPos,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    GLint uniView = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

    glm::mat4 proj = glm::perspective(45.0f, 800.0f / 600.0f, 1.0f, 10.0f);
    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

    while(!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);

        glPolygonMode( GL_FRONT_AND_BACK, wire?GL_LINE:GL_FILL );

        float time =  (float)glfwGetTime();
        glUniform1f(timePos, time*2.f);
        glUniform1f(forcePos, force);
        glUniform1f(uniAmp, amp);
        quatRot = glm::rotate(quatRot, DEG2RAD(1), glm::vec3(0.f, 0.f, 1.f));
        //trans = glm::scale(glm::mat4_cast(quatRot), glm::vec3(size*sin(time*4)));
        //trans = glm::rotate(trans, DEG2RAD(time*6), glm::vec3(0.0f, 0.0f, 1.0f));
        glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(glm::mat4_cast(quatRot)));

        view = glm::lookAt(
            cameraPos,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

        // Clear the screen to black
        glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //glUniform2f(uniPos, cos(time*4.f)*0.5f, sin(time*4.f)*0.5f);
        glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0); // we can share vertex and we specify indexes

        glUniform2f(uniPos, 0.f, 0.f);
        // Draw tweak bars
        glUseProgram(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        TwDraw();
        glUseProgram(shaderProgram);
        glBindVertexArray(vao);
        //glBindBuffer(GL_ARRAY_BUFFER, vbo);
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

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

    TwTerminate();
    glfwTerminate();

    return 0;
}
