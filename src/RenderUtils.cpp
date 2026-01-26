#include "RenderUtils.hpp"
#include <iostream>

Color ray_color(const Ray& r, const SceneBaseObject& world, int depth, const Color& bg_color) {
    HitRecord rec;

    // Recursion limit
    if (depth <= 0)
        return Color(0,0,0);

    // If no intersection, return background color
    if (!world.hit(r, 0.001, infinity, rec)) {
        return bg_color;
    }

    Ray scattered;      // Scattered ray
    Color attenuation;  // Color attenuation
    
    // 1. Get emitted color from material
    Color emitted = rec.mat_ptr->emit(rec.p);

    // 2. Light scattering (Reflection / Refraction)
    if (!rec.mat_ptr->scatter(r, rec, attenuation, scattered))
        return emitted;

    // 3. Final color = emitted + (attenuation * reflected color)
    return emitted + attenuation * ray_color(scattered, world, depth-1, bg_color);
}

void convertSceneDataToRenderScene(
    const SceneData& data, 
    Scene& render_scene, 
    CameraConfig& cam_config, 
    Color& bg_color
) {
    // 1. Iterate through all objects
    for (const auto& xml_obj : data.objects) {
        std::shared_ptr<Material> mat;
        const auto& mat_data = xml_obj.material;

        // ========== Material Parsing ==========
        if (mat_data.type == "matte") {
            // Matte material
            float r = std::stof(mat_data.properties.at("color").at("r")) / 255.0f;
            float g = std::stof(mat_data.properties.at("color").at("g")) / 255.0f;
            float b = std::stof(mat_data.properties.at("color").at("b")) / 255.0f;
            mat = std::make_shared<Matte>(Color(r, g, b));
        } else if (mat_data.type == "metal") {
            // Metal material
            float r = std::stof(mat_data.properties.at("color").at("r")) / 255.0f;
            float g = std::stof(mat_data.properties.at("color").at("g")) / 255.0f;
            float b = std::stof(mat_data.properties.at("color").at("b")) / 255.0f;
            float fuzz = std::stof(mat_data.properties.at("fuzz").at("value"));
            mat = std::make_shared<Metal>(Color(r, g, b), fuzz);
        } else if (mat_data.type == "glass") {
            // Glass material
            float ior = std::stof(mat_data.properties.at("ior").at("value"));
            mat = std::make_shared<Glass>(ior);
        } else if (mat_data.type == "light") {
            // Light source
            float intensity = std::stof(mat_data.properties.at("intensity").at("value"));
            mat = std::make_shared<PointLight>(Color(intensity, intensity, intensity));
        }

        // ========== Object Type Parsing ==========
        if (xml_obj.type == "sphere") {
            // Sphere
            float pos_x = std::stof(xml_obj.properties.at("position").at("x"));
            float pos_y = std::stof(xml_obj.properties.at("position").at("y"));
            float pos_z = std::stof(xml_obj.properties.at("position").at("z"));
            float radius = std::stof(xml_obj.properties.at("radius").at("value"));
            auto sphere = std::make_shared<Sphere>(Point3(pos_x, pos_y, pos_z), radius, mat);
            render_scene.add(sphere);
        } else if (xml_obj.type == "plane") {
            // Plane
            float pos_x = std::stof(xml_obj.properties.at("position").at("x"));
            float pos_y = std::stof(xml_obj.properties.at("position").at("y"));
            float pos_z = std::stof(xml_obj.properties.at("position").at("z"));
            float n_x = std::stof(xml_obj.properties.at("normal").at("x"));
            float n_y = std::stof(xml_obj.properties.at("normal").at("y"));
            float n_z = std::stof(xml_obj.properties.at("normal").at("z"));
            auto plane = std::make_shared<Plane>(Point3(pos_x, pos_y, pos_z), Vec3(n_x, n_y, n_z), mat);
            render_scene.add(plane);
        } else if (xml_obj.type == "parallelepiped") {
            // Parallelepiped
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

    // 2. Parse camera parameters
    if (!data.camera.properties.empty()) {
        float cam_x = std::stof(data.camera.properties.at("position").at("x"));
        float cam_y = std::stof(data.camera.properties.at("position").at("y"));
        float cam_z = std::stof(data.camera.properties.at("position").at("z"));
        
        cam_config.origin = Point3(cam_x, cam_y, cam_z);
        cam_config.focal_length = std::stof(data.camera.properties.at("focal_length").at("value"));
        cam_config.viewport_height = std::stof(data.camera.properties.at("viewport_height").at("value"));
        
        // Parse aspect ratio (handle "16.0/9.0")
        std::string ar_str = data.camera.properties.at("aspect_ratio").at("value");
        size_t div_pos = ar_str.find('/');
        if (div_pos != std::string::npos) {
            float num = std::stof(ar_str.substr(0, div_pos));
            float den = std::stof(ar_str.substr(div_pos+1));
            cam_config.aspect_ratio = num / den;
        } else {
            cam_config.aspect_ratio = std::stof(ar_str);
        }
    }

    // 3. Parse global settings (background color)
    if (!data.global_settings.properties.empty()) {
        float bg_r = std::stof(data.global_settings.properties.at("background_color").at("r")) / 255.0f;
        float bg_g = std::stof(data.global_settings.properties.at("background_color").at("g")) / 255.0f;
        float bg_b = std::stof(data.global_settings.properties.at("background_color").at("b")) / 255.0f;
        bg_color = Color(bg_r, bg_g, bg_b);
    }
}

void render_blocks_round_robin(
    int thread_id,
    int num_threads,
    int block_size,
    const Scene& render_scene,
    const Point3& origin, const Vec3& horizontal, const Vec3& vertical,
    const Point3& lower_left_corner,
    int image_width, int image_height,
    int samples_per_pixel, int max_depth,
    const Color& bg_color,
    std::vector<Pixel>& pixel_buffer,
    std::atomic<int>& completed_lines
) {
    // Calculate total blocks
    int total_blocks = (image_height + block_size - 1) / block_size;
    
    // Round robin processing
    for (int block_idx = thread_id; block_idx < total_blocks; block_idx += num_threads) {
        // Calculate row range for current block
        int block_start_j = image_height - 1 - block_idx * block_size;
        int block_end_j = std::max(0, block_start_j - block_size + 1);
        
        // Process all rows in the block
        for (int j = block_start_j; j >= block_end_j; --j) {
            for (int i = 0; i < image_width; ++i) {
                Color pixel_color(0,0,0);
                for (int s = 0; s < samples_per_pixel; ++s) {
                    auto u = (i + random_double()) / (image_width-1);
                    auto v = (j + random_double()) / (image_height-1);
                    Ray r(origin, lower_left_corner + u*horizontal + v*vertical - origin);
                    // Pass bg_color to ray_color
                    pixel_color += ray_color(r, render_scene, max_depth, bg_color);
                }
                auto scale = 1.0 / samples_per_pixel;
                auto r = sqrt(pixel_color.x() * scale);
                auto g = sqrt(pixel_color.y() * scale);
                auto b = sqrt(pixel_color.z() * scale);
                int ir = static_cast<int>(256 * clamp(r, 0.0, 0.999));
                int ig = static_cast<int>(256 * clamp(g, 0.0, 0.999));
                int ib = static_cast<int>(256 * clamp(b, 0.0, 0.999));
                size_t idx = (image_height - 1 - j) * image_width + i;
                pixel_buffer[idx] = {ir, ig, ib};  // No mutex needed
            }
            // Update progress atomically
            completed_lines.fetch_add(1, std::memory_order_relaxed);
            std::cerr << "\rScanlines completed: " << completed_lines.load(std::memory_order_relaxed) 
                      << "/" << image_height << ' ' << std::flush;
        }
    }
}