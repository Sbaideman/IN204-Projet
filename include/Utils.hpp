#pragma once
#include <cmath>
#include <limits>
#include <memory>
#include <cstdlib>
#include <random>
#include <omp.h>
#include "Vec3.hpp"
#include "Ray.hpp"

// --- Usings ---
using std::shared_ptr;
using std::make_shared;
using std::sqrt;

// --- Constants ---
const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926;

// ----------------------------------- Utility Functions -----------------------------------
// Converting degrees to radians
inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

// Returns a random real in [0,1).
inline double _random_double() {
    // thread_local 确保每个线程只初始化一次这个生成器
    static thread_local std::mt19937 generator(std::random_device{}() + omp_get_thread_num());
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(generator);
}

inline double random_double(double min=0, double max=1) {
    return min + (max - min) * _random_double();
}

// Limit the value x within [min, max]
inline double clamp(double x, double min, double max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

/**
 * @brief Calculates the reflection vector for an incoming ray.
 * @param v The incoming direction vector.
 * @param n The surface normal vector (must be a unit vector).
 * @return The reflected direction vector.
 */
inline Vec3 reflect(const Vec3& v, const Vec3& n) {
    return v - 2*dot(v,n)*n;
}

/**
 * @brief Calculates the refraction vector using Snell's Law.
 * @param v The incoming unit direction vector.
 * @param n The surface normal unit vector.
 * @param eta The ratio of refractive indices (eta_incident / eta_refracted).
 * @return The refracted direction vector.
 */
inline Vec3 refract(const Vec3& v, const Vec3& n, double eta) {
    auto cos_theta = fmin(dot(-v, n), 1.0);
    Vec3 r_out_perp =  eta * (v + cos_theta*n);
    Vec3 r_out_parallel = -sqrt(fabs(1.0 - r_out_perp.length_squared())) * n;
    return r_out_perp + r_out_parallel;
}