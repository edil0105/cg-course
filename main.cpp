// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//
//  1-АПТА: терезе, фон анимациясы, пробел->ақ/бастапқы ауыстыру, FPS
//  2-АПТА: тор (grid) сызу, әр ұяшықты диагональмен бөліп,
//          жоғарғы үшбұрышты бояу
// =====================================================================

#include <glad/gl.h>      // МІНДЕТТІ: glad әрқашан GLFW-дан БҰРЫН
#include <GLFW/glfw3.h>

#include <vector>
#include <cmath>
#include <iostream>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
const int WIDTH  = 1280;
const int HEIGHT = 720;

const int COLS = 4;   // тор бағандар саны
const int ROWS = 6;   // тор жолдар саны

// Пробел басылған сайын true/false болып ауысады (1-апта, 3-тапсырма)
bool whiteBackground = false;

// Пробелдің алдыңғы кадрдағы күйі (басып тұру мен бір рет басуды ажырату үшін)
bool spaceWasPressed = false;

// ---------------------------------------------------------------------
//  Шейдерлер (2-апта: тор мен үшбұрыштарды сызу үшін)
// ---------------------------------------------------------------------
const char* vertexShaderSrc = R"glsl(
#version 330 core
layout (location = 0) in vec2 aPos;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
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
//  Пробел: бір рет басқанда ғана фон ауысады (basıp тұрғанда емес).
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    bool spaceIsPressed = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);

    // Тек "жаңа басу" сәтінде ауыстырамыз (алдыңғы кадрда басылмаған,
    // қазір басылған болса)
    if (spaceIsPressed && !spaceWasPressed) {
        whiteBackground = !whiteBackground;
    }

    spaceWasPressed = spaceIsPressed;
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

    // Қандай OpenGL нұсқасы керек екенін айтамыз.
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

    glfwMakeContextCurrent(window);              // осы терезенің контексі белсенді
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(0);                         // VSync өшірулі (1-апта, 4-тапсырма)

    // -----------------------------------------------------------------
    //  3. GLAD: OpenGL функцияларын жүктеу
    //     Контекст белсенді болғаннан КЕЙІН ғана. Ретін бұзсаң — бәрі құлайды.
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    // -----------------------------------------------------------------
    //  4. Шейдер бағдарламасы (2-апта)
    // -----------------------------------------------------------------
    GLuint shaderProgram = createShaderProgram();
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");

    // -----------------------------------------------------------------
    //  5. Тор координаттарын есептеу (NDC: -1..1 аралығы)
    //     Әр ұяшық BL-TR диагоналімен екіге бөлінеді.
    //     Жоғарғы үшбұрыш: BL -> TL -> TR
    // -----------------------------------------------------------------
    std::vector<float> fillVerts;   // боялатын (жоғарғы) үшбұрыштар
    std::vector<float> lineVerts;   // тор + диагональ сызықтары

    float left = -0.9f, right = 0.9f;
    float top = 0.9f, bottom = -0.9f;

    float cellW = (right - left) / COLS;
    float cellH = (top - bottom) / ROWS;

    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            float x0 = left + c * cellW;
            float x1 = x0 + cellW;
            float y1 = top - r * cellH;      // жоғарғы жиек
            float y0 = y1 - cellH;           // төменгі жиек

            // Бұрыштар: BL, BR, TL, TR
            float BLx = x0, BLy = y0;
            float BRx = x1, BRy = y0;
            float TLx = x0, TLy = y1;
            float TRx = x1, TRy = y1;

            fillVerts.insert(fillVerts.end(), {
                BLx, BLy,
                TLx, TLy,
                TRx, TRy
            });

            // Ұяшық контуры (4 қабырға) + диагональ
            lineVerts.insert(lineVerts.end(), {
                BLx, BLy,  BRx, BRy,
                BRx, BRy,  TRx, TRy,
                TRx, TRy,  TLx, TLy,
                TLx, TLy,  BLx, BLy,
                BLx, BLy,  TRx, TRy   // диагональ
            });
        }
    }

    //  6. VAO/VBO-ларды дайындау

    GLuint fillVAO, fillVBO;
    glGenVertexArrays(1, &fillVAO);
    glGenBuffers(1, &fillVBO);
    glBindVertexArray(fillVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fillVBO);
    glBufferData(GL_ARRAY_BUFFER, fillVerts.size() * sizeof(float),
                 fillVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    GLuint lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVerts.size() * sizeof(float),
                 lineVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    int fillVertexCount = (int)(fillVerts.size() / 2);
    int lineVertexCount = (int)(lineVerts.size() / 2);

    //  7. Негізгі цикл
    
    int frameCount = 0;
    double lastFpsTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {

        processInput(window);

        // --- Экранды тазалау (1-апта: анимацияланған не ақ фон) ---
        if (whiteBackground) {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            float t = (float)glfwGetTime();
            float r = (std::sin(t * 3.0f) + 1.0f) * 0.5f * 0.3f;
            float g = (std::sin(t * 2.0f) + 1.0f) * 0.5f * 0.3f;
            glClearColor(r, g, 0.35f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT);

        // --- Тор мен диагональдарды сызу (2-апта) ---
        glUseProgram(shaderProgram);

        // Жоғарғы үшбұрыштарды бояу (қызғылт-сары түс)
        glUniform3f(colorLoc, 0.95f, 0.55f, 0.15f);
        glBindVertexArray(fillVAO);
        glDrawArrays(GL_TRIANGLES, 0, fillVertexCount);

        // Тор мен диагональ сызықтары (ақ түс)
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
        glBindVertexArray(lineVAO);
        glDrawArrays(GL_LINES, 0, lineVertexCount);

        // --- FPS есептеу (1-апта, 4-тапсырма) ---
        frameCount++;
        double now = glfwGetTime();
        if (now - lastFpsTime >= 1.0) {
            std::cout << "FPS: " << frameCount << "\n";
            frameCount = 0;
            lastFpsTime = now;
        }

        glfwSwapBuffers(window);   
        glfwPollEvents();          
    }

    //  8. Тазалау

    glDeleteVertexArrays(1, &fillVAO);
    glDeleteBuffers(1, &fillVBO);
    glDeleteVertexArrays(1, &lineVAO);
    glDeleteBuffers(1, &lineVBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

//  1-АПТА ТАПСЫРМАЛАРЫ (орындалды)
// =====================================================================
//  1. Терезенің өлшемін 1280x720 ет.                              
//  2. Фон түсінің өзгеру жылдамдығын арттыр.                       
//  3. Пробел басылғанда фон ақ/бастапқы арасында ауысады (toggle). 
//  4. glfwSwapInterval(0) қой да, консольге FPS шығар.             
// =====================================================================
//
//  2-АПТА ТАПСЫРМАСЫ (орындалды)
// =====================================================================
//  Тікбұрышты COLS x ROWS торға бөліп, әр ұяшықты диагональмен
//  екі үшбұрышқа бөлу және жоғарғы бөлігін бояу.
// =====================================================================