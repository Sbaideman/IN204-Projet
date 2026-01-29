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
#include <sstream>  // For building strings with timestamps
#include <iomanip>  // For formatting floating-point precision

/**
 * @brief Calculate the final color seen by a specific ray
 * @param r The ray.
 * @param world The scene.
 * @param depth Maximum number of ray bounces
 * @return The final color of the pixel
 */

// Global variables to store camera/background color parameters parsed from XML
Point3 camera_origin;
float camera_focal_length;
float camera_viewport_height;
float camera_aspect_ratio;
Color bg_color(0.05, 0.05, 0.1); // Default background color

// Pixel structure
struct Pixel { int r, g, b; };

Color ray_color(const Ray& r, const SceneBaseObject& world, int depth) {
    HitRecord rec;

    // Recursion depth limit
    if (depth <= 0)
        return Color(0,0,0);

    // Background color (if no objects are hit)
    // To clearly see the effect of point light sources, we change the originally bright sky to [pure black] or [faint starlight]
    if (!world.hit(r, 0.001, infinity, rec)) {
        // Return faint ambient light (0.05, 0.05, 0.05)
        return bg_color;

        // // If no hit, return sky background color
        // Vec3 unit_direction = unit_vector(r.direction());
        // auto t = 0.5 * (unit_direction.y() + 1.0);
        // return (1.0-t)*Color(1.0, 1.0, 1.0) + t*Color(0.5, 0.7, 1.0);
    }

    Ray scattered;
    Color attenuation;
    
    // Get the self-illuminated color of the object itself
    // Black for ordinary objects, bright color for light sources
    Color emitted = rec.mat_ptr->emit(rec.p);

    // Attempt to scatter (reflection/refraction)
    if (!rec.mat_ptr->scatter(r, rec, attenuation, scattered))
        return emitted; // If no scattering (e.g., hit a light), return the light color directly

    // Final color = self-illumination + (attenuation * color from reflected light)
    return emitted + attenuation * ray_color(scattered, world, depth-1);
}

/**
 * @brief Convert XML data to Scene structure
 */
void convertSceneDataToRenderScene(const SceneData& data, Scene& render_scene) {
    // Traverse all objects (including ground)
    for (const auto& xml_obj : data.objects) {
        std::shared_ptr<Material> mat;
        const auto& mat_data = xml_obj.material;

        // ========== Material parsing (extend point light material) ==========
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
            // Parse self-illumination intensity
            float intensity = std::stof(mat_data.properties.at("intensity").at("value"));
            mat = std::make_shared<PointLight>(Color(intensity, intensity, intensity));
        }

        // ========== Object type parsing (extend plane, parallelepiped) ==========
        if (xml_obj.type == "sphere") {
            // Sphere parsing (original logic)
            float pos_x = std::stof(xml_obj.properties.at("position").at("x"));
            float pos_y = std::stof(xml_obj.properties.at("position").at("y"));
            float pos_z = std::stof(xml_obj.properties.at("position").at("z"));
            float radius = std::stof(xml_obj.properties.at("radius").at("value"));
            auto sphere = std::make_shared<Sphere>(Point3(pos_x, pos_y, pos_z), radius, mat);
            render_scene.add(sphere);
        } else if (xml_obj.type == "plane") {
            // Plane parsing (ground)
            float pos_x = std::stof(xml_obj.properties.at("position").at("x"));
            float pos_y = std::stof(xml_obj.properties.at("position").at("y"));
            float pos_z = std::stof(xml_obj.properties.at("position").at("z"));
            float n_x = std::stof(xml_obj.properties.at("normal").at("x"));
            float n_y = std::stof(xml_obj.properties.at("normal").at("y"));
            float n_z = std::stof(xml_obj.properties.at("normal").at("z"));
            auto plane = std::make_shared<Plane>(Point3(pos_x, pos_y, pos_z), Vec3(n_x, n_y, n_z), mat);
            render_scene.add(plane);
        } else if (xml_obj.type == "parallelepiped") {
            // Parallelepiped parsing
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

    // Parse camera parameters (override hard-coded values)
    if (!data.camera.properties.empty()) {
        // Read camera position, focal length, viewport height, aspect ratio
        float cam_x = std::stof(data.camera.properties.at("position").at("x"));
        float cam_y = std::stof(data.camera.properties.at("position").at("y"));
        float cam_z = std::stof(data.camera.properties.at("position").at("z"));
        // Store to global/external variables for camera initialization in main function
        extern Point3 camera_origin;
        extern float camera_focal_length;
        extern float camera_viewport_height;
        extern float camera_aspect_ratio;
        
        camera_origin = Point3(cam_x, cam_y, cam_z);
        camera_focal_length = std::stof(data.camera.properties.at("focal_length").at("value"));
        camera_viewport_height = std::stof(data.camera.properties.at("viewport_height").at("value"));
        
        // Parse aspect ratio (handle strings like 16.0/9.0)
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

    // Parse global settings (background color)
    if (!data.global_settings.properties.empty()) {
        // Read background color and replace hard-coded value in ray_color
        extern Color bg_color;
        float bg_r = std::stof(data.global_settings.properties.at("background_color").at("r")) / 255.0f;
        float bg_g = std::stof(data.global_settings.properties.at("background_color").at("g")) / 255.0f;
        float bg_b = std::stof(data.global_settings.properties.at("background_color").at("b")) / 255.0f;
        bg_color = Color(bg_r, bg_g, bg_b);
    }
}

std::atomic<int> completed_lines(0); // Atomic variable to count completed lines, no mutex needed
std::vector<Pixel> pixel_buffer;     // Pixel buffer (stores all pixel colors)

/**
 * @brief OMP parallel line rendering function
 */
void render_omp(const Scene& render_scene,
                const Point3& origin, const Vec3& horizontal, const Vec3& vertical,
                const Point3& lower_left_corner,
                int image_width, int image_height,
                int samples_per_pixel, int max_depth,
                std::vector<Pixel>& pixel_buffer,
                std::atomic<int>& completed_lines) {
    
    // Fix: Move schedule clause after for directive, parallel only creates parallel region
    #pragma omp parallel
    {
        // Only main thread initializes progress (optional)
        #pragma omp master
        {
            std::cerr << "\rScanlines completed: 0/" << image_height << ' ' << std::flush;
        }

        #pragma omp for collapse(1) schedule(dynamic, 32)
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
            // Atomically update progress
            int current_finished = completed_lines.fetch_add(1, std::memory_order_relaxed);
            // Suggestion: Update UI every ~2% progress completion instead of restricting to a specific thread
            if (current_finished % 10 == 0 || current_finished == image_height) {
                // Although any thread can enter here, Fl::check() is best in main thread or via Fl::awake()
                // In simple FLTK structure, omp master or thread 0 check is safe:
                if (omp_get_thread_num() == 0) {
                    if (app_state.progress_bar) {
                        float progress_val = (float)current_finished / image_height * 100.0f;
                        app_state.progress_bar->value(progress_val);
                    }
                    // Only thread 0 is responsible for refreshing UI and handling click events
                    Fl::check(); 
                    
                    // Terminal output
                    std::cerr << "\rScanlines completed: " << current_finished 
                              << "/" << image_height << ' ' << std::flush;
                }
            }
        }

        // Main thread outputs final progress after parallel execution
        #pragma omp master
        {
            std::cerr << "\rScanlines completed: " << image_height << "/" << image_height << " ✔️\n";
        }
    }
}

/**
 * @brief Read scene from XML file selected by GUI, execute rendering, and write results to GUI's render buffer
 */
double gui_render_logic(const std::string& xml_path) {
    // Parse XML scene file
    SceneXMLParser parser;
    SceneData parsed_data;
    try {
        parsed_data = parser.parseFile(xml_path);
        std::cerr << "Scene parsed successfully: " << xml_path << ", total " << parsed_data.objects.size() << " objects\n";
    } catch (const std::exception& e) {
        fl_alert("Scene parsing failed: %s", e.what());
        return 0;
    }

    // Build render scene
    Scene render_scene;
    convertSceneDataToRenderScene(parsed_data, render_scene);

    // Image/camera parameters
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

    // Initialize pixel buffer
    pixel_buffer.resize(image_width * image_height);
    completed_lines.store(0, std::memory_order_relaxed);
    int block_size = 32;

    // Multi-threaded rendering (reuse original logic)
    auto render_start = std::chrono::high_resolution_clock::now();

    // Set OMP thread count (optional, default is hardware core count)
    omp_set_num_threads(std::thread::hardware_concurrency() ?: 4);
    // Execute OMP parallel rendering
    render_omp(render_scene,
               origin, horizontal, vertical,
               lower_left_corner,
               image_width, image_height,
               samples_per_pixel, max_depth,
               pixel_buffer,
               completed_lines);

    // Calculate rendering time consumption
    auto render_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> render_duration = render_end - render_start;
    double seconds = render_duration.count();

    // Write rendering results to GUI's global buffer (replace GUI's simulated data)
    app_state.buffer_width = image_width;
    app_state.buffer_height = image_height;
    // Release original buffer (avoid memory leak)
    if (app_state.render_buffer) free(app_state.render_buffer);
    // Allocate new buffer (RGB format, 3 bytes per pixel)
    app_state.render_buffer = malloc(image_width * image_height * 3);
    if (!app_state.render_buffer) {
        fl_alert("GUI render buffer allocation failed!");
        return 0;
    }

    // Copy pixel data to GUI buffer
    unsigned char* gui_buf = (unsigned char*)app_state.render_buffer;
    int idx = 0;
    for (const auto& pixel : pixel_buffer) {
        gui_buf[idx++] = static_cast<unsigned char>(pixel.r);
        gui_buf[idx++] = static_cast<unsigned char>(pixel.g);
        gui_buf[idx++] = static_cast<unsigned char>(pixel.b);
    }

    // Display rendering results to GUI's display box
    if (app_state.render_display_box && app_state.render_buffer) {
        // Convert to FLTK RGB image (3 bytes per pixel)
        Fl_RGB_Image* rgb_img = new Fl_RGB_Image(
            (unsigned char*)app_state.render_buffer,
            image_width,
            image_height,
            3 // RGB format (no Alpha channel)
        );

        // Adaptive scaling (maintain aspect ratio, fit display box)
        Fl_Box* display_box = app_state.render_display_box;
        int box_w = display_box->w();
        int box_h = display_box->h();
        float img_aspect = (float)image_width / image_height;
        float box_aspect = (float)box_w / box_h;
        int draw_w, draw_h;

        if (img_aspect > box_aspect) {
            // Image is wider, scale by display box width
            draw_w = box_w;
            draw_h = static_cast<int>(box_w / img_aspect);
        } else {
            // Image is taller, scale by display box height
            draw_h = box_h;
            draw_w = static_cast<int>(box_h * img_aspect);
        }

        // ③ Scale image and display
        Fl_Image* scaled_img = rgb_img->copy(draw_w, draw_h);
        delete rgb_img;                      // Release original image (avoid memory leak)
        display_box->label("");              // Hide "Preview Ready" text
        display_box->image(scaled_img);      // Set to display box
        display_box->redraw();               // Force redraw (display immediately)
    }

    app_state.is_rendered = true;

    return seconds;

}

/**
 * @brief Override GUI's render callback
 */
void custom_render_cb(Fl_Widget*, void*) {
    if (app_state.selected_file.empty()) {
        fl_alert("Please select an XML scene file first!");
        return;
    }

    // 1. Set rendering status
    set_status("Rendering scene, please wait...", FL_BLUE);

    // 1. Reset progress bar
    if (app_state.progress_bar) {
        app_state.progress_bar->value(0);
        app_state.progress_bar->label("Rendering...");
    }
    
    set_status("Rendering scene, please wait...", FL_BLUE);
    
    // 2. Execute rendering (progress_bar->value update will be triggered internally)
    double elapsed_seconds = gui_render_logic(app_state.selected_file);

    // 3. Post-rendering processing
    if (app_state.progress_bar) {
        app_state.progress_bar->value(100);
        app_state.progress_bar->label("Done");
    }

    // 3. Format time consumption information
    std::stringstream ss;
    ss << "Render completed in " << std::fixed << std::setprecision(3) << elapsed_seconds << "s";
    
    // 4. Update status bar
    set_status(ss.str(), FL_DARK_GREEN);
    
    // 5. Force window refresh to display final status
    app_state.render_display_box->window()->redraw();
}

/**
 * @brief Override GUI's file selection callback (filter XML files)
 */
void custom_select_file_cb(Fl_Widget*, void*) {
    Fl_Native_File_Chooser file_chooser;
    file_chooser.title("Select XML Scene File");
    file_chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    // Only display XML files (match scene parsing logic)
    file_chooser.filter("XML Scene Files\t*.xml\nAll Files\t*");
    file_chooser.directory("../scene"); // Default scene directory

    if (file_chooser.show() == 0) {
        app_state.selected_file = file_chooser.filename();
        set_status("File chosen: " + app_state.selected_file,
               FL_DARK_GREEN);
    }
}

/**
 * @brief Scan all XML files in the specified directory and populate the list
 */
void refresh_scene_list(const std::string& path) {
    if (!app_state.file_browser) return;
    app_state.file_browser->clear();
    
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::string fname = ent->d_name;
            if (fname.find(".xml") != std::string::npos) {
                app_state.file_browser->add(fname.c_str());
            }
        }
        closedir(dir);
    }
}

void refresh_btn_wrapper(Fl_Widget*, void*) {
    refresh_scene_list("../scene"); // Call the scan function you wrote earlier
}

/**
 * @brief Callback for list item click: update selected_file
 */
void browser_cb(Fl_Widget* w, void*) {
    Fl_Hold_Browser* b = (Fl_Hold_Browser*)w;
    int v = b->value();
    if (v > 0) {
        std::string filename = b->text(v);
        app_state.selected_file = "../scene/" + filename;
        set_status("Selected: " + filename, FL_YELLOW);
    }
}

int main() {
    // ========== GUI initialization ==========
    Fl_Window* main_win = init_gui(1100, 750);
    
    // Bind list callback
    app_state.file_browser->callback(browser_cb);
    refresh_scene_list("../scene");

    // Rebind button logic
    Fl_Button* refresh_btn = (Fl_Button*)main_win->child(4);     // Refresh list button
    Fl_Button* render_btn = (Fl_Button*)main_win->child(5);      // Render button
    Fl_Button* save_btn = (Fl_Button*)main_win->child(6);        // Save PNG button
    
    // Replace GUI's default callback functions (use custom logic)
    refresh_btn->callback(refresh_btn_wrapper);
    render_btn->callback(custom_render_cb);      // Custom rendering logic (real rendering)

    // ========== Start GUI main loop ==========
    main_win->show();
    int ret = Fl::run();

    // ========== Clean up resources ==========
    cleanup_resources(main_win);
    return ret;
}