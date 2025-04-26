// g++ main.cpp lecturaShader.cpp -o main -lglad -lglfw -lm -lGL -lGLU

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "estructuras.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Conversión de grados a radianes
#define gradosToRadianes 3.14159/180.0

// Estructura para definir un objeto con posición, transformación y renderizado
typedef struct {
    float posicion[3];      // Posición inicial
    float anguloTrans;      // Ángulo de traslación
    float velocidad;        // Velocidad de movimiento
    float escalado[3];      // Escalado en cada eje
    unsigned int vao;       // Identificador del VAO para renderizar
    float color[3];         // Color del objeto
    unsigned int vertices;  // Número de vértices
    float anguloRot;        // Ángulo de rotación
    int textura;            // Textura del objeto
} objeto;

// Objetos del escenario
objeto suelo        = {{0.0, 0.0, 0.0}, 0.0, 0.0, {100.0, 0.0, 100.0}, 0, {0.541176f, 0.537254f, 0.392156f}, 6, 0, 0};
objeto cubo         = {{0.0, 1.0, 0.0}, 0.0, 0.0, {1.0, 1.0, 1.0}, 0, {1.0f, 0.5f, 1.0f}, 36, 0, 0};
objeto esfera       = {{0.0, 10.0, 0.0}, 0.0, 0.0, {1.0, 1.0, 1.0}, 0, {0.5f, 1.0f, 1.0f}, 1080, 0, 0};

// Prototipos de funciones
extern GLuint setShaders(const char *nVertx, const char *nFrag);
void processInput(GLFWwindow *window);

// Configuración de la ventana
unsigned int scrWidth = 800;
unsigned int scrHeight = 800;

// Variables globales de shader y animación
int noche = 0;
GLuint shaderProgram;
GLfloat anguloRuedas = 0.0f;
GLfloat anguloBrazo = 0.0f;
extern GLfloat verticesEsfera[];
extern GLfloat verticesPlano[];
extern GLfloat verticesCubo[];

// Variables de la cámara
glm::vec3 cameraPos   = glm::vec3(-35.0f, 15.0f, 0.0f); // Posición inicial de la cámara
glm::vec3 cameraTarget= glm::vec3(0.0f, 0.0f, 0.0f);    // Punto al que mira la cámara
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);    // Vector "arriba" del mundo
float cameraSpeed = 0.1f;                              // Velocidad de la cámara
int modoCamara = 0;
unsigned int vKeyPressed = 0;

// Buffers globales (Vertex Array Objects)
unsigned int vaoEsfera;
unsigned int vaoPlano;
unsigned int vaoCubo;

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

void configurar(unsigned int *vao, float *vertices, int size) {
    unsigned int vbo;
    glGenVertexArrays(1, vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(*vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

    // Atributo para las normales
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    
    // Atributo para las coordenadas de textura
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // Atributo para los vértices
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
}

void dibujarObjeto(objeto &obj) {
    glm::mat4 transform = glm::mat4(1.0f);    

    transform = glm::translate(transform, glm::vec3(obj.posicion[0], obj.posicion[1], obj.posicion[2])); // Trasladar
    transform = glm::rotate(transform, (float)(obj.anguloTrans * gradosToRadianes), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotar
    transform = glm::scale(transform, glm::vec3(obj.escalado[0], obj.escalado[1], obj.escalado[2])); // Escalar

    // Enviar la matriz 'transform' al shader
    unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

    // Enviar la matriz 'model' al shader
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transform));

    // Enviar el color al shader
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3fv(colorLoc, 1, glm::value_ptr(glm::vec3(obj.color[0], obj.color[1], obj.color[2])));

    if (obj.textura != 0) {
        glActiveTexture(GL_TEXTURE0);  // Unidad 0
        glBindTexture(GL_TEXTURE_2D, obj.textura);
        glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
    }

    glBindVertexArray(obj.vao);
    glDrawArrays(GL_TRIANGLES, 0, obj.vertices);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0); // Desvincular la textura
}

// Configura la cámara y la proyección
void camara() {
    glm::mat4 projection = glm::perspective(glm::radians(65.0f),
                                float(scrWidth) / float(scrHeight), 0.1f, 200.0f);
    unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glm::mat4 view;
    view = glm::lookAt(cameraPos, cameraTarget, cameraUp);

    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

// Procesa las entradas del teclado
void processInput(GLFWwindow *window) {

    // Cierra la ventana al presionar ESC
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    glm::vec3 camDirection = glm::normalize(cameraTarget - cameraPos);
    glm::vec3 camRight = glm::normalize(glm::cross(camDirection, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        cameraPos += cameraSpeed * camDirection;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        cameraPos -= cameraSpeed * camDirection;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        cameraPos -= camRight * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        cameraPos += camRight * cameraSpeed;
    
    // Movimiento vertical de la cámara
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraUp;
}

// Configuración inicial de OpenGL
void openGlInit() {
    glClearDepth(1.0f);
    glClearColor(0.4f, 0.6f, 0.6f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Inicializa los objetos y configura sus VAO correspondientes
void initObjects() {
    configurar(&vaoPlano, verticesPlano, sizeof(verticesPlano));
    configurar(&vaoCubo, verticesCubo, sizeof(verticesCubo));
    configurar(&vaoEsfera, verticesEsfera, sizeof(verticesEsfera));

    suelo.vao = vaoPlano;
    cubo.vao = vaoCubo;
    esfera.vao = vaoEsfera;
}

void iluminacion() {    
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    unsigned int lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Ajusta el viewport a las nuevas dimensiones de la ventana
    glViewport(0, 0, width, height);

    // Actualiza las dimensiones de la ventana
    if(width == 0) width = 1;
    if(height == 0) height = 1;

    scrWidth = width;
    scrHeight = height;
}

int main() {
    // Inicialización de GLFW y configuración de la versión de OpenGL
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Creación de la ventana
    GLFWwindow* window = glfwCreateWindow(scrWidth, scrHeight, "Grua", NULL, NULL);
    if(window == NULL){
        std::cout << "Error al crear la ventana" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "Error al inicializar GLAD" << std::endl;
        return -1;
    }

    // Configura el callback para el redimensionamiento
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    shaderProgram = setShaders("shader.vert", "shader.frag");
    glUseProgram(shaderProgram);
    if (shaderProgram == 0) {
        std::cout << "Error al cargar los shaders" << std::endl;
        return -1;
    }
    
    openGlInit();
    initObjects();
    
    // Bucle principal de renderizado
    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        camara();
        iluminacion();
        
        // Dibujar los distintos elementos
        dibujarObjeto(suelo);
        dibujarObjeto(cubo);
        dibujarObjeto(esfera);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glDeleteProgram(shaderProgram); // Limpieza del shader    
    return 0;
}

