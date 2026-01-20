#include "SceneXMLParser.hpp"

// 移除XML注释（<!-- ... -->）
std::string SceneXMLParser::removeComments(const std::string& xml) {
    // std::regex commentRegex(R"(<!--.*?-->)", std::regex::dotall); // macOS 对 dotall 支持不完善
    std::regex commentRegex(R"((?s)<!--.*?-->)");
    return std::regex_replace(xml, commentRegex, "");
}

// 解析属性字符串为键值对
AttrMap SceneXMLParser::parseAttributes(const std::string& attrStr) {
    AttrMap attrs;
    if (attrStr.empty()) return attrs;

    // 正则匹配 "key="value"" 格式
    std::regex attrRegex(R"((\w+)\s*=\s*["']([^"']*)["'])");
    std::sregex_iterator it(attrStr.begin(), attrStr.end(), attrRegex);
    std::sregex_iterator end;

    for (; it != end; ++it) {
        std::string key = (*it)[1].str();
        std::string value = (*it)[2].str();
        attrs[key] = value;
    }
    return attrs;
}

// 处理开始标签（如 <object id="cube_01" type="cube">）
void SceneXMLParser::processStartTag(const std::string& tagContent) {
    // 分割标签名和属性
    size_t spacePos = tagContent.find_first_of(' ');
    std::string tagName = (spacePos == std::string::npos) ? tagContent : tagContent.substr(0, spacePos);
    
    // 提取属性字符串
    std::string attrStr = (spacePos == std::string::npos) ? "" : tagContent.substr(spacePos + 1);
    AttrMap attrs = parseAttributes(attrStr);

    // 重置临时状态
    m_currentParentTag = tagName;
    if (tagName == "global_settings") {
        m_currentGlobal = GlobalSettings();
    } else if (tagName == "object") {
        m_currentObject = SceneObject();
        m_currentObject.id = attrs.count("id") ? attrs["id"] : "";
        m_currentObject.type = attrs.count("type") ? attrs["type"] : "";
    } else if (tagName == "camera") {
        m_currentCamera = Camera();
        m_currentCamera.id = attrs.count("id") ? attrs["id"] : "";
        m_currentCamera.type = attrs.count("type") ? attrs["type"] : "";
    }
}

// 处理结束标签（如 </object>）
void SceneXMLParser::processEndTag(const std::string& tagName) {
    if (tagName == "global_settings") {
        m_sceneData.global_settings = m_currentGlobal;
    } else if (tagName == "object") {
        m_sceneData.objects.push_back(m_currentObject);
        m_currentObject = SceneObject(); // 重置
    } else if (tagName == "camera") {
        m_sceneData.camera = m_currentCamera;
        m_currentCamera = Camera(); // 重置
    }
    m_currentParentTag = ""; // 清空当前父标签
}

// 处理自闭合标签（如 <position x="100" y="200" z="0"/>）
void SceneXMLParser::processSelfClosingTag(const std::string& tagContent) {
    if (m_currentParentTag.empty()) return;

    // 分割子标签名和属性
    size_t spacePos = tagContent.find_first_of(' ');
    std::string subTagName = (spacePos == std::string::npos) ? tagContent : tagContent.substr(0, spacePos);
    std::string attrStr = (spacePos == std::string::npos) ? "" : tagContent.substr(spacePos + 1);
    AttrMap attrs = parseAttributes(attrStr);

    // 根据当前父标签存储子属性
    if (m_currentParentTag == "global_settings") {
        m_currentGlobal.properties[subTagName] = attrs;
    } else if (m_currentParentTag == "object") {
        m_currentObject.properties[subTagName] = attrs;
    } else if (m_currentParentTag == "camera") {
        m_currentCamera.properties[subTagName] = attrs;
    }
}

// 解析XML字符串
SceneData SceneXMLParser::parseString(const std::string& xmlContent) {
    // 重置解析状态
    m_sceneData = SceneData();
    m_currentParentTag = "";
    m_currentObject = SceneObject();
    m_currentCamera = Camera();
    m_currentGlobal = GlobalSettings();

    // 移除注释
    std::string cleanXml = removeComments(xmlContent);

    // 正则分割标签和内容（<xxx> 或 </xxx> 或 <xxx/>）
    std::regex tagRegex(R"(<([^>]+)>)");
    std::sregex_token_iterator it(cleanXml.begin(), cleanXml.end(), tagRegex, {-1, 0});
    std::sregex_token_iterator end;

    for (; it != end; ++it) {
        std::string part = (*it).str();
        // 去除空白字符
        part.erase(std::remove_if(part.begin(), part.end(), ::isspace), part.end());
        if (part.empty()) continue;

        // 处理开始标签（<tag...>）
        if (part.starts_with("<") && !part.starts_with("</") && !part.ends_with("/>")) {
            std::string tagContent = part.substr(1, part.size() - 2); // 去掉 < >
            processStartTag(tagContent);
        }
        // 处理结束标签（</tag>）
        else if (part.starts_with("</") && part.ends_with(">")) {
            std::string tagName = part.substr(2, part.size() - 3); // 去掉 </ >
            processEndTag(tagName);
        }
        // 处理自闭合标签（<tag.../>）
        else if (part.starts_with("<") && part.ends_with("/>")) {
            std::string tagContent = part.substr(1, part.size() - 3); // 去掉 < />
            processSelfClosingTag(tagContent);
        }
    }

    return m_sceneData;
}

// 解析XML文件
SceneData SceneXMLParser::parseFile(const std::string& filePath) {
    // 读取文件内容
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open XML file: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    // 解析字符串
    return parseString(buffer.str());
}