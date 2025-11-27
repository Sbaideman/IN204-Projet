#include <iostream>
#include <fstream>
// #include "src/Vec3.hpp"
// #include "src/Ray.hpp"
#include "src/Sphere.hpp"
#include "src/Scene.hpp"
// #include "src/Utils.hpp"

Color ray_color(const Ray& r, const SceneBaseObject& world, int depth) {
    HitRecord rec;

    // 递归终止条件
    if (depth <= 0)
        return Color(0,0,0); // 如果反弹次数过多，光线能量耗尽

    if (world.hit(r, 0.001, infinity, rec)) {
        Ray scattered;
        Color attenuation;
        
        // 调用材质的 scatter 函数
        if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
            // 递归调用，并将结果与材质颜色相乘
            return attenuation * ray_color(scattered, world, depth-1);
        
        return Color(0,0,0);
    }

    // 没击中，还是返回天空背景色
    Vec3 unit_direction = unit_vector(r.direction());
    auto t = 0.5 * (unit_direction.y() + 1.0);
    return (1.0-t)*Color(1.0, 1.0, 1.0) + t*Color(0.5, 0.7, 1.0);
}


int main() {
    // 1. 图像参数
    const auto aspect_ratio = 16.0 / 9.0;
    const int image_width = 400;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 100;
    const int max_depth = 50;

    // 2. 世界
    Scene world;

    auto material_ground = make_shared<Matte>(Color(0.8, 0.8, 0.0));
    // 将中间的球换成玻璃材质
    auto material_center = make_shared<Glass>(1.5); // 1.5 是玻璃的典型折射率
    auto material_left   = make_shared<Matte>(Color(0.7, 0.3, 0.3));
    auto material_right  = make_shared<Metal>(Color(0.8, 0.6, 0.2), 0.0);

    world.add(make_shared<Sphere>(Point3( 0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(make_shared<Sphere>(Point3( 0.0,    0.0, -1.0),   0.5, material_center));
    
    // 添加一个空心玻璃球，让效果更有趣
    world.add(make_shared<Sphere>(Point3( 0.0,    0.0, -1.0),  -0.4, material_center));

    world.add(make_shared<Sphere>(Point3(-1.0,    0.0, -1.0),   0.3, material_left));
    world.add(make_shared<Sphere>(Point3( 1.0,    0.0, -1.0),   0.3, material_right));

    // 3. 相机 (保持不变)
    auto viewport_height = 2.0;
    auto viewport_width = aspect_ratio * viewport_height;
    auto focal_length = 1.0;

    Point3 origin = Point3(0, 0, 0);
    Vec3 horizontal = Vec3(viewport_width, 0, 0);
    Vec3 vertical = Vec3(0, viewport_height, 0);
    Point3 lower_left_corner = origin - horizontal/2 - vertical/2 - Vec3(0, 0, focal_length);

    // 4. 渲染
    std::ofstream outfile("test.ppm");
    outfile << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = image_height-1; j >= 0; --j) {
        std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) {
            Color pixel_color(0,0,0);
            for (int s = 0; s < samples_per_pixel; ++s) {
                auto u = (i + random_double()) / (image_width-1);
                auto v = (j + random_double()) / (image_height-1);
                Ray r(origin, lower_left_corner + u*horizontal + v*vertical - origin);
                pixel_color += ray_color(r, world, max_depth);
            }
            
            // 写入颜色 (现在需要加入 Gamma 校正，让颜色更自然)
            auto scale = 1.0 / samples_per_pixel;
            auto r = sqrt(pixel_color.x() * scale); // Gamma 2 校正
            auto g = sqrt(pixel_color.y() * scale);
            auto b = sqrt(pixel_color.z() * scale);

            int ir = static_cast<int>(256 * clamp(r, 0.0, 0.999));
            int ig = static_cast<int>(256 * clamp(g, 0.0, 0.999));
            int ib = static_cast<int>(256 * clamp(b, 0.0, 0.999));
            
            outfile << ir << ' ' << ig << ' ' << ib << '\n';
        }
    }
    
    outfile.close();
    std::cerr << "\nDone.\n";
}