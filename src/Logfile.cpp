/*
 * BSD 2-Clause License
 *
 * Copyright(c) 2017-2021, Christoph Neuhauser
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
 * DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Logfile.hpp"
#include <string>
#include <iostream>
#include <fstream>

Logfile::Logfile() : closedLogfile(false)
{
}

Logfile::~Logfile()
{
    closeLogfile();
}

void Logfile::closeLogfile()
{
    if(closedLogfile) {
        std::cerr << "Tried to close logfile multiple times!" << std::endl;
        return;
    }
    write("<br><br>End of file</font></body></html>");
    logfile.close();
    closedLogfile = true;
}

void Logfile::createLogfile(const char *filename, const char *appName)
{
    // Open file and write header
    logfile.open(filename);
    write(std::string() + "<html><head><title>Logfile (" + appName + ")</title></head>");
    write("<body><font face='courier new'>");
    writeTopic(std::string() + "Logfile(" + appName + ")", 2);

    // Log information on build configuration
#ifdef _DEBUG
    std::string build = "DEBUG";
#else
    std::string build = "RELEASE";
#endif
    write(std::string() + "Build: " + build + "<br>");

    // Write link to project page
    write(
            std::string() + "<br><a href='https://github.com/chrismile/" + appName
            + "/issues'>Inform the developers about issues</a><br><br>");
}

// Create the heading
void Logfile::writeTopic(const std::string &text, int size)
{
    write("<table width='100%%' ");
    write("bgcolor='#E0E0E5'><tr><td><font face='arial' ");
    write(std::string() + "size='+" + std::to_string(size) + "'>");
    write(text);
    write("</font></td></tr></table>\n<br>");
}

// Write black text to the file
void Logfile::write(const std::string &text)
{
    logfile.write(text.c_str(), text.size());
    logfile.flush();
}

// Write colored text to the logfile
void Logfile::write(const std::string &text, int color)
{
    switch(color) {
        case BLACK:
            write("<font color=black>");  break;
        case WHITE:
            write("<font color=white>");  break;
        case RED:
            write("<font color=red>");    break;
        case GREEN:
            write("<font color=green>");  break;
        case BLUE:
            write("<font color=blue>");   break;
        case PURPLE:
            write("<font color=purple>"); break;
        case ORANGE:
            write("<font color=FF6A00>"); break;
    };

    write(text);
    write("</font>");
    write("<br>");
}

void Logfile::writeError(const std::string &text)
{
    std::cerr << text << std::endl;
    write(text, RED);
}

void Logfile::fatalError(const std::string &text)
{
    write(text, RED);
    throw std::runtime_error(text.c_str());
}

void Logfile::writeInfo(const std::string &text)
{
    std::cout << text << std::endl;
    write(text, BLUE);
}
