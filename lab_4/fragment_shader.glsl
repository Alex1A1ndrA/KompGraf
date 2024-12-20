#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform vec3 lightPos;
uniform vec3 viewPos;

uniform bool useNormalMap; // Переключатель между стандартным затенением и normal mapping

void main()
{
    // Получение нормали из нормальной карты
    vec3 normal;
    if (useNormalMap) {
        normal = texture(normalMap, TexCoords).rgb;
        normal = normalize(normal * 2.0 - 1.0);
        normal = normalize(TBN * normal);
    } else {
        normal = normalize(TBN[2]); // Используем нормаль из вершинного шейдера
    }

    // Освещение
    vec3 color = texture(diffuseMap, TexCoords).rgb;
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 diffuse = diff * color;

    // Зеркальное освещение
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = vec3(0.2) * spec;

    vec3 ambient = 0.1 * color;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
