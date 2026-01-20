#ifndef SCENE_XML_PARSER_H
#define SCENE_XML_PARSER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <fstream>
#include <sstream>
#include <algorithm>

// 通用属性映射（key-value）
using AttrMap = std::unordered_map<std::string, std::string>;

// 材质结构体
struct MaterialObject {
    std::string type;          // 材质类型：matte/metal/glass
    std::unordered_map<std::string, AttrMap> properties; // 材质属性（color/ior/fuzz等）
};

// 场景物体结构体
struct SceneObject {
    std::string id;
    std::string type;
    std::unordered_map<std::string, AttrMap> properties; // position/size/color等子属性
    MaterialObject material;
};

// 相机结构体
struct Camera {
    std::string id;
    std::string type;
    std::unordered_map<std::string, AttrMap> properties; // position/look_at/fov等子属性
};

// 全局设置结构体
struct GlobalSettings {
    std::unordered_map<std::string, AttrMap> properties; // background_color/scene_size等子属性
};

// 场景数据总结构体
struct SceneData {
    GlobalSettings global_settings;
    std::vector<SceneObject> objects;
    Camera camera;
};

// XML解析器类
class SceneXMLParser {
public:
    // 解析XML文件
    SceneData parseFile(const std::string& filePath);
    // 解析XML字符串
    SceneData parseString(const std::string& xmlContent);

private:
    // 移除XML注释
    std::string removeComments(const std::string& xml);
    // 解析属性字符串（如 r="255" g="255" b="255"）
    AttrMap parseAttributes(const std::string& attrStr);
    // 处理开始标签（<tag ...>）
    void processStartTag(const std::string& tagContent);
    // 处理结束标签（</tag>）
    void processEndTag(const std::string& tagName);
    // 处理自闭合标签（<tag .../>）
    void processSelfClosingTag(const std::string& tagContent);

    // 临时状态变量
    SceneData m_sceneData;
    std::string m_currentParentTag; // 当前父标签（global_settings/object/camera）
    SceneObject m_currentObject;    // 临时存储当前解析的物体
    Camera m_currentCamera;         // 临时存储当前解析的相机
    GlobalSettings m_currentGlobal; // 临时存储当前解析的全局设置
    MaterialObject m_currentMaterial;      // 新增：临时存储当前解析的材质
};

#endif // SCENE_XML_PARSER_H