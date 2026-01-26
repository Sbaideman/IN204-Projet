#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include "RenderUtils.hpp"
#include "SceneXMLParser.hpp"

// Global variables to hold execution state
std::atomic<int> completed_lines(0);
std::vector<Pixel> pixel_buffer;

int main() {
    SceneXMLParser parser;  // Parser instance
    SceneData parsed_data;  // Raw parsed data
    const std::string xml_path = "../scene/scene_layout_2.xml";

    // 1. Parse XML
    try {
        parsed_data = parser.parseFile(xml_path);
        std::cerr << "Parse successfully: " << xml_path << ", found " 
                  << parsed_data.objects.size() << " objects\n";
    } catch (const std::exception& e) {
        std::cerr << "Parse failed: " << e.what() << std::endl;
        return -1;
    }

    // 2. Setup Scene and Camera config
    Scene render_scene;
    CameraConfig cam_config;
    Color bg_color(0.05, 0.05, 0.1); // Default

    // Populate scene and config from parsed data
    convertSceneDataToRenderScene(parsed_data, render_scene, cam_config, bg_color);

    // ========== Image Parameters ==========
    const int image_width = 400;
    const int samples_per_pixel = 400;
    const int max_depth = 50;

    // ========== Camera Calculation ==========
    int image_height = static_cast<int>(image_width / cam_config.aspect_ratio);
    float viewport_width = cam_config.aspect_ratio * cam_config.viewport_height;
    
    Vec3 horizontal = Vec3(viewport_width, 0, 0);
    Vec3 vertical = Vec3(0, cam_config.viewport_height, 0);
    Point3 lower_left_corner = cam_config.origin 
                             - horizontal/2 
                             - vertical/2 
                             - Vec3(0, 0, cam_config.focal_length);

    // Initialize buffer
    pixel_buffer.resize(image_width * image_height);
    completed_lines.store(0, std::memory_order_relaxed);
    int block_size = 32;

    // ========== Start Timer ==========
    auto render_start = std::chrono::high_resolution_clock::now();

    // Get CPU core count
    int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;
    std::vector<std::thread> threads;
    std::cerr << "====================================\n";
    std::cerr << "CPU Info:\n";
    std::cerr << "  Cores: " << num_threads << "\n";
    std::cerr << "====================================\n\n";

    // Launch threads
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back(
            render_blocks_round_robin,
            t,                          
            num_threads,                
            block_size,
            std::ref(render_scene),
            std::ref(cam_config.origin), std::ref(horizontal), std::ref(vertical),
            std::ref(lower_left_corner),
            image_width, image_height,
            samples_per_pixel, max_depth,
            std::ref(bg_color), // Pass the background color
            std::ref(pixel_buffer),
            std::ref(completed_lines)
        );
    }

    // Join threads
    for (auto& t : threads) {
        t.join();
    }

    // ========== Stop Timer ==========
    auto render_end = std::chrono::high_resolution_clock::now();

    // Calculate duration
    std::chrono::duration<double, std::milli> render_duration = render_end - render_start;
    std::cerr << "\nRendering completed\n";
    std::cerr << "Total render time: " << render_duration.count() << " ms (" 
              << render_duration.count() / 1000.0 << " seconds)\n";

    // ===== Write to PPM file =====
    std::ofstream outfile("test.ppm");
    outfile << "P3\n" << image_width << ' ' << image_height << "\n255\n";
    for (const auto& pixel : pixel_buffer) {
        outfile << pixel.r << ' ' << pixel.g << ' ' << pixel.b << '\n';
    }
    outfile.close();

    return 0;
}