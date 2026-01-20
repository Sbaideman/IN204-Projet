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
        // 计算分母：光线方向与平面法线的点积
        auto denom = dot(r.direction(), normal);

        // 如果分母接近 0，说明光线与平面平行，没有交点
        if (std::abs(denom) < 1e-6) return false;

        // 计算 t
        // 公式推导：(Origin + t*Dir - Point) . Normal = 0
        // t = (Point - Origin) . Normal / (Dir . Normal)
        auto t = dot(point - r.origin(), normal) / denom;

        // 检查 t 是否在有效范围内
        if (t < t_min || t > t_max) return false;

        // 记录信息
        rec.t = t;
        rec.p = r.at(t);
        
        // 平面是双面的，我们需要确定光线是打在正面还是反面
        // set_face_normal 会自动处理这个问题
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
        // 1. 计算法线 n = u x v
        auto n = cross(u, v);
        normal = unit_vector(n);
        
        // 2. 平面方程参数 Ax + By + Cz = D
        D = dot(normal, Q);
        
        // 3. 预计算常数向量 w，用于后续求 alpha 和 beta
        // 这是一个数学技巧，用于快速解平面坐标
        w = n / dot(n, n);
    }

    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const override {
        auto denom = dot(normal, r.direction());

        // 1. 检查光线是否平行于平面
        if (std::abs(denom) < 1e-8) return false;

        // 2. 计算交点 t
        auto t = (D - dot(normal, r.origin())) / denom;
        if (t < t_min || t > t_max) return false;

        // 3. 计算交点 p
        auto intersection = r.at(t);
        
        // 4. 判断点是否在四边形内部 (核心逻辑)
        // 我们需要找到 alpha 和 beta，使得 p = Q + alpha*u + beta*v
        Vec3 planar_hitpt_vector = intersection - Q;
        
        // 利用预计算的 w 快速求解
        auto alpha = dot(w, cross(planar_hitpt_vector, v));
        auto beta = dot(w, cross(u, planar_hitpt_vector));

        // 检查范围
        if (alpha < 0 || alpha > 1 || beta < 0 || beta > 1) return false;

        // 5. 命中！记录数据
        rec.t = t;
        rec.p = intersection;
        rec.mat_ptr = mat_ptr;
        rec.set_face_normal(r, normal);

        return true;
    }
};