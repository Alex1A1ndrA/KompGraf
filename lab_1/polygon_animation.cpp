#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <iostream>

const float PI = 3.14159265359f;

// Функция для интерполяции
float lerp(float a, float b, float t) {
    return a * (1.0f - t) + b * t;
}

// Генерация шестиугольника
std::vector<float> generateHexagon(float radius) {
    std::vector<float> vertices;
    for (int i = 0; i < 6; ++i) {
        float angle = 2 * PI * i / 6;
        vertices.push_back(radius * cos(angle));
        vertices.push_back(radius * sin(angle));
    }
    return vertices;
}

// Применение матриц трансформации
void applyTransformations(const std::vector<float>& vertices, float dx, float dy, float angle, std::vector<float>& transformedVertices) {
    float cosA = cos(angle);
    float sinA = sin(angle);
    transformedVertices.clear();
    for (size_t i = 0; i < vertices.size(); i += 2) {
        float x = vertices[i];
        float y = vertices[i + 1];

        // Поворот и перемещение
        float rotatedX = x * cosA - y * sinA;
        float rotatedY = x * sinA + y * cosA;
        transformedVertices.push_back(rotatedX + dx);
        transformedVertices.push_back(rotatedY + dy);
    }
}

int main() {
    // Инициализация GLFW
    if (!glfwInit()) {
        return -1;
    }

    // Создаем окно
    GLFWwindow* window = glfwCreateWindow(800, 600, "Polygon Animation", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Параметры анимации
    float baseRadius = 0.5f;
    float t = 0.0f;
    bool forward = true;

    // Управление с клавиатуры
    float speed = 0.001f; // Начальная скорость передвижения
    float rotationSpeed = 0.005f; // Начальная скорость вращения
    float angle = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        // Очистка экрана
        glClear(GL_COLOR_BUFFER_BIT);

        // Интерполяция размера и позиции
        float dx = lerp(-0.5f, 0.5f, t);
        float dy = 0.2f * sin(2 * PI * t);
        float radius = lerp(0.3f, 0.7f, t);

        // Генерация шестиугольника и применение трансформаций
        std::vector<float> vertices = generateHexagon(radius);
        std::vector<float> transformedVertices;
        applyTransformations(vertices, dx, dy, angle, transformedVertices);

        // Отрисовка шестиугольника
        glBegin(GL_POLYGON);
        for (size_t i = 0; i < transformedVertices.size(); i += 2) {
            float r = (sin(t * PI) + 1.0f) / 2.0f;
            float g = (cos(t * PI) + 1.0f) / 2.0f;
            glColor3f(r, g, 0.5f);
            glVertex2f(transformedVertices[i], transformedVertices[i + 1]);
        }
        glEnd();

        // Обновление параметров
        t += forward ? speed : -speed;
        angle += rotationSpeed;

        if (t >= 1.0f || t <= 0.0f) forward = !forward;

        // Обработка ввода с клавиатуры
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) speed += 0.0002f; // Добавочная скорость передвижения
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) speed -= 0.0002f; // Добавочная скорость передвижения
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) rotationSpeed -= 0.0002f; // Добавочная скорость вращения вправо
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) rotationSpeed += 0.0002f; // Добавочная скорость вращения влево

        // Обновление экрана
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
