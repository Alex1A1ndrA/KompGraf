#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <opencv2/opencv.hpp>
#include <omp.h>

// Структура для векторов и цветов
// Используется для описания позиций, направлений и цвета
struct Vec3 {
    double x, y, z;
    Vec3(double x_=0, double y_=0, double z_=0) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& v) const { return Vec3(x+v.x, y+v.y, z+v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x-v.x, y-v.y, z-v.z); }
    Vec3 operator*(double d) const { return Vec3(x*d, y*d, z*d); }
    Vec3 operator/(double d) const { return Vec3(x/d, y/d, z/d); }

    // Нормализация вектора (приведение длины к 1)
    Vec3 normalize() const { double mg = std::sqrt(x*x + y*y + z*z); return Vec3(x/mg, y/mg, z/mg); }

    // Скалярное произведение
    double dot(const Vec3& v) const { return x*v.x + y*v.y + z*v.z; }

    // Векторное произведение
    Vec3 cross(const Vec3& v) const { return Vec3(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x); }
};

// Оператор умножения для скалярного значения * вектор
Vec3 operator*(double d, const Vec3& v) {
    return Vec3(v.x * d, v.y * d, v.z * d);
}

// Структура луча
// Содержит начало и направление луча
struct Ray {
    Vec3 origin;      // Точка начала
    Vec3 direction;   // Направление
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d.normalize()) {}
};

// Абстрактный класс для объектов сцены
// Определяет интерфейсы для пересечения, получения нормали и цвета
class Object {
public:
    virtual bool intersect(const Ray& ray, double& t) const = 0; // Пересечение луча с объектом
    virtual Vec3 getNormal(const Vec3& point) const = 0;         // Получение нормали в точке
    virtual Vec3 getColor(const Vec3& point) const = 0;          // Получение цвета в точке
    virtual bool isReflective() const = 0;                       // Проверка на отражательность
    virtual double getReflectivity() const = 0;                  // Коэффициент отражения
};

// Класс для сферы
// Реализация объекта-сферы
class Sphere : public Object {
public:
    Vec3 center;          // Центр сферы
    double radius;        // Радиус
    Vec3 color;           // Цвет
    double reflectivity;  // Коэффициент отражения

    Sphere(const Vec3& c, double r, const Vec3& col, double refl) : center(c), radius(r), color(col), reflectivity(refl) {}

    // Проверка пересечения луча со сферой
    bool intersect(const Ray& ray, double& t) const override {
        Vec3 oc = ray.origin - center;
        double b = 2 * oc.dot(ray.direction);
        double c = oc.dot(oc) - radius * radius;
        double discriminant = b*b - 4*c;
        if (discriminant < 0) return false; // Пересечения нет
        else {
            discriminant = std::sqrt(discriminant);
            double t0 = (-b - discriminant) / 2;
            double t1 = (-b + discriminant) / 2;
            t = (t0 < t1) ? t0 : t1;
            if (t < 0) t = (t0 > t1) ? t0 : t1;
            return t >= 0;
        }
    }

    // Нормаль к поверхности сферы
    Vec3 getNormal(const Vec3& point) const override {
        return (point - center).normalize();
    }

    // Цвет сферы
    Vec3 getColor(const Vec3& point) const override {
        return color;
    }

    // Проверка, отражает ли объект
    bool isReflective() const override {
        return reflectivity > 0;
    }

    // Коэффициент отражения
    double getReflectivity() const override {
        return reflectivity;
    }

    // Установка коэффициента отражения
    void setReflectivity(double refl) {
        reflectivity = refl;
    }
};

// Класс для плоскости
// Реализация объекта-плоскости с поддержкой текстур
class Plane : public Object {
public:
    Vec3 point;      // Точка на плоскости
    Vec3 normal;     // Нормаль к плоскости
    cv::Mat texture; // Текстура плоскости
    double scale;    // Масштаб текстуры
    bool reflective; // Флаг отражательной способности

    Plane(const Vec3& p, const Vec3& n, const cv::Mat& tex, double s, bool refl = false) :
        point(p), normal(n), texture(tex), scale(s), reflective(refl) {
            normal = normal.normalize();
        }

    // Проверка пересечения луча с плоскостью
    bool intersect(const Ray& ray, double& t) const override {
        double denom = normal.dot(ray.direction);
        if (std::abs(denom) > 1e-6) { // Луч не параллелен плоскости
            t = (point - ray.origin).dot(normal) / denom;
            return t >= 0;
        }
        return false;
    }

    // Нормаль к плоскости
    Vec3 getNormal(const Vec3& point_) const override {
        return normal;
    }

    // Получение цвета текстуры по координатам
    Vec3 getColor(const Vec3& point_) const override {
        // Вычисление UV-координат
        double u, v;
        if (std::abs(normal.y) > 0.999) { // Горизонтальная плоскость
            u = point_.x * scale;
            v = point_.z * scale;
        } else if (std::abs(normal.z) > 0.999) { // Вертикальная плоскость
            u = point_.x * scale;
            v = point_.y * scale;
        } else {
            u = 0;
            v = 0;
        }

        // Преобразование в координаты текстуры
        u = u - std::floor(u);
        v = v - std::floor(v);

        int tex_u = static_cast<int>(u * texture.cols);
        int tex_v = static_cast<int>(v * texture.rows);

        // Обработка выхода за границы
        tex_u = std::clamp(tex_u, 0, texture.cols - 1);
        tex_v = std::clamp(tex_v, 0, texture.rows - 1);

        // Получение цвета из текстуры
        cv::Vec3b color = texture.at<cv::Vec3b>(tex_v, tex_u);
        return Vec3(color[2] / 255.0, color[1] / 255.0, color[0] / 255.0);
    }

    // Проверка, является ли объект отражающим
    bool isReflective() const override {
        return reflective;
    }

    // Коэффициент отражения (не используется для плоскости)
    double getReflectivity() const override {
        return 0;
    }
};

// Функция трассировки луча
// Определяет цвет пикселя на основе пересечения с объектами
Vec3 trace(const Ray& ray, const std::vector<Object*>& objects, int depth) {
    if (depth <= 0) return Vec3(0, 0, 0); // Ограничение глубины рекурсии для предотвращения бесконечных отражений

    double closest_t = std::numeric_limits<double>::max(); // Инициализация ближайшего расстояния
    const Object* hit_object = nullptr; // Объект, с которым произошло пересечение

    // Проверка пересечения луча со всеми объектами сцены
    for (const auto& object : objects) {
        double t = 0;
        if (object->intersect(ray, t) && t < closest_t) {
            closest_t = t;
            hit_object = object;
        }
    }

    if (!hit_object) {
        return Vec3(0.5, 0.7, 1.0); // Фон (голубой цвет)
    }

    // Точка пересечения
    Vec3 hit_point = ray.origin + ray.direction * closest_t;
    Vec3 normal = hit_object->getNormal(hit_point); // Нормаль в точке пересечения
    Vec3 color = hit_object->getColor(hit_point);   // Цвет объекта

    // Обработка отражений
    if (hit_object->isReflective()) {
        Vec3 reflect_dir = ray.direction - 2 * ray.direction.dot(normal) * normal; // Вычисление отраженного направления
        Ray reflected_ray(hit_point + reflect_dir * 1e-4, reflect_dir); // Смещение для предотвращения самопересечения
        Vec3 reflected_color = trace(reflected_ray, objects, depth - 1); // Рекурсивный вызов для отражения
        color = color * (1 - hit_object->getReflectivity()) + reflected_color * hit_object->getReflectivity(); // Смешивание цветов
    }

    return color;
}

// Функция для отрисовки сцены
// Создает изображение, трассируя лучи для каждого пикселя
void render(int width, int height, const std::vector<Object*>& objects, const std::string& output_file) {
    cv::Mat image(height, width, CV_8UC3); // Создание изображения

    // Определение камеры
    Vec3 camera_pos(0, 0, 0); // Позиция камеры
    double fov = 90; // Угол обзора
    double aspect_ratio = static_cast<double>(width) / height;
    double scale = std::tan((fov * 0.5) * M_PI / 180);

    #pragma omp parallel for // Параллельная обработка строк изображения
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Преобразование координат пикселя в нормализованные экранные координаты
            double px = (2 * (x + 0.5) / static_cast<double>(width) - 1) * aspect_ratio * scale;
            double py = (1 - 2 * (y + 0.5) / static_cast<double>(height)) * scale;

            // Создание луча из камеры
            Vec3 ray_dir = Vec3(px, py, -1).normalize();
            Ray ray(camera_pos, ray_dir);

            // Трассировка луча
            Vec3 color = trace(ray, objects, 5); // Глубина рекурсии = 5

            // Преобразование цвета в диапазон [0, 255]
            image.at<cv::Vec3b>(y, x) = cv::Vec3b(
                static_cast<uchar>(std::clamp(color.z * 255.0, 0.0, 255.0)),
                static_cast<uchar>(std::clamp(color.y * 255.0, 0.0, 255.0)),
                static_cast<uchar>(std::clamp(color.x * 255.0, 0.0, 255.0))
            );
        }
    }

    // Сохранение изображения
    cv::imwrite(output_file, image);
}

int main() {
    // Параметры сцены
    int width = 800;  // Ширина изображения
    int height = 600; // Высота изображения

    // Загрузка текстуры для плоскости
    cv::Mat walltexture = cv::imread("wall.jpg");
    cv::Mat floortexture = cv::imread("flour.jpg");
    if (walltexture.empty() || floortexture.empty()) {
        std::cerr << "Ошибка: Не удалось загрузить текстуры." << std::endl;
        return -1;
    }

    // Создание объектов сцены
    std::vector<Object*> objects;

    // Параметры текстур и материалов
    Plane* floorPlane = new Plane(Vec3(0, 0, 0), Vec3(0, 1, 0), floortexture, 0.1); // Пол
    objects.push_back(floorPlane);

    Plane* wallPlane = new Plane(Vec3(0, 0, -5), Vec3(0, 0, 1), walltexture, 0.1); // Задняя стена
    objects.push_back(wallPlane);

    double sphereReflectivity = 0.5; // Начальная зеркальность сферы
    Sphere* sphere = new Sphere(Vec3(0, 1, 0), 1, Vec3(1, 1, 1), sphereReflectivity); // Сфера
    objects.push_back(sphere);

    // Создание окна для визуализации
    cv::namedWindow("Ray Tracing", cv::WINDOW_AUTOSIZE);

    // Параметры камеры
    Vec3 cameraPos(0, 1, 5); // Начальная позиция камеры
    double cameraSpeed = 0.2; // Скорость перемещения камеры

    // Основной цикл рендеринга
    bool running = true;
    while (running) {
        cv::Mat image(height, width, CV_8UC3); // Матрица для хранения изображения

        // Трассировка лучей
        #pragma omp parallel for schedule(dynamic) // Параллельная обработка
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // Преобразование координат экрана в нормализованные
                double u = (2.0 * (x + 0.5) / static_cast<double>(width) - 1.0) * (width / static_cast<double>(height));
                double v = 1.0 - 2.0 * (y + 0.5) / static_cast<double>(height);

                Vec3 dir = Vec3(u, v, -1).normalize(); // Направление луча
                Ray ray(cameraPos, dir); // Луч из текущей позиции камеры

                // Вычисление цвета пикселя
                Vec3 color = trace(ray, objects, 5); // Глубина рекурсии = 5

                // Ограничение цвета в диапазоне [0, 255]
                image.at<cv::Vec3b>(y, x) = cv::Vec3b(
                    static_cast<uchar>(std::clamp(color.z * 255.0, 0.0, 255.0)),
                    static_cast<uchar>(std::clamp(color.y * 255.0, 0.0, 255.0)),
                    static_cast<uchar>(std::clamp(color.x * 255.0, 0.0, 255.0))
                );
            }
        }

        // Отображение изображения
        cv::imshow("Ray Tracing", image);
        std::cout << "Текущая зеркальность сферы: " << sphere->getReflectivity() << std::endl;

        // Обработка пользовательского ввода
        int key = cv::waitKey(1);
        switch (key) {
            case 27: // ESC для выхода
                running = false;
                break;
            case 'w': case 'W': // Перемещение камеры вперёд
                cameraPos.z -= cameraSpeed;
                break;
            case 's': case 'S': // Перемещение камеры назад
                cameraPos.z += cameraSpeed;
                break;
            case 'a': case 'A': // Перемещение камеры влево
                cameraPos.x -= cameraSpeed;
                break;
            case 'd': case 'D': // Перемещение камеры вправо
                cameraPos.x += cameraSpeed;
                break;
            case 'q': case 'Q': // Перемещение камеры вверх
                cameraPos.y += cameraSpeed;
                break;
            case 'e': case 'E': // Перемещение камеры вниз
                cameraPos.y -= cameraSpeed;
                break;
            case '+': case '=': // Увеличение зеркальности сферы
                sphere->setReflectivity(std::min(1.0, sphere->getReflectivity() + 0.1));
                break;
            case '-': // Уменьшение зеркальности сферы
                sphere->setReflectivity(std::max(0.0, sphere->getReflectivity() - 0.1));
                break;
            case ' ': // Сохранение изображения
                cv::imwrite("result.png", image);
                std::cout << "Изображение сохранено в 'result.png'" << std::endl;
                break;
            default:
                break;
        }
    }

    // Очистка памяти
    for (auto obj : objects) {
        delete obj;
    }

    cv::destroyAllWindows();
    return 0;
}
