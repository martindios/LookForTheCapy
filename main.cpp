// g++ main.cpp aux.cpp glad.c -o main -ldl -lglfw -lm -lGL -lGLU

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <vector>
#include <random>

#define TINYGLTF_IMPLEMENTATION
#include "./includes/tiny_gltf.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./includes/stb_image_write.h"

// Constantes
const int terrainSize = 500; // El terreno será (0, y, 0) a (terrainSize, y, terrainSize)

// Variables globales
int scrWidth = 800;
int scrHeight = 800;

// Variables del terreno
std::vector<float> terrainVertex;
std::vector<unsigned int> terrainIndex;
GLuint terrainVAO;
GLuint terrainShader;
GLuint terrainWaterTexture, terrainRockTexture, terrainHighTexture;

// Variables de la cámara
glm::vec3 cameraPos   = glm::vec3(50.0f, 10.0f, 50.0f); // Posición inicial de la cámara
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);    // Vector "arriba" del mundo
float cameraSpeed = 0.65f;                              // Velocidad de la cámara
float cameraAngle = 0.0f;

// Variables de la capybara
tinygltf::Model model;
std::vector<GLuint> capyVBOs, capyVAOs;
std::vector<GLuint> capyTextures;
GLuint capybaraShader;
glm::vec3 capySphereCenter;
float capySphereRadius;
glm::vec3 capyPositionWorld(75.0f, 10.0f, 50.0f);

// Declaración de núms aleatorios
std::mt19937        rng{ std::random_device{}() };
std::uniform_real_distribution<float> distXZ(0.0f, 100.0f);

// Declaración de funciones
GLuint CreateShaderProgram(const char* vertexPath, const char* fragmentPath);
int cargaTextura(const char* nombre);
bool checkCapybaraCollision(const glm::vec3 &cameraPos, 
                            const glm::vec3 &sphereCenter, 
                            float sphereRadius,
                            const glm::mat4 &modelTransform);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

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


    // Cálculo de la bounding sphere para el capybara
    std::vector<glm::vec3> positions;
    for (auto &mesh : model.meshes) {
        auto &prim   = mesh.primitives[0];
        auto &posAcc = model.accessors[ prim.attributes.at("POSITION") ];
        auto &bv     = model.bufferViews[ posAcc.bufferView ];
        auto &buf    = model.buffers[bv.buffer];
        const float* data = reinterpret_cast<const float*>(
                               buf.data.data() + bv.byteOffset + posAcc.byteOffset);
        for (size_t i = 0; i < posAcc.count; ++i) {
            positions.emplace_back(data[3*i], data[3*i+1], data[3*i+2]);
        }
    }
    glm::vec3 center{0.0f};
    for (auto &p : positions) center += p;
    center /= float(positions.size());
    float radius = 0.0f;
    for (auto &p : positions) {
        radius = std::max(radius, glm::length(p - center));
    }
    capySphereCenter = center;
    capySphereRadius = radius;
    std::cout << "Capybara bounding sphere: center=" 
              << glm::to_string(center) 
              << ", radius=" << radius << std::endl;

}

void drawCapybara() {
    glUseProgram(capybaraShader);

    static float ang = 0.0f;
    ang += 0.5f;

    // Movimiento oscilante en X
    glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), capyPositionWorld); // Trasladar
    modelMat = glm::scale(modelMat, glm::vec3(1.0f, 1.0f, 1.0f)); // Escala 10x
    glUniformMatrix4fv(glGetUniformLocation(capybaraShader, "model"),
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
                glUniform1i(glGetUniformLocation(capybaraShader, "texture1"), 0); // coincide con sampler2D 
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

bool checkCapybaraCollision(const glm::vec3 &cameraPos, 
                            const glm::vec3 &sphereCenter, 
                            float sphereRadius,
                            const glm::mat4 &modelTransform)
{
    // Transformar el centro de la esfera de espacio‑modelo a espacio‑mundo
    glm::vec3 worldCenter = glm::vec3(modelTransform * glm::vec4(sphereCenter, 1.0f));
    float    worldRadius = sphereRadius;  // si tu escala es uniforme 1.0

    // Distancia cámara ↔ centro de la capybara
    float dist = glm::length(cameraPos - worldCenter);

    return (dist <= worldRadius);
}

void respawnCapybara(glm::vec3 &position) {
    position.x = distXZ(rng);
    position.z = distXZ(rng);
}

// Función para crear la malla de terreno
void createTerrainMesh() {
    terrainVertex.clear();
    terrainIndex.clear();

    terrainVertex.reserve(terrainSize * terrainSize * 3); // 3 componentes (x,y,z) por vértice
    for(int z = 0; z < terrainSize; ++z) {
        for(int x = 0; x < terrainSize; ++x) {
            // Posición (x, y, z). Inicialmente y = 0.
            terrainVertex.push_back((float)x);
            terrainVertex.push_back(0.0f);
            terrainVertex.push_back((float)z);
        }
    }

    // Ahora creamos índices para triángulos (dos por cada cuadrado)
    for(int z = 0; z < terrainSize - 1; ++z) {
        for(int x = 0; x < terrainSize - 1; ++x) {
            int topLeft     =  z      * terrainSize + x;
            int topRight    =  topLeft + 1;
            int bottomLeft  = (z + 1)* terrainSize + x;
            int bottomRight = bottomLeft + 1;
            // Triángulo superior izquierdo
            terrainIndex.push_back(topLeft);
            terrainIndex.push_back(bottomLeft);
            terrainIndex.push_back(topRight);
            // Triángulo inferior derecho
            terrainIndex.push_back(topRight);
            terrainIndex.push_back(bottomLeft);
            terrainIndex.push_back(bottomRight);
        }
    }

    // Crear buffers VAO, VBO y EBO
    GLuint vbo, ebo;
    glGenVertexArrays(1, &terrainVAO);
    glBindVertexArray(terrainVAO);

    // Vertex Buffer
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                terrainVertex.size() * sizeof(float),
                terrainVertex.data(),
                GL_STATIC_DRAW);

    // Element Buffer
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                terrainIndex.size() * sizeof(unsigned int),
                terrainIndex.data(),
                GL_STATIC_DRAW);

    // Atributo de posición (layout = 0 en shader)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Desenlazamos por ahora (opcional)
    glBindVertexArray(0);

    // Cargamos la textura
    terrainWaterTexture = cargaTextura("./textures/agua.png");
    terrainRockTexture = cargaTextura("./textures/roca.png");
    terrainHighTexture = cargaTextura("./textures/rocaAlta.png");

}

// Función para pintar el terreno
void pintarTerreno() {
    glUseProgram(terrainShader);

    // Enviamos las texturas al shader
    glUniform1i(glGetUniformLocation(terrainShader, "textureWater"), 0); // Unidad de textura 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, terrainWaterTexture);
    glUniform1i(glGetUniformLocation(terrainShader, "textureRock"), 1); // Unidad de textura 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, terrainRockTexture);
    glUniform1i(glGetUniformLocation(terrainShader, "textureHigh"), 2); // Unidad de textura 2
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, terrainHighTexture);

    // Bindeamos el VAO y dibujamos
    glBindVertexArray(terrainVAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)terrainIndex.size(), GL_UNSIGNED_INT, 0);

    // Desenlazamos
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Configura la cámara y la proyección
void camara() {
    glUseProgram(terrainShader);

    glm::mat4 projection = glm::perspective(glm::radians(65.0f),
                                float(scrWidth) / float(scrHeight), 0.1f, 200.0f);
    unsigned int projectionLoc = glGetUniformLocation(terrainShader, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glm::vec3 eye = cameraPos;
    glm::vec3 center = cameraPos + glm::vec3(cos(glm::radians(cameraAngle)), 0.0f, -sin(glm::radians(cameraAngle)));
    glm::mat4 view = glm::lookAt(eye, center, cameraUp);
    
    unsigned int viewLoc = glGetUniformLocation(terrainShader, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Enviar las matrices al shader de la capybara
    glUseProgram(capybaraShader);

    unsigned int projectionLocCapy = glGetUniformLocation(capybaraShader, "projection");
    glUniformMatrix4fv(projectionLocCapy, 1, GL_FALSE, glm::value_ptr(projection));

    unsigned int viewLocCapy = glGetUniformLocation(capybaraShader, "view");
    glUniformMatrix4fv(viewLocCapy, 1, GL_FALSE, glm::value_ptr(view));
}

void iluminacion() {
    glUseProgram(terrainShader);

    // Luz ambiente
    unsigned int lightLoc = glGetUniformLocation(terrainShader, "lightColor");
    glUniform3f(lightLoc, 0.5f, 0.5f, 0.5f);

    // Posición de la luz
    unsigned int lightPosLoc = glGetUniformLocation(terrainShader, "lightPos");
    glUniform3f(lightPosLoc, cameraPos.x, 0.0, cameraPos.z);

    unsigned int lightDirLoc = glGetUniformLocation(terrainShader, "luzDir");
    glUniform3f(lightDirLoc, cos(glm::radians(cameraAngle)), 0, -sin(glm::radians(cameraAngle)));

    // Luz especular
    unsigned int viewPosLoc = glGetUniformLocation(terrainShader, "viewPos");
    glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
}

// Procesa las entradas del teclado
void processInput(GLFWwindow *window) {
    // Cierra la ventana al presionar ESC
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Movimiento de la cámara
    glm::vec3 camDirection = glm::vec3(cos(glm::radians(cameraAngle)), 0.0f, -sin(glm::radians(cameraAngle)));
    glm::vec3 camRight = glm::normalize(glm::cross(camDirection, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) cameraPos += cameraSpeed * camDirection; // Mover hacia adelante
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) cameraPos -= cameraSpeed * camDirection; // Mover hacia atrás
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) cameraAngle += 0.65f; // Girar a la izquierda
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) cameraAngle -= 0.65f; // Girar a la derecha
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) cameraPos += cameraSpeed * cameraUp; // Subir
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraUp; // Bajar

    // Comprobar límites de la cámara
    if (cameraPos.x < 10.0f) cameraPos.x = 10.0f;
    if (cameraPos.x > terrainSize - 10.0f) cameraPos.x = terrainSize - 10.0f;
    if (cameraPos.z < 10.0f) cameraPos.z = 10.0f;
    if (cameraPos.z > terrainSize - 10.0f) cameraPos.z = terrainSize - 10.0f;
}

// Callback de redimensionado
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Ajusta el viewport a las nuevas dimensiones de la ventana
    glViewport(0, 0, width, height);

    // Actualiza las dimensiones de la ventana
    if(width == 0) width = 1;
    if(height == 0) height = 1;

    scrWidth = width;
    scrHeight = height;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    // Inicialización de GLFW y configuración de la versión de OpenGL
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Creación de la ventana
    GLFWwindow* window = glfwCreateWindow(scrWidth, scrHeight, "Capybara", NULL, NULL);
    if(window == NULL){
        std::cout << "Error al crear la ventana" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    // Inicialización de GLAD
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "Error al inicializar GLAD" << std::endl;
        return -1;
    }

    // Configura el callback para el redimensionamiento
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    // Creación de la malla de terreno
    createTerrainMesh();

    // Carga del modelo de la capybara
    loadCapybara("./models/capybara_low_poly.glb");

    // Configuración de los shaders
    terrainShader = CreateShaderProgram("./shaders/terrain.vert", "./shaders/terrain.frag");
    capybaraShader = CreateShaderProgram("./shaders/capybara.vert", "./shaders/capybara.frag");
    if (capybaraShader == 0 || terrainShader == 0) {
        std::cout << "Error al cargar los shaders" << std::endl;
        return -1;
    }

    // Configuración de OpenGL
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5f, 0.7f, 0.9f, 1.0f);
    glEnable(GL_BLEND);

    // Bucle principal de renderizado
    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 capyModelMat = 
            glm::translate(glm::mat4(1.0f), capyPositionWorld)
          * glm::scale    (glm::mat4(1.0f), glm::vec3(1.0f));

        if ( checkCapybaraCollision(cameraPos, capySphereCenter, capySphereRadius, capyModelMat) ) {
            std::cout << "¡Colisión con la capybara!\n";
            respawnCapybara(capyPositionWorld);
            // …aquí tu respuesta a la colisión…
        }

        // Prepara la matriz de modelo que usas también en drawCapybara()
        capyModelMat = 
            glm::translate(glm::mat4(1.0f), capyPositionWorld)
          * glm::scale    (glm::mat4(1.0f), glm::vec3(1.0f));


        glUniformMatrix4fv(glGetUniformLocation(capybaraShader,"model"),1,GL_FALSE,glm::value_ptr(capyModelMat));

        // Configuración de la cámara
        camara();

        // Configuración de las luces
        iluminacion();

        // Pintar el terreno
        pintarTerreno();  
        
        // Pintar la capybara
        drawCapybara();
        
        // Intercambiar buffers y procesar eventos
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glDeleteProgram(terrainShader);
    glDeleteProgram(capybaraShader);

    return 0;
}
