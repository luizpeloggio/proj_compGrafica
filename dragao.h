#pragma once
// ============================================================================
//  dragao.h - Dragao animado de Meereen (modulo independente)
//
//  Dragao original, feito so de blocos: escamas verde-petroleo, barriga
//  dourada, chifres de osso, asas de membrana vinho com 4 dedos, cauda com
//  lamina. Um cavaleiro de capa carmesim monta entre o pescoco e as asas.
//  O dragao voa em circulos em volta da Grande Piramide, inclinando nas
//  curvas, batendo as asas e cuspindo fogo em rajadas (particulas).
//
//  USO (3 linhas no seu main):
//      #include "dragao.h"
//      dragao::Dragon dragon;                       // global ou antes do loop
//      dragon.init(glm::vec3(4.0f, 0.0f, -8.0f));   // depois do gladLoadGL
//      ...
//      // no loop, DEPOIS do glDrawArrays da cidade:
//      dragon.render(deltaTime, view, projection, cameraPos, lightDir, kSkyColor);
//      ...
//      dragon.cleanup();                            // antes do glfwTerminate
//
//  Inclua DEPOIS de <glad/glad.h> e das headers do glm.
// ============================================================================

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace dragao
{

    using V3 = glm::vec3;
    const float kPi = 3.14159265f;

    // ----------------------------------------------------------------------------
    //  Cubo unitario (posicao + normal)
    // ----------------------------------------------------------------------------
    static const float kCube[] = {
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f};

    // ----------------------------------------------------------------------------
    //  Paleta do dragao
    // ----------------------------------------------------------------------------
    const V3 kScale(0.10f, 0.40f, 0.35f); // escamas verde-petroleo
    const V3 kDark(0.05f, 0.25f, 0.22f);  // escamas escuras
    const V3 kBelly(0.88f, 0.74f, 0.42f); // placas da barriga
    const V3 kGold(0.92f, 0.70f, 0.20f);  // espinhos / cristas
    const V3 kHorn(0.93f, 0.86f, 0.66f);  // chifres de osso
    const V3 kMem(0.52f, 0.14f, 0.10f);   // membrana das asas
    const V3 kClaw(0.94f, 0.92f, 0.84f);  // garras / dentes
    const V3 kEye(1.00f, 0.72f, 0.10f);   // olhos (brilham)

    inline V3 shade(const V3 &c, float f)
    {
        return glm::clamp(c * f, V3(0.0f), V3(1.0f));
    }

    inline glm::mat4 Tr(const V3 &v) { return glm::translate(glm::mat4(1.0f), v); }
    inline glm::mat4 Rx(float a) { return glm::rotate(glm::mat4(1.0f), a, V3(1, 0, 0)); }
    inline glm::mat4 Ry(float a) { return glm::rotate(glm::mat4(1.0f), a, V3(0, 1, 0)); }
    inline glm::mat4 Rz(float a) { return glm::rotate(glm::mat4(1.0f), a, V3(0, 0, 1)); }
    inline glm::mat4 Sc(const V3 &v) { return glm::scale(glm::mat4(1.0f), v); }

    inline float smooth(float a, float b, float x)
    {
        float t = std::min(1.0f, std::max(0.0f, (x - a) / (b - a)));
        return t * t * (3.0f - 2.0f * t);
    }

    // ----------------------------------------------------------------------------
    //  Shaders (mesmo visual da cidade + modo "sem luz" para o fogo e os olhos)
    // ----------------------------------------------------------------------------
    static const char *kVS = R"(
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

    static const char *kFS = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec3 Color;
out vec4 FragColor;
uniform vec3 lightDir;
uniform vec3 viewPos;
uniform vec3 skyColor;
uniform float unlit;
void main() {
    vec3 n = normalize(Normal);
    vec3 l = normalize(-lightDir);
    float diff = max(dot(n, l), 0.0);
    float hemi = 0.5 + 0.5 * n.y;
    vec3 fillDir = normalize(vec3(0.65, 0.25, 0.45));
    float fill = max(dot(n, fillDir), 0.0);
    float light = 0.34 + 0.16 * hemi + 0.58 * diff + 0.24 * fill;
    vec3 lit = Color * light;
    vec3 glow = Color * 1.25;
    vec3 col = mix(lit, glow, unlit);
    float dist = length(FragPos - viewPos);
    float fog = clamp((dist - 110.0) / 170.0, 0.0, 1.0);
    FragColor = vec4(mix(col, skyColor, fog), 1.0);
}
)";

    // ============================================================================
    //  O dragao
    // ============================================================================
    class Dragon
    {
    public:
        // center: centro da piramide principal. O dragao orbita em volta dele.
        void init(const V3 &center, float orbitRadius = 25.0f, float altitude = 32.0f)
        {
            center_ = center;
            radius_ = orbitRadius;
            alt_ = altitude;

            prog_ = glCreateProgram();
            unsigned int vs = compile(GL_VERTEX_SHADER, kVS);
            unsigned int fs = compile(GL_FRAGMENT_SHADER, kFS);
            glAttachShader(prog_, vs);
            glAttachShader(prog_, fs);
            glLinkProgram(prog_);
            glDeleteShader(vs);
            glDeleteShader(fs);

            locView_ = glGetUniformLocation(prog_, "view");
            locProj_ = glGetUniformLocation(prog_, "projection");
            locLight_ = glGetUniformLocation(prog_, "lightDir");
            locViewPos_ = glGetUniformLocation(prog_, "viewPos");
            locSky_ = glGetUniformLocation(prog_, "skyColor");
            locUnlit_ = glGetUniformLocation(prog_, "unlit");

            setupBuffer(vaoBody_, vboBody_);
            setupBuffer(vaoFire_, vboFire_);

            parts_.reserve(kMaxParticles);
            body_.reserve(900 * 36 * 9);
            fire_.reserve((kMaxParticles + 16) * 36 * 9);
        }

        void render(float dt, const glm::mat4 &view, const glm::mat4 &proj, const V3 &camPos,
                    const V3 &lightDir, const V3 &sky)
        {
            update(dt);

            glUseProgram(prog_);
            glUniformMatrix4fv(locView_, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(locProj_, 1, GL_FALSE, glm::value_ptr(proj));
            glUniform3fv(locLight_, 1, glm::value_ptr(lightDir));
            glUniform3fv(locViewPos_, 1, glm::value_ptr(camPos));
            glUniform3fv(locSky_, 1, glm::value_ptr(sky));

            upload(vaoBody_, vboBody_, body_);
            glUniform1f(locUnlit_, 0.0f);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(body_.size() / 9));

            upload(vaoFire_, vboFire_, fire_);
            glUniform1f(locUnlit_, 1.0f);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(fire_.size() / 9));

            glBindVertexArray(0);
        }

        void cleanup()
        {
            glDeleteVertexArrays(1, &vaoBody_);
            glDeleteBuffers(1, &vboBody_);
            glDeleteVertexArrays(1, &vaoFire_);
            glDeleteBuffers(1, &vboFire_);
            glDeleteProgram(prog_);
        }

        V3 position() const { return pos_; }

    private:
        // ---- estado ----
        V3 center_ = V3(0.0f);
        float radius_ = 25.0f, alt_ = 32.0f;
        float time_ = 0.0f;
        V3 pos_ = V3(0.0f);
        V3 vel_ = V3(0.0f);
        V3 mouthPos_ = V3(0.0f);
        V3 mouthDir_ = V3(0.0f, 0.0f, 1.0f);
        float spawnAcc_ = 0.0f;
        unsigned int rng_ = 2463534242u;

        struct Particle
        {
            V3 pos, vel;
            float life, maxLife, spin;
        };
        static const size_t kMaxParticles = 900;
        std::vector<Particle> parts_;

        std::vector<float> body_, fire_;
        unsigned int prog_ = 0, vaoBody_ = 0, vboBody_ = 0, vaoFire_ = 0, vboFire_ = 0;
        int locView_ = 0, locProj_ = 0, locLight_ = 0, locViewPos_ = 0, locSky_ = 0, locUnlit_ = 0;

        // ---- utilitarios ----
        float rand01()
        {
            rng_ ^= rng_ << 13;
            rng_ ^= rng_ >> 17;
            rng_ ^= rng_ << 5;
            return static_cast<float>(rng_ & 0xFFFFFFu) / 16777216.0f;
        }

        static unsigned int compile(unsigned int type, const char *src)
        {
            unsigned int s = glCreateShader(type);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            int ok;
            glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
            if (!ok)
            {
                char log[512];
                glGetShaderInfoLog(s, 512, nullptr, log);
                std::cerr << "Erro ao compilar shader do dragao:\n"
                          << log << "\n";
            }
            return s;
        }

        static void setupBuffer(unsigned int &vao, unsigned int &vbo)
        {
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            const int stride = 9 * sizeof(float);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void *)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glBindVertexArray(0);
        }

        static void upload(unsigned int vao, unsigned int vbo, const std::vector<float> &data)
        {
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, static_cast<long>(data.size() * sizeof(float)),
                         data.empty() ? nullptr : data.data(), GL_DYNAMIC_DRAW);
        }

        static void pushBox(const glm::mat4 &model, const V3 &color, std::vector<float> &o)
        {
            glm::mat3 nm = glm::transpose(glm::inverse(glm::mat3(model)));
            for (int i = 0; i < 36; ++i)
            {
                const float *v = &kCube[i * 6];
                V3 p = V3(model * glm::vec4(v[0], v[1], v[2], 1.0f));
                V3 n = glm::normalize(nm * V3(v[3], v[4], v[5]));
                o.push_back(p.x);
                o.push_back(p.y);
                o.push_back(p.z);
                o.push_back(n.x);
                o.push_back(n.y);
                o.push_back(n.z);
                o.push_back(color.r);
                o.push_back(color.g);
                o.push_back(color.b);
            }
        }

        // caixa com luz (corpo) e caixa sem luz (olhos, fogo)
        void bx(const glm::mat4 &M, const V3 &c, const V3 &sz, const V3 &col)
        {
            glm::mat4 m = glm::translate(M, c);
            m = glm::scale(m, sz);
            pushBox(m, col, body_);
        }
        void bxE(const glm::mat4 &M, const V3 &c, const V3 &sz, const V3 &col)
        {
            glm::mat4 m = glm::translate(M, c);
            m = glm::scale(m, sz);
            pushBox(m, col, fire_);
        }

        // ========================================================================
        //  Animacao + voo
        // ========================================================================
        void update(float dt)
        {
            dt = std::min(dt, 0.05f);
            time_ += dt;

            const float omega = 0.34f; // rad/s: ~18 s por volta
            const float th = 1.2f + omega * time_;
            const float w = time_ * 3.3f; // fase das asas

            // Posicao: circulo em volta da piramide, com ondulacao de altitude
            float bob = 2.6f * std::sin(3.0f * th + 0.5f);
            pos_ = V3(center_.x + radius_ * std::cos(th),
                      alt_ + bob + 0.35f * std::sin(w),
                      center_.z + radius_ * std::sin(th));
            V3 tangent = V3(-std::sin(th), 0.0f, std::cos(th));
            vel_ = tangent * (radius_ * omega);

            float psi = std::atan2(tangent.x, tangent.z); // proa ao longo da trajetoria
            float slope = 2.6f * 3.0f * std::cos(3.0f * th + 0.5f) / radius_;
            float pitch = -std::atan(slope) + 0.05f * std::sin(w);
            float bank = 0.38f; // inclina para dentro da curva

            const float DS = 0.95f;
            glm::mat4 M = Tr(pos_) * Ry(psi) * Rz(bank) * Rx(pitch) * Sc(V3(DS));

            // Rajadas de fogo: 4 s ligado a cada 7 s
            float ph = std::fmod(time_, 7.0f);
            float breath = smooth(0.3f, 0.9f, ph) * (1.0f - smooth(4.2f, 4.8f, ph));

            body_.clear();
            fire_.clear();
            buildDragon(M, time_, w, breath);
            updateParticles(dt, breath);
        }

        // ========================================================================
        //  Particulas de fogo
        // ========================================================================
        static V3 fireColor(float u)
        {
            const V3 c0(0.22f, 0.18f, 0.16f); // fumaca
            const V3 c1(0.85f, 0.20f, 0.05f); // brasa
            const V3 c2(1.00f, 0.62f, 0.12f); // laranja
            const V3 c3(1.00f, 0.97f, 0.70f); // nucleo quente
            if (u < 0.30f)
                return glm::mix(c0, c1, u / 0.30f);
            if (u < 0.65f)
                return glm::mix(c1, c2, (u - 0.30f) / 0.35f);
            return glm::mix(c2, c3, (u - 0.65f) / 0.35f);
        }

        void updateParticles(float dt, float breath)
        {
            // emissao na boca
            if (breath > 0.2f)
            {
                spawnAcc_ += 300.0f * breath * dt;
                while (spawnAcc_ >= 1.0f)
                {
                    spawnAcc_ -= 1.0f;
                    if (parts_.size() >= kMaxParticles)
                        break;
                    Particle p;
                    float speed = 20.0f + 7.0f * rand01();
                    V3 spread = V3(rand01() - 0.5f, rand01() - 0.5f, rand01() - 0.5f) * 3.6f;
                    p.pos = mouthPos_ + mouthDir_ * (rand01() * 0.8f);
                    p.vel = mouthDir_ * speed + vel_ * 0.6f + spread;
                    p.maxLife = 0.9f + 0.7f * rand01();
                    p.life = p.maxLife;
                    p.spin = rand01() * 6.28f;
                    parts_.push_back(p);
                }
            }
            else
            {
                spawnAcc_ = 0.0f;
            }

            // simulacao
            for (size_t i = 0; i < parts_.size();)
            {
                Particle &p = parts_[i];
                p.life -= dt;
                if (p.life <= 0.0f)
                {
                    parts_[i] = parts_.back();
                    parts_.pop_back();
                    continue;
                }
                p.vel *= (1.0f - 0.35f * dt);
                p.vel.y += 2.2f * dt; // o calor sobe
                p.pos += p.vel * dt;
                ++i;
            }

            // malha do fogo (sem luz, cor por idade)
            for (const Particle &p : parts_)
            {
                float u = p.life / p.maxLife;
                float size = (0.35f + (1.0f - u) * 1.5f) * (0.3f + 0.7f * std::min(1.0f, u * 4.0f));
                glm::mat4 m = Tr(p.pos) * Ry(p.spin + (1.0f - u) * 3.0f) * Rx(p.spin * 0.7f) * Sc(V3(size));
                pushBox(m, fireColor(u), fire_);
            }
        }

        // ========================================================================
        //  Modelagem do dragao (tudo em coordenadas locais: +Z = frente, +Y = cima)
        // ========================================================================
        void buildDragon(const glm::mat4 &M, float t, float w, float breath)
        {
            // ---------------- tronco ----------------
            bx(M, V3(0, 0, 1.3f), V3(1.7f, 1.5f, 2.3f), kScale);
            bx(M, V3(0, 0, -0.9f), V3(1.5f, 1.3f, 2.2f), shade(kScale, 1.08f));
            bx(M, V3(0, 0, -2.5f), V3(1.3f, 1.15f, 1.6f), kScale);
            bx(M, V3(0, 0.85f, 0.8f), V3(1.2f, 0.4f, 1.2f), kDark); // musculos dos ombros
            for (int i = 0; i < 10; ++i)                            // placas da barriga
            {
                float z = 2.3f - i * 0.52f;
                bx(M, V3(0, -0.7f, z), V3(1.35f, 0.14f, 0.46f), shade(kBelly, 0.9f + 0.1f * (i & 1)));
            }
            for (int i = 0; i < 9; ++i) // espinhos dorsais atras do cavaleiro
            {
                float z = 0.0f - i * 0.42f;
                float h = 0.58f - 0.03f * i;
                glm::mat4 K = M * Tr(V3(0, 0.66f, z)) * Rx(-0.35f);
                bx(K, V3(0, h * 0.5f, 0), V3(0.1f, h, 0.3f), kGold);
            }

            // ---------------- pescoco ----------------
            glm::mat4 N = M * Tr(V3(0, 0.3f, 2.3f)) * Rx(-0.55f);
            const int nSeg = 5;
            for (int i = 0; i < nSeg; ++i)
            {
                float f = i / static_cast<float>(nSeg - 1);
                float wd = 1.1f - 0.45f * f, hg = 1.05f - 0.4f * f, len = 0.95f;
                float rx = (i == 0) ? 0.0f : 0.20f - 0.05f * breath;
                float ry = 0.20f * std::sin(t * 1.7f - i * 0.8f);
                N = N * Ry(ry) * Rx(rx);
                bx(N, V3(0, 0, len * 0.5f), V3(wd, hg, len * 1.08f), (i & 1) ? kScale : shade(kScale, 1.1f));
                bx(N, V3(0, -hg * 0.5f - 0.03f, len * 0.5f), V3(wd * 0.7f, 0.1f, len), kBelly);
                bx(N, V3(0, hg * 0.5f + 0.14f, len * 0.5f), V3(0.1f, 0.34f, 0.3f), kGold);
                N = N * Tr(V3(0, 0, len));
            }

            // ---------------- cabeca ----------------
            const glm::mat4 H = N;
            bx(H, V3(0, 0.0f, 0.55f), V3(0.95f, 0.8f, 1.2f), kScale);
            bx(H, V3(0, -0.1f, 1.45f), V3(0.65f, 0.5f, 0.9f), shade(kScale, 1.1f));
            bx(H, V3(0, -0.1f, 1.95f), V3(0.5f, 0.42f, 0.2f), kScale);
            bx(H, V3(0.0f, 0.43f, 1.2f), V3(0.18f, 0.12f, 0.8f), kGold); // crista do focinho
            for (int s = -1; s <= 1; s += 2)
            {
                bx(H, V3(s * 0.38f, 0.45f, 0.75f), V3(0.22f, 0.18f, 0.7f), kGold);   // sobrancelhas
                bx(H, V3(s * 0.52f, -0.10f, 0.20f), V3(0.12f, 0.12f, 0.55f), kGold); // espinhos da bochecha
                bx(H, V3(s * 0.15f, 0.10f, 2.05f), V3(0.07f, 0.07f, 0.06f), kDark);  // narinas
                bxE(H, V3(s * 0.49f, 0.18f, 0.60f), V3(0.08f, 0.14f, 0.22f), kEye);  // olhos
                // chifres: tres segmentos que sobem curvando para tras
                glm::mat4 Hn = H * Tr(V3(s * 0.38f, 0.40f, 0.1f)) * Ry(kPi + s * 0.25f) * Rx(-0.35f);
                float hl[3] = {0.75f, 0.65f, 0.5f}, hw[3] = {0.24f, 0.18f, 0.11f};
                for (int k = 0; k < 3; ++k)
                {
                    bx(Hn, V3(0, 0, hl[k] * 0.5f), V3(hw[k], hw[k], hl[k] * 1.05f), kHorn);
                    Hn = Hn * Tr(V3(0, 0, hl[k])) * Rx(-0.2f);
                }
            }
            // mandibula (abre ao cuspir fogo)
            float jaw = 0.10f + 0.55f * breath;
            glm::mat4 J = H * Tr(V3(0, -0.35f, 0.2f)) * Rx(jaw);
            bx(J, V3(0, -0.10f, 0.7f), V3(0.62f, 0.22f, 1.25f), kDark);
            bx(J, V3(0, -0.10f, 1.4f), V3(0.45f, 0.20f, 0.4f), kDark);
            for (int i = 0; i < 4; ++i)
                for (int s = -1; s <= 1; s += 2)
                {
                    bx(H, V3(s * 0.28f, -0.38f, 0.95f + i * 0.28f), V3(0.06f, 0.14f, 0.08f), kClaw); // dentes de cima
                    bx(J, V3(s * 0.24f, 0.05f, 0.50f + i * 0.28f), V3(0.06f, 0.14f, 0.08f), kClaw);  // dentes de baixo
                }
            if (breath > 0.15f) // garganta em brasa
                bxE(H, V3(0, -0.28f, 1.10f), V3(0.42f, 0.10f, 0.95f), V3(1.0f, 0.45f, 0.08f));

            mouthPos_ = V3(H * glm::vec4(0.0f, -0.15f, 2.1f, 1.0f));
            mouthDir_ = glm::normalize(V3(H * glm::vec4(0.0f, -0.02f, 1.0f, 0.0f)));

            // ---------------- cauda ----------------
            glm::mat4 TL = M * Tr(V3(0, -0.05f, -3.1f)) * Ry(kPi);
            const int nT = 11;
            for (int i = 0; i < nT; ++i)
            {
                float f = i / static_cast<float>(nT);
                float wd = 1.0f * (1.0f - f) + 0.14f, hg = 0.95f * (1.0f - f) + 0.14f, len = 0.95f;
                float ry = (0.12f + 0.20f * f) * std::sin(t * 2.6f - i * 0.65f);
                float rx = (i < 3) ? 0.05f : -0.045f;
                TL = TL * Ry(ry) * Rx(rx);
                bx(TL, V3(0, 0, len * 0.5f), V3(wd, hg, len * 1.08f), (i & 1) ? kScale : shade(kScale, 1.1f));
                if (i < 6)
                    bx(TL, V3(0, -hg * 0.5f - 0.02f, len * 0.5f), V3(wd * 0.7f, 0.08f, len), kBelly);
                bx(TL, V3(0, hg * 0.5f + 0.10f, len * 0.5f), V3(0.08f, 0.34f * (1.0f - f) + 0.10f, 0.3f), kGold);
                TL = TL * Tr(V3(0, 0, len));
            }
            // lamina da ponta da cauda
            bx(TL, V3(0, 0, 0.35f), V3(0.9f, 0.08f, 0.7f), kGold);
            bx(TL, V3(0, 0, 0.85f), V3(0.5f, 0.08f, 0.5f), kGold);
            bx(TL, V3(0, 0, 0.45f), V3(0.08f, 0.8f, 0.7f), shade(kGold, 0.85f));

            // ---------------- patas (recolhidas durante o voo) ----------------
            for (int s = -1; s <= 1; s += 2)
            {
                glm::mat4 L = M * Tr(V3(s * 0.85f, -0.3f, -2.2f)) * Rx(2.2f + 0.15f * std::sin(w));
                bx(L, V3(0, 0, 0.6f), V3(0.5f, 0.55f, 1.3f), kScale);
                L = L * Tr(V3(0, 0, 1.2f)) * Rx(-1.5f);
                bx(L, V3(0, 0, 0.55f), V3(0.34f, 0.36f, 1.1f), kDark);
                L = L * Tr(V3(0, 0, 1.05f)) * Rx(0.9f);
                for (int c = -1; c <= 1; ++c)
                    bx(L, V3(c * 0.14f, 0, 0.25f), V3(0.1f, 0.1f, 0.5f), kClaw);

                glm::mat4 F = M * Tr(V3(s * 0.85f, -0.45f, 1.7f)) * Rx(1.9f - 0.1f * std::sin(w));
                bx(F, V3(0, 0, 0.5f), V3(0.4f, 0.42f, 1.0f), kScale);
                F = F * Tr(V3(0, 0, 0.95f)) * Rx(-1.2f);
                bx(F, V3(0, 0, 0.45f), V3(0.28f, 0.3f, 0.9f), kDark);
                F = F * Tr(V3(0, 0, 0.85f)) * Rx(0.6f);
                for (int c = -1; c <= 1; ++c)
                    bx(F, V3(c * 0.11f, 0, 0.2f), V3(0.08f, 0.08f, 0.4f), kClaw);
            }

            // ---------------- asas ----------------
            buildWing(M, +1.0f, w);
            buildWing(M, -1.0f, w);

            // ---------------- cavaleiro ----------------
            buildRider(M * Tr(V3(0, 0.8f, 1.5f)), t);
        }

        // ------------------------------------------------------------------------
        //  Asa: ombro, braco, antebraco, 4 dedos em leque e membrana em faixas.
        //  Construida para o lado +X; o lado -X usa escala (-1,1,1).
        // ------------------------------------------------------------------------
        void buildWing(const glm::mat4 &M, float side, float w)
        {
            float flap = 0.15f + 0.62f * std::sin(w);
            float elbow = -0.35f * std::sin(w - 1.0f);
            float tipLag = -0.45f * std::sin(w - 1.8f);

            glm::mat4 W = M * Tr(V3(side * 0.75f, 0.7f, 0.8f)) * Sc(V3(side, 1.0f, 1.0f));
            W = W * Rz(flap) * Ry(0.15f);

            // braco + musculo do ombro
            bx(W, V3(0.3f, 0.0f, 0.0f), V3(1.1f, 0.62f, 0.75f), kDark);
            bx(W, V3(1.2f, 0.0f, 0.0f), V3(2.4f, 0.34f, 0.40f), kScale);
            bx(W, V3(1.2f, 0.20f, 0.0f), V3(2.2f, 0.08f, 0.16f), kGold);
            // membrana atras do braco (ate o corpo)
            bx(W, V3(1.2f, -0.035f, -1.1f), V3(2.4f, 0.04f, 2.2f), shade(kMem, 0.9f));

            // antebraco para frente
            glm::mat4 Wr = W * Tr(V3(2.4f, 0, 0)) * Rz(elbow) * Ry(-0.35f);
            bx(Wr, V3(1.25f, 0, 0), V3(2.5f, 0.26f, 0.30f), kScale);
            bx(Wr, V3(2.0f, 0.15f, 0.0f), V3(0.2f, 0.3f, 0.2f), kGold); // espora do pulso

            // pulso: de onde saem os dedos
            glm::mat4 Wf = Wr * Tr(V3(2.5f, 0, 0)) * Rz(tipLag);
            const float ang[4] = {0.0f, 0.55f, 1.0f, 1.45f};
            const float len[4] = {3.0f, 3.5f, 3.1f, 2.4f};

            for (int i = 0; i < 4; ++i)
            {
                glm::mat4 Fi = Wf * Ry(ang[i]);
                bx(Fi, V3(len[i] * 0.5f, 0, 0), V3(len[i], 0.14f, 0.14f), shade(kScale, 1.0f - 0.05f * i));
                bx(Fi, V3(len[i] + 0.1f, -0.03f, 0), V3(0.3f, 0.1f, 0.1f), kClaw); // garra da ponta
            }

            // membrana entre os dedos: faixas que se alargam com o raio
            for (int g = 0; g < 3; ++g)
            {
                float a0 = ang[g], a1 = ang[g + 1];
                float am = 0.5f * (a0 + a1);
                float da = a1 - a0;
                float L = std::min(len[g], len[g + 1]) * 0.95f;
                glm::mat4 Fm = Wf * Ry(am);
                const int n = 4;
                for (int k = 0; k < n; ++k)
                {
                    float r0 = L * k / n, r1 = L * (k + 1) / n, rm = 0.5f * (r0 + r1);
                    float width = 2.0f * rm * std::tan(da * 0.5f) + 0.08f;
                    glm::vec3 col = shade(kMem, 0.85f + 0.15f * ((g + k) & 1));
                    bx(Fm, V3(rm, -0.02f, 0), V3(r1 - r0 + 0.02f, 0.04f, width), col);
                }
            }
            // membrana atras do antebraco
            bx(Wr, V3(1.25f, -0.06f, -1.1f), V3(2.5f, 0.04f, 2.0f), shade(kMem, 0.8f));
        }

        // ------------------------------------------------------------------------
        //  Cavaleiro: sela, pernas abertas, tronco inclinado, bracos nas rédeas,
        //  cabelo prateado com trança e capa carmesim ao vento.
        // ------------------------------------------------------------------------
        void buildRider(const glm::mat4 &Rd, float t)
        {
            const V3 tunic(0.15f, 0.30f, 0.52f), cape(0.72f, 0.12f, 0.14f), hair(0.92f, 0.90f, 0.82f);
            const V3 skin(0.82f, 0.62f, 0.46f), pants(0.25f, 0.20f, 0.18f), boots(0.18f, 0.12f, 0.08f), leather(0.35f, 0.20f, 0.10f);
            const float rs = 1.6f;

            // sela
            bx(Rd, V3(0, 0.05f, 0), V3(1.2f, 0.18f, 1.0f), leather);
            bx(Rd, V3(0, 0.28f, 0.55f), V3(0.28f, 0.30f, 0.20f), leather);
            bx(Rd, V3(0, 0.22f, -0.55f), V3(0.9f, 0.30f, 0.16f), leather);

            glm::mat4 Rr = Rd * Tr(V3(0, 0.14f, 0)) * Sc(V3(rs));

            for (int s = -1; s <= 1; s += 2)
            {
                bx(Rr, V3(s * 0.35f, 0.10f, 0.10f), V3(0.45f, 0.16f, 0.28f), pants);  // coxa
                bx(Rr, V3(s * 0.58f, -0.20f, 0.12f), V3(0.14f, 0.50f, 0.15f), pants); // canela
                bx(Rr, V3(s * 0.58f, -0.50f, 0.18f), V3(0.16f, 0.14f, 0.28f), boots); // bota
            }

            glm::mat4 Rt = Rr * Tr(V3(0, 0.1f, 0)) * Rx(0.22f); // tronco inclinado para frente
            bx(Rt, V3(0, 0.30f, 0), V3(0.40f, 0.60f, 0.24f), tunic);
            bx(Rt, V3(0, 0.05f, 0), V3(0.42f, 0.08f, 0.26f), leather);
            bx(Rt, V3(0, 0.30f, -0.15f), V3(0.44f, 0.62f, 0.05f), shade(cape, 0.9f));

            // cabeca, cabelo e trança ao vento
            bx(Rt, V3(0, 0.78f, 0.04f), V3(0.20f, 0.22f, 0.20f), skin);
            bx(Rt, V3(0, 0.85f, -0.04f), V3(0.23f, 0.14f, 0.24f), hair);
            glm::mat4 Br = Rt * Tr(V3(0, 0.80f, -0.16f));
            for (int i = 0; i < 5; ++i)
            {
                Br = Br * Ry(0.22f * std::sin(t * 7.0f - i * 0.9f)) * Tr(V3(0, 0, -0.10f));
                bx(Br, V3(0, 0, 0), V3(0.07f, 0.07f, 0.11f), hair);
            }

            // bracos para frente segurando os espinhos do pescoco
            for (int s = -1; s <= 1; s += 2)
            {
                glm::mat4 Ra = Rt * Tr(V3(s * 0.26f, 0.50f, 0.05f)) * Rx(0.9f);
                bx(Ra, V3(0, 0, 0.25f), V3(0.09f, 0.09f, 0.5f), tunic);
                bx(Ra, V3(0, 0, 0.52f), V3(0.10f, 0.10f, 0.10f), skin);
            }

            // capa ondulando atras
            glm::mat4 Rc = Rt * Tr(V3(0, 0.55f, -0.18f));
            for (int i = 0; i < 6; ++i)
            {
                Rc = Rc * Ry(0.20f * std::sin(t * 6.0f - i * 0.8f)) * Rx(0.06f) * Tr(V3(0, 0, -0.22f));
                bx(Rc, V3(0, 0, 0), V3(0.40f + 0.06f * i, 0.05f, 0.26f), (i & 1) ? cape : shade(cape, 0.75f));
            }
        }
    };

} // namespace dragao
