/*
 * Copyright 2014 Patrick Mours. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 *     disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *     following disclaimer in the documentation and/or other materials provided with the distribution.
 *   * Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 *     products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RESHADE_GRABBER_GENERIC_DEPTH_H
#define RESHADE_GRABBER_GENERIC_DEPTH_H

/**
 * This file contains structs used in the built-in ReShade generic depth add-on.
 * reshade-grabber uses `device->get_data<state_tracking_context>(state_tracking_context::GUID)` in order to gain access
 * to the used depth buffer.
 *
 * state_tracking_context
 *
 * Access to STL objects, like std::unordered_map or std::vector
 */

#include <reshade.hpp>

using namespace reshade::api;

struct draw_stats
{
    uint32_t vertices = 0;
    uint32_t drawcalls = 0;
    float last_viewport[6] = {};
};

struct clear_stats : public draw_stats
{
    bool rect = false;
};

struct depth_stencil_info
{
    draw_stats total_stats;
    draw_stats current_stats; // Stats since last clear operation
    std::vector<clear_stats> clears;
    bool copied_during_frame = false;
};

struct state_tracking
{
    static constexpr uint8_t GUID[16] = {
            0x43, 0x31, 0x9e, 0x83, 0x38, 0x7c, 0x44, 0x8e, 0x88, 0x1c, 0x7e, 0x68, 0xfc, 0x2e, 0x52, 0xc4 };

    draw_stats best_copy_stats;
    bool first_empty_stats = true;
    bool has_indirect_drawcalls = false;
    resource current_depth_stencil = { 0 };
    float current_viewport[6] = {};
    std::unordered_map<uint64_t, depth_stencil_info> counters_per_used_depth_stencil;

    void reset()
    {
        reset_on_present();
        current_depth_stencil = { 0 };
    }
    void reset_on_present()
    {
        best_copy_stats = { 0, 0 };
        first_empty_stats = true;
        has_indirect_drawcalls = false;
        counters_per_used_depth_stencil.clear();
    }

    void merge(const state_tracking &source)
    {
        // Executing a command list in a different command list inherits state
        current_depth_stencil = source.current_depth_stencil;

        if (first_empty_stats)
            first_empty_stats = source.first_empty_stats;
        has_indirect_drawcalls |= source.has_indirect_drawcalls;

        if (source.best_copy_stats.vertices > best_copy_stats.vertices)
            best_copy_stats = source.best_copy_stats;

        counters_per_used_depth_stencil.reserve(source.counters_per_used_depth_stencil.size());
        for (const auto &[depth_stencil_handle, snapshot] : source.counters_per_used_depth_stencil)
        {
            depth_stencil_info &target_snapshot = counters_per_used_depth_stencil[depth_stencil_handle];
            target_snapshot.total_stats.vertices += snapshot.total_stats.vertices;
            target_snapshot.total_stats.drawcalls += snapshot.total_stats.drawcalls;
            target_snapshot.current_stats.vertices += snapshot.current_stats.vertices;
            target_snapshot.current_stats.drawcalls += snapshot.current_stats.drawcalls;

            target_snapshot.clears.insert(target_snapshot.clears.end(), snapshot.clears.begin(), snapshot.clears.end());

            target_snapshot.copied_during_frame |= snapshot.copied_during_frame;
        }
    }
};

struct state_tracking_context
{
    static constexpr uint8_t GUID[16] = {
            0x7c, 0x63, 0x63, 0xc7, 0xf9, 0x4e, 0x43, 0x7a, 0x91, 0x60, 0x14, 0x17, 0x82, 0xc4, 0x4a, 0x98 };

    // Enable or disable the creation of backup copies at clear operations on the selected depth-stencil
    bool preserve_depth_buffers = false;
    // Enable or disable the aspect ratio check from 'check_aspect_ratio' in the detection heuristic
    bool use_aspect_ratio_heuristics = true;

    // Set to zero for automatic detection, otherwise will use the clear operation at the specific index within a frame
    size_t force_clear_index = 0;

    // Stats of the previous frame for the selected depth-stencil
    draw_stats previous_stats = {};

    // A resource used as target for a backup copy for the selected depth-stencil
    resource backup_texture = { 0 };

    // The depth-stencil that is currently selected as being the main depth target
    // Any clear operations on it are subject for special handling (backup copy or replacement)
    resource selected_depth_stencil = { 0 };

    // Resource used to override automatic depth-stencil selection
    resource override_depth_stencil = { 0 };

    // The current shader resource view bound to shaders
    // This can be created from either the original depth-stencil of the application (if it supports shader access), or
    // from the backup resource, or from one of the replacement resources
    resource_view selected_shader_resource = { 0 };

#if RESHADE_GUI
    // List of all encountered depth-stencils of the last frame
	std::vector<std::pair<resource, depth_stencil_info>> current_depth_stencil_list;
	std::unordered_map<uint64_t, unsigned int> display_count_per_depth_stencil;
#endif

    // Checks whether the aspect ratio of the two sets of dimensions is similar or not
    bool check_aspect_ratio(float width_to_check, float height_to_check, uint32_t width, uint32_t height) const
    {
        if (width_to_check == 0.0f || height_to_check == 0.0f)
            return true;

        const float w = static_cast<float>(width);
        const float w_ratio = w / width_to_check;
        const float h = static_cast<float>(height);
        const float h_ratio = h / height_to_check;
        const float aspect_ratio = (w / h) - (static_cast<float>(width_to_check) / height_to_check);

        return std::fabs(aspect_ratio) <= 0.1f && w_ratio <= 1.85f && h_ratio <= 1.85f
               && w_ratio >= 0.5f && h_ratio >= 0.5f;
    }

    // Update the backup texture to match the requested dimensions
    void update_backup_texture(device *device, resource_desc desc)
    {
        if (backup_texture != 0)
        {
            const resource_desc existing_desc = device->get_resource_desc(backup_texture);

            if (desc.texture.width == existing_desc.texture.width && desc.texture.height == existing_desc.texture.height
                && desc.texture.format == existing_desc.texture.format)
                return; // Texture already matches dimensions, so can re-use

            device->wait_idle(); // Texture may still be in use on device, so wait for all operations to finish before
            // destroying it
            device->destroy_resource(backup_texture);
            backup_texture = { 0 };
        }

        desc.type = resource_type::texture_2d;
        desc.heap = memory_heap::gpu_only;
        desc.usage = resource_usage::shader_resource | resource_usage::copy_dest;

        if (device->get_api() == device_api::d3d9)
            desc.texture.format = format::r32_float; // D3DFMT_R32F, size INTZ does not support D3DUSAGE_RENDERTARGET
            // which is required for copying
        else
            desc.texture.format = format_to_typeless(desc.texture.format);

        if (!device->create_resource(desc, nullptr, resource_usage::copy_dest, &backup_texture))
            std::cerr << "Failed to create backup depth-stencil texture!" << std::endl;
    }
};

#endif //RESHADE_GRABBER_GENERIC_DEPTH_H
