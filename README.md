# Meereen — cidade em OpenGL moderno

Maquete 3D interativa da cidade de **Meereen**, feita em C++ com OpenGL 3.3 (core profile). Toda a cidade é montada em tempo de execução a partir de blocos: muralhas de tijolos assentados em fiadas horizontais, pirâmides escalonadas, o Fosso de Daznak, casas, ruas de pedra, um porto com barcos e um dragão animado com cavaleiro sobrevoando a Grande Pirâmide.


## Destaques

- **Muralha de tijolos** com fiadas horizontais amarradas (cada fiada deslocada meio tijolo), cornijas, ameias, torres de canto e dois portões (sul e leste).
- **Grande Pirâmide** com 18 andares, escadaria e harpia de bronze no topo, mais quatro pirâmides menores. Todas em amarelo claro neutro.
- **Fosso de Daznak** com arquibancada de 8 estratos coloridos, arena de areia, parapeito com tochas, revestimento externo amarelo e escadaria monumental com arco de entrada.
- **Cidade viva**: cerca de 90 pessoas nas avenidas, na praça, na rua da arena e no cais, além de casas de tijolo aparente, ruas de pedra em fiadas alternadas e palmeiras.
- **Arcos de saída** no fim de cada avenida, feitos com pilares e parede em fiadas horizontais e arco de aduelas com pedra-chave.
- **Porto** na Baía dos Escravos, com cais de madeira e barcos.
- **Dragão animado** com cavaleiro: voa em círculos em volta da Grande Pirâmide, inclina nas curvas, bate as asas e solta fogo em rajadas (sistema de partículas).
- **Câmera livre** para explorar a cidade inteira.

## Desempenho

A cidade (cerca de 22 mil blocos, mais de 800 mil vértices) é construída **uma única vez** em um único buffer e desenhada com **uma chamada por quadro**. Apenas o dragão e o fogo são recalculados a cada quadro, em programa e buffers próprios.

## Controles

| Ação | Tecla / mouse |
|---|---|
| Mover para frente / trás | `W` / `S` ou setas ↑ / ↓ |
| Mover para esquerda / direita | `A` / `D` ou setas ← / → |
| Subir / descer | `Espaço` / `Ctrl` esquerdo |
| Mover mais rápido | `Shift` esquerdo (segure) |
| Olhar em volta | Segure o **botão direito** do mouse e mova |
| Sair | `Esc` |

## Requisitos

- Compilador com suporte a C++17 (GCC, Clang ou MSVC)
- Placa de vídeo e driver com OpenGL 3.3 ou superior
- Bibliotecas:
  - [GLFW](https://www.glfw.org/) — janela e entrada
  - [GLAD](https://glad.dav1d.de/) — carregador do OpenGL (gere para **OpenGL 3.3, perfil Core**)
  - [GLM](https://github.com/g-truc/glm) — matemática (somente cabeçalhos)

## Estrutura do projeto

```
.
├── meereen.cpp   # cidade, câmera, shaders e laço principal
├── dragao.h      # módulo independente do dragão (modelo, voo, fogo)
└── README.md
```

Se você baixou o GLAD, a pasta dele deve ficar ao lado dos arquivos, assim:

```
glad/
├── include/   (glad/glad.h e KHR/khrplatform.h)
└── src/       (glad.c)
```

## Como compilar

### Linux

Instale as dependências (exemplo no Debian/Ubuntu):

```bash
sudo apt install build-essential libglfw3-dev libglm-dev
```

Compile:

```bash
g++ -std=c++17 -O2 meereen.cpp glad/src/glad.c -Iglad/include -o meereen -lglfw -ldl
./meereen
```

### macOS

```bash
brew install glfw glm
clang++ -std=c++17 -O2 meereen.cpp glad/src/glad.c -Iglad/include \
  -I/opt/homebrew/include -L/opt/homebrew/lib -lglfw -framework OpenGL -o meereen
./meereen
```

> No macOS, o contexto OpenGL Core exige a linha `glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);` em `main()`, logo após os outros `glfwWindowHint`.

### Windows

Com o **vcpkg** e o Visual Studio:

```powershell
vcpkg install glfw3 glm
```

Crie um projeto C++17, adicione `meereen.cpp` e `glad/src/glad.c`, inclua a pasta `glad/include` e linke com `glfw3` e `opengl32`.

## Personalização

- **Câmera inicial**: variáveis `cameraPos`, `yaw` e `pitch` no topo de `meereen.cpp`.
- **Muralha**: constantes `halfWall`, `wallH`, `wallT` e `gateH` em `buildScene()`. O tamanho e o aparelho dos tijolos ficam na estrutura `BrickStyle`.
- **Cores**: `kPyrYellow`, `kArenaYellow` e `kStone` definem os amarelos neutros das pirâmides, da arena e dos arcos.
- **Pessoas**: posições e quantidades em `addPeople()`.
- **Dragão**: `dragon.init(centro, raio, altitude)`, cujos padrões são raio 25 e altitude 32. A velocidade (`omega`) e o ciclo de fogo (4 s a cada 7 s) ficam em `update()` no `dragao.h`.

## Como o dragão se integra à cena

O `dragao.h` é independente e precisa de apenas 4 chamadas:

```cpp
#include "dragao.h"

dragao::Dragon dragon;
dragon.init(glm::vec3(4.0f, 0.0f, -8.0f));   // após inicializar o GLAD
// ...
dragon.render(deltaTime, view, projection, cameraPos, lightDir, kSkyColor); // após desenhar a cidade
// ...
dragon.cleanup();                            // antes de glfwTerminate()
```


