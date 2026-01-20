#pragma once
#include "Utils.hpp"


class Material;

/**
 * @struct HitRecord
 * @brief A structure to store detailed information about a ray-object intersection.
 * 
 * When a ray hits an object, we need to know:
 * 1. Where it hit (p).
 * 2. The surface normal at that point (normal) for lighting calc.
 * 3. The distance along the ray (t).
 * 4. Whether the hit was on the front or back face.
 * 5. The material of the hit point
 */
struct HitRecord {
    Point3 p;                     // The intersection point in 3D space
    Vec3 normal;                  // The surface normal vector at point p
    double t;                     // The ray parameter t where intersection occurred
    bool front_face;              // True if ray hits the outside surface, False if inside
    shared_ptr<Material> mat_ptr; // Material of the hit point

    /**
     * @brief Determines the face normal direction.
     * Ensure the normal always points against the ray.
     * If ray and normal face the same way, the ray is inside the object.
     * 
     * @param r The ray who hits the object
     * @param outward_normal The surface normal at the hitting point who points outside the object
     */
    inline void set_face_normal(const Ray& r, const Vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};


/**
 * @class SceneBaseObject
 * @brief Abstract Base Class for all geometric objects in the scene.
 * 
 * This class defines the interface that all specific shapes (Sphere, Cube, Plane)
 * must implement. It uses pure virtual functions to enforce this contract.
 */
class SceneBaseObject {
public:
    virtual ~SceneBaseObject() = default;

    /**
     * @brief Determines if a ray hits this object.
     * 
     * @param r The ray being cast.
     * @param t_min The minimum valid distance (usually close to 0).
     * @param t_max The maximum valid distance (usually infinity).
     * @param rec Reference to a HitRecord to store result data.
     * @return true if the ray hits the object, false otherwise.
     */
    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const = 0;
};