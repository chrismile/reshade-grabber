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

#include <string>
#include <iostream>
#include <filesystem>

#include "ImageRaw.hpp"
#include "IO.hpp"

void convertFromRaw(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        std::cerr << "Error in convertFromRaw: Path \""<< path.generic_u8string() << "\" does not exist!" << std::endl;
        return;
    } else if (std::filesystem::is_directory(path)) {
        std::filesystem::directory_iterator end;
        for (std::filesystem::directory_iterator i(path); i != end; ++i) {
            convertFromRaw(i->path());
        }
    } else {
        uint8_t* byteData = nullptr;
        uint32_t width = 0, height = 0, numChannels = 0;
        ChannelDataType channelDataType;
        std::string inputPath = path.generic_u8string();
        if (!loadImageRaw(inputPath, width, height, numChannels, channelDataType, byteData)) {
            std::cerr << "Error in convertFromRaw: Can't load data from \""<< inputPath << "\"!" << std::endl;
            if (byteData != nullptr) {
                delete[] byteData;
            }
            return;
        }

        const std::string outputPath = inputPath.substr(0, inputPath.find_last_of('.')) + ".png";
        if (channelDataType == ChannelDataType::UINT8) {
            saveImage8Bit(outputPath, width, height, byteData, numChannels);
        } else if (channelDataType == ChannelDataType::UINT16) {
            saveImage16Bit(outputPath, width, height, reinterpret_cast<uint16_t*>(byteData), numChannels);
        } else {
            std::cerr << "Error in convertFromRaw: Can't load data from \""<< path.string() << "\"!" << std::endl;
        }
        delete[] byteData;
    }
}

void printHelp() {
    std::cerr << "Error: Please call the program like this: 'convert_image_raw <path>'" << std::endl;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printHelp();
        return 1;
    }

    std::filesystem::path path(argv[1]);
    convertFromRaw(path);

    return 0;
}
