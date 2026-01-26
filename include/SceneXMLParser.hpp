#ifndef SCENE_XML_PARSER_H
#define SCENE_XML_PARSER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <fstream>
#include <sstream>
#include <algorithm>

// Generic property mapping (key-value)
using AttrMap = std::unordered_map<std::string, std::string>;
using NestedAttrMap = std::unordered_map<std::string, AttrMap>;

// Material Structure
struct MaterialObject {
    std::string type;          // Material type: matte/metal/glass
    NestedAttrMap properties;  // Material properties (color/ior/fuzz, etc.)
};

// Scene object structure
struct SceneObject {
    std::string id;
    std::string type;
    NestedAttrMap properties;  // Sub-properties such as position, size, and color
    MaterialObject material;
};

// Camera structure
struct Camera {
    std::string id;
    std::string type;
    NestedAttrMap properties;  // Sub-attributes such as position/look_at/fov
};

// Global settings structure
struct GlobalSettings {
    NestedAttrMap properties; // Sub-properties like background_color/scene_size
};

// Main structure holding all scene data
struct SceneData {
    GlobalSettings global_settings;
    std::vector<SceneObject> objects;
    Camera camera;
};

// XML Parser Class
class SceneXMLParser {
public:
    // Parses an XML file
    SceneData parseFile(const std::string& filePath);
    // Parses an XML string content
    SceneData parseString(const std::string& xmlContent);

private:
    // Removes XML comments
    std::string removeComments(const std::string& xml);
    // Parses attribute strings (e.g., r="255" g="255" b="255")
    AttrMap parseAttributes(const std::string& attrStr);
    // Processes start tags (<tag ...>)
    void processStartTag(const std::string& tagContent);
    // Processes end tags (</tag>)
    void processEndTag(const std::string& tagName);
    // Processes self-closing tags (<tag .../>)
    void processSelfClosingTag(const std::string& tagContent);

    // Temporary state variables
    SceneData m_sceneData;
    std::string m_currentParentTag; // Current parent tag (global_settings/object/camera)
    SceneObject m_currentObject;    // Temporarily stores the object currently being parsed
    Camera m_currentCamera;         // Temporarily stores the camera currently being parsed
    GlobalSettings m_currentGlobal; // Temporarily stores the global settings currently being parsed
    MaterialObject m_currentMaterial; // Temporarily stores the material currently being parsed
};

#endif // SCENE_XML_PARSER_H