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

#ifndef RESHADE_GRABBER_CONVERT_HPP
#define RESHADE_GRABBER_CONVERT_HPP

#include <sstream>

template <class T>
std::string toString(T obj) {
    std::ostringstream ostr;
    ostr << obj;
    return ostr.str();
}

template <class T>
T fromString(const std::string &stringObject) {
    std::stringstream strstr;
    strstr << stringObject;
    T type;
    strstr >> type;
    return type;
}

inline std::string fromString(const std::string &stringObject) {
    return stringObject;
}

//! Conversion to and from string
template <class T>
std::string toString(
        T obj, int precision, bool fixed = true, bool noshowpoint = false, bool scientific = false) {
    std::ostringstream ostr;
    ostr.precision(precision);
    if (fixed) {
        ostr << std::fixed;
    }
    if (noshowpoint) {
        ostr << std::noshowpoint;
    }
    if (scientific) {
        ostr << std::scientific;
    }
    ostr << obj;
    return ostr.str();
}

/**
 * Converts a positive integer value to a string with a minimum length.
 * @param intValue A positive integer value.
 * @param numDigits The minimum length of the string in digits.
 * @return The integer converted to a string with minimum length.
 */
std::string intToFixedLengthString(int intValue, int numDigits) {
    std::string stringValue = toString(intValue);
    while (stringValue.length() < numDigits) {
        stringValue = "0" + stringValue;
    }
    return stringValue;
}

#endif //RESHADE_GRABBER_CONVERT_HPP
