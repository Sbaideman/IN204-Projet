# 1. 指定CMake最低版本（兼容大部分系统，可根据需要升级，如3.20）
cmake_minimum_required(VERSION 3.10)

# 2. 项目名称 + 指定C++标准（解析器建议至少C++11，按需升级到17/20）
project(SceneXMLParser_Project LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)        # 设置C++标准
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 强制使用指定的C++标准
set(CMAKE_BUILD_TYPE Release)     # 编译模式：Release（生产）/Debug（调试）

# ========== 新增：查找并配置libpng（核心解决png.h找不到） ==========
# 查找系统中的libpng库（会自动识别头文件和库文件路径）
find_package(PNG REQUIRED)
# 验证libpng是否找到（可选，便于排查）
if(NOT PNG_FOUND)
    message(FATAL_ERROR "libpng 未找到！请先安装：brew install libpng (macOS) 或 sudo apt install libpng-dev (Linux)")
endif()

# 3. 头文件目录（告诉CMake去哪里找#include的文件）
include_directories(
    ${PROJECT_SOURCE_DIR}/include  # 原有自定义头文件目录
    ${PNG_INCLUDE_DIR}             # 新增：libpng的头文件目录（解决png.h找不到）
)

# 4. 收集要编译的源文件（src目录下的.cpp）
# 如果你有更多源文件，直接加在后面即可，如 "src/OtherFile.cpp"
file(GLOB SOURCES 
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

# 5. 生成可执行文件（最终生成的程序名：main，可自定义）
add_executable(main ${SOURCES})

# ========== 新增：链接libpng库（编译时找到png相关函数） ==========
target_link_libraries(main PRIVATE ${PNG_LIBRARIES})

# ========== 可选：macOS下适配brew安装的libpng（兜底） ==========
if(APPLE)
    # 强制指定brew安装的libpng路径（防止CMake识别不到）
    set(PNG_INCLUDE_DIR "/opt/homebrew/include" CACHE PATH "libpng include dir")
    set(PNG_LIBRARY "/opt/homebrew/lib/libpng.dylib" CACHE FILEPATH "libpng library")
    target_include_directories(main PRIVATE ${PNG_INCLUDE_DIR})
    target_link_libraries(main PRIVATE ${PNG_LIBRARY})
endif()

# # 6. 【可选但推荐】复制XML文件到编译目录（避免运行时找不到文件）
# # 作用：编译后，scene_layout.xml会被复制到可执行文件同目录，直接运行即可
# configure_file(
#     ${PROJECT_SOURCE_DIR}/scene_layout.xml
#     ${CMAKE_BINARY_DIR}/scene_layout.xml
#     COPYONLY  # 仅复制，不修改
# )

# 7. 【可选】如果你的解析器依赖第三方库（如libxml2等XML解析库），需添加以下配置
# 示例：如果用libxml2（大部分Linux/macOS自带，Windows需手动安装）
# find_package(PNG REQUIRED)
# target_include_directories(raytracer PRIVATE ${PNG_INCLUDE_DIR})
# target_link_libraries(raytracer PRIVATE ${PNG_LIBRARIES})