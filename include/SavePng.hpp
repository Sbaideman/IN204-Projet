#ifndef SAVEPNG_H
#define SAVEPNG_H

#include <string>
#include <vector>
#include <stdexcept>

// 存储图像数据的结构体（供外部调用）
struct PPMImage {
    int width;
    int height;
    int max_color;
    std::vector<unsigned char> pixels; // RGB顺序，每个像素3字节
};

/**
 * @brief 将PPMImage数据写入PNG文件（核心函数声明）
 * @param img 图像数据结构体
 * @param filename 输出PNG文件路径
 * @throw std::runtime_error 写入失败时抛出异常
 */
void write_png(const PPMImage& img, const std::string& filename);

#endif // SAVEPNG_H