#version 330 core

layout (location = 0) in vec3 aPos;  
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 ourNormal;
out vec3 FragPos;
out vec2 TexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
    FragPos = vec3(model * vec4(aPos, 1.0));
    ourNormal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
}
