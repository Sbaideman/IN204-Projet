#pragma once
#include <vector>
#include <memory>
#include "Material.hpp"

using std::shared_ptr;
using std::make_shared;

/**
 * @class Scene
 * @brief A container class that stores a list of objects.
 * 
 * This class itself inherits from SceneBaseObject. Why?
 * Because "a collection of objects" can be treated just like "a single object".
 * You can ask: "Did the ray hit anything in this list?"
 */
class Scene : public SceneBaseObject {
public:
    // A list of pointers to SceneBaseObjects
    std::vector<shared_ptr<SceneBaseObject>> objects;

    Scene() {}
    Scene(shared_ptr<SceneBaseObject> object) { add(object); }

    void clear() { objects.clear(); }
    void add(shared_ptr<SceneBaseObject> object) { objects.push_back(object); }

    /**
     * @brief Checks intersection with ALL objects in the list.
     * 
     * Key Logic:
     * We need to find the CLOSEST hit. So as we iterate through objects,
     * we shrink the 'closest_so_far' distance (t_max).
     */
    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const {
        HitRecord temp_rec;
        bool hit_anything = false;
        double closest_so_far = t_max;

        for (const auto& object : objects) {
            // Check hit with current closest distance constraint
            if (object->hit(r, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t; // Update the closest distance
                rec = temp_rec;              // Update the final record
            }
        }

        return hit_anything;
    }
};


/**
 * @class Parallelepiped
 * @brief A container class that stores a list of six parallelograms to form a parallelepiped.
 */
class Parallelepiped : public Scene {
public:
    // 构造函数
    // origin: 起点
    // u, v, w: 从起点发出的三条边向量
    Parallelepiped(const Point3& origin, const Vec3& u, const Vec3& v, const Vec3& w, shared_ptr<Material> m) {
        // 我们利用 Scene 类的 add 方法，把 6 个面加进去
        // 这样 Parallelepiped 本身就是一个包含 6 个 Parallelogram 的容器
        
        // 面 1 (前)
        add(make_shared<Parallelogram>(origin, u, v, m));
        // 面 2 (后) - 起点在 origin + w
        add(make_shared<Parallelogram>(origin + w, u, v, m));
        
        // 面 3 (顶) - 起点在 origin + v
        add(make_shared<Parallelogram>(origin + v, u, w, m));
        // 面 4 (底)
        add(make_shared<Parallelogram>(origin, u, w, m));
        
        // 面 5 (右) - 起点在 origin + u
        add(make_shared<Parallelogram>(origin + u, v, w, m));
        // 面 6 (左)
        add(make_shared<Parallelogram>(origin, v, w, m));
    }
};