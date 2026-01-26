#pragma once
#include "Utils.hpp"
#include "SceneBaseObject.hpp"

// Forward Declaration
struct HitRecord;

/**
 * @class Material
 * @brief Abstract Base Class for all material types.
 * 
 * Defines the `scatter` function, which determines how a ray interacts
 * with a surface. It computes the bounced (scattered) ray and how much
 * the light is attenuated (reduced in color/intensity).
 */
class Material {
public:
    virtual ~Material() = default;

    /**
     * @brief Computes the amount of light emitted by the material at a specific point.
     * 
     * By default, materials are non-emissive and return black (0,0,0). 
     * This function is overridden by light source materials (e.g., PointLight) 
     * to return their intrinsic color/intensity.
     * 
     * @param p The geometric point on the surface where the emission is calculated.
     * @return The color (radiance) of the light emitted.
     */
    virtual Color emit(const Point3& p) const {
        return Color(0,0,0);
    }

    /**
     * @brief Computes the scattered ray after a hit.
     * @param r_in The incoming ray.
     * @param rec The HitRecord of the intersection.
     * @param attenuate_color The color attenuation of the material.
     * @param r_out The resulting scattered ray.
     * @return true if the ray was scattered; false if it was absorbed.
     */
    virtual bool scatter(
        const Ray& r_in, const HitRecord& rec, Color& attenuate_color, Ray& r_out
    ) const = 0;
};


/**
 * @class Matte
 * @brief A diffuse (matte) material.
 * 
 * Light that hits a Matte surface scatters uniformly in a random direction.
 * The color of the surface is determined by its albedo.
 */
class Matte : public Material {
public:
    Color albedo; // The base color of the material

    Matte(const Color& a) : albedo(a) {}

    virtual bool scatter(
        const Ray& r_in, const HitRecord& rec, Color& attenuate_color, Ray& r_out
    ) const {
        // Calculate a random scatter direction
        auto scatter_direction = rec.normal + random_unit_vector();

        // Catch degenerate scatter direction
        // If the random unit vector we generate is exactly opposite the normal, the sum will be zero.
        if (scatter_direction.length_squared() < 1e-8)
            scatter_direction = rec.normal;
            
        r_out = Ray(rec.p, scatter_direction);
        attenuate_color = albedo; // The ray's color is attenuated by the material's albedo
        return true; // A diffuse material always scatters
    }

private:
    // Helper function to generate a random vector on the surface of a unit sphere
    static Vec3 random_unit_vector() {
        // Keep generating random points inside a cube until we find one inside the sphere
        while (true) {
            auto p = Vec3(random_double(-1,1), random_double(-1,1), random_double(-1,1));
            if (p.length_squared() >= 1) continue; // If outside the sphere, try again
            return unit_vector(p); // Normalize to get a point on the surface
        }
    }
};


/**
 * @class Metal
 * @brief A reflective (specular) material.
 * 
 * Simulates polished or fuzzy metal surfaces. The reflection can be perturbed
 * by a "fuzz" factor to create a blurred reflection effect.
 */
class Metal : public Material {
public:
    Color albedo;  // The base color of the material
    double fuzz;   // fuzz effect (0~1)

    Metal(const Color& a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}

    virtual bool scatter(
        const Ray& r_in, const HitRecord& rec, Color& attenuate_color, Ray& r_out
    ) const {
        // 1. Calculate the reflection vector
        Vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        
        // 2. Add a blur effect
        r_out = Ray(rec.p, reflected + fuzz * random_in_unit_sphere());
        
        attenuate_color = albedo;
        
        // 3. Check if the reflected light is effective
        // If the blurred light enters the object, then the light will be absorbed.
        return (dot(r_out.direction(), rec.normal) > 0);
    }

private:
    static Vec3 random_in_unit_sphere() {
        while (true) {
            auto p = Vec3(random_double(-1,1), random_double(-1,1), random_double(-1,1));
            if (p.length_squared() >= 1) continue;
            return p;
        }
    }
};


/**
 * @class Glass
 * @brief A transparent material that refracts and reflects light (e.g., glass, water).
 *
 * This material uses Snell's Law for refraction and Schlick's approximation
 * for reflectance to decide whether a ray refracts or reflects. It also handles
 * total internal reflection.
 */
class Glass : public Material {
public:
    double ir; // Index of refraction of glass 

    Glass(double index_of_refraction) : ir(index_of_refraction) {}

    virtual bool scatter(
        const Ray& r_in, const HitRecord& rec, Color& attenuate_color, Ray& r_out
    ) const {
        attenuate_color = Color(1.0, 1.0, 1.0); // Glass does not absorb color
        double refraction_ratio = rec.front_face ? (1.0/ir) : ir;

        Vec3 unit_direction = unit_vector(r_in.direction());
        
        // Verify the total internal reflection
        double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = sqrt(1.0 - cos_theta*cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        Vec3 direction;

        // Schlick approximation of reflection probability
        if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double()) {
            // Total internal reflection
            direction = reflect(unit_direction, rec.normal);
        } else {
            // Total refraction of light
            direction = refract(unit_direction, rec.normal, refraction_ratio);
        }

        r_out = Ray(rec.p, direction);
        return true;
    }

private:
    // Schlick's approximation for reflectance.
    static double reflectance(double cosine, double ref_idx) {
        auto r0 = (1-ref_idx) / (1+ref_idx);
        r0 = r0*r0;
        return r0 + (1-r0)*pow((1 - cosine),5);
    }
};


/**
 * @class PointLight
 * @brief A material that emits light.
 * 
 * Unlike other materials, it does NOT scatter rays (it absorbs them or passes them through).
 * Instead, it adds light energy to the ray path.
 */
class PointLight : public Material {
public:
    Color emit_color;

    PointLight(Color c) : emit_color(c) {}

    // Scattering: The simplified light source does not reflect light.
    virtual bool scatter(
        const Ray& r_in, const HitRecord& rec, Color& attenuation, Ray& scattered
    ) const {
        return false;
    }

    // Emission: Returns the color of the light source
    virtual Color emit(const Point3& p) const {
        return emit_color;
    }
};