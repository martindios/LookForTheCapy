// main.cpp

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "lecturaShader.cpp"
#include "tiny_gltf.h"           // tiny_gltf.h en tu carpeta

#include <glm/glm.hpp>                          // tipos básicos: vec3, mat4, …
#include <glm/gtc/matrix_transform.hpp>         // glm::translate, glm::rotate, …
#include <glm/gtc/type_ptr.hpp>                 // glm::value_ptr


// globals para el modelo
tinygltf::Model model;
std::vector<GLuint> capyVBOs, capyVAOs;

// --- función para cargar .glb (basada en la propuesta anterior)
void loadCapybara(const char* path) {
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    if (!warn.empty())  printf("Warn: %s\n", warn.c_str());
    if (!err.empty())   printf("Err:  %s\n", err.c_str());
    if (!ret) {
        printf("Failed to load glb: %s\n", path);
        exit(-1);
    }
    // Genera VBOs
    capyVBOs.resize(model.bufferViews.size());
    capyVAOs.resize(model.meshes.size());
    glGenBuffers((GLsizei)capyVBOs.size(), capyVBOs.data());
    for (size_t i = 0; i < model.bufferViews.size(); i++) {
        auto &bv = model.bufferViews[i];
        auto &buf = model.buffers[bv.buffer];
        glBindBuffer(bv.target, capyVBOs[i]);
        glBufferData(bv.target, (GLsizeiptr)bv.byteLength,
                     buf.data.data() + bv.byteOffset,
                     GL_STATIC_DRAW);
    }
    // Genera VAO por malla
    for (size_t m = 0; m < model.meshes.size(); m++) {
        glGenVertexArrays(1, &capyVAOs[m]);
        glBindVertexArray(capyVAOs[m]);
        auto &prim = model.meshes[m].primitives[0];
        // Índices
        auto &iAcc = model.accessors[prim.indices];
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                     capyVBOs[iAcc.bufferView]);
        // Atributos
        for (auto &attr : prim.attributes) {
            auto &aAcc = model.accessors[attr.second];
            glBindBuffer(GL_ARRAY_BUFFER,
                         capyVBOs[aAcc.bufferView]);
            int loc = (attr.first == "POSITION" ? 0 :
                       attr.first == "NORMAL"   ? 1 : 2);
            glVertexAttribPointer(loc,
                                  aAcc.type,
                                  aAcc.componentType,
                                  aAcc.normalized ? GL_TRUE : GL_FALSE,
                                  model.bufferViews[aAcc.bufferView].byteStride,
                                  (void*)(intptr_t)aAcc.byteOffset);
            glEnableVertexAttribArray(loc);
        }
    }
}

// --- dibuja la capibara
void drawCapybara(GLuint program) {
    static float ang = 0.0f;
    ang += 0.5f;
    glm::mat4 modelMat = glm::translate(glm::mat4(1.0f),
                                        glm::vec3(sin(ang*3.14159f/180.0f)*5.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(program,"model"),
                       1, GL_FALSE, glm::value_ptr(modelMat));

    for (size_t m = 0; m < capyVAOs.size(); ++m) {
        glBindVertexArray(capyVAOs[m]);
        auto &prim = model.meshes[m].primitives[0];
        auto &iAcc = model.accessors[prim.indices];
        glDrawElements(GL_TRIANGLES,
                       (GLsizei)iAcc.count,
                       iAcc.componentType,
                       0);
    }
    glBindVertexArray(0);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s model.glb\n", argv[0]);
        return -1;
    }

    // 1) Init GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(800,800,"Capybara",NULL,NULL);
    if(!win){ glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);

    // 2) Load GL functions via GLAD
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    // 3) Compila y usa tu shader
    GLuint shader = setShaders("shader.vert","shader.frag");
    glUseProgram(shader);

    // 4) Carga el modelo .glb
    loadCapybara(argv[1]);

    // 5) Render loop
    while(!glfwWindowShouldClose(win)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawCapybara(shader);
        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}

