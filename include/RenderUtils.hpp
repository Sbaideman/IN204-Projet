#ifndef RENDER_UTILS_HPP
#define RENDER_UTILS_HPP

#include <vector>
#include <atomic>
#include <cmath>
#include "Object.hpp"
#include "Scene.hpp"
#include "SceneXMLParser.hpp"

// Pixel structure
struct Pixel { int r, g, b; };

// Container for camera settings to avoid global variables
struct CameraConfig {
    Point3 origin;
    float focal_length;
    float viewport_height;
    float aspect_ratio;
};

/**
 * @brief Calculate the color that a specific ray of light will eventually see.
 * @param r The ray.
 * @param world The scene.
 * @param depth Maximum number of ray bounces.
 * @param bg_color The background color if the ray hits nothing.
 * @return The final color of the pixel.
 */
Color ray_color(const Ray& r, const SceneBaseObject& world, int depth, const Color& bg_color);

/**
 * @brief Converts parsed XML data into actual renderable scene objects and configuration.
 * 
 * This function acts as a factory that iterates through the raw data structure (SceneData), 
 * instantiates specific materials (Matte, Metal, Glass) and geometric primitives 
 * (Sphere, Plane, Parallelepiped), and adds them to the rendering scene. 
 * It also configures global camera parameters and background settings based on the input.
 * 
 * @param data The raw data structure containing string-based properties parsed from XML.
 * @param render_scene The destination scene object where created objects will be added.
 * @param cam_config Reference to a CameraConfig struct to be populated with camera parameters.
 * @param bg_color Reference to a Color object to be updated with the scene's background color.
 */
void convertSceneDataToRenderScene(
    const SceneData& data, 
    Scene& render_scene, 
    CameraConfig& cam_config, 
    Color& bg_color
);

/**
 * @brief Renders a portion of the scene using a block-based round-robin scheduling strategy.
 *
 * This function is designed to be run by multiple threads in parallel. Instead of assigning
 * a single contiguous chunk of rows to each thread, it divides the image into small blocks
 * and assigns them in an interleaved pattern (e.g., Thread 0 takes block 0, N, 2N...).
 * This ensures better load balancing, as complex areas of the image are distributed among all threads.
 *
 * For each pixel, it performs anti-aliasing (multi-sampling), gamma correction,
 * and writes the final RGB values directly into the shared pixel buffer without mutexes
 * (since write areas are disjoint).
 *
 * @param thread_id The unique ID of the current thread (0 to num_threads-1).
 * @param num_threads The total number of threads utilized.
 * @param block_size The number of scanlines per block (e.g., 32) for cache efficiency.
 * @param render_scene The scene object containing all geometry and lights.
 * @param origin The camera origin.
 * @param horizontal The horizontal viewport vector.
 * @param vertical The vertical viewport vector.
 * @param lower_left_corner The lower-left corner of the viewport.
 * @param image_width Width of the image in pixels.
 * @param image_height Height of the image in pixels.
 * @param samples_per_pixel Number of random samples per pixel for anti-aliasing.
 * @param max_depth Maximum recursion depth for ray bouncing.
 * @param bg_color The environmental background color used when a ray hits nothing.
 * @param pixel_buffer The output buffer where calculated pixel colors are stored.
 * @param completed_lines Atomic counter used to track and display global progress.
 */
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
);

#endif // RENDER_UTILS_HPP