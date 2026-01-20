#include <iostream>
#include <fstream>
#include "Object.hpp"
#include "Scene.hpp"
#include "SceneXMLParser.hpp"

/**
 * @brief 计算一条特定光线最终看到的颜色
 * @param r The ray.
 * @param world The scene.
 * @param depth Maximum number of ray bounces
 * @return The final color of the pixel
 */
Color ray_color(const Ray& r, const SceneBaseObject& world, int depth) {
    HitRecord rec;

    // 递归深度限制
    if (depth <= 0)
        return Color(0,0,0);

    // 背景颜色 (如果不撞击任何物体)
    // 为了看清点光源的效果，我们将原本明亮的天空改成【全黑】或【微弱的星光】
    if (!world.hit(r, 0.001, infinity, rec)) {
        // 返回微弱的环境光 (0.05, 0.05, 0.05)
        return Color(0.05, 0.05, 0.1);

        // // 没击中，还是返回天空背景色
        // Vec3 unit_direction = unit_vector(r.direction());
        // auto t = 0.5 * (unit_direction.y() + 1.0);
        // return (1.0-t)*Color(1.0, 1.0, 1.0) + t*Color(0.5, 0.7, 1.0);
    }

    Ray scattered;
    Color attenuation;
    
    // 1. 获取物体本身的自发光颜色
    // 对于普通物体是黑色，对于光源是亮色
    Color emitted = rec.mat_ptr->emit(rec.p);

    // 2. 尝试散射 (反射/折射)
    if (!rec.mat_ptr->scatter(r, rec, attenuation, scattered))
        return emitted; // 如果不散射(比如撞到了灯)，直接返回光的颜色

    // 3. 最终颜色 = 自发光 + (衰减 * 反射光带来的颜色)
    return emitted + attenuation * ray_color(scattered, world, depth-1);
}

void convertSceneDataToRenderScene(const SceneData& data, Scene& render_scene) {
    // 1. 遍历解析出的所有物体，转换为渲染用的几何对象
    for (const auto& xml_obj : data.objects) {
        // 示例：假设 XML 中物体类型为 "sphere"（球体），需适配你的 XML 定义
        if (xml_obj.type == "sphere") {
            // 从 XML 解析的属性中获取球体参数（position/x/y/z, radius 等）
            float pos_x = std::stof(xml_obj.properties.at("position").at("x"));
            float pos_y = std::stof(xml_obj.properties.at("position").at("y"));
            float pos_z = std::stof(xml_obj.properties.at("position").at("z"));
            float radius = std::stof(xml_obj.properties.at("radius").at("value"));

            // 创建材质（示例：漫反射材质，从 XML 属性获取颜色）
            float mat_r = std::stof(xml_obj.properties.at("color").at("r")) / 255.0f;
            float mat_g = std::stof(xml_obj.properties.at("color").at("g")) / 255.0f;
            float mat_b = std::stof(xml_obj.properties.at("color").at("b")) / 255.0f;
            auto mat = std::make_shared<Matte>(Color(mat_r, mat_g, mat_b));

            // 创建球体对象，添加到渲染场景
            auto sphere = std::make_shared<Sphere>(Point3(pos_x, pos_y, pos_z), radius, mat);
            render_scene.add(sphere);
        }
        // 扩展：添加其他物体类型（cube/triangle 等）的转换逻辑
        else if (xml_obj.type == "cube") {
            // 按需实现立方体的转换逻辑
        }
    }

    // 2. （可选）从 XML 解析相机参数，覆盖硬编码的相机配置
    // if (!data.camera.properties.empty()) {
    //     // 示例：读取相机位置、焦距等
    //     float cam_x = std::stof(data.camera.properties["position"]["x"]);
    //     // ... 其他相机参数
    // }

    // 3. （可选）应用全局设置（如背景色）
    // if (!data.global_settings.properties.empty()) {
    //     float bg_r = std::stof(data.global_settings.properties["background_color"]["r"]) / 255.0f;
    //     // ... 替换背景色逻辑
    // }
}


int main() {
    SceneXMLParser parser;  // 实例化解析器（正确类名）
    SceneData parsed_data;  // 存储解析后的原始数据（关键：不是直接的渲染Scene）
    const std::string xml_path = "../scene/scene_layout.xml";

    try {
        // 调用正确的解析接口：parseFile
        parsed_data = parser.parseFile(xml_path);
        std::cerr << "Parse successfully" << xml_path << ", get " 
                  << parsed_data.objects.size() << " objects\n";
    } catch (const std::exception& e) {
        std::cerr << "Parse failed: " << e.what() << std::endl;
        return -1;
    }

    Scene world;
    Scene render_scene;  // 用于渲染的最终场景
    convertSceneDataToRenderScene(parsed_data, render_scene);

    // 1. 图像参数（可从 parsed_data.global_settings 解析，这里先保留硬编码）
    const auto aspect_ratio = 16.0 / 9.0;
    const int image_width = 400;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    // 注意：因为点光源很难打中，我们需要更多的采样数来减少噪点，或者把光源做大一点
    const int samples_per_pixel = 400;
    const int max_depth = 50;

    // 3. 相机
    auto viewport_height = 2.0;
    auto viewport_width = aspect_ratio * viewport_height;
    auto focal_length = 1.0;
    Point3 origin = Point3(0, 0, 2); // 相机位置
    Vec3 horizontal = Vec3(viewport_width, 0, 0);
    Vec3 vertical = Vec3(0, viewport_height, 0);
    Point3 lower_left_corner = origin - horizontal/2 - vertical/2 - Vec3(0, 0, focal_length);

    // --- 材质 ---
    auto mat_ground = make_shared<Matte>(Color(0.5, 0.5, 0.5)); // 灰色地面
    auto mat_glass  = make_shared<Glass>(1.5);
    auto mat_metal  = make_shared<Metal>(Color(0.7, 0.6, 0.5), 0.0);
    auto mat_red    = make_shared<Matte>(Color(0.6, 0.1, 0.1));
    auto mat_blue   = make_shared<Matte>(Color(0.1, 0.1, 0.8));
    
    // --- 创建光源材质 ---
    // 颜色值大于 1.0 表示非常亮（HDR）
    auto mat_light  = make_shared<PointLight>(Color(15, 15, 15)); 

    // --- 物体 ---
    
    // 地面
    world.add(make_shared<Plane>(Point3(0, -0.5, 0), Vec3(0, 1, 0), mat_ground));
    render_scene.add(make_shared<Plane>(Point3(0, -0.5, 0), Vec3(0, 1, 0), mat_ground));

    // --- 点光源 ---
    // 在上方悬浮一个发光的小球
    // 坐标: (0, 1.5, -1), 半径: 0.2 (像一个大灯泡)
    render_scene.add(make_shared<Sphere>(Point3(-1.8, 2.2, 1.5), 1.6, mat_light));
    world.add(make_shared<Sphere>(Point3(-1.8, 2.2, 1.5), 1.6, mat_light));

    // 2. 玻璃球 (中间)
    world.add(make_shared<Sphere>(Point3(0, 0, -1), 0.5, mat_glass));
    
    // --- 3. 红色平行六面体 (左边) ---
    // 这是一个斜着的柱子
    Point3 p_origin(-2.0, -0.5, -1.5);
    Vec3 u(1.0, 0.0, 0.0);    // 宽度 (X轴)
    Vec3 v(0.2, 1.0, 0.0);    // 高度 (注意！这里 X=0.2，说明它是向右歪的)
    Vec3 w(0.0, 0.0, 1.0);    // 深度 (Z轴)
    
    world.add(make_shared<Parallelepiped>(p_origin, u, v, w, mat_red));

    // --- 4. 蓝色平行六面体 (右边) ---
    // 这是一个扭曲的扁盒子
    Point3 p_origin2(1.0, -0.5, -1.5);
    Vec3 u2(1.0, 0.0, -0.2);  // 稍微转向
    Vec3 v2(0.0, 0.8, 0.0);   // 高度 0.5
    Vec3 w2(0.0, 0.0, 1.0);
    
    world.add(make_shared<Parallelepiped>(p_origin2, u2, v2, w2, mat_blue));

    // 5. 一个金属球悬浮在上方
    world.add(make_shared<Sphere>(Point3(0, 1.0, -1.5), 0.4, mat_metal));

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
                
                // 简单的相机调整，让它稍微往下看一点点（非必须，为了效果）
                // 这里不做复杂变换，直接用标准光线
                Ray r(origin, lower_left_corner + u*horizontal + v*vertical - origin);
                
                pixel_color += ray_color(r, render_scene, max_depth);
            }
            
            auto scale = 1.0 / samples_per_pixel;
            auto r = sqrt(pixel_color.x() * scale);
            auto g = sqrt(pixel_color.y() * scale);
            auto b = sqrt(pixel_color.z() * scale);

            int ir = static_cast<int>(256 * clamp(r, 0.0, 0.999));
            int ig = static_cast<int>(256 * clamp(g, 0.0, 0.999));
            int ib = static_cast<int>(256 * clamp(b, 0.0, 0.999));
            
            outfile << ir << ' ' << ig << ' ' << ib << '\n';
        }
    }
    
    outfile.close();
    std::cerr << "\nRendering completed\n";
    return 0;
}