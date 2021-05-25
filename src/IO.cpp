/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <vector>
#include <iostream>
#include <filesystem>
#include <cmath>
#include <cstdio>

#include <png.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "IO.hpp"

inline bool stringEndsWith(const std::string& input, const std::string& test) {
    return input.size() >= test.size() && input.compare(input.size() - test.size(), test.size(), test) == 0;
}

void savePngToFile(
        const std::string& filename, const uint32_t width, const uint32_t height,
        uint32_t numChannels, uint32_t bitsPerChannel, bool mirror, void* data) {
    FILE* file = nullptr;
    file = fopen(filename.c_str(), "wb");
    if (!file) {
        std::string errorMessage = "Error in savePngToFile: Couldn't open the file \"" + filename + "\"!";
        throw std::runtime_error(errorMessage.c_str());
    }

    png_structp pngPointer = png_create_write_struct(
            PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop pngInfoPointer = png_create_info_struct(pngPointer);

    png_init_io(pngPointer, file);

    int colorType = -1;
    if (numChannels == 1) {
        colorType = PNG_COLOR_TYPE_GRAY;
    } else if (numChannels == 2) {
        colorType = PNG_COLOR_TYPE_GRAY_ALPHA;
    } else if (numChannels == 3) {
        colorType = PNG_COLOR_TYPE_RGB;
    } else if (numChannels == 4) {
        colorType = PNG_COLOR_TYPE_RGB_ALPHA;
    } else {
        std::string errorMessage = "Error in savePngToFile: Invalid number of channels for file \"" + filename + "\"!";
        throw std::runtime_error(errorMessage.c_str());
    }
    png_set_IHDR(
            pngPointer, pngInfoPointer, width, height, bitsPerChannel, colorType,
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(pngPointer, pngInfoPointer);

    png_uint_32 pngHeight = height;
    png_uint_32 rowBytes = width * numChannels * bitsPerChannel / 8;
    png_bytep* rowPointers = new png_bytep[pngHeight];

    // Convert to MSB order.
    uint8_t* dataUint8Msb;
    if (bitsPerChannel == 16) {
        uint16_t* dataUint16Lsb = static_cast<uint16_t*>(data);
        dataUint8Msb = new uint8_t[width * height * numChannels * bitsPerChannel / 8];
        uint8_t* currentDataPtr = dataUint8Msb;
        const int n = width * height * numChannels;
        for (int i = 0; i < n; i++) {
            uint16_t colorUint16 = dataUint16Lsb[i];
            *currentDataPtr++ = png_byte(colorUint16 >> 8);
            *currentDataPtr++ = png_byte(colorUint16 & 0xff);
        }
        data = dataUint8Msb;
    }

    // Does the program need to reverse the direction?
    if (mirror) {
        for (unsigned int i = 0; i < pngHeight; i++) {
            rowPointers[i] = reinterpret_cast<png_byte*>(data) + ((pngHeight - i - 1) * rowBytes);
        }
    } else {
        for (unsigned int i = 0; i < pngHeight; i++) {
            rowPointers[i] = reinterpret_cast<png_byte*>(data) + (i * rowBytes);
        }
    }

    png_write_image(pngPointer, rowPointers);
    png_write_end(pngPointer, pngInfoPointer);
    png_destroy_write_struct(&pngPointer, &pngInfoPointer);

    delete[] rowPointers;
    if (bitsPerChannel == 16) {
        delete[] dataUint8Msb;
    }
    fclose(file);
}

void saveImage8Bit(
        const std::string& filename, const uint32_t width, const uint32_t height, uint8_t* data, int numChannels) {
    if (stringEndsWith(filename, ".png")) {
        savePngToFile(filename, width, height, numChannels, 8, false, data);
        //stbi_write_png(filename.c_str(), width, height, mat.cols(), colorImageUint.data(), width * mat.cols());
    } else if (stringEndsWith(filename, ".jpg")) {
        stbi_write_jpg(filename.c_str(), width, height, numChannels, data, 80);
    } else {
        throw std::runtime_error("saveColorImage: Unknown file ending.");
    }
}

void saveImage16Bit(
        const std::string& filename, const uint32_t width, const uint32_t height,
        uint16_t* data, int numChannels) {
    savePngToFile(filename, width, height, numChannels, 16, false, data);
}

void saveFloatImageNormalized(
        const std::string& filename, const uint32_t width, const uint32_t height,
        float* data, bool isDepth) {
    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::lowest();
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            size_t i = x + y * width;
            minVal = std::min(minVal, data[i]);
            maxVal = std::max(maxVal, data[i]);
        }
    }
    if (isDepth) {
        minVal = 0.0f;
    }

    uint8_t* dataUint = new uint8_t[width * height];
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            size_t i = x + y * width;
            dataUint[i] = std::clamp(
                    int(std::round((data[i] - minVal) / (maxVal - minVal) * 255)), 0, 255);
        }
    }

    if (stringEndsWith(filename, ".png")) {
        savePngToFile(filename, width, height, 1, 8, false, dataUint);
        //stbi_write_png(filename.c_str(), width, height, mat.cols(), colorImageUint.data(), width * mat.cols());
    } else if (stringEndsWith(filename, ".jpg")) {
        stbi_write_jpg(filename.c_str(), width, height, 1, dataUint, 80);
    } else {
        throw std::runtime_error("saveColorImageToFile: Unknown file ending.");
    }
    delete[] dataUint;
}

std::vector<std::string> getFileNamesInDirectory(const std::string &dirPath) {
    std::filesystem::path dir(dirPath);
    if (!std::filesystem::exists(dir)) {
        std::cerr << "getFileNamesInDirectory: Path \""<< dir.string() << "\" does not exist!" << std::endl;
        return std::vector<std::string>();
    }
    if (!std::filesystem::is_directory(dir)) {
        std::cerr << "getFileNamesInDirectory: \"" << dir.string() << "\" is not a directory!" << std::endl;
        return std::vector<std::string>();
    }

    std::vector<std::string> files;
    std::filesystem::directory_iterator end;
    for (std::filesystem::directory_iterator i(dir); i != end; ++i) {
        files.push_back(i->path().filename().generic_u8string());
#ifdef WIN32
        std::string &currPath = files.back();
        for (std::string::iterator it = currPath.begin(); it != currPath.end(); ++it) {
            if (*it == '\\') *it = '/';
        }
#endif
    }
    return files;
}

std::vector<std::string> getFilePathsInDirectory(const std::string &dirPath) {
    std::filesystem::path dir(dirPath);
    if (!std::filesystem::exists(dir)) {
        std::cerr << "getFilePathsInDirectory: Path \""<< dir.string() << "\" does not exist!" << std::endl;
        return std::vector<std::string>();
    }
    if (!std::filesystem::is_directory(dir)) {
        std::cerr << "getFilePathsInDirectory: \"" << dir.string() << "\" is not a directory!" << std::endl;
        return std::vector<std::string>();
    }

    std::vector<std::string> files;
    std::filesystem::directory_iterator end;
    for (std::filesystem::directory_iterator i(dir); i != end; ++i) {
        files.push_back(i->path().string());
#ifdef WIN32
        std::string &currPath = files.back();
        for (std::string::iterator it = currPath.begin(); it != currPath.end(); ++it) {
            if (*it == '\\') *it = '/';
        }
#endif
    }
    return files;
}
