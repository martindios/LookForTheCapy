#version 330 core
// shader.vert
layout (location = 0) in vec3 aPos;  
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;

out vec3 ourNormal;
out vec3 FragPos;
out vec3 ourColor;
out vec2 TexCoord;

uniform vec3 color; 
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 transform;

void main() {
    gl_Position = projection * view * transform * vec4(aPos, 1.0f);
    FragPos = vec3(model * vec4(aPos, 1.0));
    ourNormal = mat3(transpose(inverse(model))) * aNormal;
    ourColor = color;
    TexCoord = aTexCoord;
}
