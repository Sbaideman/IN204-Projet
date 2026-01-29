#include <iostream>
#include "SceneXMLParser.hpp"

// Remove XML comments (<!-- ... -->)
std::string SceneXMLParser::removeComments(const std::string& xml) {
    std::string cleanXml = xml;
    size_t commentStart = cleanXml.find("<!--");
    
    while (commentStart != std::string::npos) {
        // Find comment end position
        size_t commentEnd = cleanXml.find("-->", commentStart);
        if (commentEnd == std::string::npos) {
            // No end tag, truncate to comment start position (defensive handling)
            cleanXml.erase(commentStart);
            break;
        }
        // Remove entire comment block (including <!-- and -->)
        cleanXml.erase(commentStart, commentEnd - commentStart + 3);
        // Continue to find next comment
        commentStart = cleanXml.find("<!--", commentStart);
    }
    
    return cleanXml;
}

// Parse attribute string into key-value pairs
AttrMap SceneXMLParser::parseAttributes(const std::string& attrStr) {
    AttrMap attrs;
    if (attrStr.empty()) return attrs;

    // Regex match "key="value"" format
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

// Process start tags (e.g. <object id="cube_01" type="cube">)
void SceneXMLParser::processStartTag(const std::string& tagContent) {
    // Split tag name and attributes
    size_t spacePos = tagContent.find_first_of(' ');
    std::string tagName = (spacePos == std::string::npos) ? tagContent : tagContent.substr(0, spacePos);
    
    // Extract attribute string
    std::string attrStr = (spacePos == std::string::npos) ? "" : tagContent.substr(spacePos + 1);
    AttrMap attrs = parseAttributes(attrStr);

    // Reset temporary state
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
    } else if (tagName == "material") { // Add: Process material start tag
        m_currentMaterial = MaterialObject();
        m_currentMaterial.type = attrs.count("type") ? attrs["type"] : "";
    }
}

// Process end tags (e.g. </object>)
void SceneXMLParser::processEndTag(const std::string& tagName) {
    if (tagName == "global_settings") {
        m_sceneData.global_settings = m_currentGlobal;
    } else if (tagName == "object") {
        m_sceneData.objects.push_back(m_currentObject);
        m_currentObject = SceneObject(); // Reset
    } else if (tagName == "camera") {
        m_sceneData.camera = m_currentCamera;
        m_currentCamera = Camera(); // Reset
    } else if (tagName == "material") { // Add: Associate to current object when ending material tag
        m_currentObject.material = m_currentMaterial;
        m_currentMaterial = MaterialObject(); // Reset temporary material
    }
    m_currentParentTag = ""; // Clear current parent tag
}

// Process self-closing tags (e.g. <position x="100" y="200" z="0"/>)
void SceneXMLParser::processSelfClosingTag(const std::string& tagContent) {
    if (m_currentParentTag.empty()) return;

    // Split sub-tag name and attributes
    size_t spacePos = tagContent.find_first_of(' ');
    std::string subTagName = (spacePos == std::string::npos) ? tagContent : tagContent.substr(0, spacePos);
    std::string attrStr = (spacePos == std::string::npos) ? "" : tagContent.substr(spacePos + 1);
    AttrMap attrs = parseAttributes(attrStr);

    // Store sub-attributes according to current parent tag
    // If current parent tag is material, store material sub-attributes
    if (m_currentParentTag == "material") {
        m_currentMaterial.properties[subTagName] = attrs;
    } else if (m_currentParentTag == "global_settings") {
        m_currentGlobal.properties[subTagName] = attrs;
    } else if (m_currentParentTag == "object") {
        m_currentObject.properties[subTagName] = attrs;
    } else if (m_currentParentTag == "camera") {
        m_currentCamera.properties[subTagName] = attrs;
    }
}

// Parse XML string
SceneData SceneXMLParser::parseString(const std::string& xmlContent) {
    // Reset parsing state
    m_sceneData = SceneData();
    m_currentParentTag = "";
    m_currentObject = SceneObject();
    m_currentCamera = Camera();
    m_currentGlobal = GlobalSettings();

    // Remove comments
    std::string cleanXml = removeComments(xmlContent);

    // Regex split tags and content (<xxx> or </xxx> or <xxx/>)
    std::regex tagRegex(R"(<([^>]+)>)");
    std::sregex_token_iterator it(cleanXml.begin(), cleanXml.end(), tagRegex, {-1, 0});
    std::sregex_token_iterator end;

    for (; it != end; ++it) {
        std::string part = (*it).str();
        // Only trim leading and trailing whitespace, preserve middle spaces (separator between tag name and attributes)
        // Step 1: Remove leading whitespace
        part.erase(part.begin(), std::find_if(part.begin(), part.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        // Step 2: Remove trailing whitespace
        part.erase(std::find_if(part.rbegin(), part.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), part.end());
        if (part.empty()) continue;

        // Process start tag (<tag...>)
        if (part.starts_with("<") && !part.starts_with("</") && !part.ends_with("/>")) {
            std::string tagContent = part.substr(1, part.size() - 2); // Remove < >
            processStartTag(tagContent);
        }
        // Process end tag (</tag>)
        else if (part.starts_with("</") && part.ends_with(">")) {
            std::string tagName = part.substr(2, part.size() - 3); // Remove </ >
            processEndTag(tagName);
        }
        // Process self-closing tag (<tag.../>)
        else if (part.starts_with("<") && part.ends_with("/>")) {
            std::string tagContent = part.substr(1, part.size() - 3); // Remove < />
            processSelfClosingTag(tagContent);
        }
    }

    return m_sceneData;
}

// Parse XML file
SceneData SceneXMLParser::parseFile(const std::string& filePath) {
    // Read file content
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open XML file: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    // Parse string
    return parseString(buffer.str());
}