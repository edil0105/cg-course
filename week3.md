// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  1-АПТА: терезе, фон анимациясы, пробел->ақ/бастапқы ауыстыру, FPS
//  СЕМИНАР 4: шеңбер бойымен қозғалатын үшбұрыш
//      - шейдерге uniform vec2 (orxoду) қосу
//      - uniform орнын кэштеу (локацияны циклден тыс бір рет алу)
//      - dt есептеу
//      - циклді "жаңарту" (update) және "сызу" (draw) деп бөлу
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
const int WIDTH  = 1280;
const int HEIGHT = 720;

// Пробел басылған сайын true/false болып ауысады (1-апта, 3-тапсырма)
bool whiteBackground = false;
bool spaceWasPressed = false;

// -----------------------------------------------------------------
//  Семинар 4: қозғалыс параметрлері
// -----------------------------------------------------------------
float angle       = 0.0f;   // шеңбер бойындағы ағымдағы бұрыш (радиан)
float orbitRadius = 0.5f;   // шеңбердің радиусы (NDC бойынша)
float orbitSpeed  = 1.5f;   // бұрыштық жылдамдық (радиан/секунд)

const float MIN_SPEED = 0.1f;
const float MAX_SPEED = 8.0f;

// ---------------------------------------------------------------------
//  Шейдерлер
//  uOffset — үшбұрыштың орталығын шеңбер бойымен жылжытады
//  uScale  — пульсация (кішірейіп-үлкейіп тұру)
// ---------------------------------------------------------------------
const char* vertexShaderSrc = R"glsl(
#version 330 core
layout (location = 0) in vec2 aPos;

uniform vec2  uOffset;
uniform float uScale;

void main() {
    vec2 pos = aPos * uScale + uOffset;
    gl_Position = vec4(pos, 0.0, 1.0);
}
)glsl";

const char* fragmentShaderSrc = R"glsl(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main() {
    FragColor = vec4(uColor, 1.0);
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
//  W/S — шеңбер бойындағы жылдамдықты басқарады (dt арқылы, тегіс)
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Пробел: бір рет басқанда ғана фон ауысады
    bool spaceIsPressed = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
    if (spaceIsPressed && !spaceWasPressed) {
        whiteBackground = !whiteBackground;
    }
    spaceWasPressed = spaceIsPressed;

    // W/S: жылдамдықты dt-мен тегіс өзгерту (мұнда да dt керек!)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        orbitSpeed += 2.0f * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        orbitSpeed -= 2.0f * dt;
    }
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

    // -----------------------------------------------------------------
    //  4. Шейдер бағдарламасы
    // -----------------------------------------------------------------
    GLuint shaderProgram = createShaderProgram();

    // Uniform орындарын ЦИКЛДЕН ТЫС, БІР РЕТ алып, кэштеп қоямыз.
    // Әр кадр сайын glGetUniformLocation шақырудың қажеті жоқ.
    GLint colorLoc  = glGetUniformLocation(shaderProgram, "uColor");
    GLint offsetLoc = glGetUniformLocation(shaderProgram, "uOffset");
    GLint scaleLoc  = glGetUniformLocation(shaderProgram, "uScale");

    // -----------------------------------------------------------------
    //  5. Үшбұрыштың локал координаттары (орталығы (0,0))
    // -----------------------------------------------------------------
    float triVerts[] = {
         0.00f,  0.07f,
        -0.06f, -0.05f,
         0.06f, -0.05f,
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVerts), triVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // -----------------------------------------------------------------
    //  6. dt және FPS үшін уақыт айнымалылары
    // -----------------------------------------------------------------
    double lastFrame  = glfwGetTime();
    int    frameCount = 0;
    double lastFpsTime = lastFrame;

    // -----------------------------------------------------------------
    //  7. Негізгі цикл: ЖАҢАРТУ (update) + СЫЗУ (draw) деп бөлінген
    // -----------------------------------------------------------------
    while (!glfwWindowShouldClose(window)) {

        // --- dt есептеу (3 жол) ---
        double currentFrame = glfwGetTime();
        float  dt = (float)(currentFrame - lastFrame);
        lastFrame = currentFrame;

        processInput(window, dt);

        // ===============================================================
        //  ЖАҢАРТУ (update) — тек сандарды есептейміз, әлі сызбаймыз
        // ===============================================================
        angle += orbitSpeed * dt;
        if (angle > 6.2831853f) angle -= 6.2831853f;   // 2*PI-ден асса, қайта бастау

        float offsetX = std::cos(angle) * orbitRadius;
        float offsetY = std::sin(angle) * orbitRadius;

        // Пульсация: уақытқа байланысты кішірейіп-үлкейіп тұрады
        float scale = 1.0f + 0.3f * std::sin((float)currentFrame * 4.0f);

        // ===============================================================
        //  СЫЗУ (draw) — тек экранға шығарамыз
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

        glUseProgram(shaderProgram);
        glUniform3f(colorLoc, 0.95f, 0.55f, 0.15f);
        glUniform2f(offsetLoc, offsetX, offsetY);
        glUniform1f(scaleLoc, scale);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // --- FPS есептеу ---
        frameCount++;
        if (currentFrame - lastFpsTime >= 1.0) {
            std::cout << "FPS: " << frameCount
                      << "  | orbitSpeed: " << orbitSpeed << "\n";
            frameCount = 0;
            lastFpsTime = currentFrame;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // -----------------------------------------------------------------
    //  8. Тазалау
    // -----------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}


// =====================================================================
//  СЕМИНАР 4 — ТАПСЫРМАЛАР (өзің орында, бұлар код ішінде ӘДЕЙІ
//  жазылмаған, себебі оларды өзің тексеруің керек)
// =====================================================================
//  1. dt-ны уақытша алып тастап (angle += orbitSpeed; деп жазып көр),
//     нәтижені көршіңнің компьютеріндегімен салыстыр.
//     Әр компьютерде жылдамдық бірдей бола ма, жоқ па — соны байқа.
//
//  2. uScale-ды өзің өзгертіп, пульсацияның жиілігі мен амплитудасын
//     (4.0f және 0.3f сандарын) өзгертіп көр.
//
//  3. W/S батырмалары арқылы жылдамдықты басқару дайын — өзің сынап
//     көр, MIN_SPEED/MAX_SPEED шектерін өзгертіп баптап көр.
// =====================================================================
