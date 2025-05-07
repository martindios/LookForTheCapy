#version 330 core

in vec2 TexCoords;    // Coordenadas de textura (o posición xz)
in vec3 FragPos;
in vec3 Normal;
in float n;          // Altura

out vec4 FragColor;

// Texturas
uniform sampler2D textureWater;
uniform sampler2D textureRock;
uniform sampler2D textureHigh;

uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 luzDir;
uniform vec3 viewPos;

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

    // Calcular luces
    vec3 ambient = 0.5 * lightColor;

    if(acos(dot(normalize(FragPos - lightPos), luzDir)) < radians(25.0)) {
        // Atenuación 
        float distance = length(lightPos - FragPos);
        float attenuation = 20.0 / (1 + 0.075 * distance + 0.03 * distance * distance);

        // Difusa
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;

        // Especular
        float specularStrength = 1.0f;
        vec3 viewDir    = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 128);
        vec3 specular   = specularStrength * spec * lightColor;

        vec3 result = (ambient + diffuse * attenuation + specular * attenuation)
                      * mixColor * (1 + attenuation);
        FragColor = vec4(result, alpha);        
    } else {
        vec3 result = ambient * mixColor;
        FragColor = vec4(result, alpha);
    }
}
