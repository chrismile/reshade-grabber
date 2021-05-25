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

#ifndef RESHADE_GRABBER_VEC3_HPP
#define RESHADE_GRABBER_VEC3_HPP

struct vec3 {
    union{
        struct { float x, y, z; };
        float data[3];
    };

    vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

inline vec3 normalize(const vec3& p) {
    float invLength = 1.0f / std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    return vec3(invLength * p.x, invLength * p.y, invLength * p.z);
}

inline vec3 cross(const vec3& p, const vec3& q) {
    return vec3(p.y * q.z - p.z * q.y, p.z * q.x - p.x * q.z, p.x * q.y - p.y * q.x);
}

inline vec3 operator+(const vec3& p, const vec3& q) {
    return vec3(p.x + q.x, p.y + q.y, p.z + q.z);
}

inline vec3 operator-(const vec3& p, const vec3& q) {
    return vec3(p.x - q.x, p.y - q.y, p.z - q.z);
}

#endif //RESHADE_GRABBER_VEC3_HPP
