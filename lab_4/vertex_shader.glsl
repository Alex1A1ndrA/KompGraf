#version 330 core

layout (location = 0) in vec3 aPos;      // Позиция
layout (location = 1) in vec3 aNormal;   // Нормаль
layout (location = 2) in vec2 aTexCoords;// Текстурные координаты
layout (location = 3) in vec3 aTangent;  // Тангенты
layout (location = 4) in vec3 aBitangent;// Битангенты

out vec2 TexCoords;
out vec3 FragPos;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    TexCoords = aTexCoords;

    // Вычисление TBN матрицы
    vec3 T = normalize(vec3(model * vec4(aTangent, 0.0)));
    vec3 B = normalize(vec3(model * vec4(aBitangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(aNormal, 0.0)));
    TBN = mat3(T, B, N);

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
