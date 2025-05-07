# Look for the Capy 

## Descripción

Look for the Capy es un juego desarrollado en C++ que utiliza OpenGL 3.3 para crear un terreno 3D procedimental usando ruido de Perlin. 
El objetivo del juego es encontrar una capibara a lo largo del terreno todas las veces que sea posible. Ten en cuenta que si tardas demasiado en encontrarla,
perderás el juego.

![imaxe](https://github.com/user-attachments/assets/17bb2e22-c07a-4474-8e9f-677e9788cac1)


## Características

- Gráficos 3D utilizando OpenGL 3.3 y modelos 3D.
- Terreno procedimental generado con ruido de Perlin.
- Uso de distintos shaders para los objetos.
- Implementación de colisiones.
- Interacción con el jugador usando el teclado.

## Funcionamiento
Usando las librerías de OpenGL 3.3 junto con el modelo 3D de la capibara, creamos la escena. Implementamos la función de ruido de Perlin en su propio
shader para que se ejecute directamente en la GPU y optimizar el código. Con esta función le asignamos la altura al terreno de forma irregular, haciéndolo un 
poco más realista. 
En cuanto a la cámara, ajustamos las matrices de transformación según la entrada del usuario por teclado, usando esta posición también para la implementación
de las luces.
Finalmente, implementamos colisiones entre la cámara y la capibara para poder aumentar los puntos del jugador y reubicar la capybara dentro del terreno. El propio
programa cuenta con un temporizador para terminar la partida en caso de que no se produzca ninguna colisión en un intervalo de 30 segundos.

## Requisitos

- Sistema operativo: Linux
- Compilador: GCC
- Bibliotecas necesarias:
  - [GLFW](https://www.glfw.org/)
  - [GLM](https://github.com/g-truc/glm) 
  - [glad](https://glad.dav1d.de/) 

## Instalación

1. Clona el repositorio:
   ```bash
   git clone https://github.com/tu_usuario/capibara-quest.git
2. Instala las dependencias necesarias como libglfw3-dev
3. Compila el proyecyo
   ```bash
   cd LookForTheCapy
   g++ main.cpp aux.cpp glad.c -o capybara -ldl -lglfw -lm -lGL -lGLU
4. Ejecuta el juego
   ``` bash
   ./capybara

## Uso

Para jugar a Look for the Capy, sigue estos pasos:

1. **Iniciar el juego**: Ejecuta el archivo compilado `capybara` desde la terminal.
   
2. **Controles**:
   Muévete con las flechas del teclado, la cámara y la luz se moverán contigo.

4. **Objetivo**: Explora el terreno generado proceduralmente y busca la capibara. Una vez que la encuentres, atraviésala para anotar un punto. En ese momento,
5. reaparecerá en otro sitio para volver a buscarla. Hazlo rápido, si tardas más de 30 segundos, perderás el juego.

¡Diviértete explorando y buscando la capibara!

## Créditos del modelo 3D

- **Modelo:** “Capybara Low-Poly”  
- **Autor:** Nyilonelycompany  
- **Fuente:** [Sketchfab – Capybara Low-Poly](https://sketchfab.com/3d-models/capybara-low-poly-2b9e0100da7245079fa3d54eedd81030)  
- **Licencia:** CC Attribution‑NonCommercial (Creative Commons Attribution‐NonCommercial)  

## Nuestra licencia
Compartimos este proyecto bajo una licencia MIT. Puedes consultar más información sobre la licencia en la sección LICENSE del repositorio.
