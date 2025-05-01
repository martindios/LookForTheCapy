#include <iostream>
#include <fstream>
#include <sstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "./includes/stb_image.h"

GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, NULL, info);
        std::cerr << "Error compiling shader: " << info << std::endl;
    }

    return shader;
}

std::string ReadFile(const char* path) {
    std::ifstream file(path);
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

GLuint CreateShaderProgram(const char* vertexPath, const char* fragmentPath) {
    std::string vCode = ReadFile(vertexPath);
    std::string fCode = ReadFile(fragmentPath);
    GLuint vShader = CompileShader(GL_VERTEX_SHADER, vCode.c_str());
    GLuint fShader = CompileShader(GL_FRAGMENT_SHADER, fCode.c_str());

    GLuint program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(program, 512, NULL, info);
        std::cerr << "Error linking program: " << info << std::endl;
    }

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    return program;
}

int cargaTextura(const char* nombre) {
    GLuint textura;

    glGenTextures(1, &textura);
    glBindTexture(GL_TEXTURE_2D, textura);

    // Configuración de parámetros de la textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Carga la imagen
    int width, height, nrChannels;
    unsigned char *data = stbi_load(nombre, &width, &height, &nrChannels, 0);
    if (data) {
        // Determina el formato de la imagen (por ejemplo, GL_RGB o GL_RGBA)
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Error al cargar la textura" << std::endl;
    }
    stbi_image_free(data);

    return textura;
}
