/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2017-2021, Christoph Neuhauser
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

#ifndef RESHADE_GRABBER_LOGFILE_HPP
#define RESHADE_GRABBER_LOGFILE_HPP

#include <fstream>
#include "Singleton.hpp"

/// Colors for the output text.
enum FONTCOLORS {
    BLACK, WHITE, RED, GREEN, BLUE, PURPLE, ORANGE
};

class Logfile : public Singleton<Logfile>
{
public:
    Logfile();
    ~Logfile();
    void createLogfile(const char *filename, const char *appName);
    void closeLogfile();

    /// Write to log file.
    void writeTopic(const std::string &text, int size);
    void write(const std::string &text);
    void write(const std::string &text, int color);
    /// Outputs text on stderr, too.
    void writeError(const std::string &text);
    /// Writes the error message to the logfile and throws a std::runtime_exception object.
    void fatalError(const std::string &text);
    /// Outputs text on stdout, too.
    void writeInfo(const std::string &text);

private:
    bool closedLogfile;
    std::ofstream logfile;
};

#endif //RESHADE_GRABBER_LOGFILE_HPP
