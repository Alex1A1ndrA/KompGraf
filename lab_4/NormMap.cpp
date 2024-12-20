#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

// Структура для вершины
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

// Функция для компиляции шейдера
GLuint compileShader(const char* shaderCode, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderCode, NULL);
    glCompileShader(shader);

    // Проверка на ошибки компиляции
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Ошибка компиляции шейдера:\n" << infoLog << std::endl;
    }
    return shader;
}

// Функция для создания шейдерной программы
GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    // Загрузка вершинного шейдера
    std::ifstream vShaderFile(vertexPath);
    std::stringstream vShaderStream;
    vShaderStream << vShaderFile.rdbuf();
    std::string vShaderCode = vShaderStream.str();

    // Загрузка фрагментного шейдера
    std::ifstream fShaderFile(fragmentPath);
    std::stringstream fShaderStream;
    fShaderStream << fShaderFile.rdbuf();
    std::string fShaderCode = fShaderStream.str();

    // Компиляция шейдеров
    GLuint vertexShader = compileShader(vShaderCode.c_str(), GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fShaderCode.c_str(), GL_FRAGMENT_SHADER);

    // Создание шейдерной программы
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Проверка на ошибки линковки
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Ошибка линковки шейдерной программы:\n" << infoLog << std::endl;
    }

    // Удаление шейдеров после линковки
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Функция для загрузки текстуры
GLuint loadTexture(const char* path) {
    sf::Image image;
    if (!image.loadFromFile(path)) {
        std::cerr << "Не удалось загрузить текстуру: " << path << std::endl;
        exit(-1);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Определение формата изображения
    GLenum format = GL_RGBA;
    if (image.getPixelsPtr()[3] == 255)
        format = GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, format, image.getSize().x, image.getSize().y, 0, format, GL_UNSIGNED_BYTE, image.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);

    // Параметры текстуры
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}

// Функция для вычисления тангента и битангента
void calculateTangents(std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
    // Инициализация тангентов и битангентов
    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].tangent = glm::vec3(0.0f);
        vertices[i].bitangent = glm::vec3(0.0f);
    }

    // Вычисление тангентов и битангентов
    for (size_t i = 0; i < indices.size(); i += 3) {
        Vertex& v0 = vertices[indices[i]];
        Vertex& v1 = vertices[indices[i+1]];
        Vertex& v2 = vertices[indices[i+2]];

        glm::vec3 edge1 = v1.position - v0.position;
        glm::vec3 edge2 = v2.position - v0.position;

        glm::vec2 deltaUV1 = v1.texCoords - v0.texCoords;
        glm::vec2 deltaUV2 = v2.texCoords - v0.texCoords;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        glm::vec3 tangent, bitangent;

        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent = glm::normalize(tangent);

        bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent = glm::normalize(bitangent);

        v0.tangent += tangent;
        v1.tangent += tangent;
        v2.tangent += tangent;

        v0.bitangent += bitangent;
        v1.bitangent += bitangent;
        v2.bitangent += bitangent;
    }

    // Нормализация тангентов и битангентов
    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].tangent = glm::normalize(vertices[i].tangent);
        vertices[i].bitangent = glm::normalize(vertices[i].bitangent);
    }
}

int main() {
    // Создание окна и контекста OpenGL
    sf::Window window(sf::VideoMode(800, 600), "Normal Mapping", sf::Style::Default, sf::ContextSettings(24));
    window.setFramerateLimit(60);

    // Инициализация GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "Ошибка инициализации GLEW: " << glewGetErrorString(err) << std::endl;
        return -1;
    }

    // Настройка OpenGL
    glEnable(GL_DEPTH_TEST);

    // Загрузка шейдеров
    GLuint shaderProgram = createShaderProgram("vertex_shader.glsl", "fragment_shader.glsl");

    // Загрузка текстур
    GLuint diffuseMap = loadTexture("diffuse.jpg");
    GLuint normalMap = loadTexture("normal.png");

    // Вершины куба
    std::vector<Vertex> vertices = {
        // Позиции                // Нормали             // Текстурные координаты
        // Передняя грань
        {{-1.0f, -1.0f,  1.0f},  {0.0f,  0.0f,  1.0f},  {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f},  {0.0f,  0.0f,  1.0f},  {1.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f},  {0.0f,  0.0f,  1.0f},  {1.0f, 1.0f}},
        {{-1.0f,  1.0f,  1.0f},  {0.0f,  0.0f,  1.0f},  {0.0f, 1.0f}},
        // Задняя грань
        {{-1.0f, -1.0f, -1.0f},  {0.0f,  0.0f, -1.0f},  {1.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f},  {0.0f,  0.0f, -1.0f},  {1.0f, 1.0f}},
        {{ 1.0f,  1.0f, -1.0f},  {0.0f,  0.0f, -1.0f},  {0.0f, 1.0f}},
        {{ 1.0f, -1.0f, -1.0f},  {0.0f,  0.0f, -1.0f},  {0.0f, 0.0f}},
        // Левая грань
        {{-1.0f, -1.0f, -1.0f},  {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f},  {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f},  {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, -1.0f},  {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
        // Правая грань
        {{ 1.0f, -1.0f, -1.0f},  {1.0f,  0.0f,  0.0f},  {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f},  {1.0f,  0.0f,  0.0f},  {1.0f, 1.0f}},
        {{ 1.0f,  1.0f,  1.0f},  {1.0f,  0.0f,  0.0f},  {0.0f, 1.0f}},
        {{ 1.0f, -1.0f,  1.0f},  {1.0f,  0.0f,  0.0f},  {0.0f, 0.0f}},
        // Нижняя грань
        {{-1.0f, -1.0f, -1.0f},  {0.0f, -1.0f,  0.0f},  {0.0f, 1.0f}},
        {{ 1.0f, -1.0f, -1.0f},  {0.0f, -1.0f,  0.0f},  {1.0f, 1.0f}},
        {{ 1.0f, -1.0f,  1.0f},  {0.0f, -1.0f,  0.0f},  {1.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f},  {0.0f, -1.0f,  0.0f},  {0.0f, 0.0f}},
        // Верхняя грань
        {{-1.0f,  1.0f, -1.0f},  {0.0f,  1.0f,  0.0f},  {1.0f, 1.0f}},
        {{-1.0f,  1.0f,  1.0f},  {0.0f,  1.0f,  0.0f},  {1.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f},  {0.0f,  1.0f,  0.0f},  {0.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f},  {0.0f,  1.0f,  0.0f},  {0.0f, 1.0f}},
    };

    // Индексы для отрисовки куба
    std::vector<GLuint> indices = {
        // Передняя грань
        0, 1, 2, 2, 3, 0,
        // Задняя грань
        4, 5, 6, 6, 7, 4,
        // Левая грань
        8, 9,10,10,11, 8,
        // Правая грань
        12,13,14,14,15,12,
        // Нижняя грань
        16,17,18,18,19,16,
        // Верхняя грань
        20,21,22,22,23,20
    };

    // Вычисление тангентов и битангентов
    calculateTangents(vertices, indices);

    // Создание VAO и VBO
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Настройка VAO
    glBindVertexArray(VAO);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

    // Атрибуты вершин
    // Позиция
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // Нормаль
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // Текстурные координаты
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

    // Тангенты
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

    // Битангенты
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

    glBindVertexArray(0);

    // Переменные для управления
    bool useNormalMap = true;
    sf::Clock clock;

    // Основной цикл
    while (window.isOpen()) {
        // Обработка событий
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) {
                    useNormalMap = !useNormalMap;
                    std::cout << "Normal Mapping: " << (useNormalMap ? "Enabled" : "Disabled") << std::endl;
                }
            }
        }

        // Очистка экрана
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Использование шейдерной программы
        glUseProgram(shaderProgram);

        // Передача униформ-переменных в шейдеры
        // Модельная матрица (вращение куба)
        float time = clock.getElapsedTime().asSeconds();
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(time * 20.0f), glm::vec3(0.5f, 1.0f, 0.0f));

        // Видовая матрица
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), // Позиция камеры
                                     glm::vec3(0.0f, 0.0f, 0.0f), // Точка, на которую смотрит камера
                                     glm::vec3(0.0f, 1.0f, 0.0f)); // Вектор "вверх"

        // Проекционная матрица
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                                800.0f / 600.0f,
                                                0.1f, 100.0f);

        GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLuint viewLoc  = glGetUniformLocation(shaderProgram, "view");
        GLuint projLoc  = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc,  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc,  1, GL_FALSE, glm::value_ptr(projection));

        // Параметры освещения
        glm::vec3 lightPos(5.0f, 5.0f, 5.0f);
        glm::vec3 viewPos(0.0f, 0.0f, 5.0f);

        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(viewPos));

        // Передача значения useNormalMap
        glUniform1i(glGetUniformLocation(shaderProgram, "useNormalMap"), useNormalMap);

        // Передача текстур
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glUniform1i(glGetUniformLocation(shaderProgram, "diffuseMap"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalMap);
        glUniform1i(glGetUniformLocation(shaderProgram, "normalMap"), 1);

        // Рисование куба
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Отображение результата
        window.display();
    }

    // Очистка ресурсов
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    return 0;
}
