#include <iostream>
#include "SceneXMLParser.hpp"

// 移除XML注释（<!-- ... -->）
// std::string SceneXMLParser::removeComments(const std::string& xml) {
//     // std::regex commentRegex(R"(<!--.*?-->)", std::regex::dotall); // macOS 对 dotall 支持不完善
//     // std::regex commentRegex(R"((?s)<!--.*?-->)");
//     return std::regex_replace(xml, commentRegex, "");
// }
std::string SceneXMLParser::removeComments(const std::string& xml) { // 折中方案
    std::string cleanXml = xml;
    size_t commentStart = cleanXml.find("<!--");
    
    while (commentStart != std::string::npos) {
        // 查找注释结束位置
        size_t commentEnd = cleanXml.find("-->", commentStart);
        if (commentEnd == std::string::npos) {
            // 无结束标签，截断到注释开始位置（防御性处理）
            cleanXml.erase(commentStart);
            break;
        }
        // 移除整个注释块（包括 <!-- 和 -->）
        cleanXml.erase(commentStart, commentEnd - commentStart + 3);
        // 继续查找下一个注释
        commentStart = cleanXml.find("<!--", commentStart);
    }
    
    return cleanXml;
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

    // std::cout << "[parseAttributes] 原始属性字符串：" << attrStr << std::endl;
    // std::cout << "[parseAttributes] 解析结果（键=值）：" << std::endl;
    // for (const auto& pair : attrs) {
    //     std::cout << "  - " << pair.first << " = " << pair.second << std::endl;
    // }
    // std::cout << "----------------------------------------" << std::endl;
    // // =====================================================

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
        // 只删首尾空白，保留中间空格（标签名和属性的分隔）
        // 第一步：删除开头空白
        part.erase(part.begin(), std::find_if(part.begin(), part.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        // 第二步：删除结尾空白
        part.erase(std::find_if(part.rbegin(), part.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), part.end());
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