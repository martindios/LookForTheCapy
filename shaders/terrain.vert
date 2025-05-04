#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal; 
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
    float maxAltura = 10.0;
    float altura = noise(aPos.xz * escala) * maxAltura;
    n = altura / maxAltura;

    // Calcular desplazamiento
    vec3 displacedPos = vec3(aPos.x, altura, aPos.z);

    // Calcular normales usando derivadas centrales
    float delta = 0.1; // Pequeño desplazamiento para calcular derivadas
    float alturaL = noise((aPos.xz + vec2(-delta, 0.0)) * escala) * maxAltura;
    float alturaR = noise((aPos.xz + vec2(delta, 0.0)) * escala) * maxAltura;
    float alturaD = noise((aPos.xz + vec2(0.0, -delta)) * escala) * maxAltura;
    float alturaU = noise((aPos.xz + vec2(0.0, delta)) * escala) * maxAltura;

    vec3 tangent = vec3(2.0 * delta, alturaR - alturaL, 0.0);
    vec3 bitangent = vec3(0.0, alturaU - alturaD, 2.0 * delta);
    Normal = normalize(cross(tangent, bitangent)); // Normalizar la normal

    gl_Position = projection * view * vec4(displacedPos, 1.0);
    FragPos = displacedPos;
    TexCoords = aPos.xz; // Sigue enviando las coords para color
}
