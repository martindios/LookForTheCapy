#version 330 core

in vec2 TexCoords;    // Coordenadas de textura (o posición xz)
in float n;          // Altura

out vec4 FragColor;

// Texturas
uniform sampler2D textureWater;
uniform sampler2D textureRock;
uniform sampler2D textureHigh;

void main() {
    // Definir los colores para el gradiente
    vec3 colorLow = vec3(0.5, 0.5, 0.8);  
    vec3 colorMid = vec3(0.5, 0.25, 0.0);   
    vec3 colorHigh = vec3(1.0, 1.0, 1.0); 

    // Normalizar n a un rango de 0.0 a 1.0
    float normalizedHeight = clamp(n, 0.0, 1.0); // Asegúrate de que n esté en el rango [0, 1]

    // Interpolar entre los colores según la altura
    vec3 color;
    vec3 mixColor;
    vec4 Texture;
    float alpha = 1.0;
    if (normalizedHeight < 0.4) {
        color = mix(vec3(0.0, 0.0, 0.0), colorLow, normalizedHeight * 2.5);
        Texture = texture(textureWater, TexCoords);
        mixColor = Texture.rgb * color;
        alpha = Texture.a;
    } else if (normalizedHeight < 0.6) {
        color = mix(colorLow, colorMid, (normalizedHeight - 0.3) * 2.0);
        Texture = texture(textureRock, TexCoords);
        mixColor = Texture.rgb * color;
        alpha = Texture.a;
    } else {
        color = mix(colorMid, colorHigh, (normalizedHeight - 0.6) * 2.0);
        Texture = texture(textureRock, TexCoords);
        mixColor = Texture.rgb * color;
        alpha = Texture.a;
    }

    FragColor = vec4(mixColor, alpha);
}
