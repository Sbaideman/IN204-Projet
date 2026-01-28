#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <png.h>
#include "SavePng.hpp"

/**
 * @brief 解析P3格式的PPM文件
 * @param filename PPM文件路径
 * @return 解析后的PPMImage结构体
 */
PPMImage parse_ppm(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        throw std::runtime_error("无法打开PPM文件: " + filename);
    }

    PPMImage img;
    std::string magic;
    infile >> magic;

    // 验证PPM格式（P3）
    if (magic != "P3") {
        throw std::runtime_error("不支持的PPM格式，仅支持P3文本格式");
    }

    // 读取宽、高、最大颜色值
    infile >> img.width >> img.height >> img.max_color;
    if (img.max_color != 255) {
        throw std::runtime_error("仅支持最大颜色值为255的PPM文件");
    }

    // 读取所有像素的RGB值
    img.pixels.resize(img.width * img.height * 3);
    int r, g, b;
    int idx = 0;
    while (infile >> r >> g >> b) {
        img.pixels[idx++] = static_cast<unsigned char>(r);
        img.pixels[idx++] = static_cast<unsigned char>(g);
        img.pixels[idx++] = static_cast<unsigned char>(b);
    }

    // 验证像素数量是否匹配
    if (idx != img.width * img.height * 3) {
        throw std::runtime_error("PPM文件像素数量与宽高不匹配");
    }

    infile.close();
    return img;
}

/**
 * @brief 将PPM图像数据写入PNG文件
 * @param img 解析后的PPMImage
 * @param filename 输出PNG文件路径
 */
void write_png(const PPMImage& img, const std::string& filename) {
    // 1. 创建PNG结构体
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        throw std::runtime_error("创建PNG写结构体失败");
    }

    // 2. 创建PNG信息结构体
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, nullptr);
        throw std::runtime_error("创建PNG信息结构体失败");
    }

    // 3. 设置错误处理（防止libpng崩溃）
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        throw std::runtime_error("PNG写入过程出错");
    }

    // 4. 打开输出文件
    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        throw std::runtime_error("无法创建PNG文件: " + filename);
    }

    // 5. 绑定文件指针到PNG结构体
    png_init_io(png_ptr, fp);

    // 6. 设置PNG图像信息（RGB格式、8位深度）
    png_set_IHDR(
        png_ptr,
        info_ptr,
        img.width,
        img.height,
        8,                      // 每个颜色通道8位
        PNG_COLOR_TYPE_RGB,     // RGB格式（无Alpha通道）
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png_ptr, info_ptr);

    // 7. 准备行指针（libpng要求按行写入）
    std::vector<png_bytep> row_pointers(img.height);
    for (int y = 0; y < img.height; ++y) {
        row_pointers[y] = const_cast<png_byte*>(&img.pixels[y * img.width * 3]);
    }

    // 8. 写入像素数据
    png_write_image(png_ptr, row_pointers.data());

    // 9. 结束写入并清理资源
    png_write_end(png_ptr, nullptr);
    fclose(fp);
    png_destroy_write_struct(&png_ptr, &info_ptr);
}