/*
Digital Interactive Background for Live Performances
v1.0

MIT License

Copyright (c) 2024 The Nashville Nights Band LLC, Vreiras Technologies

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define GLFW_INCLUDE_NONE
#define PI 3.1415926535897932384626
#define MAX_ULONG_STR ((ULONG) sizeof("4294967295"))

#define D_COLOR1 0
#define D_COLOR2 1
#define D_COLOR3 2
#define D_FLAGS 3

#define F_SLIDESHOW_MODE 0x01
#define F_AUTOSTART 0x02
#define F_BASELIGHT 0x04
#define F_METAPOSTS 0x08

#define C_BLACK 0
#define C_RED 1
#define C_GREEN 2
#define C_BLUE 3
#define C_CYAN 4
#define C_MAGENTA 5
#define C_YELLOW 6
#define C_WHITE 7

#define D_DOWNBEAT 4
#define D_VENUENAME 8
#define D_NAMESIZE 248

#define T_ERROR -1
#define T_LOADING 0
#define T_RUNNING 1
#define T_WAITING 2
#define T_STOPPING 3
#define T_STOPPED 4

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <map>
#include <string>
#include <stdlib.h>
#include <windows.h>
#include <http.h>
#include <math.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#include <Audioclient.h>
#include <Audiopolicy.h>

#define INITIALIZE_HTTP_RESPONSE( resp, status, reason )    \
    do                                                      \
    {                                                       \
        RtlZeroMemory( (resp), sizeof(*(resp)) );           \
        (resp)->StatusCode = (status);                      \
        (resp)->pReason = (reason);                         \
        (resp)->ReasonLength = (USHORT) strlen(reason);     \
    } while (FALSE)

#define ADD_KNOWN_HEADER(Response, HeaderId, RawValue)               \
    do                                                               \
    {                                                                \
        (Response).Headers.KnownHeaders[(HeaderId)].pRawValue =      \
                                                          (RawValue);\
        (Response).Headers.KnownHeaders[(HeaderId)].RawValueLength = \
            (USHORT) strlen(RawValue);                               \
    } while(FALSE)

int streq(char* a, const char* b, int start, int stop) {
    for (int i = start; i < stop; i++) {
        char aa = a[i];
        char bb = b[i];
        if (aa >= 'a' && aa <= 'z') aa -= 0x20;
        if (bb >= 'a' && bb <= 'z') bb -= 0x20;
        if (aa != bb) return 0;
        if (aa == 0) return 1;
    }
    return 1;
}

float colors[8][3] = {
    {0.0, 0.0, 0.0}, //BLACK
    {1.0, 0.1, 0.1}, //RED
    {0.1, 1.0, 0.1}, //GREEN
    {0.1, 0.1, 1.0}, //BLUE
    {0.0, 0.8, 0.8}, //CYAN
    {0.8, 0.0, 0.8}, //MAGENTA
    {0.8, 0.8, 0.0}, //YELLOW
    {0.7, 0.7, 0.7}, //WHITE
};

const char* colorNames[8] = {
    "\033[0;90mBLACK\033[0m",
    "\033[0;91mRED\033[0m",
    "\033[0;92mGREEN\033[0m",
    "\033[0;94mBLUE\033[0m",
    "\033[0;96mCYAN\033[0m",
    "\033[0;95mMAGENTA\033[0m",
    "\033[0;93mYELLOW\033[0m",
    "\033[0;97mWHITE\033[0m"
};

#define URL_COUNT 2
const LPCWSTR urls[URL_COUNT] = {
    L"http://localhost:80/",
    L"http://127.0.0.1:80/"
};

typedef struct threadData {
    int status;
    char data[256];
} TDATA;

int SCR_WIDTH, SCR_HEIGHT;

void errorCallback(int code, const char* desc) {
    std::cout << "\033[0;91m" << desc << std::endl << "Error Code: " << std::hex << code << "\033[0m" << std::endl;
}

void resizeCanvas(GLFWwindow* window, int w, int h) {
    glViewport(0, 0, w, h);
    SCR_WIDTH = w;
    SCR_HEIGHT = h;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
}

unsigned char* loadImage(const char* path, int* w, int* h, int* c) {
    unsigned char* data = stbi_load(path, w, h, c, 0);
    if (!data) {
        errorCallback(-1, "Unable to load texture!");
        glfwTerminate();
        return NULL;
    }
    return data;
}

char* readFile(const char* path, std::streamsize* size) {
    std::ifstream F;
    F.open(path, std::ios::ate | std::ios::binary);
    if (!F.is_open()) return NULL;
    std::streamsize s = F.tellg();
    s++;
    F.seekg(std::ios::beg);
    char* retval = (char*) HeapAlloc(GetProcessHeap(), 0, s);
    retval[s - 1] = EOF;
    F.read(retval, s);
    F.close();
    if (size != NULL) *size = s;
    return retval;
}
char* readFile(const char* path) { return readFile(path, NULL); }

char* readFileStr(const char* path, std::streamsize* size) {
    char* result = readFile(path, size);
    result[*size-1] = '\0';
    return result;
}
char* readFileStr(const char* path) {
    std::streamsize size;
    return readFileStr(path, &size);
}

unsigned int initShader(const char* vpath, const char* fpath) {
    char* vSource = readFileStr(vpath);
    char* fSource = readFileStr(fpath);

    unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vSource, NULL);
    glCompileShader(vShader);
    int success;
    char log[512];
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vShader, 512, NULL, log);
        errorCallback(-1, log);
        ExitProcess(-1);
    }

    unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fSource, NULL);
    glCompileShader(fShader);

    glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fShader, 512, NULL, log);
        errorCallback(-1, log);
        ExitProcess(-1);
    }

    HeapFree(GetProcessHeap(), 0, vSource);
    HeapFree(GetProcessHeap(), 0, fSource);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, log);
        errorCallback(-1, log);
        ExitProcess(-1);
    }

    glDeleteShader(vShader);
    glDeleteShader(fShader);
    std::cout << vpath << " and " << fpath << " compiled successfully." << std::endl;
    return program;
}

struct texture {
    unsigned int texture;
    int width;
    int height;
    int channels;
};

struct texture generateTexture(const char* path, unsigned int active) {
    glActiveTexture(active);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    struct texture texture;
    glGenTextures(1, &texture.texture);
    glActiveTexture(active);
    glBindTexture(GL_TEXTURE_2D, texture.texture);
    unsigned char* data = loadImage(path, &texture.width, &texture.height, &texture.channels);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return texture;
}

float textColor[3] = { 1.0f, 1.0f, 1.0f };
void setTextColor(float r, float g, float b) {
    textColor[0] = r;
    textColor[1] = g;
    textColor[2] = b;
}

struct Glyph {
    unsigned int texture;
    glm::ivec2 size;
    glm::ivec2 bearing;
    unsigned int advance;
};
std::map<char, Glyph> charMap;

void drawText(char* message, float x, float y, float size, unsigned int vbo) {
    float xinit = x;
    for (char* c = message; *c != '\0'; c++) {
        if (*c == '\n') {
            x = xinit;
            y += 68;
        }
        Glyph g = charMap[*c];
        float xpos = x + g.bearing.x * size;
        float ypos = y - (g.size.y - g.bearing.y) * size;
        float w = g.size.x * size;
        float h = g.size.y * size;
        float vertices[24] = {
            xpos,   ypos + h, 0.0f,   0.0f,
            xpos,   ypos,   0.0f,   1.0f,
            xpos + w, ypos,   1.0f,   1.0f,
            xpos,   ypos + h, 0.0f,   0.0f,
            xpos + w, ypos,   1.0f,   1.0f,
            xpos + w, ypos + h, 1.0f,   0.0f
        };
        glBindTexture(GL_TEXTURE_2D, g.texture);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 24 * sizeof(float), vertices);
        glGetBufferSubData(GL_ARRAY_BUFFER, 0, 24 * sizeof(float), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (g.advance >> 6) * size;
    }
}

const float IDENTITY_MATRIX_4X4_BECAUSE_I_CANT_TRUST_GLM_IMPLEMENTATION_FOR_SHIT[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

char readFlags(char* elByte, char mask) {
    return ((*elByte)&mask);
}

void writeFlags(char* elByte, char mask, char content) {
    *elByte = ((~mask & *elByte) | (mask & content));
}

DWORD WINAPI GLmain (LPVOID lpParam) {
    TDATA* threadData = (TDATA*) lpParam;
    std::cout << "GL thread initialized" << std::endl;
    threadData->status = T_LOADING;
    char* COLOR1 = threadData->data + D_COLOR1;
    char* COLOR2 = threadData->data + D_COLOR2;
    char* COLOR3 = threadData->data + D_COLOR3;
    char* FLAGS = threadData->data +  D_FLAGS;
    int* SHOWTIME = (int*)(threadData->data + D_DOWNBEAT);
    char* VENUE_NAME = threadData->data + D_VENUENAME;
    /*Map of thread data:
    * 0 [ COLOR1 ][ COLOR2 ][ COLOR3 ][0000MBAS]
    * 4 [            SHOWTIME (int)            ]
    * 8 [ VENUE N][AME GOES][HERE ...][  etc...]
    */

    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
        glfwTerminate();
        return glfwGetError(NULL);
    };
    std::cout << "GLFW: Initialized" << std::endl;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Nashville Nights Band Digital Banner", glfwGetPrimaryMonitor(), NULL);
    if (!window) {
        glfwTerminate();
        return glfwGetError(NULL);
    }
    glfwMakeContextCurrent(window);
    std::cout << "GLFW: Window Created" << std::endl;
    glfwSetFramebufferSizeCallback(window, resizeCanvas);

    GLFWimage icon;
    icon.pixels = stbi_load("./img/icon.png", &icon.width, &icon.height, 0, STBI_rgb_alpha);
    if (icon.pixels == NULL) {
        errorCallback(-1, stbi_failure_reason());
    }
    glfwSetWindowIcon(window, 1, &icon);
    stbi_image_free(icon.pixels);
    std::cout << "Image loaded!" << std::endl;

    gladLoadGL();
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    std::cout << "Confiruging GL Viewport..." << std::endl;
    resizeCanvas(window, 1920, 1080);

    unsigned int BGprogram = initShader("shader\\bgmain.vs", "shader\\light.fs");

    unsigned int bloom = initShader("shader\\flat.vs", "shader\\bloom.fs");

    unsigned int assembly = initShader("shader\\flat.vs", "shader\\assembly.fs");

    unsigned int fullbanner = initShader("shader\\flat.vs", "shader\\flat.fs");

    unsigned int textprog = initShader("shader\\text.vs", "shader\\text.fs");

    unsigned int dots = initShader("shader\\dot.vs", "shader\\flat.fs");

    std::cout << "Shaders Compiled!" << std::endl;

    float vertices[] = {
        //POSITIONAL VERTICES       TEXTURE COORDS
        -1.0f,  -1.0f,  -1.0f,      0.0f,   1.0f,   //0
        -1.0f,  1.0f,   -1.0f,      0.0f,   0.0f,   //1
        1.0f,   1.0f,   -1.0f,      1.0f,   0.0f,   //2
        1.0f,   -1.0f,  -1.0f,      1.0f,   1.0f    //3
    };
    unsigned int indices[] = {
        1, 0, 2,
        3, 2, 0
    };
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    float slY1 = 0.7657407407f;
    float overlayVertices[] = {
        //POSITIONAL VERTICES       TEXTURE COORDS
        -1.0f,  -1.0f,  1.0f,       0.0f,   0.0f,   //0
        -1.0f,  1.0f,   1.0f,       0.0f,   1.0f,   //1
        1.0f,   1.0f,   1.0f,       1.0f,   1.0f,   //2
        1.0f,   -1.0f,  1.0f,       1.0f,   0.0f,   //3
    };
    float slideVertices[] = {
        -1.0f,  -slY1,  0.0f,       0.0f,   0.0f,   //0
        -1.0f,  slY1,   0.0f,       0.0f,   1.0f,   //1
        1.0f,   slY1,   0.0f,       1.0f,   1.0f,   //2
        1.0f,   -slY1,  0.0f,       1.0f,   0.0f    //3
    };
    float dotVertices[] = {
        0.0f,  -1.0f,  0.0f,      0.0f,   0.0f,   //0
        0.0f,  1.0f,   0.0f,      0.0f,   1.0f,   //1
        2.0f,   1.0f,   0.0f,     1.0f,   1.0f,   //2
        2.0f,   -1.0f,  0.0f,     1.0f,   0.0f,   //3
    };
    unsigned int oVBO;
    glGenBuffers(1, &oVBO);
    unsigned int oVAO;
    glGenVertexArrays(1, &oVAO);
    glBindVertexArray(oVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, oVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(overlayVertices), overlayVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int sVBO;
    glGenBuffers(1, &sVBO);
    unsigned int sVAO;
    glGenVertexArrays(1, &sVAO);
    glBindVertexArray(sVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(slideVertices), slideVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int tVBO;
    glGenBuffers(1, &tVBO);
    unsigned int tVAO;
    glGenVertexArrays(1, &tVAO);
    glBindVertexArray(tVAO);

    glBindBuffer(GL_ARRAY_BUFFER, tVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*6*4, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int dVBO;
    glGenBuffers(1, &dVBO);
    unsigned int dVAO;
    glGenVertexArrays(1, &dVAO);
    glBindVertexArray(dVAO);

    glBindBuffer(GL_ARRAY_BUFFER, dVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(dotVertices), dotVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    std::cout << "Generating Textures..." << std::endl;
    struct texture texture = generateTexture("./img/nnb.png", GL_TEXTURE0);
    struct texture normal = generateTexture("./img/normal.png", GL_TEXTURE0);
    struct texture specular = generateTexture("./img/alpha.png", GL_TEXTURE0);

    unsigned int FBO[3];
    unsigned int cbuffers[4];
    glGenFramebuffers(1, FBO);
    glGenFramebuffers(2, FBO + 1);
    glGenTextures(4, cbuffers);
    for (int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, FBO[0]);
        glBindTexture(GL_TEXTURE_2D, cbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, cbuffers[i], 0);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO[i + 1]);
        glBindTexture(GL_TEXTURE_2D, cbuffers[i + 2]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cbuffers[i + 2], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[0]);
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    GLint uPM = glGetUniformLocation(BGprogram, "uPM");
    GLint uRL = glGetUniformLocation(BGprogram, "uRL");
    GLint uGL = glGetUniformLocation(BGprogram, "uGL");
    GLint uBL = glGetUniformLocation(BGprogram, "uBL");
    GLint uWL = glGetUniformLocation(BGprogram, "uWL");
    GLint uTS = glGetUniformLocation(BGprogram, "diff");
    GLint uNS = glGetUniformLocation(BGprogram, "norm");
    GLint uSS = glGetUniformLocation(BGprogram, "smap");
    GLint uRC = glGetUniformLocation(BGprogram, "red");
    GLint uGC = glGetUniformLocation(BGprogram, "green");
    GLint uBC = glGetUniformLocation(BGprogram, "blue");

    GLint bBB = glGetUniformLocation(bloom, "bb");
    GLint bH = glGetUniformLocation(bloom, "horizontal");
    GLint bW = glGetUniformLocation(bloom, "weight");

    GLint cE = glGetUniformLocation(assembly, "exposure");
    GLint cF = glGetUniformLocation(assembly, "frag");
    GLint cB = glGetUniformLocation(assembly, "bloom");
    GLint cX = glGetUniformLocation(assembly, "xOffs");

    GLint sX = glGetUniformLocation(fullbanner, "xOffs");

    GLint tP = glGetUniformLocation(textprog, "proj");
    GLint tT = glGetUniformLocation(textprog, "text");
    GLint tC = glGetUniformLocation(textprog, "textColor");

    GLint dO = glGetUniformLocation(dots, "offs");
    GLint dR = glGetUniformLocation(dots, "rot");

    std::cout << "Loading font..." << std::endl;
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cout << "Couldn't load FreeType." << std::endl;
        ExitProcess(-1);
    }
    FT_Face face;
    if (FT_New_Face(ft, "./fonts/Times New Roman Bold.ttf", 0, &face)) {
        std::cout << "Couldn't load font." << std::endl;
        ExitProcess(-1);
    }
    FT_Set_Pixel_Sizes(face, 0, 92);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "Failed to load '" << c << "'." << std::endl;
            continue;
        }
        unsigned int tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        FT_GlyphSlot g = face->glyph;
        FT_Bitmap b = g->bitmap;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, b.width, b.rows, 0, GL_RED, GL_UNSIGNED_BYTE, b.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        Glyph character;
        character.texture = tex;
        character.size = glm::ivec2(b.width, b.rows);
        character.bearing = glm::ivec2(g->bitmap_left, g->bitmap_top);
        character.advance = g->advance.x;
        charMap.insert(std::pair<char, Glyph>(c, character));
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    float rl[3] = { 5.0f, 1.5f, -0.1f };
    float gl[3] = { -5.0f, 1.5f, -0.1f };
    float bl[3] = { 0.0f, 1.5f, -0.1f };
    float wl[3] = { 0.0f,-2.0f, -0.1f };
    float rc[3] = { 0.0f,0.0f,0.0f };
    float gc[3] = { 0.0f,0.0f,0.0f };
    float bc[3] = { 0.0f,0.0f,0.0f };

    unsigned int frameCount = 0;
    double phase = 0;
    float slideTransition = 0.0f;
    float bannerTransition = 0.0f;
    unsigned int slideID = 0;
    unsigned int slides[32];
    unsigned int slideCount = 0;
    char* fileContents;
    char slidePath[60] = "./http/slides/s0.png";
    std::ifstream slideFile;
    for (; slideCount < 32; slideCount++) {
        sprintf_s(slidePath, "./http/slides/s%d.png",slideCount);
        slideFile.open(slidePath);
        if (!slideFile.is_open()) break; //No more slides
        slideFile.close();
        slides[slideCount] = generateTexture(slidePath, GL_TEXTURE2 + slideCount).texture;
    }
    unsigned int slideOverlay = generateTexture("./img/90banner.png", GL_TEXTURE0).texture;
    unsigned int dotMatrix = generateTexture("./img/dotmatrix.png", GL_TEXTURE1).texture;

    threadData->status = T_RUNNING;
    double time_span = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        std::chrono::high_resolution_clock::time_point before = std::chrono::high_resolution_clock::now();
        GLint vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);
        processInput(window);
        if (readFlags(FLAGS, F_SLIDESHOW_MODE) == 0) {
            glm::mat4 pm = glm::perspective(2.65625f, (1.0f * vp[2]) / vp[3], 0.1f, 100.0f);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(BGprogram);
            glUniformMatrix4fv(uPM, 1, true, &pm[0][0]);
            glUniform3fv(uRL, 1, rl);
            glUniform3fv(uGL, 1, gl);
            glUniform3fv(uBL, 1, bl);
            glUniform3fv(uWL, 1, wl);
            glUniform3fv(uRC, 1, rc);
            glUniform3fv(uGC, 1, gc);
            glUniform3fv(uBC, 1, bc);
            for (int i = 0; i < 3; i++) {
                float rt = colors[*COLOR1][i];
                if (rc[i] > rt) rc[i] -= 0.01;
                else if (rc[i] < rt) rc[i] += 0.01;
                float gt = colors[*COLOR2][i];
                if (gc[i] > gt) gc[i] -= 0.01;
                else if (gc[i] < gt) gc[i] += 0.01;
                float bt = colors[*COLOR3][i];
                if (bc[i] > bt) bc[i] -= 0.01;
                else if (bc[i] < bt) bc[i] += 0.01;
            }
            glUniform1i(uTS, 0);
            glUniform1i(uNS, 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture.texture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, normal.texture);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, specular.texture);
            glBindVertexArray(VAO);
            glBindFramebuffer(GL_FRAMEBUFFER, FBO[0]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glUseProgram(bloom);
            glUniform1i(bBB, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindVertexArray(VAO);

            glUniform1i(bH, GL_TRUE);
            glBindFramebuffer(GL_FRAMEBUFFER, FBO[1]);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindTexture(GL_TEXTURE_2D, cbuffers[1]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glUniform1i(bH, GL_FALSE);
            glBindFramebuffer(GL_FRAMEBUFFER, FBO[2]);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindTexture(GL_TEXTURE_2D, cbuffers[2]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            for (int i = 0; i < 6; i++) {
                glUniform1i(bH, GL_TRUE);
                glBindFramebuffer(GL_FRAMEBUFFER, FBO[1]);
                glClear(GL_COLOR_BUFFER_BIT);
                glBindTexture(GL_TEXTURE_2D, cbuffers[3]);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                glUniform1i(bH, GL_FALSE);
                glBindFramebuffer(GL_FRAMEBUFFER, FBO[2]);
                glClear(GL_COLOR_BUFFER_BIT);
                glBindTexture(GL_TEXTURE_2D, cbuffers[2]);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            glUseProgram(assembly);
            glUniform1f(cX, 0.0f);
            glUniform1f(cE, 1.0f);
            glUniform1i(cF, 0);
            glUniform1i(cB, 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cbuffers[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, cbuffers[3]);
            glBindVertexArray(VAO);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            slideTransition = 0;
        }else {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            for (int i = 0; i < 3; i++) {
                rc[i] = 0;
                gc[i] = 0;
                bc[i] = 0;
            }
            glUseProgram(dots);
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0), (float)PI / 3, glm::vec3(0, 0, 1));
            for (int i = 0; i < 4; i++) rotation[i][1] = rotation[i][1] * vp[2] / vp[3];
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, dotMatrix);
            glBindVertexArray(dVAO);
            glUniformMatrix4fv(dR, 1, GL_FALSE, &rotation[0][0]);
            glUniform1f(dO, (float)0x3p-13 * frameCount);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            rotation = glm::rotate(glm::mat4(1.0), (float)PI*7 / 6, glm::vec3(0, 0, 1));
            for (int i = 0; i < 4; i++) rotation[i][1] = rotation[i][1] * vp[2] / vp[3];
            glUniformMatrix4fv(dR, 1, GL_FALSE, &rotation[0][0]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            
            glUseProgram(fullbanner);
            glUniform1f(sX, 0.0f);
            glBindTexture(GL_TEXTURE_2D, slides[slideID]);
            glBindVertexArray(sVAO);
            glUniform1f(sX, -slideTransition);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glUniform1f(sX, -slideTransition + 2.0f);
            int nextSlide = slideID + 1;
            if (nextSlide >= slideCount) nextSlide = 0;
            glBindTexture(GL_TEXTURE_2D, slides[nextSlide]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glUniform1f(sX, 0.0f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, slideOverlay);
            glBindVertexArray(oVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            setTextColor(1.0f, 1.0f, 1.0f);
            glm::mat4 orth = glm::ortho(0.0f, (float)vp[2], 0.0f, (float)vp[3], -1.f, 1.f);
            glUseProgram(textprog);
            glUniformMatrix4fv(tP, 1, GL_FALSE, &orth[0][0]);
            glUniform3fv(tC, 1, textColor);
            glActiveTexture(GL_TEXTURE0);
            glBindVertexArray(tVAO);
            char message[32] = "Current time = 11:11 PM";
            const std::time_t now = std::time(0);
            std::tm t;
            localtime_s(&t, &now);
            int hr = t.tm_hour;
            int mn = t.tm_min;
            char pm = 'A';
            if (hr >= 12) {
                hr -= 12;
                pm = 'P';
            }
            if (hr == 0) hr = 12;
            sprintf_s(message, "Current time: %d%d:%d%d %cM", hr/10, hr%10, mn/10, mn%10, pm);
            drawText(message, 100, vp[3] - 50, 0.5f, tVBO);

            hr = *SHOWTIME / 60;
            mn = *SHOWTIME % 60;
            if (readFlags(FLAGS, F_AUTOSTART) && hr == t.tm_hour && mn == t.tm_min) writeFlags(FLAGS, F_SLIDESHOW_MODE, 0);
            pm = 'A';
            if (hr >= 12) {
                hr -= 12;
                pm = 'P';
            }
            if (hr == 0) hr = 12;
            sprintf_s(message, "Showtime: %d%d:%d%d %cM", hr / 10, hr % 10, mn / 10, mn % 10, pm);
            drawText(message, 100, vp[3] - 105, 0.5f, tVBO);

            setTextColor(0.97647f, 0.92549f, 0.35686f);
            glUniform3fv(tC, 1, textColor);
            drawText(VENUE_NAME, 650, vp[3] - 90, 0.7f, tVBO);
            
            if ((frameCount & 1023) == 0) phase = 0;
            if (phase < PI) {
                phase += 0.03125;
                slideTransition = -cos(phase)+1;
                if (phase >= PI) {
                    phase = PI;
                    slideTransition = 0;
                    slideID++;
                    if (slideID >= slideCount) slideID = 0;
                }
            }
        }
        frameCount++;
        rl[0] = 6 * sin(phase);
        gl[0] = 6 * sin(phase + 2 * PI / 3);
        bl[0] = 6 * sin(phase - 2 * PI / 3);
        rl[1] = 1.5f + abs(cos(phase));
        gl[1] = 1.5f + abs(cos(phase + 2 * PI / 3));
        bl[1] = 1.5f + abs(cos(phase - 2 * PI / 3));

        std::chrono::high_resolution_clock::time_point after = std::chrono::high_resolution_clock::now();
        time_span = std::chrono::duration_cast<std::chrono::duration<double>>(after - before).count();
        double fps = 1 / time_span;
        phase += time_span/5;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    threadData->status = T_STOPPED;
    ExitProcess(0);
    return 0;
}

DWORD WINAPI CLImain(LPVOID lpParam) {
    TDATA* threadData = (TDATA*)lpParam;
    std::cout << "GL thread initialized" << std::endl;
    LPWSTR hostdata = (LPWSTR)threadData->data;
    wchar_t hostname[16];
    for (int i = 0; i < 16; i++) hostname[i] = hostdata[i];
    system("cls");
    std::wcout << "\033[0;93m"
        << "              .',,;;;,,,;:clc;,,,,,..             " << std::endl
        << "          .,;,,..      .;lll'.',;'',;;,.          " << std::endl
        << "       .';;'.       ..',dOOkdx0K0c   .';:,.       " << std::endl
        << "      ,:,.         ... .:xO0000kl.      .,:,.     " << std::endl
        << "    ':;.           .,.  :xk00x;.          .;c'    " << std::endl
        << "   ;c.             :ko;;ldkOo.              .c;.  " << std::endl
        << " .::.    .'.      .ll,...,;,.                .::. " << std::endl
        << " :c.     .;.      .',,c:..                    .:: " << std::endl
        << ",l,.          ..    .:x;     90's Country      .l;" << std::endl
        << "c;...   .;,  .;,    .do.        Reboot         .:l" << std::endl
        << "l'..;:.  ..        .lk:                         .l" << std::endl
        << "l.  ..     ';.     ;kd.  .                      .l" << std::endl
        << "l.     .;.  .  .. .xO:   ..                     .l" << std::endl
        << "l.      .      :;.lKk,.,cl:...........          .l" << std::endl
        << ";c.           ;c,,lxKKKOol::d0kk0000O0o.       .c:" << std::endl
        << ".c;         'lc.   ;OK0o:;,.cOxoxkO00Ko.       ;c." << std::endl
        << " .c;       :o,.',,:x0K0l'''.,kxldkO000l.      ,c. " << std::endl
        << "  .:;.    ;x;  .dKKKKKKd;;;..o0kO0O0K0c     .;c.  " << std::endl
        << "   .;c'   lk;  'xKKKKKKd:::..okxkkxO0k:.   .:;.   " << std::endl
        << "     .::. 'xkllkKKKKKK0l,::,'cllkkxk0Ol'':c:.     " << std::endl
        << "       .;:,,cokOO0000kl:cloddddkxdxkkdc,;;.       " << std::endl
        << "         .,;;;,,'.''..............';c:;,.         " << std::endl
        << "            .',;;;,''........',,,,,,'.            " << std::endl
        << "                ..,:ccloddolcc:,..                " << std::endl << std::endl
        << "            The Nashville Nights Band             " << std::endl
        << "       The Ultimate 90's Country Experience       " << std::endl << "\033[0m" << std::endl
        << "Digital Interactive Banner | The Nashville Nights Band, LLC" << std::endl
        << "Developed by Vreiras Technologies" << std::endl << std::endl
        << "Control panel: http://" << hostname << std::endl
        << "Please ensure that this device is connected to band wifi." << std::endl
        << "Type \"HELP\" for a list of commands" << std::endl << std::endl << "> ";
    threadData->status = T_RUNNING;
    char command[256] = "";
    std::cin >> command;
    while (!streq(command, "EXIT", 0, 5)) {
        if (streq(command, "HELP", 0, 5)) {
            std::cout << "COLOR [1-3] [COLOR]: Change the color of the three lights\n"
                "COLORS: List the available colors\n"
                "EXIT: Exit the application. You may also focus the banner and press [ESC] (Please don't exit during the show!)\n"
                "HELP: Display this message\n"
                "BANNER: Switch to Banner display\n"
                "SLIDESHOW: Switch to Slideshow display\n"
                "MONITOR [1-2]: Switch display monitor\n"
                "ADDRESS: Display control panel URL\n"
                "DOWNBEAT [TIME]: Change show start time (military 24-hour time HHMM)\n"
                "VENUE [NAME]: Change the name of the venue to be displayed\n"
                "AUTOSTART: Automatically switch slideshow off at showtime\n";
        }else if(streq(command, "AUTOSTART", 0, 10)){
            threadData->data[0] = 'a';
            threadData->status = T_WAITING;
            while (threadData->status == T_WAITING) {}
            std::cout << "Autostart is now ";
            if (threadData->data[1]) std::cout << "ENABLED." << std::endl;
            else std::cout << "DISABLED." << std::endl;
        }else if(streq(command, "VENUE", 0, 6)){
            std::string venue;
            std::getline(std::cin, venue);
            sprintf_s(threadData->data + 1, D_NAMESIZE, venue.c_str()+1);
            threadData->data[0] = 'v';
            threadData->status = T_WAITING;
            while (threadData->status == T_WAITING) {}
            std::cout << "Updated venue name to \"" << threadData->data + 1 << "\"" << std::endl;
        }else if (streq(command, "DOWNBEAT", 0, 9)) {
            std::cin >> command;
            if (strlen(command) != 4) {
                std::cout << "Please enter a time in 24-hour military format.\n"
                    "Example: 0800 (8:00 AM), 2000 (8:00 PM)" << std::endl;
                continue;
            }
            int minutes = 0;
            minutes += (command[0] - '0') * 600;
            minutes += (command[1] - '0') * 60;
            if (minutes > 60 * 23) {
                std::cout << "Please enter a time in 24-hour military format.\n"
                    "Example: 0800 (8:00 AM), 2000 (8:00 PM)" << std::endl;
                continue;
            }
            minutes += (command[2] - '0') * 10;
            minutes += (command[3] - '0');
            if (minutes >= 60 * 24) {
                std::cout << "Please enter a time in 24-hour military format.\n"
                    "Example: 0800 (8:00 AM), 2000 (8:00 PM)" << std::endl;
            }

            *((int*)(threadData->data + 4)) = minutes;

            threadData->data[0] = 't';
            threadData->status = T_WAITING;
            while (threadData->status == T_WAITING) {}
            std::cout << "Time updated." << std::endl;
        }else if (streq(command, "ADDRESS", 0, 8)) {
            std::wcout << "Address: http://" << hostname << std::endl;
            std::cout << "HTTP port 80" << std::endl;
            std::cout << "Ensure both your device and this device are connected to the Nashville Nights Band Wifi" << std::endl;
        }else if (streq(command, "COLORS", 0, 7)) {
            threadData->data[0] = 'r';
            threadData->status = T_WAITING;
            while (threadData->status == T_WAITING) {}
            std::cout << "Current light colors:" << std::endl;
            for (int i = 0; i < 3; i++) std::cout << i + 1 << ": " << colorNames[threadData->data[i]] << std::endl;
            std::cout << "Available light colors:" << std::endl;
            for (int i = 0; i < 8; i++) std::cout << colorNames[i] << std::endl;
        }else if (streq(command, "COLOR", 0, 6)) {
            char id;
            std::cin >> id;
            if (id >= '1' && id <= '3') {
                id -= '1';
                char tg = -1;
                std::cin >> command;
                if (streq(command, "BLACK", 0, 6))          tg = 0;
                else if (streq(command, "RED", 0, 4))       tg = 1;
                else if (streq(command, "GREEN", 0, 6))     tg = 2;
                else if (streq(command, "BLUE", 0, 5))      tg = 3;
                else if (streq(command, "CYAN", 0, 5))      tg = 4;
                else if (streq(command, "MAGENTA", 0, 8))   tg = 5;
                else if (streq(command, "YELLOW", 0, 7))    tg = 6;
                else if (streq(command, "WHITE", 0, 6))     tg = 7;
                else {
                    std::cout << "Invalid color \"" << command << "\". Type \"COLORS\" for a list of available colors." << std::endl;
                    tg = -1;
                }if (tg != -1) {
                    threadData->data[0] = 'c';
                    threadData->data[1] = id;
                    threadData->data[2] = tg;
                    threadData->status = T_WAITING;
                    std::cout << "Color " << id + 1 << " switched to " << colorNames[tg] << std::endl;
                }
            }else std::cout << "Invalid color ID. Please select 1-3." << std::endl;
        }else if (streq(command, "SLIDESHOW", 0, 10)) {
            threadData->data[0] = 's';
            threadData->data[1] = true;
            threadData->status = T_WAITING;
            while (threadData->status == T_WAITING) {}
            std::cout << "Banner is now in Slideshow mode." << std::endl;
        }else if (streq(command, "BANNER", 0, 7)) {
            threadData->data[0] = 's';
            threadData->data[1] = false;
            threadData->status = T_WAITING;
            while (threadData->status == T_WAITING) {}
            std::cout << "Banner is now in Banner mode." << std::endl;
        }else{
            std::cout << "Unknown command. Type \"HELP\" for a list of commands." << std::endl;
        }
        std::cout << "> ";
        std::cin >> command;
    }
    threadData->status = T_STOPPED;
    return 0;
}

DWORD DoReceiveRequests(TDATA* threadData, HANDLE queue) {
    ULONG result;
    HTTP_REQUEST_ID id;
    DWORD read;
    PHTTP_REQUEST request;
    PCHAR buffer;
    ULONG bufferLength = sizeof(HTTP_REQUEST) + 2048;
    buffer = (PCHAR)HeapAlloc(GetProcessHeap(), 0, bufferLength);
    if (buffer == NULL) return ERROR_NOT_ENOUGH_MEMORY;
    request = (PHTTP_REQUEST)buffer;
    HTTP_SET_NULL_ID(&id);
    std::cout << "HTTP Server Started" << std::endl;
    threadData->status = T_RUNNING;
    while(1) {
        RtlZeroMemory(request, bufferLength);
        result = HttpReceiveHttpRequest(queue, id, 0, request, bufferLength, &read, NULL);
        if (result == NO_ERROR) {
            HTTP_RESPONSE response;
            DWORD sent;
            HTTP_DATA_CHUNK chunk;
            PUCHAR ebuffer;
            ULONG ebufferLength;
            ULONG bread = 0;
            ULONG tempWritten;
            HANDLE tempFile = INVALID_HANDLE_VALUE;
            TCHAR tempName[MAX_PATH + 1];
            CHAR contentLength[MAX_ULONG_STR];
            ULONG totalRead = 0;
            char filePath[256] = "./http";
            int filePathSize;
            int t;
            std::streamsize size;
            char* fileExtension = NULL;
            char* fileContents;
            for (int i = 0; ; i++) {
                filePath[i + 6] = request->CookedUrl.pAbsPath[i];
                if (filePath[i + 6] == '?') filePath[i + 6] = '\0';
                if (filePath[i + 6] == '\0') {
                    filePathSize = i + 6;
                    break;
                }
            }
            if (streq(filePath, "./HTTP/UPDATE.JSON", 0, 19)) {
                const wchar_t* query = request->CookedUrl.pQueryString;
                char flag = 0;
                char data = 0;
                if (query != NULL && wcslen(query) >= 3) {
                    int i = 1;
                    switch (query[1]) {
                        case 'v':
                            for (int c = 2; query[c - 1] != '\0' && c < D_NAMESIZE; c++) {
                                threadData->data[i] = query[c];
                                if (query[c] == '%' && query[c+1] != '\0' && query[c+2] != '\0') {
                                    char newc = 0;
                                    if (query[c + 1] >= '0' || query[c + 1] <= '9') newc += (query[c + 1] - '0') * 16;
                                    else newc += ((query[c + 1] & 0xdf) - 'A' + 10) * 16;
                                    if (query[c + 2] >= '0' || query[c + 2] <= '9') newc += query[c + 2] - '0';
                                    else newc += (query[c + 2] & 0xdf) - 'A' + 10;
                                    threadData->data[i] = newc;
                                    c += 2;
                                }
                                i++;
                            }
                            threadData->data[0] = D_VENUENAME;
                            break;
                        case 't':
                            if (wcslen(query) < 6 || query[2] < '0' || query[2] > '2' || query[3] < '0' || query[3] > '9' || query[4] < '0' || query[4] > '5' || query[5] < '0' || query[5] > '9') {
                                threadData->data[0] = -1;
                                break;
                            }
                            threadData->data[0] = D_DOWNBEAT;
                            t = query[2] * 600 + query[3] * 60 + query[4] * 10 + query[5] - 48*671;
                            *((int*)(threadData->data + D_DOWNBEAT)) = t;
                            break;
                        case 'c':
                            if (wcslen(query) < 4 || query[2] < '0' || query[2] > '2' || query[3] < '0' || query[3] > '7') {
                                threadData->data[0] = -1;
                                break;
                            }
                            threadData->data[0] = query[2] - '0';
                            threadData->data[1] = query[3] - '0';
                            break;
                        default:
                            if (query[1] < '0' || query[1] > '3' || (query[2] != '0' && query[2] != '1')) {
                                threadData->data[0] = -1;
                                break;
                            }
                            threadData->data[0] = D_FLAGS;
                            threadData->data[1] = 1 << (query[1] - '0');
                            threadData->data[2] = -!!(query[2] - '0');
                            break;
                    }
                }else threadData->data[0] = -1;
                fileContents = (char*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 512);
                threadData->status = T_WAITING;
                while (threadData->status == T_WAITING) {}
                const char strue[5] = "true";
                const char sfalse[6] = "false";
                sprintf_s(fileContents, 512,
                    "{\"red\":%d,\"green\":%d,\"blue\":%d,"
                    "\"slideshow\":%s,\"autostart\":%s,\"baselight\":%s,\"metaposts\":%s,"
                    "\"downbeat\":%d,\"name\":\"%s\"}",
                    threadData->data[D_COLOR1], threadData->data[D_COLOR2], threadData->data[D_COLOR3],
                    readFlags(&threadData->data[D_FLAGS], F_SLIDESHOW_MODE) ? strue: sfalse,
                    readFlags(&threadData->data[D_FLAGS], F_AUTOSTART) ? strue: sfalse,
                    readFlags(&threadData->data[D_FLAGS], F_BASELIGHT) ? strue: sfalse,
                    readFlags(&threadData->data[D_FLAGS], F_METAPOSTS) ? strue: sfalse,
                    *((int*)(threadData->data + D_DOWNBEAT)), threadData->data + D_VENUENAME);
                fileExtension = filePath+14;
                size = strlen(fileContents)+1;
            }else {
                if (filePath[filePathSize - 1] == '/') {
                    sprintf_s(filePath, "%sindex.html", filePath);
                    filePathSize += 10;
                }
                for (char* c = filePath + filePathSize; *c != '.'; c--) fileExtension = c;
                fileContents = readFile(filePath, &size);
            }
            if (fileContents == NULL) {
                INITIALIZE_HTTP_RESPONSE(&response, 404, "Rippy Dippy");
                ADD_KNOWN_HEADER(response, HttpHeaderContentType, "text/html");
                chunk.DataChunkType = HttpDataChunkFromMemory;
                chunk.FromMemory.pBuffer = (PVOID)"<h1>Error 404</h1>File not found.";
                chunk.FromMemory.BufferLength = strlen("<h1>Error 404</h1>File not found.") + 1;
                response.EntityChunkCount = 1;
                response.pEntityChunks = &chunk;
                result = HttpSendHttpResponse(queue, request->RequestId, 0, &response, NULL, &sent, NULL, 0, NULL, NULL);
            }else {
                switch (request->Verb) {
                    case HttpVerbGET:
                        INITIALIZE_HTTP_RESPONSE(&response, 200, "Okey Dokey");
                        if (streq(fileExtension, "html", 0, 4)) ADD_KNOWN_HEADER(response, HttpHeaderContentType, "text/html");
                        else if (streq(fileExtension, "css", 0, 3)) ADD_KNOWN_HEADER(response, HttpHeaderContentType, "text/css");
                        else if (streq(fileExtension, "png", 0, 3)) ADD_KNOWN_HEADER(response, HttpHeaderContentType, "image/png");
                        else if (streq(fileExtension, "ico", 0, 3)) ADD_KNOWN_HEADER(response, HttpHeaderContentType, "image/x-icon");
                        else if (streq(fileExtension, "js", 0, 2)) ADD_KNOWN_HEADER(response, HttpHeaderContentType, "text/javascript");
                        else if (streq(fileExtension, "otf", 0, 3)) ADD_KNOWN_HEADER(response, HttpHeaderContentType, "font/otf");
                        else if (streq(fileExtension, "json", 0, 4)) ADD_KNOWN_HEADER(response, HttpHeaderContentType, "application/json");
                        else ADD_KNOWN_HEADER(response, HttpHeaderContentType, "text/plain");
                        chunk.DataChunkType = HttpDataChunkFromMemory;
                        chunk.FromMemory.pBuffer = fileContents;
                        chunk.FromMemory.BufferLength = size-1;
                        response.EntityChunkCount = 1;
                        response.pEntityChunks = &chunk;
                        result = HttpSendHttpResponse(queue, request->RequestId, 0, &response, NULL, &sent, NULL, 0, NULL, NULL);
                        if (result != NO_ERROR) std::cout << "HTTP Response failed. Womp womp." << std::endl;
                        break;
                    case HttpVerbPOST:
                        ebufferLength = 2048;
                        ebuffer = (PUCHAR)HeapAlloc(GetProcessHeap(), 0, ebufferLength);
                        if (ebuffer == NULL) {
                            result = ERROR_NOT_ENOUGH_MEMORY;
                            goto done;
                        }
                        INITIALIZE_HTTP_RESPONSE(&response, 200, "OK");
                        if (request->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS) {
                            if (GetTempFileName(L".", L"New", 0, tempName) == 0) {
                                result = GetLastError();
                                goto done;
                            }
                            tempFile = CreateFile(tempName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                            if (tempFile == INVALID_HANDLE_VALUE) {
                                result = GetLastError();
                                goto done;
                            }
                            do {
                                bread = 0;
                                result = HttpReceiveRequestEntityBody(queue, request->RequestId, 0, ebuffer, ebufferLength, &bread, NULL);
                                switch (result) {
                                case NO_ERROR:
                                    if (bread != 0) {
                                        totalRead += bread;
                                        WriteFile(tempFile, ebuffer, bread, &tempWritten, NULL);
                                    }
                                    break;
                                case ERROR_HANDLE_EOF:
                                    if (bread != 0) {
                                        totalRead += bread;
                                        WriteFile(tempFile, ebuffer, bread, &tempWritten, NULL);
                                    }
                                    sprintf_s(contentLength, MAX_ULONG_STR, "%lu", totalRead);
                                    ADD_KNOWN_HEADER(response, HttpHeaderContentLength, contentLength);
                                    result = HttpSendHttpResponse(queue, request->RequestId, HTTP_SEND_RESPONSE_FLAG_MORE_DATA, &response, NULL, &sent, NULL, 0, NULL, NULL);
                                    if (result != NO_ERROR) goto done;
                                    chunk.DataChunkType = HttpDataChunkFromFileHandle;
                                    chunk.FromFileHandle.ByteRange.StartingOffset.QuadPart = 0;
                                    chunk.FromFileHandle.ByteRange.Length.QuadPart = HTTP_BYTE_RANGE_TO_EOF;
                                    chunk.FromFileHandle.FileHandle = tempFile;
                                    result = HttpSendResponseEntityBody(queue, request->RequestId, 0, 1, &chunk, NULL, NULL, 0, NULL, NULL);
                                    goto done;
                                    break;
                                default:
                                    std::cout << "HttpReceiveRequestEntityBody failed with " << result << std::endl;
                                    goto done;
                                }
                            } while (true);
                        }
                        else {
                            result = HttpSendHttpResponse(queue, request->RequestId, 0, &response, NULL, &sent, NULL, 0, NULL, NULL);
                            if (result != NO_ERROR) std::cout << "HTTP Response failed. Womp womp." << std::endl;
                        }
                    done:               if (ebuffer) HeapFree(GetProcessHeap(), 0, ebuffer);
                        if (tempFile == INVALID_HANDLE_VALUE) {
                            CloseHandle(tempFile);
                            DeleteFile(tempName);
                        }
                        break;
                    default:
                        std::cout << "Unknown Request" << std::endl;
                }
            }
            HeapFree(GetProcessHeap(), 0, fileContents);
            HTTP_SET_NULL_ID(&id);
        }else if (result == ERROR_MORE_DATA) {
            id = request->RequestId;
            bufferLength = read;
            HeapFree(GetProcessHeap(), 0, buffer);
            buffer = (PCHAR)HeapAlloc(GetProcessHeap(), 0, bufferLength);
            if (buffer == NULL) {
                return ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            request = (PHTTP_REQUEST)buffer;
        }else if (result == ERROR_CONNECTION_INVALID && !HTTP_IS_NULL_ID(&request)) HTTP_SET_NULL_ID(&id);
        else break;
    }
    if (request) HeapFree(GetProcessHeap(), 0, request);
    return result;
}

DWORD sixteen = 16;

DWORD WINAPI HTTPmain(LPVOID lpParam) {
    TDATA* threadData = (TDATA*) lpParam;
    ULONG retCode;
    HANDLE queue = NULL;
    int url = 0;
    HTTPAPI_VERSION httpversion = HTTPAPI_VERSION_1;
    wchar_t* sdat;

    retCode = HttpInitialize(httpversion, HTTP_INITIALIZE_SERVER, NULL);
    if (retCode != NO_ERROR) goto CleanUp;
    retCode = HttpCreateHttpHandle(&queue, 0);
    if (retCode != NO_ERROR) goto CleanUp;
    wchar_t hostname[16];
    GetComputerName(hostname, &sixteen);
    std::wcout << "Host name: " << hostname << std::endl;
    sdat = (wchar_t*)threadData->data;
    for (int i = 0; i < sixteen; i++) sdat[i] = hostname[i];
    //threadData->status = T_RUNNING;
    wchar_t url0[30];
    wsprintf(url0, L"http://%s:80/", hostname);
    std::wcout << "Listening on URL " << url0 << std::endl;
    retCode = HttpAddUrl(queue, url0, NULL);
    std::cout << "Status: " << retCode << std::endl;
    if (retCode != NO_ERROR) goto CleanUp;
    std::cout << "Hosting on: " << url0 << std::endl;
    for (int i = 0; i < URL_COUNT; i++) {
        std::cout << "Listening on URL " << urls[i] << std::endl;
        retCode = HttpAddUrl(queue, urls[i], NULL);
        std::cout << "Status: " << retCode << std::endl;
        if (retCode != NO_ERROR) goto CleanUp;
    }
    DoReceiveRequests(threadData, queue);
CleanUp: if (queue) CloseHandle(queue);
    if (retCode == ERROR_ACCESS_DENIED) {
        std::cout << "Access Denied. Please run as administrator" << std::endl;
        threadData->status = T_STOPPED;
        return retCode;
    }
    HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
    std::cout << "HTTP Server exited with code \"" << retCode << "\"" << std::endl;
    threadData->status = T_STOPPED;
    return retCode;
}

int main(int argc, char** argv) {
    TDATA* glData = (TDATA*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TDATA));
    if (glData == NULL) return -2;
    glData->data[D_COLOR1] = 1;
    glData->data[D_COLOR2] = 2;
    glData->data[D_COLOR3] = 3;
    glData->data[D_FLAGS] = F_AUTOSTART | F_BASELIGHT | F_METAPOSTS;
    int* showtime_pointer = (int*) (glData->data + 4);
    *showtime_pointer = 1200;
    sprintf_s((char*) (glData->data+D_VENUENAME), D_NAMESIZE, "Your Venue Name Here");
    glData->data[255] = 0;
    DWORD glID;
    HANDLE glThread = CreateThread(
        NULL,
        0,
        GLmain,
        glData,
        0,
        &glID
    );

    //Wait for GL to finish initializing before running CLI
    while (glData->status != T_RUNNING) {}

    TDATA* httpData = (TDATA*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TDATA));
    if (httpData == NULL) return -2;
    DWORD httpID;
    HANDLE httpThread = CreateThread(
        NULL,
        0,
        HTTPmain,
        httpData,
        0,
        &httpID
    );

    while (httpData->status != T_RUNNING) {}
    LPWSTR hostname = (LPWSTR)httpData->data;
    
    TDATA* cliData = (TDATA*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TDATA));
    if (cliData == NULL) return -2;
    DWORD cliID;
    LPWSTR destination = (LPWSTR)cliData->data;
    for (int i = 0; i < sixteen; i++) destination[i] = hostname[i];
    
    HANDLE cliThread = CreateThread(
        NULL,
        0,
        CLImain,
        cliData,
        0,
        &cliID
    );

    while (cliData->status != T_STOPPED && glData->status != T_STOPPED && httpData->status != T_STOPPED) {
        if (httpData->status == T_WAITING) {
            switch (httpData->data[0]) {
                case -1:
                    break;
                case D_FLAGS:
                    writeFlags(&glData->data[D_FLAGS], httpData->data[1], httpData->data[2]);
                    break;
                case D_DOWNBEAT:
                    *((int*)(glData->data + D_DOWNBEAT)) = *((int*)(httpData->data + 4));
                    break;
                case D_VENUENAME:
                    sprintf_s(glData->data + D_VENUENAME, D_NAMESIZE, "%s", httpData->data + 1);
                    break;
                default:
                    glData->data[httpData->data[0]] = httpData->data[1];
                    break;
            }
            for (int i = 0; i < 255; i++) httpData->data[i] = glData->data[i];
            httpData->status = T_RUNNING;
        }
        if (cliData->status == T_WAITING) {
            switch (cliData->data[0]) {
                case 'c':
                    glData->data[cliData->data[1]] = cliData->data[2];
                    break;
                case 'r':
                    for(int i = 0; i < 3; i++) cliData->data[i] = glData->data[i];
                    break;
                case 's':
                    writeFlags(&glData->data[D_FLAGS], F_SLIDESHOW_MODE, -!!cliData->data[1]);
                    break;
                case 't':
                    *((int*)(glData->data + D_DOWNBEAT)) = *((int*)(cliData->data + 4));
                    break;
                case 'v':
                    sprintf_s(glData->data + D_VENUENAME, D_NAMESIZE, cliData->data + 1);
                    break;
                case 'a':
                    writeFlags(&glData->data[D_FLAGS], F_AUTOSTART, -!readFlags(&glData->data[D_FLAGS], F_AUTOSTART));
                    cliData->data[1] = !!readFlags(&glData->data[D_FLAGS], F_AUTOSTART);
                    break;

            }
            cliData->status = T_RUNNING;
        }
    }
    ExitProcess(0);
}