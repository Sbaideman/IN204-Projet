#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <png.h>
#include "SavePng.hpp"

/**
 * @brief Parse PPM file in P3 format
 * @param filename Path of the PPM file
 * @return Parsed PPMImage structure
 */
PPMImage parse_ppm(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        throw std::runtime_error("Failed to open PPM file: " + filename);
    }

    PPMImage img;
    std::string magic;
    infile >> magic;

    // Verify PPM format (P3)
    if (magic != "P3") {
        throw std::runtime_error("Unsupported PPM format, only P3 text format is supported");
    }

    // Read width, height and maximum color value
    infile >> img.width >> img.height >> img.max_color;
    if (img.max_color != 255) {
        throw std::runtime_error("Only PPM files with maximum color value of 255 are supported");
    }

    // Read RGB values of all pixels
    img.pixels.resize(img.width * img.height * 3);
    int r, g, b;
    int idx = 0;
    while (infile >> r >> g >> b) {
        img.pixels[idx++] = static_cast<unsigned char>(r);
        img.pixels[idx++] = static_cast<unsigned char>(g);
        img.pixels[idx++] = static_cast<unsigned char>(b);
    }

    // Verify if the number of pixels matches
    if (idx != img.width * img.height * 3) {
        throw std::runtime_error("The number of pixels in the PPM file does not match the width and height");
    }

    infile.close();
    return img;
}

/**
 * @brief Write PPM image data to PNG file
 * @param img Parsed PPMImage structure
 * @param filename Output PNG file path
 */
void write_png(const PPMImage& img, const std::string& filename) {
    // Create PNG write structure
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        throw std::runtime_error("Failed to create PNG write structure");
    }

    // Create PNG info structure
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, nullptr);
        throw std::runtime_error("Failed to create PNG info structure");
    }

    // Set error handling (prevent libpng from crashing)
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        throw std::runtime_error("Error occurred during PNG writing process");
    }

    // Open output file
    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        throw std::runtime_error("Failed to create PNG file: " + filename);
    }

    // Bind file pointer to PNG structure
    png_init_io(png_ptr, fp);

    // Set PNG image information (RGB format, 8-bit depth)
    png_set_IHDR(
        png_ptr,
        info_ptr,
        img.width,
        img.height,
        8,                      // 8 bits per color channel
        PNG_COLOR_TYPE_RGB,     // RGB format (no Alpha channel)
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png_ptr, info_ptr);

    // Prepare row pointers (libpng requires writing by row)
    std::vector<png_bytep> row_pointers(img.height);
    for (int y = 0; y < img.height; ++y) {
        row_pointers[y] = const_cast<png_byte*>(&img.pixels[y * img.width * 3]);
    }

    // Write pixel data
    png_write_image(png_ptr, row_pointers.data());

    // Finish writing and clean up resources
    png_write_end(png_ptr, nullptr);
    fclose(fp);
    png_destroy_write_struct(&png_ptr, &info_ptr);
}