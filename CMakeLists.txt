# 1. 指定CMake最低版本（兼容大部分系统）
cmake_minimum_required(VERSION 3.10)

# 2. 项目名称 + 指定C++标准
project(SceneXMLParser_Project LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)        
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)    

# ========== 核心修复：手动指定libomp路径（适配你的brew安装目录） ==========
if(APPLE)
    # 从brew list libomp的输出确认的真实路径
    set(OPENMP_INCLUDE_DIR "/opt/homebrew/Cellar/libomp/21.1.8/include")
    set(OPENMP_LIB_DIR "/opt/homebrew/Cellar/libomp/21.1.8/lib")
    
    # Clang编译OpenMP的专属选项（必须加-Xpreprocessor）
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xpreprocessor -fopenmp -I${OPENMP_INCLUDE_DIR}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${OPENMP_LIB_DIR} -lomp")
    
    # 标记OpenMP已找到，避免后续依赖检查报错
    set(OpenMP_FOUND TRUE)
    set(OpenMP_CXX_LIBRARIES "${OPENMP_LIB_DIR}/libomp.dylib")
else()
    # 非Mac系统仍用自动查找（兼容Linux）
    find_package(OpenMP REQUIRED)
    if(OpenMP_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    endif()
endif()

# ========== 原有libpng配置（不变） ==========
find_package(PNG REQUIRED)
if(NOT PNG_FOUND)
    message(FATAL_ERROR "libpng 未找到！请先安装：brew install libpng (macOS) 或 sudo apt install libpng-dev (Linux)")
endif()

# ========== 原有FLTK配置（补全路径，不变） ==========
if(APPLE AND NOT DEFINED FLTK_CONFIG)
    set(FLTK_CONFIG "/opt/homebrew/bin/fltk-config")
endif()
execute_process(COMMAND ${FLTK_CONFIG} --version OUTPUT_VARIABLE FLTK_REAL_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${FLTK_CONFIG} --includedir OUTPUT_VARIABLE FLTK_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${FLTK_CONFIG} --cxxflags OUTPUT_VARIABLE FLTK_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${FLTK_CONFIG} --ldflags --use-images --use-nativefilechooser OUTPUT_VARIABLE FLTK_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)

if(FLTK_REAL_VERSION VERSION_LESS "1.3.0")
    message(FATAL_ERROR "FLTK version must be ≥ 1.3.0, found ${FLTK_REAL_VERSION}")
else()
    message(STATUS "Found FLTK version: ${FLTK_REAL_VERSION} (OK)")
endif()

# 3. 头文件目录（新增OpenMP头文件路径）
include_directories(
    ${PROJECT_SOURCE_DIR}/include  
    ${PNG_INCLUDE_DIR}             
    ${FLTK_INCLUDE_DIRS}
    ${OPENMP_INCLUDE_DIR}          # 关键：添加libomp的头文件目录
)

# 4. 收集源文件（不变）
file(GLOB SOURCES 
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

# 5. 生成可执行文件（不变）
add_executable(main ${SOURCES})

# ========== macOS下适配libpng/FLTK（不变） ==========
if(APPLE)
    set(PNG_INCLUDE_DIR "/opt/homebrew/include" CACHE PATH "libpng include dir")
    set(PNG_LIBRARY "/opt/homebrew/lib/libpng.dylib" CACHE FILEPATH "libpng library")
    target_include_directories(main PRIVATE ${PNG_INCLUDE_DIR})
    target_link_libraries(main PRIVATE ${PNG_LIBRARY})

    set(FLTK_INCLUDE_DIR "/opt/homebrew/include")
    set(FLTK_LDFLAGS "-L/opt/homebrew/lib -lfltk -lfltk_images -lfltk_nativefilechooser")
endif()

# 应用编译选项（不变）
target_compile_options(main PRIVATE -I/opt/homebrew/include -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE)

# 链接库（补全OpenMP）
target_link_libraries(main PRIVATE
    -L/opt/homebrew/lib -lfltk -lfltk_images
    ${PNG_LIBRARIES}
    ${OpenMP_CXX_LIBRARIES}  # 链接libomp库
)