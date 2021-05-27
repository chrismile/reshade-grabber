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

#include <stdexcept>
#include <cassert>

#include "ImageRaw.hpp"

inline bool writeBinary(const void* data, size_t size, FILE* file) {
    size_t writtenBytes = fwrite(data, size, 1, file);
    if (writtenBytes != size) {
        return false;
    }
    return true;
}

bool saveImageRaw(
        const std::string& filename, const uint32_t width, const uint32_t height,
        const uint32_t numChannels, const ChannelDataType channelDataType, uint8_t* byteData) {
    FILE* file = fopen(filename.c_str(), "wb");
    if (file == nullptr) {
        throw std::runtime_error("Error in saveImageRaw.");
    }

    uint32_t bytesPerChannel = 0;
    if (channelDataType == ChannelDataType::UINT8 || channelDataType == ChannelDataType::INT8) {
        bytesPerChannel = 1;
    } else if (channelDataType == ChannelDataType::UINT16 || channelDataType == ChannelDataType::INT16
               || channelDataType == ChannelDataType::FLOAT16) {
        bytesPerChannel = 2;
    } else if (channelDataType == ChannelDataType::UINT32 || channelDataType == ChannelDataType::INT32
               || channelDataType == ChannelDataType::FLOAT32) {
        bytesPerChannel = 4;
    } else if (channelDataType == ChannelDataType::UINT64 || channelDataType == ChannelDataType::INT64
               || channelDataType == ChannelDataType::FLOAT64) {
        bytesPerChannel = 8;
    }

    const uint32_t VERSION = 1;
    writeBinary(&VERSION, sizeof(uint32_t), file);
    writeBinary(&width, sizeof(uint32_t), file);
    writeBinary(&height, sizeof(uint32_t), file);
    writeBinary(&numChannels, sizeof(uint32_t), file);
    writeBinary(&channelDataType, sizeof(uint32_t), file);
    writeBinary(byteData, width * height * numChannels * bytesPerChannel, file);

    fclose(file);
    return true;
}

inline bool readBinary(void* data, size_t size, FILE* file) {
    size_t readBytes = fread(data, size, 1, file);
    if (readBytes != size) {
        return false;
    }
    return true;
}

bool loadImageRaw(
        const std::string& filename, uint32_t& width, uint32_t& height,
        uint32_t& numChannels, ChannelDataType& channelDataType, uint8_t* byteData) {
    FILE* file = fopen(filename.c_str(), "rb");
    if (file == nullptr) {
        throw std::runtime_error("Error in loadImageRaw.");
    }

    uint32_t VERSION = 1;
    readBinary(&VERSION, sizeof(uint32_t), file);
    assert(VERSION == 1);

    readBinary(&width, sizeof(uint32_t), file);
    readBinary(&height, sizeof(uint32_t), file);
    readBinary(&numChannels, sizeof(uint32_t), file);
    readBinary(&channelDataType, sizeof(uint32_t), file);

    uint32_t bytesPerChannel = 0;
    if (channelDataType == ChannelDataType::UINT8 || channelDataType == ChannelDataType::INT8) {
        bytesPerChannel = 1;
    } else if (channelDataType == ChannelDataType::UINT16 || channelDataType == ChannelDataType::INT16
               || channelDataType == ChannelDataType::FLOAT16) {
        bytesPerChannel = 2;
    } else if (channelDataType == ChannelDataType::UINT32 || channelDataType == ChannelDataType::INT32
               || channelDataType == ChannelDataType::FLOAT32) {
        bytesPerChannel = 4;
    } else if (channelDataType == ChannelDataType::UINT64 || channelDataType == ChannelDataType::INT64
               || channelDataType == ChannelDataType::FLOAT64) {
        bytesPerChannel = 8;
    }
    byteData = new uint8_t[width * height * numChannels * bytesPerChannel];

    readBinary(byteData, width * height * numChannels * bytesPerChannel, file);

    fclose(file);
    return true;
}
