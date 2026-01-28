#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include "SavePng.hpp"
#include "Object.hpp"
#include "Scene.hpp"
#include "SceneXMLParser.hpp"
#include "GUI.hpp"
#include <omp.h>

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
    std::atomic<int>& completed_lines // 原子级变量
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
                pixel_buffer[idx] = {ir, ig, ib}; // 无需互斥锁，因为写入范围完全不重叠
            }
            // 原子变量更新进度
            completed_lines.fetch_add(1, std::memory_order_relaxed);
            std::cerr << "\rScanlines completed: " << completed_lines.load(std::memory_order_relaxed) 
                      << "/" << image_height << ' ' << std::flush;
        }
    }
}

// 改为OMP并行的行渲染函数
void render_omp(const Scene& render_scene,
                const Point3& origin, const Vec3& horizontal, const Vec3& vertical,
                const Point3& lower_left_corner,
                int image_width, int image_height,
                int samples_per_pixel, int max_depth,
                std::vector<Pixel>& pixel_buffer,
                std::atomic<int>& completed_lines) {
    
    // 修复：schedule 子句移到 for 指令后，parallel 仅创建并行区域
    #pragma omp parallel
    {
        // 仅主线程初始化进度（可选）
        #pragma omp master
        {
            std::cerr << "\rScanlines completed: 0/" << image_height << ' ' << std::flush;
        }

        // 核心修复：schedule(dynamic, 32) 附加在 for 指令上
        #pragma omp for schedule(dynamic, 32)
        for (int j = 0; j < image_height; ++j) {
            int original_j = image_height - 1 - j;
            for (int i = 0; i < image_width; ++i) {
                Color pixel_color(0,0,0);
                for (int s = 0; s < samples_per_pixel; ++s) {
                    auto u = (i + random_double()) / (image_width-1);
                    auto v = (original_j + random_double()) / (image_height-1);
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
                size_t idx = j * image_width + i;
                pixel_buffer[idx] = {ir, ig, ib};
            }
            // 原子更新进度
            completed_lines.fetch_add(1, std::memory_order_relaxed);
            // 仅主线程输出进度（替代 master 指令）
            if (omp_get_thread_num() == 0) {
                std::cerr << "\rScanlines completed: " << completed_lines.load(std::memory_order_relaxed) 
                          << "/" << image_height << ' ' << std::flush;
            }
        }

        // 并行结束后主线程输出最终进度
        #pragma omp master
        {
            std::cerr << "\rScanlines completed: " << image_height << "/" << image_height << " ✔️\n";
        }
    }
}

/**
 * 2. 新增：适配GUI的渲染函数（替换GUI.cpp中模拟的render_cb逻辑）
 * 从GUI选择的XML文件读取场景，执行渲染，并将结果写入GUI的渲染缓冲区
 */
void gui_render_logic(const std::string& xml_path) {
    // 1. 解析XML场景文件（复用原有逻辑）
    SceneXMLParser parser;
    SceneData parsed_data;
    try {
        parsed_data = parser.parseFile(xml_path);
        std::cerr << "解析场景成功：" << xml_path << "，共 " << parsed_data.objects.size() << " 个物体\n";
    } catch (const std::exception& e) {
        fl_alert("场景解析失败：%s", e.what());
        return;
    }

    // 2. 构建渲染场景
    Scene render_scene;
    convertSceneDataToRenderScene(parsed_data, render_scene);

    // 3. 图像/相机参数（复用原有逻辑）
    const int image_width = 400;
    const int samples_per_pixel = 400;
    const int max_depth = 50;
    float aspect_ratio = camera_aspect_ratio;
    int image_height = static_cast<int>(image_width / aspect_ratio);
    float viewport_width = aspect_ratio * camera_viewport_height;
    Point3 origin = camera_origin;
    Vec3 horizontal = Vec3(viewport_width, 0, 0);
    Vec3 vertical = Vec3(0, camera_viewport_height, 0);
    Point3 lower_left_corner = origin - horizontal/2 - vertical/2 - Vec3(0, 0, camera_focal_length);

    // 4. 初始化像素缓冲区
    pixel_buffer.resize(image_width * image_height);
    completed_lines.store(0, std::memory_order_relaxed);
    int block_size = 32;

    // 5. 多线程渲染（复用原有逻辑）
    auto render_start = std::chrono::high_resolution_clock::now();
    
    // int num_threads = std::thread::hardware_concurrency() ?: 4;
    // std::vector<std::thread> threads;
    // for (int t = 0; t < num_threads; ++t) {
    //     threads.emplace_back(
    //         render_blocks_round_robin,
    //         t, num_threads, block_size,
    //         std::ref(render_scene),
    //         std::ref(origin), std::ref(horizontal), std::ref(vertical),
    //         std::ref(lower_left_corner),
    //         image_width, image_height,
    //         samples_per_pixel, max_depth,
    //         std::ref(pixel_buffer),
    //         std::ref(completed_lines)
    //     );
    // }
    // for (auto& t : threads) t.join();

    // 设置OMP线程数（可选，默认是硬件核心数）
    omp_set_num_threads(std::thread::hardware_concurrency() ?: 4);
    // 执行OMP并行渲染
    render_omp(render_scene,
               origin, horizontal, vertical,
               lower_left_corner,
               image_width, image_height,
               samples_per_pixel, max_depth,
               pixel_buffer,
               completed_lines);

    // 6. 计算渲染耗时
    auto render_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> render_duration = render_end - render_start;
    std::cerr << "渲染完成，耗时：" << render_duration.count() << " 秒\n";

    // 7. 将渲染结果写入GUI的全局缓冲区（替换GUI的模拟数据）
    app_state.buffer_width = image_width;
    app_state.buffer_height = image_height;
    // 释放原有缓冲区（避免内存泄漏）
    if (app_state.render_buffer) free(app_state.render_buffer);
    // 分配新缓冲区（RGB格式，每个像素3字节）
    app_state.render_buffer = malloc(image_width * image_height * 3);
    if (!app_state.render_buffer) {
        fl_alert("GUI渲染缓冲区分配失败！");
        return;
    }

    // 8. 拷贝像素数据到GUI缓冲区
    unsigned char* gui_buf = (unsigned char*)app_state.render_buffer;
    int idx = 0;
    for (const auto& pixel : pixel_buffer) {
        gui_buf[idx++] = static_cast<unsigned char>(pixel.r);
        gui_buf[idx++] = static_cast<unsigned char>(pixel.g);
        gui_buf[idx++] = static_cast<unsigned char>(pixel.b);
    }

    // ===== 新增：9. 将渲染结果显示到GUI的显示框 =====
    if (app_state.render_display_box && app_state.render_buffer) {
        // ① 转换为FLTK的RGB图像（3字节/像素）
        Fl_RGB_Image* rgb_img = new Fl_RGB_Image(
            (unsigned char*)app_state.render_buffer,
            image_width,
            image_height,
            3 // RGB格式（无Alpha通道）
        );

        // ② 自适应缩放（保持宽高比，适配显示框）
        Fl_Box* display_box = app_state.render_display_box;
        int box_w = display_box->w();
        int box_h = display_box->h();
        float img_aspect = (float)image_width / image_height;
        float box_aspect = (float)box_w / box_h;
        int draw_w, draw_h;

        if (img_aspect > box_aspect) {
            // 图像更宽，按显示框宽度缩放
            draw_w = box_w;
            draw_h = static_cast<int>(box_w / img_aspect);
        } else {
            // 图像更高，按显示框高度缩放
            draw_h = box_h;
            draw_w = static_cast<int>(box_h * img_aspect);
        }

        // ③ 缩放图像并显示
        Fl_Image* scaled_img = rgb_img->copy(draw_w, draw_h);
        delete rgb_img; // 释放原图像（避免内存泄漏）
        display_box->label(""); // 隐藏 "Preview Ready" 文字
        display_box->image(scaled_img); // 设置到显示框
        display_box->redraw(); // 强制重绘（立即显示）
    }

    app_state.is_rendered = true;
    set_status("Render completed!", FL_DARK_GREEN);
    app_state.render_display_box->redraw();
}

/**
 * 3. 重写GUI的渲染回调（替换GUI.cpp中的render_cb）
 */
void custom_render_cb(Fl_Widget*, void*) {
    if (app_state.selected_file.empty()) {
        fl_alert("请先选择待渲染的XML场景文件！");
        return;
    }
    // 执行真实渲染逻辑
    gui_render_logic(app_state.selected_file);
}

/**
 * 4. 重写GUI的文件选择回调（过滤XML文件）
 */
void custom_select_file_cb(Fl_Widget*, void*) {
    Fl_Native_File_Chooser file_chooser;
    file_chooser.title("选择XML场景文件");
    file_chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    // 仅显示XML文件（匹配场景解析逻辑）
    file_chooser.filter("XML场景文件\t*.xml\n所有文件\t*");
    file_chooser.directory("../scene"); // 默认场景目录

    if (file_chooser.show() == 0) {
        app_state.selected_file = file_chooser.filename();
        set_status("File chosen: " + app_state.selected_file,
               FL_DARK_GREEN);
    }
}

int main() {
    // ========== GUI初始化 ==========
    Fl_Window* main_win = init_gui(1000, 600); // 创建400x300的GUI窗口
    // 替换GUI默认的回调函数（使用自定义逻辑）
    // 1. 获取GUI按钮并重新绑定回调
    Fl_Button* select_btn = (Fl_Button*)main_win->child(2); // 第一个子控件：选择文件按钮
    Fl_Button* render_btn = (Fl_Button*)main_win->child(3); // 第二个子控件：渲染按钮
    Fl_Button* save_btn = (Fl_Button*)main_win->child(4);   // 第三个子控件：保存PNG按钮
    
    select_btn->callback(custom_select_file_cb); // 自定义文件选择（仅选XML）
    render_btn->callback(custom_render_cb);      // 自定义渲染逻辑（真实渲染）
    // 保存PNG回调复用GUI.cpp的save_png_cb（无需修改）

    // ========== 启动GUI主循环 ==========
    main_win->show();
    int ret = Fl::run(); // FLTK主事件循环

    // ========== 清理资源 ==========
    cleanup_resources(main_win);
    return ret;
}