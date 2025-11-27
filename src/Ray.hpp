#pragma once
#include "Vec3.hpp"

/**
 * @file Ray.hpp
 * @brief Class representing a directed ray in 3D space.
 *
 * The `Ray` class is the fundamental tool for the ray tracing algorithm. 
 * It conceptually represents a half-line originating from a specific point 
 * and extending infinitely in a specific direction.
 *
 * Mathematically, it implements the linear equation:
 *      P(t) = A + t*b
 * where:
 *      P(t) is the 3D position at parameter t.
 *      A is the ray orig.
 *      b is the ray direction.
 *      t is a real number (scalar) representing the distance from the orig.
 */
class Ray {
public:
    Point3 orig; // Origin of ray
    Vec3 dir;    // Direction of ray

    Ray() {}
    Ray(const Point3& origin, const Vec3& direction)
        : orig(origin), dir(direction){}

    Point3 origin() const  { return orig; }
    Vec3 direction() const { return dir; }

    Point3 at(double t) const {
        return orig + t * dir;
    }
};
