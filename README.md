# Object-Oriented Ray Tracer

## 1. Project Analysis

### 1.1 Project Description
This project is a C++ library and application designed to generate photorealistic images using **Path Tracing** techniques. It simulates the physical propagation of light in an environment—including reflection, refraction, and global illumination—to create high-quality 3D renderings.

The core objective is to construct a modular and extensible architecture based on **Object-Oriented Programming (OOP)** principles. Users can define 3D scenes via XML configuration files, specifying geometry, material properties, and camera settings. To handle the high computational load of ray tracing, the engine utilizes multi-threading optimizations tailored for modern multi-core processors.

### 1.2 Project File Structure
*   `src/`: Contains source code files (`.cpp`).
    *   `main.cpp`: Entry point and workflow control.
    *   `RenderUtils.cpp`: Core rendering logic and scene conversion factory.
    *   `SceneXMLParser.cpp`: Implementation of XML parsing logic.
*   `include/`: Contains header files (`.hpp`).
    *   `Object.hpp`: Defines concrete geometric shapes (Sphere, Plane, Parallelepiped).
    *   `Material.hpp`: Defines material behaviors (Matte, Metal, Glass, Light).
    *   `Scene.hpp`: Scene container responsible for managing object lists.
    *   `Vec3.hpp`: Vector mathematics library.
    *   `RenderUtils.hpp` & `SceneXMLParser.hpp`: Auxiliary interface definitions.
*   `scene/`: Contains XML scene configuration files.

### 1.3 Implemented Features
**Core Rendering Engine:**
*   **Path Tracing Algorithm:** Implements recursive ray tracing using Monte Carlo integration to solve anti-aliasing and soft shadow problems.
*   **Gamma Correction:** Applies Gamma 2.0 correction to ensure accurate color output.

**Geometry Support:**
*   **Basic Primitives:** Spheres, Infinite Planes.
*   **Complex Shapes:** Parallelepipeds (Boxes), constructed by combining multiple quadrilaterals.
*   **Scene Management:** Uses the Composite Pattern to manage complex scenes containing multiple objects.

**Materials & Optics:**
*   **Matte (Lambertian):** Simulates rough diffuse surfaces.
*   **Metal:** Simulates specular reflection with adjustable fuzziness.
*   **Glass (Dielectric):** Simulates transparent media, implementing Snell's Law and the Fresnel effect (Schlick's approximation).
*   **Emissive Lights:** Supports volumetric area lights to illuminate the scene.

**System Architecture:**
*   **XML Scene Parser:** Custom parser to load scene configurations, camera settings, and global settings from XML files.
*   **Multi-threading Acceleration:** Implements a **Block-based Round-Robin** scheduling strategy to balance the load across CPU cores.
*   **Image Output:** Generates images in PPM format.

### 1.4 Usage of Object-Oriented Concepts
This project deeply applies OOP concepts to ensure modularity and maintainability:

*   **Polymorphism & Inheritance (Mandatory):**
    *   `SceneBaseObject`: Abstract base class for all geometries. It defines a unified `hit()` interface.
    *   `Material`: Abstract base class for surface properties. Virtual functions `scatter()` and `emit()` are overridden by concrete material classes to implement polymorphic behavior.
*   **Smart Pointers (Mandatory):**
    *   Extensive usage of `std::shared_ptr` within the scene graph (`Scene`) to automatically manage memory and prevent leaks.
*   **STL Containers (Mandatory):**
    *   Uses `std::vector` to manage object lists and pixel buffer data.
*   **Encapsulation (Mandatory):**
    *   Classes like `Ray`, `Vec3`, and `HitRecord` encapsulate low-level mathematical operations and state data.
*   **Parallelism (Bonus):**
    *   Utilizes `std::thread` and `std::atomic` to achieve multi-core parallel rendering.
*   **Exception Handling (Bonus):**
    *   The XML parsing module uses `try-catch` blocks to capture and gracefully handle format errors.

---

## 2. Compilation & Execution

This project is built using **CMake**. Run the following commands from the project root directory:

**1. Build:**
```bash
# For Windows
mkdir build 
cd build
cmake ..
cmake --build . --config Release
```

**2. Run:** \
Ensure the ../scene/scene_layout.xml file exists relative to the executable (this is the default configuration), then run:
```bash
./main
```

**3. View Results:** \
Upon completion, a test.ppm image file will be generated in the build directory. You can view it using any PPM-compatible viewer (e.g., IrfanView, GIMP, or VS Code extensions).