#pragma once
#include "Material.hpp"

/**
 * @class Sphere
 * @brief A concrete implementation of a SceneBaseObject representing a 3D sphere.
 * 
 * Intersection is calculated using the analytic quadratic equation:
 *      (P(t) - Center) . (P(t) - Center) = Radius^2
 */
class Sphere : public SceneBaseObject {
public:
    Point3 center;                // Center coordinate of the sphere
    double radius;                // Radius of the sphere
    shared_ptr<Material> mat_ptr; // Material of the sphere

    Sphere() {}
    Sphere(Point3 cen, double r, shared_ptr<Material> m) : center(cen), radius(r), mat_ptr(m) {}
    ~Sphere() = default;

    /**
     * @brief Checks if a ray intersects this sphere.
     * Solves the quadratic discriminant (b^2 - 4ac) to find intersection points.
     */
    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const override {
        // 1. Setup the quadratic equation coefficients
        Vec3 oc = r.origin() - center;
        auto a = r.direction().length_squared();
        auto half_b = dot(oc, r.direction());
        auto c = oc.length_squared() - radius*radius;

        // 2. Calculate discriminant
        auto discriminant = half_b*half_b - a*c;
        if (discriminant < 0) return false; // No intersection (ray misses)

        auto sqrtd = sqrt(discriminant);

        // 3. Find the nearest root that lies in the acceptable range [t_min, t_max]
        auto root = (-half_b - sqrtd) / a;
        if (root < t_min || t_max < root) {
            root = (-half_b + sqrtd) / a;
            if (root < t_min || t_max < root)
                return false;
        }

        // 4. Update HitRecord with intersection details
        rec.t = root;
        rec.p = r.at(rec.t);
        
        // Calculate outward normal: (Point - Center) / Radius
        Vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.mat_ptr = mat_ptr;

        return true;
    }
};