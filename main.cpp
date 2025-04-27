// g++ main.cpp lecturaShader.cpp glad.c -o main -lglfw -lm -lGL -lGLU

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "estructuras.h"

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

std::vector<GLuint> capyTextures;

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

// globals para el modelo
tinygltf::Model model;
std::vector<GLuint> capyVBOs, capyVAOs;

void loadCapybara(const char* path) {
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);            // carga .glb :contentReference[oaicite:1]{index=1}
    if (!warn.empty())  printf("Warn: %s\n", warn.c_str());
    if (!err.empty())   printf("Err:  %s\n", err.c_str());
    if (!ret) {
        printf("Failed to load glb: %s\n", path);
        exit(-1);
    }

    // 1) Generar VBOs a partir de bufferViews
    capyVBOs.resize(model.bufferViews.size());
    glGenBuffers((GLsizei)capyVBOs.size(), capyVBOs.data());
    for (size_t i = 0; i < model.bufferViews.size(); i++) {
        auto &bv = model.bufferViews[i];
        auto &buf = model.buffers[bv.buffer];
        glBindBuffer(bv.target, capyVBOs[i]);
        glBufferData(bv.target, (GLsizeiptr)bv.byteLength,
                     buf.data.data() + bv.byteOffset,
                     GL_STATIC_DRAW);
    }

    // 2) Generar VAO por cada mesh
    capyVAOs.resize(model.meshes.size());
    glGenVertexArrays((GLsizei)capyVAOs.size(), capyVAOs.data());
    for (size_t m = 0; m < model.meshes.size(); m++) {
        glBindVertexArray(capyVAOs[m]);
        auto &prim = model.meshes[m].primitives[0];

        // Índices
        auto &iAcc = model.accessors[prim.indices];
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                     capyVBOs[iAcc.bufferView]);

        // Atributos POSITION, NORMAL, TEXCOORD_0
        for (auto &attr : prim.attributes) {
            auto &aAcc = model.accessors[attr.second];
            glBindBuffer(GL_ARRAY_BUFFER,
                         capyVBOs[aAcc.bufferView]);
            int loc =
                (attr.first == "POSITION"         ? 0 :
                 attr.first == "NORMAL"           ? 1 :
                 attr.first == "TEXCOORD_0"       ? 2 : -1);                          // UVs en location=2 
            if (loc < 0) continue;
            glVertexAttribPointer(loc,
                                  aAcc.type,
                                  aAcc.componentType,
                                  aAcc.normalized ? GL_TRUE : GL_FALSE,
                                  model.bufferViews[aAcc.bufferView].byteStride,
                                  (void*)(intptr_t)aAcc.byteOffset);
            glEnableVertexAttribArray(loc);
        }
    }
    glBindVertexArray(0);

    // 3) Generar Texturas GL a partir de model.images
    capyTextures.resize(model.textures.size());
    glGenTextures((GLsizei)capyTextures.size(), capyTextures.data());
    for (size_t i = 0; i < model.textures.size(); ++i) {
        auto &tex  = model.textures[i];
        if (tex.source < 0) continue;
        auto &img  = model.images[tex.source];
        auto &samp = (tex.sampler >= 0 ? model.samplers[tex.sampler]
                                      : model.samplers[0]);

        glBindTexture(GL_TEXTURE_2D, capyTextures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, samp.wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, samp.wrapT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, samp.minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, samp.magFilter);

        GLenum format = (img.component == 4 ? GL_RGBA : GL_RGB);                   // detecta RGB/RGBA 
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format,
                     img.width, img.height, 0,
                     format, GL_UNSIGNED_BYTE,
                     img.image.data());                                             // sube datos con stb_image 
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}



void drawCapybara(GLuint program) {
    static float ang = 0.0f;
    ang += 0.5f;

    // Movimiento oscilante en X
    glm::mat4 modelMat = glm::translate(glm::mat4(1.0f),
        glm::vec3(sin(ang * 3.14159f/180.0f) * 5.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(program, "model"),
                       1, GL_FALSE, glm::value_ptr(modelMat));

    for (size_t m = 0; m < capyVAOs.size(); ++m) {
        glBindVertexArray(capyVAOs[m]);
        auto &prim = model.meshes[m].primitives[0];

        // Si el primitive tiene material, enlazar su baseColorTexture
        if (prim.material >= 0) {
            auto &mat = model.materials[prim.material];
            int bi = mat.pbrMetallicRoughness.baseColorTexture.index;            // glTF PBR baseColorTexture 
            if (bi >= 0 && bi < (int)capyTextures.size()) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, capyTextures[bi]);
                glUniform1i(glGetUniformLocation(program, "texture1"), 0); // coincide con sampler2D 
            }
        }

        auto &iAcc = model.accessors[prim.indices];
        glDrawElements(GL_TRIANGLES,
                       (GLsizei)iAcc.count,
                       iAcc.componentType,
                       0);                                                           // dibuja con textura activa 
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
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

int main(int argc, char** argv) {
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

    loadCapybara(argv[1]);
    
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
        drawCapybara(shaderProgram);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glDeleteProgram(shaderProgram); // Limpieza del shader    
    return 0;
}

