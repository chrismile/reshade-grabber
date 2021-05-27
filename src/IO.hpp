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

#ifndef SH_AUG_IO_HPP
#define SH_AUG_IO_HPP

#include <string>
#include <vector>

/**
 * Returns whether the input strings ends with the passed suffix.
 * @param input The input string.
 * @param suffix The suffix string.
 * @return True if the input strings ends with the passed suffix, and false otherwise.
 */
bool stringEndsWith(const std::string& input, const std::string& suffix);

/**
 * Saves the passed 'data' object as a width x height image with 8 bits per channel.
 * @param filename The file name of the image (.png and .jpg are supported).
 * @param width The width of the image.
 * @param height The height of the image.
 * @param data A depth image of size width*height.
 * @param numChannels The number of channels.
 */
void saveImage8Bit(
        const std::string& filename, const uint32_t width, const uint32_t height, uint8_t* data, int numChannels);

/**
 * Saves the passed 'data' object as a width x height image with 16 bits per channel.
 * @param filename The file name of the image (only .png is supported).
 * @param width The width of the image.
 * @param height The height of the image.
 * @param data A depth image of size width*height.
 * @param numChannels The number of channels.
 */
void saveImage16Bit(
        const std::string& filename, const uint32_t width, const uint32_t height, uint16_t* data, int numChannels);

/**
 * Saves the passed 'data' object as a width x height image with 8 bits per channel.
 * The image is normalized by computing the minimum and maximum floating point value in the image.
 * @param filename The file name of the image (only .png is supported at the).
 * @param width The width of the image.
 * @param height The height of the image.
 * @param data A depth image of size width*height.
 */
void saveFloatImageNormalized(
        const std::string& filename, const uint32_t width, const uint32_t height, float* data, bool isDepth = true);

/**
 * Returns a list of all files in a directory.
 * @param dirPath The path to the directory.
 * @return The list of files in the directory.
 */
std::vector<std::string> getFileNamesInDirectory(const std::string &dirPath);

/**
 * Returns a list of all file paths of files in a directory.
 * @param dirPath The path to the directory.
 * @return The list of all file paths of files in the directory.
 */
std::vector<std::string> getFilePathsInDirectory(const std::string &dirPath);

#endif //SH_AUG_IO_HPP
