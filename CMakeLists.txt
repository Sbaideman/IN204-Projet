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

# 步骤1：调用 fltk-config 获取真实版本号
execute_process(COMMAND ${FLTK_CONFIG} --version OUTPUT_VARIABLE FLTK_REAL_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${FLTK_CONFIG} --includedir OUTPUT_VARIABLE FLTK_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${FLTK_CONFIG} --cxxflags OUTPUT_VARIABLE FLTK_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
# 启用FLTK图片/本地文件选择器组件（必须加，否则找不到Fl_Native_File_Chooser）
execute_process(COMMAND ${FLTK_CONFIG} --ldflags --use-images --use-nativefilechooser OUTPUT_VARIABLE FLTK_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)

# 步骤2：验证版本（≥1.3.0）
if(FLTK_REAL_VERSION VERSION_LESS "1.3.0")
    message(FATAL_ERROR "FLTK version must be ≥ 1.3.0, found ${FLTK_REAL_VERSION}")
else()
    message(STATUS "Found FLTK version: ${FLTK_REAL_VERSION} (OK)")
endif()

# 3. 头文件目录（告诉CMake去哪里找#include的文件）
include_directories(
    ${PROJECT_SOURCE_DIR}/include  # 原有自定义头文件目录
    ${PNG_INCLUDE_DIR}             # 新增：libpng的头文件目录（解决png.h找不到）
    ${FLTK_INCLUDE_DIRS}
)

# 4. 收集要编译的源文件（src目录下的.cpp）
# 如果你有更多源文件，直接加在后面即可，如 "src/OtherFile.cpp"
file(GLOB SOURCES 
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

# 5. 生成可执行文件（最终生成的程序名：main，可自定义）
add_executable(main ${SOURCES})

# # 1. 应用FLTK编译选项（解决FLTK头文件宏定义缺失）
# target_compile_options(main PRIVATE ${FLTK_CXXFLAGS_LIST})

# # 2. 链接FLTK + libpng（核心：解决Undefined symbols）
# separate_arguments(FLTK_LDFLAGS_LIST UNIX_COMMAND "${FLTK_LDFLAGS}")
# target_link_libraries(main PRIVATE
#     ${PNG_LIBRARIES}       # libpng库
#     ${FLTK_LDFLAGS_LIST}   # FLTK核心库（包含fltk、fltk_images、fltk_nativefilechooser）
# )

# ========== 可选：macOS下适配brew安装的libpng（兜底） ==========
if(APPLE)
    # 强制指定brew安装的libpng路径（防止CMake识别不到）
    set(PNG_INCLUDE_DIR "/opt/homebrew/include" CACHE PATH "libpng include dir")
    set(PNG_LIBRARY "/opt/homebrew/lib/libpng.dylib" CACHE FILEPATH "libpng library")
    target_include_directories(main PRIVATE ${PNG_INCLUDE_DIR})
    target_link_libraries(main PRIVATE ${PNG_LIBRARY})

    # 手动指定FLTK头文件路径
    set(FLTK_INCLUDE_DIR "/opt/homebrew/include")
    # 手动指定FLTK链接参数（无逗号问题）
    set(FLTK_LDFLAGS "-L/opt/homebrew/lib -lfltk -lfltk_images -lfltk_nativefilechooser")

endif()

# 应用编译选项（直接写死，避免解析错误）
target_compile_options(main PRIVATE -I/opt/homebrew/include -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE)
# 链接FLTK（手动指定库）
target_link_libraries(main PRIVATE
    -L/opt/homebrew/lib -lfltk -lfltk_images
)