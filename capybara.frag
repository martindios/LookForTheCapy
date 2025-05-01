#version 330 core

in vec3 ourNormal;
in vec3 FragPos;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;

void main() {
    // Muestreamos RGBA
    vec4 texSample = texture(texture1, TexCoord);
    vec3 texColor = texSample.rgb;
    float alpha    = texSample.a;

    // Opcional: recortamos píxeles casi transparentes
    if (alpha < 0.1)
        discard;

    FragColor = vec4(texColor, alpha);
}

