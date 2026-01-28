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


/**
 * @class Plane
 * @brief A concrete implementation of a SceneBaseObject representing an infinitive plane.
 * 
 * An infinitive plane is defined by the following equation: 
 *                      dot((P-Q), n) = 0
 * where:
 * P is a fix point on the plane,
 * n is a normal of the plane.
 */
class Plane : public SceneBaseObject {
public:
    Point3 point; // A fix point on the plane
    Vec3 normal;  // A normal of the plane
    shared_ptr<Material> mat_ptr; // The material of the plane

    Plane() {}
    Plane(Point3 p, Vec3 n, shared_ptr<Material> m) 
        : point(p), normal(unit_vector(n)), mat_ptr(m) {}

    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const override {
        // Denominator: Dot product of ray direction and plane normal
        auto denom = dot(r.direction(), normal);

        // Denominator close to 0 => Ray is parallel to the plane
        if (std::abs(denom) < 1e-6) return false;

        // Calculate t
        // (Origin + t*Dir - Point) . Normal = 0
        // t = (Point - Origin) . Normal / (Dir . Normal)
        auto t = dot(point - r.origin(), normal) / denom;

        if (t < t_min || t > t_max) return false;

        // Record information
        rec.t = t;
        rec.p = r.at(t);
        
        // Determine whether the light hits the front or the back of the plane.
        rec.set_face_normal(r, normal);
        rec.mat_ptr = mat_ptr;

        return true;
    }
};


/**
 * @class Parallelogram
 * @brief A concrete implementation of a SceneBaseObject representing a parallelogram.
 * 
 * A parallelogram is defined by the following equation: 
 *                      Q = P + α u + β v
 * where:
 * P is a vertex of the parallelogram,
 * u,v are the edge vectors of the parallelogram.
 * α,β are the parameters (0 ≤ α,β ≤ 1)
 */
class Parallelogram : public SceneBaseObject {
public:
    Point3 Q; // A vertex
    Vec3 u, v; // The edge vectors
    shared_ptr<Material> mat_ptr;
    
    // Pre-computed constants to optimize ray tracing computation performance.
    Vec3 normal;
    double D;
    Vec3 w; 

    Parallelogram(const Point3& _Q, const Vec3& _u, const Vec3& _v, shared_ptr<Material> m)
        : Q(_Q), u(_u), v(_v), mat_ptr(m)
    {
        // 1. Calculate the normal: n = u x v
        auto n = cross(u, v);
        normal = unit_vector(n);
        
        // 2. Plane equation parameters Ax + By + Cz = D
        D = dot(normal, Q);
        
        // 3. Pre-calculate the constant vector w, which will be used to calculate alpha and beta later.
        w = n / dot(n, n);
    }

    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const override {
        auto denom = dot(normal, r.direction());

        // 1. Check if the light rays are parallel to the plane.
        if (std::abs(denom) < 1e-8) return false;

        // 2. Calculate the intersection time t
        auto t = (D - dot(normal, r.origin())) / denom;
        if (t < t_min || t > t_max) return false;

        // 3. Calculate the intersection point p
        auto intersection = r.at(t);
        
        // 4. Determine if a point is inside a quadrilateral
        // Find alpha and beta such that p = Q + alpha*u + beta*v
        Vec3 planar_hitpt_vector = intersection - Q;
        auto alpha = dot(w, cross(planar_hitpt_vector, v));
        auto beta = dot(w, cross(u, planar_hitpt_vector));

        // Check the range of alpha and beta
        if (alpha < 0 || alpha > 1 || beta < 0 || beta > 1) return false;

        // 5. Hit! Record data
        rec.t = t;
        rec.p = intersection;
        rec.mat_ptr = mat_ptr;
        rec.set_face_normal(r, normal);

        return true;
    }
};