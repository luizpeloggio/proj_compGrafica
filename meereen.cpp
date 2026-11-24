#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

#include "dragao.h"

// ============================================================================
//  MEEREEN - cidade em OpenGL moderno
//  Toda a cena (milhares de tijolos) e construida UMA vez em um unico buffer
//  de vertices e desenhada com uma unica chamada por quadro.
// ============================================================================

const float kPi = 3.14159265f;

// Configuracoes da janela
unsigned int SCR_WIDTH = 1280;
unsigned int SCR_HEIGHT = 800;

// Camera livre
glm::vec3 cameraPos = glm::vec3(0.0f, 48.0f, 125.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.25f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw = -90.0f;
float pitch = -20.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

const glm::vec3 kSkyColor = glm::vec3(0.58f, 0.74f, 0.86f);

const char *vertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;

void main() {
    FragPos = aPos;
    Normal = aNormal;
    Color = aColor;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

const char *fragmentShaderSrc = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec3 Color;
out vec4 FragColor;

uniform vec3 lightDir;
uniform vec3 viewPos;
uniform vec3 skyColor;

void main() {
    vec3 n = normalize(Normal);
    vec3 l = normalize(-lightDir);

    float diff = max(dot(n, l), 0.0);
    float hemi = 0.5 + 0.5 * n.y;
    float light = 0.22 + 0.14 * hemi + 0.66 * diff;

    vec3 col = Color * light;

    // neblina de calor / poeira ao longe
    float dist = length(FragPos - viewPos);
    float fog = clamp((dist - 180.0) / 260.0, 0.0, 1.0);

    FragColor = vec4(mix(col, skyColor, fog), 1.0);
}
)";

// Cubo unitario (-0.5 a +0.5) com normais
float cubeVertices[] = {
    // Frente (+Z)
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
    // Tras (-Z)
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
    // Esquerda (-X)
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    // Direita (+X)
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
    // Topo (+Y)
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    // Base (-Y)
    -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f};

// ============================================================================
//  Callbacks / entrada
// ============================================================================

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    SCR_WIDTH = (width > 1) ? width : 1;
    SCR_HEIGHT = (height > 1) ? height : 1;
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS &&
        glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
    {
        lastX = static_cast<float>(xposIn);
        lastY = static_cast<float>(yposIn);
        return;
    }

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.12f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = 16.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed *= 2.2f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;

    glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        cameraPos -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        cameraPos += right * speed;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += speed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        cameraPos -= speed * cameraUp;
}

// ============================================================================
//  Construtor de malha: tudo vira vertices (pos, normal, cor) em g_scene
// ============================================================================

static std::vector<float> g_scene;

float hash3(int a, int b, int c)
{
    unsigned int h = static_cast<unsigned int>(a) * 73856093u ^
                     static_cast<unsigned int>(b) * 19349663u ^
                     static_cast<unsigned int>(c) * 83492791u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return static_cast<float>(h & 0xFFFFFFu) / 16777216.0f;
}

glm::vec3 shade(const glm::vec3 &c, float f)
{
    return glm::clamp(c * f, glm::vec3(0.0f), glm::vec3(1.0f));
}

void pushBox(const glm::mat4 &model, const glm::vec3 &color)
{
    glm::mat3 nm = glm::transpose(glm::inverse(glm::mat3(model)));
    for (int i = 0; i < 36; ++i)
    {
        const float *v = &cubeVertices[i * 6];
        glm::vec3 p = glm::vec3(model * glm::vec4(v[0], v[1], v[2], 1.0f));
        glm::vec3 n = glm::normalize(nm * glm::vec3(v[3], v[4], v[5]));
        g_scene.push_back(p.x);
        g_scene.push_back(p.y);
        g_scene.push_back(p.z);
        g_scene.push_back(n.x);
        g_scene.push_back(n.y);
        g_scene.push_back(n.z);
        g_scene.push_back(color.r);
        g_scene.push_back(color.g);
        g_scene.push_back(color.b);
    }
}

void addBox(const glm::vec3 &pos, const glm::vec3 &scale, const glm::vec3 &color)
{
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, pos);
    m = glm::scale(m, scale);
    pushBox(m, color);
}

void addBoxY(const glm::vec3 &pos, const glm::vec3 &scale, float angleY, const glm::vec3 &color)
{
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, pos);
    m = glm::rotate(m, angleY, glm::vec3(0.0f, 1.0f, 0.0f));
    m = glm::scale(m, scale);
    pushBox(m, color);
}

// Paleta de tijolos multicolores de Meereen
glm::vec3 brickPalette(int idx)
{
    static const glm::vec3 palette[] = {
        glm::vec3(0.68f, 0.25f, 0.16f), // vermelho tijolo
        glm::vec3(0.80f, 0.62f, 0.24f), // amarelo ocre
        glm::vec3(0.76f, 0.45f, 0.42f), // rosa queimado
        glm::vec3(0.45f, 0.28f, 0.18f), // marrom
        glm::vec3(0.26f, 0.42f, 0.56f), // azul desbotado
        glm::vec3(0.22f, 0.22f, 0.24f), // preto/cinza escuro
        glm::vec3(0.84f, 0.80f, 0.72f)  // branco/areia clara
    };
    int n = sizeof(palette) / sizeof(palette[0]);
    return palette[std::abs(idx) % n];
}

// Amarelos neutros: piramides (um pouco mais claro) e arena/arcos
static const glm::vec3 kPyrYellow = glm::vec3(0.90f, 0.82f, 0.58f);
static const glm::vec3 kArenaYellow = glm::vec3(0.88f, 0.78f, 0.50f);
static const glm::vec3 kStone = glm::vec3(0.94f, 0.90f, 0.76f);

// ----------------------------------------------------------------------------
//  Estilo de tijolo
// ----------------------------------------------------------------------------
struct BrickStyle
{
    float len = 1.5f;                                  // comprimento do tijolo (ao longo da fiada)
    float h = 0.5f;                                    // altura desejada de cada fiada
    bool mono = false;                                 // true: todos da mesma cor base (com variacao)
    glm::vec3 base = glm::vec3(0.7f, 0.4f, 0.25f);     // cor base (se mono)
    int seed = 0;                                      // semente da variacao
    glm::vec3 mortar = glm::vec3(0.62f, 0.55f, 0.42f); // cor da argamassa
    bool band = false;                                 // faixa clara decorativa perto do topo
};

glm::vec3 brickColor(const BrickStyle &st, int row, int k)
{
    float h1 = hash3(st.seed, row, k);
    float h2 = hash3(k + 17, st.seed, row + 5);
    float f = 0.86f + 0.24f * h2;
    if (st.mono)
        return shade(st.base, f);
    return shade(brickPalette(static_cast<int>(h1 * 7.0f)), f);
}

struct Gap
{
    float a, b; // intervalo ao longo da muralha (a partir do inicio)
    float h;    // altura da abertura (>= 50 => abertura total, ex.: torre do portao)
};

// ----------------------------------------------------------------------------
//  MURALHA DE TIJOLOS EM FIADAS HORIZONTAIS (aparelho "amarrado":
//  cada fiada eh deslocada meio tijolo em relacao a anterior)
// ----------------------------------------------------------------------------
void addBrickWall(const glm::vec3 &start, const glm::vec3 &end, float height, float thick,
                  const BrickStyle &st, const std::vector<Gap> &gaps, bool crenel)
{
    glm::vec3 d = end - start;
    float L = glm::length(d);
    if (L < 0.01f)
        return;
    glm::vec3 u = d / L;
    float ang = std::atan2(u.x, u.z);

    // caixa alinhada ao eixo da muralha: [a,b] ao longo, [y0,y1] em altura
    auto emit = [&](float a, float b, float y0, float y1, float th, const glm::vec3 &col)
    {
        if (b - a < 0.01f || y1 - y0 < 0.01f)
            return;
        glm::vec3 c = start + u * (0.5f * (a + b)) + glm::vec3(0.0f, 0.5f * (y0 + y1), 0.0f);
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, c);
        m = glm::rotate(m, ang, glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::scale(m, glm::vec3(th, y1 - y0, b - a));
        pushBox(m, col);
    };

    auto inTower = [&](float mid)
    {
        for (const Gap &g : gaps)
            if (g.h >= 50.0f && mid > g.a && mid < g.b)
                return true;
        return false;
    };

    // --- miolo de argamassa (aparece nas juntas entre os tijolos) ---
    std::vector<float> cuts;
    cuts.push_back(0.0f);
    cuts.push_back(L);
    for (const Gap &g : gaps)
    {
        cuts.push_back(std::min(std::max(g.a, 0.0f), L));
        cuts.push_back(std::min(std::max(g.b, 0.0f), L));
    }
    std::sort(cuts.begin(), cuts.end());
    for (size_t k = 0; k + 1 < cuts.size(); ++k)
    {
        float a = cuts[k], b = cuts[k + 1];
        if (b - a < 0.01f)
            continue;
        float mid = 0.5f * (a + b);
        float y0 = 0.0f;
        for (const Gap &g : gaps)
            if (mid > g.a && mid < g.b)
                y0 = std::max(y0, g.h);
        if (y0 >= height)
            continue;
        emit(a, b, y0, height, thick * 0.94f, st.mortar);
    }

    // --- fiadas horizontais de tijolos ---
    int nC = std::max(1, static_cast<int>(std::round(height / st.h)));
    float ch = height / static_cast<float>(nC);
    const float m = 0.06f; // espessura da junta

    for (int r = 0; r < nC; ++r)
    {
        float y0 = r * ch;
        float y1 = y0 + ch;
        float off = (r & 1) ? st.len * 0.5f : 0.0f;
        int k = 0;
        for (float s = -off; s < L - 0.01f; s += st.len, ++k)
        {
            float a = std::max(s, 0.0f);
            float b = std::min(s + st.len, L);
            bool skip = false;
            for (const Gap &g : gaps)
            {
                if (y0 >= g.h - 1e-3f)
                    continue;
                if (b <= g.a || a >= g.b)
                    continue;
                if (a >= g.a && b <= g.b)
                {
                    skip = true;
                    break;
                }
                if (a < g.a && b > g.b)
                {
                    b = g.a;
                    continue;
                }
                if (a < g.a)
                    b = g.a;
                else
                    a = g.b;
            }
            if (skip || b - a < 0.25f)
                continue;

            glm::vec3 col;
            if (st.band && r == nC - 3)
                col = shade(glm::vec3(0.84f, 0.80f, 0.72f), 0.9f + 0.2f * hash3(st.seed, r, k));
            else
                col = brickColor(st, r, k);

            emit(a + m * 0.5f, b - m * 0.5f, y0 + m * 0.5f, y1 - m * 0.5f, thick, col);
        }
    }

    // --- cornija + ameias (tambem em fiadas) ---
    if (crenel)
    {
        glm::vec3 stone = glm::vec3(0.80f, 0.72f, 0.56f);
        for (size_t k = 0; k + 1 < cuts.size(); ++k)
        {
            float a = cuts[k], b = cuts[k + 1];
            if (b - a < 0.01f || inTower(0.5f * (a + b)))
                continue;
            emit(a, b, height, height + 0.18f, thick + 0.3f, stone);
        }
        int mi = 0;
        for (float s = 0.6f; s + 1.4f <= L; s += 2.6f, ++mi)
        {
            if (inTower(s + 0.7f))
                continue;
            for (int layer = 0; layer < 2; ++layer)
            {
                float y0 = height + 0.18f + layer * 0.45f;
                emit(s, s + 1.4f, y0, y0 + 0.45f - 0.04f, thick * 0.95f, brickColor(st, 100 + layer, mi));
            }
            emit(s - 0.05f, s + 1.45f, height + 1.08f, height + 1.18f, thick, stone);
        }
    }
}

// Bloco retangular com as 4 faces em fiadas horizontais (torres, casas, degraus)
void addBrickBlock(float cx, float cz, float w, float d, float h, float t,
                   BrickStyle st, const glm::vec3 &fill, float baseY)
{
    std::vector<Gap> none;
    addBox(glm::vec3(cx, baseY + h * 0.5f, cz), glm::vec3(w - 2 * t + 0.04f, h, d - 2 * t + 0.04f), fill);
    int s0 = st.seed;
    st.seed = s0 + 1;
    addBrickWall(glm::vec3(cx - w / 2, baseY, cz - d / 2 + t / 2), glm::vec3(cx + w / 2, baseY, cz - d / 2 + t / 2), h, t, st, none, false);
    st.seed = s0 + 2;
    addBrickWall(glm::vec3(cx + w / 2, baseY, cz + d / 2 - t / 2), glm::vec3(cx - w / 2, baseY, cz + d / 2 - t / 2), h, t, st, none, false);
    st.seed = s0 + 3;
    addBrickWall(glm::vec3(cx + w / 2 - t / 2, baseY, cz - d / 2 + t), glm::vec3(cx + w / 2 - t / 2, baseY, cz + d / 2 - t), h, t, st, none, false);
    st.seed = s0 + 4;
    addBrickWall(glm::vec3(cx - w / 2 + t / 2, baseY, cz + d / 2 - t), glm::vec3(cx - w / 2 + t / 2, baseY, cz - d / 2 + t), h, t, st, none, false);
}

// ----------------------------------------------------------------------------
//  Torres
// ----------------------------------------------------------------------------
void addTower(float cx, float cz, float w, float d, float h, bool roof, int seed)
{
    BrickStyle st;
    st.len = 1.0f;
    st.h = 0.5f;
    st.seed = seed;
    st.band = true;
    glm::vec3 stone = glm::vec3(0.80f, 0.72f, 0.56f);
    addBrickBlock(cx, cz, w, d, h, 0.5f, st, glm::vec3(0.50f, 0.42f, 0.32f), 0.0f);

    // cornija
    addBox(glm::vec3(cx, h + 0.125f, cz), glm::vec3(w + 0.5f, 0.25f, d + 0.5f), stone);

    // seteiras escuras
    for (int lvl = 0; lvl < 2; ++lvl)
    {
        float y = h * (0.45f + 0.28f * lvl);
        addBox(glm::vec3(cx, y, cz + d / 2 + 0.02f), glm::vec3(0.35f, 1.1f, 0.06f), glm::vec3(0.07f, 0.06f, 0.06f));
        addBox(glm::vec3(cx, y, cz - d / 2 - 0.02f), glm::vec3(0.35f, 1.1f, 0.06f), glm::vec3(0.07f, 0.06f, 0.06f));
        addBox(glm::vec3(cx + w / 2 + 0.02f, y, cz), glm::vec3(0.06f, 1.1f, 0.35f), glm::vec3(0.07f, 0.06f, 0.06f));
        addBox(glm::vec3(cx - w / 2 - 0.02f, y, cz), glm::vec3(0.06f, 1.1f, 0.35f), glm::vec3(0.07f, 0.06f, 0.06f));
    }

    if (roof)
    {
        // telhado em degraus azul-escuro
        glm::vec3 tile = glm::vec3(0.20f, 0.34f, 0.48f);
        float y = h + 0.25f;
        float rw = w * 0.9f, rd = d * 0.9f;
        for (int i = 0; i < 4; ++i)
        {
            addBox(glm::vec3(cx, y + 0.3f, cz), glm::vec3(rw, 0.6f, rd), shade(tile, 0.9f + 0.07f * i));
            y += 0.6f;
            rw *= 0.68f;
            rd *= 0.68f;
        }
        addBox(glm::vec3(cx, y + 0.8f, cz), glm::vec3(0.12f, 1.6f, 0.12f), glm::vec3(0.55f, 0.40f, 0.15f));
    }
    else
    {
        // ameias em fiadas
        int nx = std::max(2, static_cast<int>(w / 1.0f));
        int nz = std::max(2, static_cast<int>(d / 1.0f));
        for (int layer = 0; layer < 2; ++layer)
        {
            float y = h + 0.25f + 0.45f * layer + 0.225f;
            for (int i = 0; i < nx; ++i)
            {
                float x = cx - w / 2 + 0.45f + i * (w - 0.9f) / (nx - 1);
                addBox(glm::vec3(x, y, cz - d / 2 + 0.35f), glm::vec3(0.7f, 0.41f, 0.7f), brickColor(st, 50 + layer, i));
                addBox(glm::vec3(x, y, cz + d / 2 - 0.35f), glm::vec3(0.7f, 0.41f, 0.7f), brickColor(st, 60 + layer, i));
            }
            for (int i = 1; i < nz - 1; ++i)
            {
                float z = cz - d / 2 + 0.45f + i * (d - 0.9f) / (nz - 1);
                addBox(glm::vec3(cx - w / 2 + 0.35f, y, z), glm::vec3(0.7f, 0.41f, 0.7f), brickColor(st, 70 + layer, i));
                addBox(glm::vec3(cx + w / 2 - 0.35f, y, z), glm::vec3(0.7f, 0.41f, 0.7f), brickColor(st, 80 + layer, i));
            }
        }
        // bandeira
        addBox(glm::vec3(cx, h + 2.2f, cz), glm::vec3(0.1f, 3.0f, 0.1f), glm::vec3(0.35f, 0.25f, 0.12f));
        addBox(glm::vec3(cx + 0.55f, h + 3.2f, cz), glm::vec3(1.0f, 0.6f, 0.05f), glm::vec3(0.80f, 0.62f, 0.12f));
    }
}

// ----------------------------------------------------------------------------
//  Harpia de bronze (simbolo de Meereen) - olha para +Z
// ----------------------------------------------------------------------------
void addHarpy(const glm::vec3 &p, float s)
{
    glm::vec3 bronze = glm::vec3(0.72f, 0.50f, 0.20f);
    glm::vec3 dark = glm::vec3(0.48f, 0.31f, 0.11f);
    glm::vec3 light = glm::vec3(0.86f, 0.64f, 0.28f);

    // pernas e garras
    for (int side = -1; side <= 1; side += 2)
    {
        addBox(p + glm::vec3(side * 0.35f, 0.55f, 0.0f) * s, glm::vec3(0.25f, 1.1f, 0.3f) * s, dark);
        addBox(p + glm::vec3(side * 0.35f, 0.08f, 0.25f) * s, glm::vec3(0.38f, 0.14f, 0.6f) * s, dark);
    }
    // corpo e peito
    addBox(p + glm::vec3(0.0f, 2.0f, 0.0f) * s, glm::vec3(1.0f, 1.8f, 0.8f) * s, bronze);
    addBox(p + glm::vec3(0.0f, 2.1f, 0.35f) * s, glm::vec3(0.8f, 1.2f, 0.3f) * s, light);
    // pescoco, cabeca e bico
    addBox(p + glm::vec3(0.0f, 3.05f, 0.1f) * s, glm::vec3(0.4f, 0.5f, 0.4f) * s, bronze);
    addBox(p + glm::vec3(0.0f, 3.45f, 0.15f) * s, glm::vec3(0.62f, 0.62f, 0.62f) * s, bronze);
    addBox(p + glm::vec3(0.0f, 3.35f, 0.6f) * s, glm::vec3(0.2f, 0.22f, 0.4f) * s, dark);
    addBox(p + glm::vec3(0.0f, 3.8f, 0.0f) * s, glm::vec3(0.3f, 0.3f, 0.5f) * s, dark); // crista

    // cauda inclinada
    {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, p + glm::vec3(0.0f, 1.2f, -0.85f) * s);
        m = glm::rotate(m, -0.6f, glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(0.6f, 0.2f, 1.7f) * s);
        pushBox(m, dark);
    }

    // asas abertas, 3 camadas de penas
    for (int side = -1; side <= 1; side += 2)
    {
        for (int j = 0; j < 3; ++j)
        {
            float ang = side * (0.80f - 0.27f * j);
            float len = (2.8f - 0.25f * j) * s;
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, p + glm::vec3(side * 0.5f, 2.7f - 0.15f * j, -0.15f - 0.28f * j) * s);
            m = glm::rotate(m, ang, glm::vec3(0.0f, 0.0f, 1.0f));
            m = glm::translate(m, glm::vec3(side * len * 0.5f, 0.0f, 0.0f));
            m = glm::scale(m, glm::vec3(len, 0.16f * s, (1.0f - 0.1f * j) * s));
            pushBox(m, (j == 1) ? light : bronze);
        }
    }
}

// ----------------------------------------------------------------------------
//  Piramides escalonadas (fiadas horizontais em cada degrau + escadaria)
// ----------------------------------------------------------------------------
struct PyrDef
{
    float x, z, w, h;
    int levels, seed;
};

static const PyrDef kPyramids[] = {
    {4.0f, -8.0f, 22.0f, 24.0f, 18, 0}, // Grande Piramide
    {-22.0f, -22.0f, 8.5f, 7.5f, 8, 1},
    {25.0f, -24.0f, 9.5f, 8.2f, 9, 3},
    {26.0f, 14.0f, 7.5f, 6.4f, 7, 5},
    {-4.0f, -29.0f, 7.0f, 6.0f, 7, 2}};

void addPyramid(const PyrDef &pd, bool isGreat)
{
    float sh = pd.h / static_cast<float>(pd.levels);
    float minW = pd.w * 0.22f;
    std::vector<float> widths;
    for (int i = 0; i < pd.levels; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(pd.levels - 1 > 0 ? pd.levels - 1 : 1);
        widths.push_back(pd.w * (1.0f - t) + minW * t);
    }

    for (int i = 0; i < pd.levels; ++i)
    {
        BrickStyle st;
        st.len = 0.6f + pd.w * 0.03f;
        st.h = 0.45f;
        st.mono = true;
        st.base = shade(kPyrYellow, ((i & 1) ? 1.03f : 0.97f) + 0.015f * (pd.seed % 3));
        st.seed = pd.seed * 100 + i * 7;
        float t = std::min(0.4f, widths[i] * 0.2f);
        addBrickBlock(pd.x, pd.z, widths[i], widths[i], sh, t, st, shade(st.base, 0.95f), i * sh);
    }

    // escadaria central na face +Z (degraus intermediarios sobre cada terraco)
    float sw = std::min(3.2f, widths.back() * 0.7f);
    glm::vec3 stair = glm::vec3(0.95f, 0.91f, 0.78f);
    for (int i = 0; i < pd.levels; ++i)
    {
        float fi = pd.z + widths[i] / 2;
        float dep = (i == 0) ? 1.2f : (widths[i - 1] - widths[i]) / 2;
        float hh = sh * 0.5f;
        addBox(glm::vec3(pd.x, i * sh + hh * 0.5f, fi + dep * 0.25f), glm::vec3(sw, hh, dep * 0.5f), shade(stair, 0.95f + 0.08f * (i % 2)));
    }
    // mureta de cada lado da escada (pequena)
    for (int side = -1; side <= 1; side += 2)
    {
        float topZ = pd.z + widths[0] / 2;
        addBox(glm::vec3(pd.x + side * (sw / 2 + 0.2f), sh * 0.5f, topZ + 0.4f), glm::vec3(0.35f, sh, 1.2f), shade(stair, 0.8f));
    }

    float top = pd.levels * sh;
    float tw = widths.back();
    if (isGreat)
    {
        // plinto + harpia de bronze no topo
        addBox(glm::vec3(pd.x, top + 0.3f, pd.z), glm::vec3(tw * 0.85f, 0.6f, tw * 0.85f), glm::vec3(0.80f, 0.72f, 0.56f));
        addBox(glm::vec3(pd.x, top + 0.75f, pd.z), glm::vec3(tw * 0.6f, 0.3f, tw * 0.6f), glm::vec3(0.62f, 0.55f, 0.42f));
        addHarpy(glm::vec3(pd.x, top + 0.9f, pd.z), 1.6f);
        // quatro vasos de fogo nos cantos do topo
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
            {
                glm::vec3 c = glm::vec3(pd.x + sx * tw * 0.42f, top, pd.z + sz * tw * 0.42f);
                addBox(c + glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.5f, 1.0f, 0.5f), glm::vec3(0.60f, 0.52f, 0.40f));
                addBox(c + glm::vec3(0.0f, 1.1f, 0.0f), glm::vec3(0.8f, 0.25f, 0.8f), glm::vec3(0.25f, 0.2f, 0.18f));
                addBox(c + glm::vec3(0.0f, 1.5f, 0.0f), glm::vec3(0.4f, 0.55f, 0.4f), glm::vec3(0.98f, 0.55f, 0.12f));
            }
    }
    else
    {
        // pequeno santuario no topo
        addBox(glm::vec3(pd.x, top + 0.5f, pd.z), glm::vec3(tw * 0.7f, 1.0f, tw * 0.7f), shade(kPyrYellow, 1.05f));
        addBox(glm::vec3(pd.x, top + 1.1f, pd.z), glm::vec3(tw * 0.9f, 0.2f, tw * 0.9f), glm::vec3(0.80f, 0.72f, 0.56f));
        addBox(glm::vec3(pd.x, top + 0.45f, pd.z + tw * 0.35f + 0.02f), glm::vec3(0.4f, 0.8f, 0.06f), glm::vec3(0.08f, 0.07f, 0.06f));
    }
}

// ----------------------------------------------------------------------------
//  Fosso de Daznak - 8 estratos de cores, cada degrau com fiadas de tijolos
// ----------------------------------------------------------------------------
void addPit(const glm::vec3 &c)
{
    const glm::vec3 strataColors[8] = {
        glm::vec3(0.10f, 0.10f, 0.12f), // 1. Preto (topo)
        glm::vec3(0.42f, 0.14f, 0.55f), // 2. Roxo
        glm::vec3(0.14f, 0.36f, 0.75f), // 3. Azul
        glm::vec3(0.16f, 0.58f, 0.28f), // 4. Verde
        glm::vec3(0.92f, 0.92f, 0.90f), // 5. Branco
        glm::vec3(0.92f, 0.78f, 0.14f), // 6. Amarelo
        glm::vec3(0.90f, 0.45f, 0.10f), // 7. Laranja
        glm::vec3(0.76f, 0.12f, 0.14f)  // 8. Vermelho (perto da areia)
    };

    const int segments = 72;
    const float outerR = 15.0f, innerR = 6.0f, totalDrop = 5.6f;
    const float stepDrop = totalDrop / 8.0f;
    const float ringStep = (outerR - innerR) / 8.0f;
    const float sandTop = c.y + 0.15f;
    const glm::vec3 sand = glm::vec3(0.84f, 0.68f, 0.40f);

    // piso de areia
    addBox(glm::vec3(c.x, c.y, c.z), glm::vec3(innerR * 2.0f, 0.3f, innerR * 2.0f), sand);

    // circulo de combate de pedras escuras + poste central
    for (int i = 0; i < 28; ++i)
    {
        float th = 2.0f * kPi * i / 28.0f;
        addBoxY(glm::vec3(c.x + std::cos(th) * 3.3f, sandTop + 0.03f, c.z + std::sin(th) * 3.3f),
                glm::vec3(0.5f, 0.08f, 0.95f), -th, shade(glm::vec3(0.45f, 0.36f, 0.24f), 0.85f + 0.3f * hash3(i, 3, 3)));
    }
    addBox(glm::vec3(c.x, sandTop + 1.0f, c.z), glm::vec3(0.35f, 2.0f, 0.35f), glm::vec3(0.30f, 0.22f, 0.14f));
    addBox(glm::vec3(c.x, sandTop + 1.9f, c.z), glm::vec3(0.9f, 0.15f, 0.15f), glm::vec3(0.30f, 0.22f, 0.14f));

    for (int s = 0; s < 8; ++s)
    {
        float rOuter = outerR - s * ringStep;
        float rInner = rOuter - ringStep;
        float rMid = 0.5f * (rOuter + rInner);
        float radialThick = (rOuter - rInner) * 1.15f;
        float topY = c.y + totalDrop - s * stepDrop;
        float baseY = sandTop - 0.2f;
        float blockH = topY - baseY;
        float yC = baseY + blockH * 0.5f;
        float arcLen = (2.0f * kPi * rMid) / static_cast<float>(segments);

        // arquibancada (topo do degrau)
        for (int i = 0; i < segments; ++i)
        {
            float th = (2.0f * kPi * i) / static_cast<float>(segments);
            glm::vec3 col = shade(strataColors[s], 0.9f + 0.14f * hash3(s, i, 9));
            addBoxY(glm::vec3(c.x + std::cos(th) * rMid, yC, c.z + std::sin(th) * rMid),
                    glm::vec3(radialThick, blockH, arcLen * 1.04f), -th, col);
        }

        // face interna do degrau: 2 fiadas horizontais de tijolos curvos
        float yLow = (s < 7) ? (c.y + totalDrop - (s + 1) * stepDrop) : sandTop;
        float rr = rInner - (radialThick - ringStep) * 0.5f;
        float courseH = (topY - yLow) / 2.0f;
        for (int row = 0; row < 2; ++row)
        {
            int n = static_cast<int>(std::ceil(2.0f * kPi * rr / 0.95f));
            float off = (row & 1) ? 0.5f : 0.0f;
            float arc = 2.0f * kPi * rr / n;
            for (int i = 0; i < n; ++i)
            {
                float th = 2.0f * kPi * (i + off) / n;
                glm::vec3 col = shade(strataColors[s], 0.78f + 0.18f * hash3(s * 10 + row, i, 4));
                addBoxY(glm::vec3(c.x + std::cos(th) * rr, yLow + courseH * (row + 0.5f), c.z + std::sin(th) * rr),
                        glm::vec3(0.22f, courseH - 0.05f, arc * 0.93f), -th, col);
            }
        }
    }

    // revestimento externo da arena: fiadas horizontais de tijolo amarelo neutro
    {
        const float rr = 15.32f;
        const int courses = 14;
        const float ch = totalDrop / courses;
        for (int row = 0; row < courses; ++row)
        {
            int n = static_cast<int>(std::ceil(2.0f * kPi * rr / 1.0f));
            float off = (row & 1) ? 0.5f : 0.0f;
            float arc = 2.0f * kPi * rr / n;
            for (int i = 0; i < n; ++i)
            {
                float th = 2.0f * kPi * (i + off) / n;
                if (std::fabs(std::fmod(th, 2.0f * kPi) - kPi * 0.5f) < 0.20f)
                    continue; // vao da escadaria de entrada
                glm::vec3 col = shade(kArenaYellow, 0.86f + 0.24f * hash3(row, i, 31));
                addBoxY(glm::vec3(c.x + std::cos(th) * rr, c.y + ch * (row + 0.5f), c.z + std::sin(th) * rr),
                        glm::vec3(0.34f, ch - 0.04f, arc * 0.94f), -th, col);
            }
        }
        // contrafortes
        for (int k = 0; k < 24; ++k)
        {
            float th = 2.0f * kPi * k / 24.0f + 0.065f;
            if (std::fabs(th - kPi * 0.5f) < 0.32f)
                continue;
            glm::vec3 pos = glm::vec3(c.x + std::cos(th) * 15.55f, c.y, c.z + std::sin(th) * 15.55f);
            addBoxY(pos + glm::vec3(0.0f, totalDrop * 0.5f, 0.0f), glm::vec3(0.8f, totalDrop, 0.7f), -th, shade(kStone, 0.95f));
            addBoxY(pos + glm::vec3(0.0f, totalDrop + 0.1f, 0.0f), glm::vec3(1.0f, 0.2f, 0.9f), -th, kStone);
        }
    }

    // parapeito externo em fiadas curvas, com entrada ao sul (+Z)
    {
        float rr = 14.75f;
        float y0 = c.y + totalDrop;
        const int courses = 4;
        float ch = 1.4f / courses;
        for (int row = 0; row < courses; ++row)
        {
            int n = static_cast<int>(std::ceil(2.0f * kPi * rr / 0.95f));
            float off = (row & 1) ? 0.5f : 0.0f;
            float arc = 2.0f * kPi * rr / n;
            for (int i = 0; i < n; ++i)
            {
                float th = 2.0f * kPi * (i + off) / n;
                float thn = std::fmod(th, 2.0f * kPi);
                if (std::fabs(thn - kPi * 0.5f) < 0.17f)
                    continue; // portal de entrada
                glm::vec3 col = shade(kArenaYellow, 0.88f + 0.22f * hash3(i, row, 5));
                addBoxY(glm::vec3(c.x + std::cos(th) * rr, y0 + ch * (row + 0.5f), c.z + std::sin(th) * rr),
                        glm::vec3(0.5f, ch - 0.04f, arc * 0.93f), -th, col);
            }
        }
        // cornija do parapeito
        for (int i = 0; i < segments; ++i)
        {
            float th = (2.0f * kPi * i) / static_cast<float>(segments);
            if (std::fabs(th - kPi * 0.5f) < 0.17f)
                continue;
            float arcL = 2.0f * kPi * rr / segments;
            addBoxY(glm::vec3(c.x + std::cos(th) * rr, y0 + 1.4f + 0.06f, c.z + std::sin(th) * rr),
                    glm::vec3(0.8f, 0.12f, arcL * 1.04f), -th, glm::vec3(0.80f, 0.72f, 0.56f));
        }
        // tochas de fogo
        for (int k = 0; k < 16; ++k)
        {
            float th = 2.0f * kPi * k / 16.0f + 0.2f;
            if (std::fabs(std::fmod(th, 2.0f * kPi) - kPi * 0.5f) < 0.35f)
                continue;
            glm::vec3 p = glm::vec3(c.x + std::cos(th) * rr, y0 + 1.52f, c.z + std::sin(th) * rr);
            addBox(p + glm::vec3(0.0f, 0.6f, 0.0f), glm::vec3(0.5f, 1.2f, 0.5f), glm::vec3(0.60f, 0.52f, 0.40f));
            addBox(p + glm::vec3(0.0f, 1.3f, 0.0f), glm::vec3(0.8f, 0.25f, 0.8f), glm::vec3(0.25f, 0.2f, 0.18f));
            addBox(p + glm::vec3(0.0f, 1.7f, 0.0f), glm::vec3(0.4f, 0.55f, 0.4f), glm::vec3(0.98f, 0.55f, 0.12f));
        }
    }
}

// ----------------------------------------------------------------------------
//  Ruas de pedra (paralelepipedos assentados em fiadas alternadas)
// ----------------------------------------------------------------------------
void addCobbles(float x0, float x1, float z0, float z1, int seed)
{
    const float cw = 1.15f, cd = 0.75f;
    int row = 0;
    for (float z = z0; z < z1 - 0.05f; z += cd, ++row)
    {
        float off = (row & 1) ? cw * 0.5f : 0.0f;
        int k = 0;
        for (float x = x0 - off; x < x1 - 0.05f; x += cw, ++k)
        {
            float a = std::max(x, x0), b = std::min(x + cw, x1);
            float zb = std::min(z + cd, z1);
            if (b - a < 0.15f || zb - z < 0.15f)
                continue;
            float hv = hash3(seed, row, k);
            glm::vec3 col = shade(glm::vec3(0.62f, 0.54f, 0.42f), 0.82f + 0.32f * hv);
            addBox(glm::vec3((a + b) * 0.5f, 0.04f, (z + zb) * 0.5f), glm::vec3(b - a - 0.07f, 0.08f, zb - z - 0.07f), col);
        }
    }
}

// ----------------------------------------------------------------------------
//  Palmeiras
// ----------------------------------------------------------------------------
void addPalm(float x, float z, float s, int seed)
{
    glm::vec3 trunk = glm::vec3(0.45f, 0.32f, 0.20f);
    for (int i = 0; i < 6; ++i)
    {
        float lean = 0.07f * i;
        addBox(glm::vec3(x + lean * s, (0.5f + i) * s, z), glm::vec3(0.46f - 0.035f * i, 1.02f, 0.46f - 0.035f * i) * s,
               shade(trunk, 0.85f + 0.3f * hash3(seed, i, 1)));
    }
    glm::vec3 top = glm::vec3(x + 0.42f * s, 6.0f * s, z);
    for (int k = 0; k < 8; ++k)
    {
        float a = 2.0f * kPi * k / 8.0f;
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, top);
        m = glm::rotate(m, a, glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::rotate(m, -0.45f, glm::vec3(0.0f, 0.0f, 1.0f));
        m = glm::translate(m, glm::vec3(1.4f * s, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(2.8f * s, 0.07f, 0.55f * s));
        pushBox(m, shade(glm::vec3(0.22f, 0.45f, 0.18f), 0.85f + 0.3f * hash3(seed, k, 2)));
    }
    addBox(top + glm::vec3(0.0f, -0.2f, 0.0f), glm::vec3(0.35f, 0.35f, 0.35f) * s, glm::vec3(0.55f, 0.38f, 0.14f)); // tamaras
}

// ----------------------------------------------------------------------------
//  Casas de tijolo aparente (fiadas horizontais), janelas e portas
// ----------------------------------------------------------------------------
struct Rect
{
    float x0, x1, z0, z1;
};

bool overlaps(const Rect &a, const Rect &b, float m)
{
    return a.x0 - m < b.x1 && a.x1 + m > b.x0 && a.z0 - m < b.z1 && a.z1 + m > b.z0;
}

void addHouse(float cx, float cz, float w, float d, float h, int seed)
{
    BrickStyle st;
    st.len = 0.9f;
    st.h = 0.4f;
    st.mono = true;
    st.base = brickPalette(seed);
    st.seed = seed * 31;
    addBrickBlock(cx, cz, w, d, h, 0.35f, st, shade(st.base, 0.9f), 0.0f);

    // telhado plano com platibanda
    addBox(glm::vec3(cx, h + 0.1f, cz), glm::vec3(w + 0.3f, 0.2f, d + 0.3f), glm::vec3(0.80f, 0.72f, 0.56f));
    addBox(glm::vec3(cx, h + 0.35f, cz - d / 2 - 0.1f), glm::vec3(w + 0.3f, 0.3f, 0.15f), shade(st.base, 1.1f));
    addBox(glm::vec3(cx, h + 0.35f, cz + d / 2 + 0.1f), glm::vec3(w + 0.3f, 0.3f, 0.15f), shade(st.base, 1.1f));
    addBox(glm::vec3(cx - w / 2 - 0.1f, h + 0.35f, cz), glm::vec3(0.15f, 0.3f, d + 0.3f), shade(st.base, 1.1f));
    addBox(glm::vec3(cx + w / 2 + 0.1f, h + 0.35f, cz), glm::vec3(0.15f, 0.3f, d + 0.3f), shade(st.base, 1.1f));

    // casinha no terraco (e cupula em alguns casos)
    float hv = hash3(seed, 5, 5);
    if (hv > 0.55f)
    {
        addBox(glm::vec3(cx, h + 0.2f + 0.55f, cz), glm::vec3(w * 0.4f, 1.1f, d * 0.4f), shade(brickPalette(seed + 3), 1.0f));
        addBox(glm::vec3(cx, h + 0.2f + 1.15f, cz), glm::vec3(w * 0.48f, 0.15f, d * 0.48f), glm::vec3(0.80f, 0.72f, 0.56f));
        if (hv > 0.8f)
        {
            addBox(glm::vec3(cx, h + 1.55f, cz), glm::vec3(w * 0.28f, 0.45f, d * 0.28f), glm::vec3(0.84f, 0.80f, 0.72f));
            addBox(glm::vec3(cx, h + 1.95f, cz), glm::vec3(w * 0.14f, 0.4f, d * 0.14f), glm::vec3(0.84f, 0.80f, 0.72f));
        }
    }

    // janelas
    glm::vec3 dark = glm::vec3(0.12f, 0.09f, 0.08f);
    int floors = std::max(1, static_cast<int>(h / 2.6f));
    for (int f = 0; f < floors; ++f)
    {
        float y = 1.3f + f * 2.4f;
        if (y + 0.6f > h - 0.3f)
            break;
        for (int sgn = -1; sgn <= 1; sgn += 2)
        {
            addBox(glm::vec3(cx - w * 0.26f, y, cz + sgn * (d / 2 + 0.03f)), glm::vec3(0.55f, 0.85f, 0.08f), dark);
            addBox(glm::vec3(cx + w * 0.26f, y, cz + sgn * (d / 2 + 0.03f)), glm::vec3(0.55f, 0.85f, 0.08f), dark);
            addBox(glm::vec3(cx + sgn * (w / 2 + 0.03f), y, cz), glm::vec3(0.08f, 0.85f, 0.55f), dark);
        }
    }
    // porta (face +Z)
    addBox(glm::vec3(cx, 0.85f, cz + d / 2 + 0.04f), glm::vec3(1.0f, 1.7f, 0.1f), glm::vec3(0.30f, 0.20f, 0.12f));
}

void addHouses()
{
    std::vector<Rect> blocked;
    blocked.push_back({-6.8f, 6.8f, 3.0f, 66.0f});    // avenida norte-sul
    blocked.push_back({-5.0f, 17.5f, 3.0f, 13.5f});   // praca diante da Grande Piramide
    blocked.push_back({0.0f, 56.0f, 19.5f, 32.5f});   // avenida leste
    blocked.push_back({-27.0f, -3.0f, 21.0f, 40.0f}); // escadaria e rua da arena
    for (const PyrDef &p : kPyramids)
        blocked.push_back({p.x - p.w / 2 - 2.5f, p.x + p.w / 2 + 2.5f, p.z - p.w / 2 - 2.5f, p.z + p.w / 2 + 2.5f});

    std::mt19937 rng(20260);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    auto rnd = [&](float a, float b)
    { return a + (b - a) * dist(rng); };

    int count = 0;
    for (float gz = -38.0f; gz <= 38.0f; gz += 7.0f)
    {
        for (float gx = -38.0f; gx <= 38.0f; gx += 7.0f)
        {
            float w = rnd(4.0f, 5.6f), d = rnd(4.0f, 5.6f), h = rnd(2.6f, 5.8f);
            float cx = gx + rnd(-0.4f, 0.4f), cz = gz + rnd(-0.4f, 0.4f);
            Rect r = {cx - w / 2, cx + w / 2, cz - d / 2, cz + d / 2};
            bool bad = false;
            for (const Rect &b : blocked)
                if (overlaps(r, b, 0.5f))
                {
                    bad = true;
                    break;
                }
            if (bad)
                continue;
            // fosso de Daznak (circulo)
            if (std::hypot(cx + 20.0f, cz - 8.0f) < 19.0f + std::max(w, d) * 0.5f)
                continue;
            addHouse(cx, cz, w, d, h, count + 11);
            ++count;
        }
    }
}

// ----------------------------------------------------------------------------
//  Porto: cais de madeira e barcos na Baia dos Escravos
// ----------------------------------------------------------------------------
void addBoat(float x, float z, float yawB, float len, const glm::vec3 &sailCol)
{
    glm::vec3 wood = glm::vec3(0.42f, 0.27f, 0.14f);
    glm::vec3 dark = glm::vec3(0.28f, 0.18f, 0.10f);
    auto part = [&](const glm::vec3 &p, const glm::vec3 &sc, const glm::vec3 &col)
    {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, glm::vec3(x, 0.0f, z));
        m = glm::rotate(m, yawB, glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::translate(m, p);
        m = glm::scale(m, sc);
        pushBox(m, col);
    };
    // casco em tabuas horizontais
    for (int i = 0; i < 3; ++i)
        part(glm::vec3(0.0f, -0.15f + i * 0.28f, 0.0f), glm::vec3(2.4f - 0.0f * i, 0.26f, len), shade(wood, 0.85f + 0.12f * i));
    part(glm::vec3(0.0f, 0.62f, 0.0f), glm::vec3(2.0f, 0.1f, len * 0.92f), glm::vec3(0.62f, 0.46f, 0.26f));
    part(glm::vec3(0.0f, 0.35f, len / 2 + 0.5f), glm::vec3(1.6f, 0.7f, 1.2f), wood);
    part(glm::vec3(0.0f, 0.55f, len / 2 + 1.2f), glm::vec3(0.8f, 0.6f, 0.8f), wood);
    part(glm::vec3(0.0f, 0.95f, -len / 2 + 0.3f), glm::vec3(2.0f, 0.5f, 1.0f), dark);
    // mastro e vela
    part(glm::vec3(0.0f, 3.0f, 0.3f), glm::vec3(0.18f, 5.5f, 0.18f), dark);
    part(glm::vec3(0.0f, 5.4f, 1.2f), glm::vec3(0.12f, 0.12f, 2.8f), dark);
    part(glm::vec3(0.0f, 3.5f, 1.3f), glm::vec3(0.06f, 3.6f, 2.5f), sailCol);
    part(glm::vec3(0.0f, 2.2f, 1.3f), glm::vec3(0.07f, 0.35f, 2.5f), shade(sailCol, 0.7f));
}

void addHarbor()
{
    // agua
    addBox(glm::vec3(0.0f, -0.65f, 105.0f), glm::vec3(300.0f, 1.0f, 80.0f), glm::vec3(0.12f, 0.42f, 0.55f));
    // cais
    for (int i = 0; i < 52; ++i)
    {
        float z = 63.0f + i * 0.55f;
        addBox(glm::vec3(0.0f, 0.35f, z), glm::vec3(5.0f, 0.12f, 0.5f), shade(glm::vec3(0.50f, 0.36f, 0.20f), 0.8f + 0.3f * hash3(i, 2, 8)));
    }
    for (int i = 0; i < 10; ++i)
    {
        float z = 64.0f + i * 3.0f;
        for (int sx = -1; sx <= 1; sx += 2)
        {
            addBox(glm::vec3(sx * 2.4f, 0.1f, z), glm::vec3(0.3f, 1.5f, 0.3f), glm::vec3(0.30f, 0.20f, 0.12f));
            addBox(glm::vec3(sx * 2.4f, 0.4f, z), glm::vec3(0.34f, 0.1f, 0.34f), glm::vec3(0.22f, 0.15f, 0.09f));
        }
        addBox(glm::vec3(0.0f, 0.1f, z), glm::vec3(5.0f, 0.15f, 0.2f), glm::vec3(0.30f, 0.20f, 0.12f));
    }
    // cais lateral
    for (int i = 0; i < 20; ++i)
        addBox(glm::vec3(-2.5f - 0.5f * i - 0.25f, 0.35f, 76.0f), glm::vec3(0.5f, 0.12f, 2.2f), shade(glm::vec3(0.50f, 0.36f, 0.20f), 0.8f + 0.3f * hash3(i, 3, 8)));

    // barcos
    addBoat(-6.5f, 70.0f, 0.1f, 7.0f, glm::vec3(0.90f, 0.86f, 0.74f));
    addBoat(7.0f, 80.0f, -0.15f, 8.0f, glm::vec3(0.72f, 0.22f, 0.16f));
    addBoat(-16.0f, 84.0f, 0.5f, 9.0f, glm::vec3(0.26f, 0.42f, 0.56f));
    addBoat(18.0f, 96.0f, 3.3f, 7.5f, glm::vec3(0.90f, 0.86f, 0.74f));
    addBoat(-5.0f, 100.0f, -0.4f, 8.5f, glm::vec3(0.80f, 0.62f, 0.24f));
}

// ----------------------------------------------------------------------------
//  Arco: pilares em fiadas horizontais, arco de aduelas e parede em fiadas
//  acima dele. O vao fica no plano local XY; yaw gira o arco em torno de Y.
// ----------------------------------------------------------------------------
void addArch(float cx, float baseY, float cz, float yaw, float r, float pillarH, float depth,
             const glm::vec3 &col, const glm::vec3 &trim, int seed)
{
    const float pw = 0.9f;  // largura dos pilares
    const float rad = 0.8f; // espessura radial do arco
    glm::mat4 B = glm::translate(glm::mat4(1.0f), glm::vec3(cx, baseY, cz));
    B = glm::rotate(B, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    auto part = [&](const glm::vec3 &c, const glm::vec3 &sc, const glm::vec3 &colr)
    {
        glm::mat4 m = glm::translate(B, c);
        m = glm::scale(m, sc);
        pushBox(m, colr);
    };

    // pilares
    int nC = std::max(1, static_cast<int>(std::round(pillarH / 0.45f)));
    float chh = pillarH / nC;
    for (int sd = -1; sd <= 1; sd += 2)
    {
        float px = sd * (r + pw * 0.5f);
        part(glm::vec3(px, 0.15f, 0.0f), glm::vec3(pw + 0.3f, 0.3f, depth + 0.3f), trim);
        for (int j = 0; j < nC; ++j)
            part(glm::vec3(px, 0.3f + (pillarH - 0.3f) * (j + 0.5f) / nC, 0.0f),
                 glm::vec3(pw, (pillarH - 0.3f) / nC - 0.04f, depth),
                 shade(col, 0.88f + 0.22f * hash3(seed + j, sd + 2, 5)));
        part(glm::vec3(px, pillarH + 0.12f, 0.0f), glm::vec3(pw + 0.3f, 0.24f, depth + 0.2f), trim);
    }
    (void)chh;

    // arco de aduelas
    const float springY = pillarH + 0.24f;
    const int N = 11;
    const float rm = r + rad * 0.5f;
    const float tang = kPi * rm / N * 1.04f;
    for (int k = 0; k < N; ++k)
    {
        float a = kPi * (k + 0.5f) / N;
        glm::mat4 m = glm::translate(B, glm::vec3(rm * std::cos(a), springY + rm * std::sin(a), 0.0f));
        m = glm::rotate(m, a, glm::vec3(0.0f, 0.0f, 1.0f));
        bool key = (k == N / 2);
        m = glm::scale(m, key ? glm::vec3(rad + 0.25f, tang * 1.15f, depth + 0.2f) : glm::vec3(rad, tang, depth));
        pushBox(m, (key || (k & 1)) ? trim : shade(col, 0.92f + 0.12f * hash3(seed, k, 9)));
    }

    // parede em fiadas horizontais acima do arco (preenche os cantos)
    const float cH = 0.4f;
    const float outer = r + rad;
    int nR = static_cast<int>(std::ceil(outer / cH));
    for (int j = 0; j < nR; ++j)
    {
        float dyTop = cH * (j + 1);
        float dyMid = cH * (j + 0.5f);
        float xi = (dyTop < outer) ? std::sqrt(outer * outer - dyTop * dyTop) : 0.0f;
        float x1 = r + pw;
        glm::vec3 cc = shade(col, 0.88f + 0.22f * hash3(seed + 50, j, 3));
        if (xi < 0.3f)
        {
            part(glm::vec3(0.0f, springY + dyMid, 0.0f), glm::vec3(2.0f * x1, cH - 0.04f, depth * 0.86f), cc);
        }
        else
        {
            for (int sd = -1; sd <= 1; sd += 2)
                part(glm::vec3(sd * (xi + x1) * 0.5f, springY + dyMid, 0.0f), glm::vec3(x1 - xi, cH - 0.04f, depth * 0.86f), cc);
        }
    }

    // cornija, coroa e braseiros
    const float topY = springY + nR * cH;
    part(glm::vec3(0.0f, topY + 0.15f, 0.0f), glm::vec3(2.0f * (r + pw) + 0.5f, 0.3f, depth + 0.3f), trim);
    part(glm::vec3(0.0f, topY + 0.55f, 0.0f), glm::vec3(r * 0.9f, 0.5f, depth * 0.8f), col);
    part(glm::vec3(0.0f, topY + 0.9f, 0.0f), glm::vec3(r * 1.0f, 0.2f, depth), trim);
    for (int sd = -1; sd <= 1; sd += 2)
    {
        float px = sd * (r + pw * 0.5f);
        part(glm::vec3(px, topY + 0.45f, 0.0f), glm::vec3(0.55f, 0.3f, 0.55f), glm::vec3(0.25f, 0.2f, 0.18f));
        part(glm::vec3(px, topY + 0.8f, 0.0f), glm::vec3(0.32f, 0.4f, 0.32f), glm::vec3(0.98f, 0.55f, 0.12f));
    }
}

// ----------------------------------------------------------------------------
//  Escadaria + arco de entrada da arena (portal ao sul, +Z)
// ----------------------------------------------------------------------------
static const float kStairZ0 = 14.9f, kStairDp = 0.62f, kStairDh = 0.4f;
static const int kStairN = 14;

float arenaStairTop(float zRel) // altura do degrau em zRel (a partir do centro da arena)
{
    int k = static_cast<int>(std::floor((zRel - kStairZ0) / kStairDp));
    k = std::max(0, std::min(kStairN - 1, k));
    return 5.6f - k * kStairDh + 0.02f;
}

void addArenaEntrance(const glm::vec3 &c)
{
    const float width = 5.0f;
    for (int k = 0; k < kStairN; ++k)
    {
        float top = 5.6f - k * kStairDh + 0.02f;
        float zc = c.z + kStairZ0 + (k + 0.5f) * kStairDp;
        addBox(glm::vec3(c.x, top * 0.5f, zc), glm::vec3(width, top, kStairDp * 1.02f), shade(kStone, (k & 1) ? 0.93f : 1.0f));
        for (int sd = -1; sd <= 1; sd += 2)
            addBox(glm::vec3(c.x + sd * (width * 0.5f + 0.35f), (top + 0.5f) * 0.5f, zc), glm::vec3(0.7f, top + 0.5f, kStairDp * 1.02f),
                   shade(kArenaYellow, 0.95f));
    }
    addArch(c.x, 5.6f, c.z + 14.75f, 0.0f, 2.5f, 2.2f, 1.1f, kArenaYellow, kStone, 71);
}

// ----------------------------------------------------------------------------
//  Pessoas (bonecos de blocos): pose 0 = andando, 1 = parado, 2 = acenando
// ----------------------------------------------------------------------------
void addPerson(float x, float y, float z, float yawA, int seed, int pose)
{
    static const glm::vec3 robes[8] = {
        glm::vec3(0.88f, 0.85f, 0.78f), glm::vec3(0.78f, 0.60f, 0.28f), glm::vec3(0.62f, 0.18f, 0.14f), glm::vec3(0.22f, 0.36f, 0.52f),
        glm::vec3(0.42f, 0.28f, 0.18f), glm::vec3(0.20f, 0.19f, 0.20f), glm::vec3(0.30f, 0.45f, 0.28f), glm::vec3(0.76f, 0.45f, 0.42f)};
    static const glm::vec3 skins[4] = {
        glm::vec3(0.88f, 0.68f, 0.52f), glm::vec3(0.76f, 0.54f, 0.38f), glm::vec3(0.60f, 0.40f, 0.28f), glm::vec3(0.46f, 0.30f, 0.20f)};
    const glm::vec3 pants(0.30f, 0.24f, 0.20f), boots(0.20f, 0.13f, 0.08f), hairDark(0.14f, 0.11f, 0.09f);

    float h1 = hash3(seed, 1, 7), h2 = hash3(seed, 2, 7), h3 = hash3(seed, 3, 7), h4 = hash3(seed, 4, 7);
    glm::vec3 robe = robes[static_cast<int>(h1 * 8.0f) % 8];
    glm::vec3 skin = skins[static_cast<int>(h2 * 4.0f) % 4];
    glm::vec3 accent = robes[static_cast<int>(h3 * 8.0f) % 8];
    float sc = 0.92f + 0.18f * h4;

    glm::mat4 B = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
    B = glm::rotate(B, yawA, glm::vec3(0.0f, 1.0f, 0.0f));
    B = glm::scale(B, glm::vec3(sc));
    const glm::mat4 I = glm::mat4(1.0f);
    auto part = [&](const glm::mat4 &pm, const glm::vec3 &c, const glm::vec3 &s3, const glm::vec3 &colr)
    {
        glm::mat4 m = B * pm;
        m = glm::translate(m, c);
        m = glm::scale(m, s3);
        pushBox(m, colr);
    };

    float stride = (pose == 0) ? ((h4 > 0.5f) ? 0.55f : -0.55f) : 0.0f;
    for (int sd = -1; sd <= 1; sd += 2)
    {
        glm::mat4 pm = glm::translate(I, glm::vec3(sd * 0.11f, 0.80f, 0.0f));
        pm = glm::rotate(pm, sd * stride, glm::vec3(1.0f, 0.0f, 0.0f));
        part(pm, glm::vec3(0.0f, -0.40f, 0.0f), glm::vec3(0.17f, 0.80f, 0.19f), pants);
        part(pm, glm::vec3(0.0f, -0.78f, 0.05f), glm::vec3(0.19f, 0.12f, 0.28f), boots);
    }
    part(I, glm::vec3(0.0f, 0.78f, 0.0f), glm::vec3(0.46f, 0.50f, 0.30f), shade(robe, 0.92f)); // saia da tunica
    part(I, glm::vec3(0.0f, 1.18f, 0.0f), glm::vec3(0.46f, 0.50f, 0.27f), robe);               // torso
    part(I, glm::vec3(0.0f, 1.42f, 0.0f), glm::vec3(0.54f, 0.14f, 0.32f), accent);             // manto nos ombros
    for (int sd = -1; sd <= 1; sd += 2)
    {
        float ang = (pose == 0) ? -sd * stride * 0.9f : 0.0f;
        if (pose == 2 && sd == 1)
            ang = -2.6f;
        glm::mat4 pm = glm::translate(I, glm::vec3(sd * 0.29f, 1.40f, 0.0f));
        pm = glm::rotate(pm, ang, glm::vec3(1.0f, 0.0f, 0.0f));
        part(pm, glm::vec3(0.0f, -0.28f, 0.0f), glm::vec3(0.12f, 0.56f, 0.14f), shade(robe, 0.85f));
        part(pm, glm::vec3(0.0f, -0.58f, 0.0f), glm::vec3(0.11f, 0.11f, 0.11f), skin);
    }
    part(I, glm::vec3(0.0f, 1.60f, 0.0f), glm::vec3(0.24f, 0.26f, 0.24f), skin);
    part(I, glm::vec3(0.0f, 1.74f, -0.01f), glm::vec3(0.27f, 0.12f, 0.27f), (h1 > 0.5f) ? accent : hairDark);
    if (h2 > 0.85f && pose != 2) // jarra na cabeca
    {
        part(I, glm::vec3(0.0f, 1.97f, 0.0f), glm::vec3(0.22f, 0.30f, 0.22f), glm::vec3(0.62f, 0.34f, 0.20f));
        part(I, glm::vec3(0.0f, 2.14f, 0.0f), glm::vec3(0.16f, 0.06f, 0.16f), glm::vec3(0.50f, 0.27f, 0.16f));
    }
}

void addPeople()
{
    std::mt19937 rng(4242);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    auto rnd = [&](float a, float b)
    { return a + (b - a) * dist(rng); };
    int id = 0;

    auto poseOf = [&]()
    {
        float r = dist(rng);
        return r < 0.68f ? 0 : (r < 0.90f ? 1 : 2);
    };
    // yawBase: direcao de caminhada; parados olham para qualquer lado
    auto place = [&](float x, float y, float z, float yawBase)
    {
        int pose = poseOf();
        float yaw = (pose == 0) ? yawBase + rnd(-0.12f, 0.12f) : rnd(0.0f, 2.0f * kPi);
        addPerson(x, y, z, yaw, 500 + id++, pose);
    };

    // avenida do portao sul
    for (int i = 0; i < 30; ++i)
    {
        float z = 14.0f + 47.0f * (i + dist(rng)) / 30.0f;
        float x = rnd(-3.4f, 3.4f);
        place(x, 0.08f, z, x > 0.0f ? 0.0f : kPi);
    }
    // avenida do portao leste
    for (int i = 0; i < 26; ++i)
    {
        float x = 6.0f + 44.0f * (i + dist(rng)) / 26.0f;
        float z = rnd(22.6f, 29.4f);
        place(x, 0.08f, z, z < 26.0f ? kPi * 0.5f : -kPi * 0.5f);
    }
    // praca diante da Grande Piramide
    for (int i = 0; i < 10; ++i)
        place(rnd(-3.0f, 15.0f), 0.08f, rnd(5.0f, 11.5f), rnd(0.0f, 2.0f * kPi));
    // rua ate a arena
    for (int i = 0; i < 12; ++i)
    {
        float z = rnd(32.4f, 37.1f);
        place(rnd(-22.5f, -5.0f), 0.08f, z, z < 34.7f ? kPi * 0.5f : -kPi * 0.5f);
    }
    // escadaria da arena (subindo, -Z)
    for (int i = 0; i < 6; ++i)
    {
        float zRel = rnd(15.6f, 22.0f);
        addPerson(-20.0f + rnd(-1.9f, 1.9f), arenaStairTop(zRel), 8.0f + zRel, kPi + rnd(-0.1f, 0.1f), 500 + id++, 0);
    }
    // cais
    for (int i = 0; i < 6; ++i)
    {
        float x = rnd(-1.9f, 1.9f);
        place(x, 0.42f, rnd(65.0f, 86.0f), x > 0.0f ? 0.0f : kPi);
    }
}

// ============================================================================
//  CENA COMPLETA
// ============================================================================
void buildScene()
{
    g_scene.reserve(40u * 1000u * 1000u / sizeof(float));

    // 1. Chao da cidade / areia (estendido ate a praia ao sul)
    addBox(glm::vec3(0.0f, -0.5f, 5.0f), glm::vec3(110.0f, 1.0f, 120.0f), glm::vec3(0.76f, 0.62f, 0.38f));

    // 2. Ruas de pedra
    addCobbles(-4.0f, 4.0f, 12.0f, 63.0f, 1);   // avenida do portao sul
    addCobbles(-4.0f, 16.0f, 3.0f, 12.0f, 2);   // praca diante da Grande Piramide
    addCobbles(4.0f, 54.0f, 22.0f, 30.0f, 3);   // avenida do portao leste
    addCobbles(-23.0f, -4.0f, 31.5f, 38.0f, 4); // rua ate a escadaria da arena

    // 3. Muralha de tijolos em fiadas horizontais
    const float halfWall = 44.0f;
    const float wallH = 6.5f;
    const float wallT = 1.6f;
    const float cornerHalf = 2.1f; // metade da torre de canto
    const float gateHalf = 4.0f;   // metade da abertura do portao
    const float gateTowerW = 3.2f; // largura de cada torre do portao
    const float gateH = 4.5f;      // altura da abertura do portao

    BrickStyle ws;
    ws.len = 1.5f;
    ws.h = 0.5f;
    ws.band = true;

    // Norte (sem portao): x de -41.9 a 41.9 em z = -44
    ws.seed = 10;
    addBrickWall(glm::vec3(-halfWall + cornerHalf, 0.0f, -halfWall), glm::vec3(halfWall - cornerHalf, 0.0f, -halfWall), wallH, wallT, ws, std::vector<Gap>(), true);
    // Oeste (sem portao): z de 41.9 a -41.9 em x = -44
    ws.seed = 20;
    addBrickWall(glm::vec3(-halfWall, 0.0f, halfWall - cornerHalf), glm::vec3(-halfWall, 0.0f, -halfWall + cornerHalf), wallH, wallT, ws, std::vector<Gap>(), true);

    // Sul (portao em x = 0): vai de x=+41.9 ate x=-41.9 em z = +44
    {
        float st0 = halfWall - cornerHalf; // coordenada x do inicio
        float c0 = 0.0f;                   // centro do portao
        std::vector<Gap> gaps;
        gaps.push_back({st0 - (c0 + gateHalf + gateTowerW), st0 - (c0 + gateHalf), 99.0f});
        gaps.push_back({st0 - (c0 + gateHalf), st0 - (c0 - gateHalf), gateH});
        gaps.push_back({st0 - (c0 - gateHalf), st0 - (c0 - gateHalf - gateTowerW), 99.0f});
        ws.seed = 30;
        addBrickWall(glm::vec3(st0, 0.0f, halfWall), glm::vec3(-st0, 0.0f, halfWall), wallH, wallT, ws, gaps, true);
        float tc = gateHalf + gateTowerW * 0.5f;
        addTower(-tc, halfWall, gateTowerW, wallT + 1.2f, 9.5f, false, 41);
        addTower(tc, halfWall, gateTowerW, wallT + 1.2f, 9.5f, false, 42);
    }
    // Leste (portao em z = 26): vai de z=-41.9 ate z=+41.9 em x = +44
    {
        float st0 = -halfWall + cornerHalf; // coordenada z do inicio
        float c0 = 26.0f;
        std::vector<Gap> gaps;
        gaps.push_back({(c0 - gateHalf - gateTowerW) - st0, (c0 - gateHalf) - st0, 99.0f});
        gaps.push_back({(c0 - gateHalf) - st0, (c0 + gateHalf) - st0, gateH});
        gaps.push_back({(c0 + gateHalf) - st0, (c0 + gateHalf + gateTowerW) - st0, 99.0f});
        ws.seed = 40;
        addBrickWall(glm::vec3(halfWall, 0.0f, st0), glm::vec3(halfWall, 0.0f, -st0), wallH, wallT, ws, gaps, true);
        float tc = gateHalf + gateTowerW * 0.5f;
        addTower(halfWall, c0 - tc, wallT + 1.2f, gateTowerW, 9.5f, false, 43);
        addTower(halfWall, c0 + tc, wallT + 1.2f, gateTowerW, 9.5f, false, 44);
    }

    // Torres de canto
    const float cs[4][2] = {{-halfWall, -halfWall}, {halfWall, -halfWall}, {halfWall, halfWall}, {-halfWall, halfWall}};
    for (int c = 0; c < 4; ++c)
        addTower(cs[c][0], cs[c][1], cornerHalf * 2.0f, cornerHalf * 2.0f, 10.5f, true, 50 + c);

    // lintel decorativo dos portoes (faixa de pedra clara sobre a abertura)
    addBox(glm::vec3(0.0f, gateH + 0.1f, halfWall), glm::vec3(gateHalf * 2.0f + 0.4f, 0.2f, wallT + 0.5f), glm::vec3(0.84f, 0.80f, 0.72f));
    addBox(glm::vec3(halfWall, gateH + 0.1f, 26.0f), glm::vec3(wallT + 0.5f, 0.2f, gateHalf * 2.0f + 0.4f), glm::vec3(0.84f, 0.80f, 0.72f));

    // 4. Piramides (a primeira eh a Grande Piramide com a harpia)
    for (int i = 0; i < 5; ++i)
        addPyramid(kPyramids[i], i == 0);

    // 5. Fosso de Daznak
    addPit(glm::vec3(-20.0f, 0.0f, 8.0f));

    // 6. Casas
    addHouses();

    // 7. Palmeiras ao longo da avenida do portao sul
    int pi = 0;
    for (float z = 26.0f; z <= 40.0f; z += 7.0f)
    {
        addPalm(-5.5f, z, 1.0f, pi++);
        addPalm(5.5f, z, 1.0f, pi++);
    }
    addPalm(-3.5f, 8.0f, 1.1f, pi++);
    addPalm(15.0f, 9.0f, 1.1f, pi++);

    // 8. Porto e baia
    addHarbor();

    // 9. Arcos de saida no fim das duas avenidas e entrada da arena
    addArch(0.0f, 0.0f, 61.5f, 0.0f, 4.0f, 2.8f, 1.2f, kArenaYellow, kStone, 61);
    addArch(52.0f, 0.0f, 26.0f, kPi * 0.5f, 4.0f, 2.8f, 1.2f, kArenaYellow, kStone, 62);
    addArenaEntrance(glm::vec3(-20.0f, 0.0f, 8.0f));

    // 10. Mais palmeiras: avenida leste, avenida sul (fora do portao), rua da arena e praca
    for (float x = 10.0f; x <= 40.0f; x += 6.0f)
    {
        addPalm(x, 20.6f, 1.0f + 0.12f * hash3(pi, 1, 1), pi);
        ++pi;
        addPalm(x + 3.0f, 31.4f, 1.0f + 0.12f * hash3(pi, 2, 1), pi);
        ++pi;
    }
    for (float z = 50.0f; z <= 60.0f; z += 5.0f)
    {
        addPalm(-5.5f, z, 1.05f, pi++);
        addPalm(5.5f, z + 2.5f, 1.0f, pi++);
    }
    for (float z = 13.5f; z <= 19.5f; z += 6.0f)
    {
        addPalm(-5.5f, z, 1.0f, pi++);
        addPalm(5.5f, z, 1.05f, pi++);
    }
    for (float x = -14.0f; x <= -6.0f; x += 8.0f)
    {
        addPalm(x, 30.2f, 1.0f, pi++);
        addPalm(x, 39.3f, 1.1f, pi++);
    }
    addPalm(-15.5f, 31.0f, 1.0f, pi++);
    addPalm(-25.0f, 36.0f, 1.1f, pi++);
    addPalm(17.5f, 4.5f, 1.1f, pi++);
    addPalm(17.5f, 11.0f, 1.0f, pi++);
    addPalm(-5.5f, 6.0f, 1.0f, pi++);

    // 11. Gente nas ruas
    addPeople();
}

// ============================================================================
//  Shaders
// ============================================================================
unsigned int compileShader(unsigned int type, const char *src)
{
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int success;
    glGetShaderiv(s, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Erro ao compilar shader:\n"
                  << log << "\n";
    }
    return s;
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Falha ao inicializar GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Meereen - OpenGL Moderno", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Falha ao criar janela GLFW\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Falha ao inicializar GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // dragao com cavaleiro voando em volta da Grande Piramide (centro 4,-8)
    dragao::Dragon dragon;
    dragon.init(glm::vec3(4.0f, 0.0f, -8.0f));

    // direcao inicial da camera coerente com yaw/pitch
    cameraFront = glm::normalize(glm::vec3(
        std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch)),
        std::sin(glm::radians(pitch)),
        std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch))));

    // constroi a cidade inteira uma unica vez
    buildScene();
    const int vertexCount = static_cast<int>(g_scene.size() / 9);
    std::cout << "Meereen: " << vertexCount / 36 << " blocos, " << vertexCount << " vertices\n";

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, g_scene.size() * sizeof(float), g_scene.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    g_scene.clear();
    g_scene.shrink_to_fit();

    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    unsigned int shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    int locView = glGetUniformLocation(shader, "view");
    int locProj = glGetUniformLocation(shader, "projection");
    int locLight = glGetUniformLocation(shader, "lightDir");
    int locViewPos = glGetUniformLocation(shader, "viewPos");
    int locSky = glGetUniformLocation(shader, "skyColor");

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Ceu arido da Baia dos Escravos
        glClearColor(kSkyColor.r, kSkyColor.g, kSkyColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                                static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT),
                                                0.1f, 600.0f);
        glm::vec3 lightDir = glm::vec3(-0.65f, -0.95f, -0.45f);

        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(locLight, 1, glm::value_ptr(lightDir));
        glUniform3fv(locViewPos, 1, glm::value_ptr(cameraPos));
        glUniform3fv(locSky, 1, glm::value_ptr(kSkyColor));

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);

        // dragao (usa programa/VAO proprios)
        dragon.render(deltaTime, view, projection, cameraPos, lightDir, kSkyColor);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader);
    dragon.cleanup();
    glfwTerminate();
    return 0;
}