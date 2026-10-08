// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  1-АПТА:    терезе, фон анимациясы, пробел -> ақ/бастапқы фон, FPS
//  СЕМИНАР 4: uniform vec2 (uOffset), dt, шеңбер бойымен қозғалыс,
//             цикл "жаңарту (update)" және "сызу (draw)" болып бөлінген
//  5-АПТА:    индекс буфері (EBO), glDrawElements
//             - базалық:      түрлі-түсті төртбұрыш, 4 вершина + 6 индекс
//             - 2-тапсырма:   бесбұрыш (5 вершина, 9 индекс)
//             - 3-тапсырма:   екі объект (бір VAO, екі сызу)
//
//  Басқару:
//      1       — шеңбер бойымен қозғалатын төртбұрыш (базалық бөлім)
//      2       — екі төртбұрыш (3-тапсырма)
//      3       — бесбұрыш (2-тапсырма)
//      TAB     — wireframe режимін қосу/өшіру (қосымша)
//      W / S   — айналу жылдамдығын арттыру / азайту
//      ПРОБЕЛ  — фонды ақ/бастапқы арасында ауыстыру
//      ESC     — шығу
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstddef>
#include <iostream>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
const int   WIDTH  = 1280;
const int   HEIGHT = 720;
const float PI     = 3.14159265f;

// Пробел басылған сайын true/false болып ауысады (1-апта)
bool whiteBackground = false;
bool spaceWasPressed = false;

// TAB: wireframe режимі (қосымша тапсырма)
bool wireframe       = false;
bool tabWasPressed   = false;

// Қазіргі сахна: 1 — орбитадағы төртбұрыш, 2 — екі төртбұрыш, 3 — бесбұрыш
int sceneMode = 1;

// Орбита параметрлері (4-семинар)
float angle       = 0.0f;   // шеңбер бойындағы бұрыш (радиан)
float orbitRadius = 0.5f;   // 0.5 + 0.3 < 1.0 — фигура NDC-ден шықпайды
float orbitSpeed  = 1.5f;   // радиан/секунд

const float MIN_SPEED = 0.1f;
const float MAX_SPEED = 8.0f;

// ---------------------------------------------------------------------
//  Шейдерлер
//  Енді вершина өз түсін өзі алып жүреді (location 1), сондықтан
//  uColor uniform-ы керек емес. uOffset мен uScale — 4-аптадан.
// ---------------------------------------------------------------------
const char* vertexShaderSrc = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform vec2  uOffset;
uniform float uScale;

out vec3 vColor;

void main() {
    vec3 pos = aPos * uScale;
    gl_Position = vec4(pos.xy + uOffset, pos.z, 1.0);
    vColor = aColor;
}
)glsl";

const char* fragmentShaderSrc = R"glsl(
#version 330 core
in vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
    // ҚЫЗЫЛ ТЕСТ: ештеңе көрінбесе, жоғарыдағы жолды өшіріп,
    // төмендегіні қос. Экран қызыл болмаса — мәселе буферде/индексте.
    // FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)glsl";

// ---------------------------------------------------------------------
//  Терезе өлшемі өзгергенде шақырылады
// ---------------------------------------------------------------------
void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// ---------------------------------------------------------------------
//  Пернетақтаны тексеру. Әр кадрда шақырылады.
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Пробел: бір рет басқанда фон ауысады
    bool spaceIsPressed = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
    if (spaceIsPressed && !spaceWasPressed) {
        whiteBackground = !whiteBackground;
    }
    spaceWasPressed = spaceIsPressed;

    // TAB: wireframe қосу/өшіру
    bool tabIsPressed = (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS);
    if (tabIsPressed && !tabWasPressed) {
        wireframe = !wireframe;
    }
    tabWasPressed = tabIsPressed;

    // 1 / 2 / 3: сахна таңдау
    int newMode = sceneMode;
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) newMode = 1;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) newMode = 2;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) newMode = 3;
    if (newMode != sceneMode) {
        sceneMode = newMode;
        std::cout << "Сахна: " << sceneMode << "\n";
    }

    // W/S: орбита жылдамдығы (dt арқылы тегіс өзгереді)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) orbitSpeed += 2.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) orbitSpeed -= 2.0f * dt;
    if (orbitSpeed < MIN_SPEED) orbitSpeed = MIN_SPEED;
    if (orbitSpeed > MAX_SPEED) orbitSpeed = MAX_SPEED;
}

// ---------------------------------------------------------------------
//  Бір шейдерді компиляциялау (қатені консольге шығарады)
// ---------------------------------------------------------------------
GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << "\n";
    }
    return shader;
}

// ---------------------------------------------------------------------
//  Шейдер бағдарламасын (vertex + fragment) құру
// ---------------------------------------------------------------------
GLuint createShaderProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Program link error:\n" << log << "\n";
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// ---------------------------------------------------------------------
//  VAO + VBO + EBO жасау.
//  Вершина форматы: 6 float = xyz (орны) + rgb (түсі), stride = 6 * float.
//
//  Реті маңызды (үш тұзақ осы жерде):
//    1) VAO ӘУЕЛІ байланады (glBindVertexArray)
//    2) EBO (GL_ELEMENT_ARRAY_BUFFER) VAO байланған кезде жасалады —
//       сонда VAO индекс буферін "есіне сақтайды"
//    3) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) ЖАЗЫЛМАЙДЫ — VAO әлі
//       байланған кезде жазсақ, EBO VAO-дан өшіп қалады
// ---------------------------------------------------------------------
void createIndexedMesh(const float* vertices, std::size_t vertexBytes,
                       const unsigned int* indices, std::size_t indexBytes,
                       GLuint& vao, GLuint& vbo, GLuint& ebo) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);                                   // 1) VAO байлау

    glBindBuffer(GL_ARRAY_BUFFER, vbo);                       // вершина деректері
    glBufferData(GL_ARRAY_BUFFER, vertexBytes, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);               // 2) EBO — VAO байулы кезде
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBytes, indices, GL_STATIC_DRAW);

    // location 0: орны (xyz)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // location 1: түсі (rgb), 3 float кейін басталады
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    // 3) мұнда glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) ЖОҚ — әдейі
}

// =====================================================================
//  MAIN
// =====================================================================
int main() {

    // -----------------------------------------------------------------
    //  1. GLFW-ны іске қосу
    // -----------------------------------------------------------------
    if (!glfwInit()) {
        std::cerr << "GLFW іске қосылмады\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    // -----------------------------------------------------------------
    //  2. Терезе жасау
    // -----------------------------------------------------------------
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
                                          "Компьютерлік графика",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "Терезе жасалмады. Видеокарта OpenGL 3.3-ті "
                     "қолдамауы мүмкін.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(0);   // VSync өшірулі — FPS шынайы көрінеді

    // -----------------------------------------------------------------
    //  3. GLAD: OpenGL функцияларын жүктеу
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";
    std::cout << "1 / 2 / 3 — сахна, TAB — wireframe, W/S — жылдамдық\n";

    // -----------------------------------------------------------------
    //  4. Шейдер бағдарламасы + uniform орындарын кэштеу
    // -----------------------------------------------------------------
    GLuint shaderProgram = createShaderProgram();
    GLint  locOffset = glGetUniformLocation(shaderProgram, "uOffset");
    GLint  locScale  = glGetUniformLocation(shaderProgram, "uScale");

    // -----------------------------------------------------------------
    //  5. БАЗАЛЫҚ БӨЛІМ: төртбұрыш — 4 вершина (6 емес!) + 6 индекс
    // -----------------------------------------------------------------
    // 1) Вершина деректері: орны (xyz) + түсі (rgb)
    float vertices[] = {
         0.3f,  0.3f, 0.0f,   1.0f, 0.0f, 0.0f,   // 0 — оң жоғарғы
         0.3f, -0.3f, 0.0f,   0.0f, 1.0f, 0.0f,   // 1 — оң төменгі
        -0.3f, -0.3f, 0.0f,   0.0f, 0.0f, 1.0f,   // 2 — сол төменгі
        -0.3f,  0.3f, 0.0f,   1.0f, 1.0f, 0.0f    // 3 — сол жоғарғы
    };

    // 2) Индекс массиві: 2 үшбұрыш = 6 индекс (вершина нөмірі 0..3 ғана)
    unsigned int indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    // 3) VAO + VBO + EBO (EBO VAO байулы кезде жасалады — функция ішінде)
    GLuint vao, vbo, ebo;
    createIndexedMesh(vertices, sizeof(vertices),
                      indices,  sizeof(indices),
                      vao, vbo, ebo);

    // -----------------------------------------------------------------
    //  6. 2-ТАПСЫРМА: бесбұрыш — 5 вершина, 3 үшбұрыш, 9 индекс
    //     Бұрыштар: -90°, -18°, 54°, 126°, 198° (әрқайсысы +72°)
    //     0-вершина — барлық үшбұрышта қайталанатын "орталық" нүкте
    // -----------------------------------------------------------------
    const float pentColors[5][3] = {
        {1.0f, 0.0f, 0.0f},   // 0 — қызыл
        {1.0f, 0.5f, 0.0f},   // 1 — сарғыш
        {0.0f, 1.0f, 0.0f},   // 2 — жасыл
        {0.0f, 0.5f, 1.0f},   // 3 — көк
        {0.8f, 0.0f, 1.0f}    // 4 — күлгін
    };

    float pentVertices[5 * 6];
    for (int i = 0; i < 5; ++i) {
        float a = (-90.0f + 72.0f * i) * PI / 180.0f;
        pentVertices[i * 6 + 0] = std::cos(a) * 0.3f;   // x
        pentVertices[i * 6 + 1] = std::sin(a) * 0.3f;   // y
        pentVertices[i * 6 + 2] = 0.0f;                 // z
        pentVertices[i * 6 + 3] = pentColors[i][0];     // r
        pentVertices[i * 6 + 4] = pentColors[i][1];     // g
        pentVertices[i * 6 + 5] = pentColors[i][2];     // b
    }

    unsigned int pentIndices[] = {
        0, 1, 2,
        0, 2, 3,
        0, 3, 4
    };

    GLuint pentVAO, pentVBO, pentEBO;
    createIndexedMesh(pentVertices, sizeof(pentVertices),
                      pentIndices,  sizeof(pentIndices),
                      pentVAO, pentVBO, pentEBO);

    // -----------------------------------------------------------------
    //  7. dt және FPS үшін уақыт айнымалылары
    // -----------------------------------------------------------------
    double lastFrame   = glfwGetTime();
    int    frameCount  = 0;
    double lastFpsTime = lastFrame;

    // -----------------------------------------------------------------
    //  8. Негізгі цикл: ЖАҢАРТУ (update) + СЫЗУ (draw)
    // -----------------------------------------------------------------
    while (!glfwWindowShouldClose(window)) {

        // --- dt есептеу ---
        double currentFrame = glfwGetTime();
        float  dt = (float)(currentFrame - lastFrame);
        lastFrame = currentFrame;

        processInput(window, dt);

        // ===============================================================
        //  ЖАҢАРТУ (update) — тек сандарды есептейміз
        // ===============================================================
        angle += orbitSpeed * dt;
        if (angle > 2.0f * PI) angle -= 2.0f * PI;

        float offsetX = std::cos(angle) * orbitRadius;
        float offsetY = std::sin(angle) * orbitRadius;

        // Пульсация тек 1-сахнада (2-сахнада екі төртбұрыш тимеуі керек)
        float scale = 1.0f + 0.3f * std::sin((float)currentFrame * 4.0f);

        // ===============================================================
        //  СЫЗУ (draw)
        // ===============================================================
        if (whiteBackground) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            float t = (float)currentFrame;
            float r = (std::sin(t * 3.0f) + 1.0f) * 0.5f * 0.3f;
            float g = (std::sin(t * 2.0f) + 1.0f) * 0.5f * 0.3f;
            glClearColor(r, g, 0.35f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT);

        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        glUseProgram(shaderProgram);

        if (sceneMode == 1) {
            // --- Базалық: орбитадағы төртбұрыш ---
            glBindVertexArray(vao);
            glUniform2f(locOffset, offsetX, offsetY);
            glUniform1f(locScale, scale);
            // 2-параметр — ИНДЕКС саны (6), вершина саны (4) емес!
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

        } else if (sceneMode == 2) {
            // --- 3-тапсырма: бір VAO, екі uniform + екі сызу ---
            glBindVertexArray(vao);
            glUniform1f(locScale, 1.0f);

            glUniform2f(locOffset, -0.4f, 0.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

            glUniform2f(locOffset, 0.4f, 0.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

        } else {
            // --- 2-тапсырма: бесбұрыш (9 индекс) ---
            glBindVertexArray(pentVAO);
            glUniform1f(locScale, 1.0f);
            glUniform2f(locOffset, 0.0f, 0.0f);
            glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, (void*)0);
        }

        // --- FPS есептеу ---
        frameCount++;
        if (currentFrame - lastFpsTime >= 1.0) {
            std::cout << "FPS: " << frameCount
                      << "  | сахна: " << sceneMode
                      << "  | orbitSpeed: " << orbitSpeed << "\n";
            frameCount = 0;
            lastFpsTime = currentFrame;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // -----------------------------------------------------------------
    //  9. Тазалау
    // -----------------------------------------------------------------
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);

    glDeleteVertexArrays(1, &pentVAO);
    glDeleteBuffers(1, &pentVBO);
    glDeleteBuffers(1, &pentEBO);

    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}


// =====================================================================
//  5-АПТА — 1-ТАПСЫРМА (индекстерді бұзу): ӨЗІҢ орында
// =====================================================================
//  indices массивін { 0, 1, 2 } деп өзгерт, ал 1-сахнадағы
//  glDrawElements-тің екінші параметрін 6 емес, 3 ет.
//  Бір ғана үшбұрыш көрінуі керек. Тексергеннен кейін ҚАЙТАР:
//      indices = { 0, 1, 3,  1, 2, 3 },  параметр = 6.
//
//  Қорғауда сұралады: қай үшбұрыш жоғалды? Неге дәл сол? Индекс
//  массивін өзгерткенде вершина деректері өзгерді ме?
// =====================================================================