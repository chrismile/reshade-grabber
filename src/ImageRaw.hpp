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

#ifndef CONVERT_RAW_IMAGE_IMAGERAW_HPP
#define CONVERT_RAW_IMAGE_IMAGERAW_HPP

#include <string>

enum class ChannelDataType {
    UINT8 = 0, UINT16 = 1, UINT32 = 2, UINT64 = 2,
    INT8 = 3, INT16 = 4, INT32 = 5, INT64 = 5,
    FLOAT16 = 6, FLOAT32 = 6, FLOAT64 = 7
};

bool saveImageRaw(
        const std::string& filename, const uint32_t width, const uint32_t height,
        const uint32_t numChannels, const ChannelDataType channelDataType, uint8_t* byteData);

bool loadImageRaw(
        const std::string& filename, uint32_t& width, uint32_t& height,
        uint32_t& numChannels, ChannelDataType& channelDataType, uint8_t*& byteData);

#endif //CONVERT_RAW_IMAGE_IMAGERAW_HPP
