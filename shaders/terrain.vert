#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec2 TexCoords;
out float n;

// -------- Funciones de ruido (igual que en fragment shader) --------
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898,78.233))) * 43758.5453);
}

float noise(vec2 pos) {
    vec2 i = floor(pos);
    vec2 f = fract(pos);
    f = f*f*(3.0 - 2.0*f);
    float a = rand(i);
    float b = rand(i + vec2(1.0, 0.0));
    float c = rand(i + vec2(0.0, 1.0));
    float d = rand(i + vec2(1.0, 1.0));
    float u = mix(a, b, f.x);
    float v = mix(c, d, f.x);
    return mix(u, v, f.y);
}
// -------------------------------------------------------------------

void main() {
    float escala = 0.1;
    float altura = noise(aPos.xz * escala) * 5.0;
    n = altura/5;

    vec3 displacedPos = vec3(aPos.x, altura, aPos.z); // Elevamos Y
    gl_Position = projection * view * vec4(displacedPos, 1.0);
    
    TexCoords = aPos.xz; // Sigue enviando las coords para color
}
