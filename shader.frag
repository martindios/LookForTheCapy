#version 330 core

in vec3 ourNormal;
in vec3 FragPos;
in vec3 ourColor;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;
uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 luzDir;

uniform bool noche;

void main() {
    // Muestreamos RGBA
    vec4 texSample = texture(texture1, TexCoord);
    vec3 texColor = texSample.rgb;
    float alpha    = texSample.a;

    // Opcional: recortamos píxeles casi transparentes
    if (alpha < 0.1)
        discard;

    vec3 color = texColor;
    if (texColor == vec3(0.0, 0.0, 0.0)) {
        color = ourColor;
    }

    float ambiente = 0.5;

    vec3 ambient = ambiente * lightColor * color;

    vec3 fd = normalize(FragPos - lightPos);

    if (noche && acos(dot(fd, luzDir)) < radians(15.0)) {
        // Atenuación 
        float distance    = length(lightPos - FragPos);
        float attenuation = 10.0 / (1 + 0.09 * distance + 0.032 * distance * distance);

        // Difusa
        vec3 norm     = normalize(ourNormal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff    = max(dot(norm, lightDir), 0.0);
        vec3 diffuse  = diff * lightColor;

        // Especular
        float specularStrength = 1.0;
        vec3 viewDir    = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 128);
        vec3 specular   = specularStrength * spec * lightColor;

        vec3 result = (ambient + diffuse * attenuation + specular * attenuation)
                      * ourColor * (1 + attenuation);

        FragColor = vec4(result, alpha);
    } else {
        vec3 result = ambient * ourColor;
        FragColor = vec4(result, alpha);
    }
}

