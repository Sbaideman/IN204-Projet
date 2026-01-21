#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
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

// 定义全局变量存储从XML解析的相机/背景色参数
Point3 camera_origin;
float camera_focal_length;
float camera_viewport_height;
float camera_aspect_ratio;
Color bg_color(0.05, 0.05, 0.1); // 默认背景色

// 像素结构
struct Pixel { int r, g, b; };

Color ray_color(const Ray& r, const SceneBaseObject& world, int depth) {
    HitRecord rec;

    // 递归深度限制
    if (depth <= 0)
        return Color(0,0,0);

    // 背景颜色 (如果不撞击任何物体)
    // 为了看清点光源的效果，我们将原本明亮的天空改成【全黑】或【微弱的星光】
    if (!world.hit(r, 0.001, infinity, rec)) {
        // 返回微弱的环境光 (0.05, 0.05, 0.05)
        return bg_color;

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
    // 1. 遍历所有物体（包括地面）
    for (const auto& xml_obj : data.objects) {
        std::shared_ptr<Material> mat;
        const auto& mat_data = xml_obj.material;

        // ========== 材质解析（扩展点光源材质） ==========
        if (mat_data.type == "matte") {
            float r = std::stof(mat_data.properties.at("color").at("r")) / 255.0f;
            float g = std::stof(mat_data.properties.at("color").at("g")) / 255.0f;
            float b = std::stof(mat_data.properties.at("color").at("b")) / 255.0f;
            mat = std::make_shared<Matte>(Color(r, g, b));
        } else if (mat_data.type == "metal") {
            float r = std::stof(mat_data.properties.at("color").at("r")) / 255.0f;
            float g = std::stof(mat_data.properties.at("color").at("g")) / 255.0f;
            float b = std::stof(mat_data.properties.at("color").at("b")) / 255.0f;
            float fuzz = std::stof(mat_data.properties.at("fuzz").at("value"));
            mat = std::make_shared<Metal>(Color(r, g, b), fuzz);
        } else if (mat_data.type == "glass") {
            float ior = std::stof(mat_data.properties.at("ior").at("value"));
            mat = std::make_shared<Glass>(ior);
        } else if (mat_data.type == "light") {
            // 解析自发光强度
            float intensity = std::stof(mat_data.properties.at("intensity").at("value"));
            mat = std::make_shared<PointLight>(Color(intensity, intensity, intensity));
        }

        // ========== 物体类型解析（扩展平面、平行六面体） ==========
        if (xml_obj.type == "sphere") {
            // 球体解析（原有逻辑）
            float pos_x = std::stof(xml_obj.properties.at("position").at("x"));
            float pos_y = std::stof(xml_obj.properties.at("position").at("y"));
            float pos_z = std::stof(xml_obj.properties.at("position").at("z"));
            float radius = std::stof(xml_obj.properties.at("radius").at("value"));
            auto sphere = std::make_shared<Sphere>(Point3(pos_x, pos_y, pos_z), radius, mat);
            render_scene.add(sphere);
        } else if (xml_obj.type == "plane") {
            // 平面解析（地面）
            float pos_x = std::stof(xml_obj.properties.at("position").at("x"));
            float pos_y = std::stof(xml_obj.properties.at("position").at("y"));
            float pos_z = std::stof(xml_obj.properties.at("position").at("z"));
            float n_x = std::stof(xml_obj.properties.at("normal").at("x"));
            float n_y = std::stof(xml_obj.properties.at("normal").at("y"));
            float n_z = std::stof(xml_obj.properties.at("normal").at("z"));
            auto plane = std::make_shared<Plane>(Point3(pos_x, pos_y, pos_z), Vec3(n_x, n_y, n_z), mat);
            render_scene.add(plane);
        } else if (xml_obj.type == "parallelepiped") {
            // 平行六面体解析
            float o_x = std::stof(xml_obj.properties.at("origin").at("x"));
            float o_y = std::stof(xml_obj.properties.at("origin").at("y"));
            float o_z = std::stof(xml_obj.properties.at("origin").at("z"));
            float u_x = std::stof(xml_obj.properties.at("u").at("x"));
            float u_y = std::stof(xml_obj.properties.at("u").at("y"));
            float u_z = std::stof(xml_obj.properties.at("u").at("z"));
            float v_x = std::stof(xml_obj.properties.at("v").at("x"));
            float v_y = std::stof(xml_obj.properties.at("v").at("y"));
            float v_z = std::stof(xml_obj.properties.at("v").at("z"));
            float w_x = std::stof(xml_obj.properties.at("w").at("x"));
            float w_y = std::stof(xml_obj.properties.at("w").at("y"));
            float w_z = std::stof(xml_obj.properties.at("w").at("z"));
            
            Point3 origin(o_x, o_y, o_z);
            Vec3 u(u_x, u_y, u_z);
            Vec3 v(v_x, v_y, v_z);
            Vec3 w(w_x, w_y, w_z);
            auto para = std::make_shared<Parallelepiped>(origin, u, v, w, mat);
            render_scene.add(para);
        }
    }

    // 2. 解析相机参数（覆盖硬编码）
    if (!data.camera.properties.empty()) {
        // 示例：读取相机位置、焦距、视口高度、宽高比
        float cam_x = std::stof(data.camera.properties.at("position").at("x"));
        float cam_y = std::stof(data.camera.properties.at("position").at("y"));
        float cam_z = std::stof(data.camera.properties.at("position").at("z"));
        // 存储到全局/外部变量，供main函数的相机初始化使用
        extern Point3 camera_origin;
        extern float camera_focal_length;
        extern float camera_viewport_height;
        extern float camera_aspect_ratio;
        
        camera_origin = Point3(cam_x, cam_y, cam_z);
        camera_focal_length = std::stof(data.camera.properties.at("focal_length").at("value"));
        camera_viewport_height = std::stof(data.camera.properties.at("viewport_height").at("value"));
        
        // 解析宽高比（处理 16.0/9.0 这类字符串）
        std::string ar_str = data.camera.properties.at("aspect_ratio").at("value");
        size_t div_pos = ar_str.find('/');
        if (div_pos != std::string::npos) {
            float num = std::stof(ar_str.substr(0, div_pos));
            float den = std::stof(ar_str.substr(div_pos+1));
            camera_aspect_ratio = num / den;
        } else {
            camera_aspect_ratio = std::stof(ar_str);
        }
    }

    // 3. 解析全局设置（背景色）
    if (!data.global_settings.properties.empty()) {
        // 读取背景色并替换 ray_color 中的硬编码值
        extern Color bg_color;
        float bg_r = std::stof(data.global_settings.properties.at("background_color").at("r")) / 255.0f;
        float bg_g = std::stof(data.global_settings.properties.at("background_color").at("g")) / 255.0f;
        float bg_b = std::stof(data.global_settings.properties.at("background_color").at("b")) / 255.0f;
        bg_color = Color(bg_r, bg_g, bg_b);
    }
}

// 2. 全局/局部变量（替换原有的 row_indices 和 progress_mutex）
std::atomic<int> completed_lines(0); // 原子变量统计完成行数，无需互斥锁
std::vector<Pixel> pixel_buffer;     // 像素缓冲区（存储所有像素颜色）

// 分块轮询：每个线程处理“间隔为线程数”的块，块内是连续行
void render_blocks_round_robin(
    int thread_id,                // 线程ID
    int num_threads,              // 总线程数
    int block_size,               // 块大小（建议 16/32 行，适配 Apple Silicon 缓存）
    const Scene& render_scene,
    const Point3& origin, const Vec3& horizontal, const Vec3& vertical,
    const Point3& lower_left_corner,
    int image_width, int image_height,
    int samples_per_pixel, int max_depth,
    std::vector<Pixel>& pixel_buffer,
    std::atomic<int>& completed_lines
) {
    // 计算总块数
    int total_blocks = (image_height + block_size - 1) / block_size;
    
    // 轮询处理块：线程ID处理 thread_id, thread_id+num_threads, ... 块
    for (int block_idx = thread_id; block_idx < total_blocks; block_idx += num_threads) {
        // 计算当前块的行范围（块内是连续行，缓存友好）
        int block_start_j = image_height - 1 - block_idx * block_size;
        int block_end_j = std::max(0, block_start_j - block_size + 1);
        
        // 处理块内的所有行（连续行，缓存命中率高）
        for (int j = block_start_j; j >= block_end_j; --j) {
            for (int i = 0; i < image_width; ++i) {
                Color pixel_color(0,0,0);
                for (int s = 0; s < samples_per_pixel; ++s) {
                    auto u = (i + random_double()) / (image_width-1);
                    auto v = (j + random_double()) / (image_height-1);
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
                size_t idx = (image_height - 1 - j) * image_width + i;
                pixel_buffer[idx] = {ir, ig, ib};
            }
            // 原子变量更新进度
            completed_lines.fetch_add(1, std::memory_order_relaxed);
            std::cerr << "\rScanlines completed: " << completed_lines.load(std::memory_order_relaxed) 
                      << "/" << image_height << ' ' << std::flush;
        }
    }
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

    // Scene world;
    Scene render_scene;  // 用于渲染的最终场景
    convertSceneDataToRenderScene(parsed_data, render_scene);

    // ========== 图像参数（可从XML全局设置扩展） ==========
    const int image_width = 400;
    const int samples_per_pixel = 400;
    const int max_depth = 50;

    // ========== 相机初始化（从XML解析的值） ==========
    float aspect_ratio = camera_aspect_ratio; // 从XML读取
    int image_height = static_cast<int>(image_width / aspect_ratio);
    float viewport_width = aspect_ratio * camera_viewport_height;
    Point3 origin = camera_origin; // 从XML读取
    Vec3 horizontal = Vec3(viewport_width, 0, 0);
    Vec3 vertical = Vec3(0, camera_viewport_height, 0);
    Point3 lower_left_corner = origin - horizontal/2 - vertical/2 - Vec3(0, 0, camera_focal_length);

    // 初始化像素缓冲区
    pixel_buffer.resize(image_width * image_height);
    completed_lines.store(0, std::memory_order_relaxed);
    int block_size = 32; // 关键：块大小（适配 Apple Silicon 缓存，建议 16/32/64）

    // ========== 记录渲染开始时间 ==========
    auto render_start = std::chrono::high_resolution_clock::now();

    // 获取 CPU 核心数（Apple Silicon 自动识别 M 系列核心数）
    int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4; // 兜底
    std::vector<std::thread> threads;
    std::cerr << "====================================\n";
    std::cerr << "CPU 信息检测：\n";
    std::cerr << "  核心数：" << num_threads << "\n";
    std::cerr << "====================================\n\n";

    // 启动线程：每个线程分配唯一ID（0~num_threads-1）
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back(
            render_blocks_round_robin,
            t,                          // 线程ID
            num_threads,                // 总线程数
            block_size,
            std::ref(render_scene),
            std::ref(origin), std::ref(horizontal), std::ref(vertical),
            std::ref(lower_left_corner),
            image_width, image_height,
            samples_per_pixel, max_depth,
            std::ref(pixel_buffer),
            std::ref(completed_lines)
        );
    }

    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }

    // ===== （后面的文件写入、耗时统计逻辑完全不变）=====
    std::ofstream outfile("test.ppm");
    outfile << "P3\n" << image_width << ' ' << image_height << "\n255\n";
    for (const auto& pixel : pixel_buffer) {
        outfile << pixel.r << ' ' << pixel.g << ' ' << pixel.b << '\n';
    }
    outfile.close();

    // ========== 计算并输出渲染耗时 ==========
    auto render_end = std::chrono::high_resolution_clock::now();
    // 计算毫秒级耗时（也可以用秒：std::chrono::duration<double>）
    std::chrono::duration<double, std::milli> render_duration = render_end - render_start;
    std::cerr << "\nRendering completed\n";
    std::cerr << "Total render time: " << render_duration.count() << " ms (" 
              << render_duration.count() / 1000.0 << " seconds)\n";

    return 0;
}