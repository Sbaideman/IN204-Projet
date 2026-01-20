#pragma once
#include <cmath>
#include <iostream>

/**
 * @class Vec3.h
 * @brief Fundamental 3D vector class for the Ray Tracing project.
 *
 * This file defines the `Vec3` class, which serves as the primary mathematical 
 * building block for the entire rendering engine. It is a versatile class designed 
 * to represent two distinct concepts using the same data structure:
 * 
 * 1. Geometric 3D vectors: representing points in space (x, y, z) and directions.
 * 2. RGB Colors: representing pixel color values (r, g, b).
 *
 * The file includes comprehensive operator overloading to facilitate vector arithmetic 
 * (addition, subtraction, scaling) and provides essential linear algebra utility 
 * functions such as dot products, cross products, and vector normalization.
 */
class Vec3 {
public:
    double e[3];

    Vec3() : e{0,0,0} {}
    Vec3(double e0, double e1, double e2) : e{e0, e1, e2} {}
    ~Vec3() = default;

    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }

    // Operator overloading
    Vec3 operator-() const { return Vec3(-e[0], -e[1], -e[2]); }
    double operator[](int i) const { return e[i]; }
    double& operator[](int i) { return e[i]; }

    Vec3& operator+=(const Vec3 &v) {
        e[0] += v[0]; e[1] += v[1]; e[2] += v[2];
        return *this;
    }

    Vec3& operator*=(const double t) {
        e[0] *= t; e[1] *= t; e[2] *= t;
        return *this;
    }

    double length_squared() const {
        return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }

    double length() const {
        return std::sqrt(length_squared());
    }
};

using Point3 = Vec3;   // Position
using Color = Vec3;    // RGB color

// Vector operations
inline std::ostream& operator<<(std::ostream &out, const Vec3 &v) {
    return out << "(" << v[0] << "," << v[1] << "," << v[2] << ")";
}

inline Vec3 operator+(const Vec3 &u, const Vec3 &v) {
    return Vec3(u[0] + v[0], u[1] + v[1], u[2] + v[2]);
}

inline Vec3 operator-(const Vec3 &u, const Vec3 &v) {
    return Vec3(u[0] - v[0], u[1] - v[1], u[2] - v[2]);
}

inline Vec3 operator*(const Vec3 &u, const Vec3 &v) {
    return Vec3(u[0] * v[0], u[1] * v[1], u[2] * v[2]);
}

inline Vec3 operator*(double t, const Vec3 &v) {
    return Vec3(t*v[0], t*v[1], t*v[2]);
}

inline Vec3 operator*(const Vec3 &v, double t) {
    return t * v;
}

inline Vec3 operator/(Vec3 v, double t) {
    return (1/t) * v;
}

// Dot Product
inline double dot(const Vec3 &u, const Vec3 &v) {
    return u[0] * v[0]
         + u[1] * v[1]
         + u[2] * v[2];
}

// Cross Product
inline Vec3 cross(const Vec3 &u, const Vec3 &v) {
    return Vec3(u[1] * v[2] - u[2] * v[1],
                u[2] * v[0] - u[0] * v[2],
                u[0] * v[1] - u[1] * v[0]);
}

// Vector normalization
inline Vec3 unit_vector(Vec3 v) {
    return v / v.length();
}

