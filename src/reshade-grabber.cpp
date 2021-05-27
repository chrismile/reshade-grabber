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

#include <iostream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <omp.h>

#define NOMINMAX
#include <shlobj.h>
#include <windef.h>

#define RESHADE_ADDON_IMPL // Define this before including the ReShade header in exactly one source file
#include <imgui.h>
#include "imgui_stdlib.h"
#include <reshade.hpp>
#include <reshade_api_format.hpp>
#include <dll_config.hpp>

#include "generic_depth.h"
#include "Convert.hpp"
#include "Logfile.hpp"
#include "TimeMeasurement.hpp"
#include "IO.hpp"
#include "ImageRaw.hpp"
#include "vec3.hpp"

using namespace reshade::api;

static std::string appName; // The application name (e.g. "MyProgram" for "C:/dir/MyProgram.exe").
static bool isDepthUpsideDown = false; // RESHADE_DEPTH_INPUT_IS_UPSIDE_DOWN
static bool isDepthReversed = false; // RESHADE_DEPTH_INPUT_IS_REVERSED
static bool isDepthLogarithmic = false; // RESHADE_DEPTH_INPUT_IS_LOGARITHMIC
static float nearPlaneDist = 1.0f;
static float farPlaneDist = 1000.0f; // RESHADE_DEPTH_LINEARIZATION_FAR_PLANE

// If a set of screenshots are recorded, the screenshots of one recording session are bundled into one scene.
static int scene_idx = 0;
// The index of the current frame (the index within the scene if multiple screenshots are recorded).
static int frame_idx = 0;

/**
 * The global state of the reshade-grabber add-on.
 */
struct grabber_context {
    static constexpr uint8_t GUID[16] = {
            0x3a, 0x19, 0xfa, 0x37, 0xb1, 0xc6, 0x70, 0xdd, 0x31, 0xac, 0x7a, 0xe3, 0x45, 0x8c, 0x91, 0x27 };

    // Which G-buffer attributes to save.
    bool outputColor = true;
    bool outputDepth = true;
    bool outputNormals = true;

    bool record = false;
    bool onlyOneScreenshot = false;

    // screenshotsRootDirectory = "C:/Users/<user-name>/Pictures/reshade-grabber/<app-name>/"
    std::string screenshotsRootDirectory;
    // screenshotsDirectory = screenshotsRootDirectory + (onlyOneScreenshot ? "" : "scene{scene_idx}/")
    std::string screenshotsDirectory;
    // If !onlyOneScreenshot: Separate directories for color, depth and normal data for the current scene.
    std::string screenshotsColorDirectory;
    std::string screenshotsDepthDirectory;
    std::string screenshotsNormalDirectory;

    // A resource used as the target for a staging copy of the selected depth-stencil texture.
    resource stagingTexture = { 0 };
    resource_desc stagingTextureDesc;
    resource_desc lastDepthStencilDesc;

    /**
     * Updates the staging texture used for copying the selected depth-stencil texture to the CPU.
     * @param device The device on which the depth-stencil texture is stored.
     * @param desc The resource descriptor of the selected depth-stencil texture.
     */
    void updateStagingTexture(device* device, resource_desc desc) {
        lastDepthStencilDesc = desc;
        stagingTextureDesc = desc;

        if (stagingTexture != 0) {
            const resource_desc existing_desc = device->get_resource_desc(stagingTexture);

            if (desc.texture.width == existing_desc.texture.width && desc.texture.height == existing_desc.texture.height
                    && desc.texture.format == existing_desc.texture.format)
                return;

            device->wait_idle(); // Texture may still be in use on device, so wait for all operations to finish before destroying it
            device->destroy_resource(stagingTexture);
            stagingTexture = {0 };
        }

        stagingTextureDesc.type = resource_type::texture_2d;
        stagingTextureDesc.heap = memory_heap::gpu_to_cpu;
        stagingTextureDesc.usage = resource_usage::copy_dest;

        // D3DFMT_R32F, size INTZ does not support D3DUSAGE_RENDERTARGET which is required for copying.
        if (device->get_api() == device_api::d3d9) {
            stagingTextureDesc.texture.format = format::r32_float;
        } else {
            stagingTextureDesc.texture.format = format_to_default_typed(desc.texture.format);
        }

        if (!device->create_resource(
                stagingTextureDesc, nullptr, resource_usage::copy_dest, &stagingTexture)) {
            Logfile::get()->fatalError(
                    "Error in updateStagingTexture: Failed to create staging depth-stencil texture!");
        }
    }
};

/**
 * Returns the program name of the base application (i.e., the file name of the executable without the extension).
 * @return The program name (e.g. "MyProgram" for "C:/dir/MyProgram.exe").
 */
static std::string getProgramName() {
    TCHAR szExeFileName[MAX_PATH];
    GetModuleFileName(NULL, szExeFileName, MAX_PATH);
    std::string programName = std::filesystem::path(szExeFileName).filename().generic_u8string();
    size_t extensionPoint = programName.find_last_of(".");
    if (extensionPoint != std::string::npos) {
        programName = programName.substr(0, extensionPoint);
    }
    return programName;
}

static void onInitDevice(device *device) {
    grabber_context& grabber_state = device->create_user_data<grabber_context>(grabber_context::GUID);

    // Create the folder 'reshade-grabber' in the 'Pictures' user folder.
    char userDataPath[MAX_PATH];
    SHGetSpecialFolderPathA(NULL, userDataPath, CSIDL_MYPICTURES, true);
    std::string dir = std::string() + userDataPath + "/";
    for (std::string::iterator it = dir.begin(); it != dir.end(); ++it) {
        if (*it == '\\') *it = '/';
    }
    grabber_state.screenshotsRootDirectory = dir + "reshade-grabber/";
    if (!std::filesystem::is_directory(grabber_state.screenshotsRootDirectory)) {
        std::filesystem::create_directory(grabber_state.screenshotsRootDirectory);
    }

    // Create a sub-folder using the name of the .exe application.
    grabber_state.screenshotsRootDirectory = grabber_state.screenshotsRootDirectory + appName + "/";
    if (!std::filesystem::is_directory(grabber_state.screenshotsRootDirectory)) {
        std::filesystem::create_directory(grabber_state.screenshotsRootDirectory);
    }

    // Search if directories starting with 'scene' already exist and get the highest scene index.
    std::vector<std::string> sceneDirectoryList = getFileNamesInDirectory(grabber_state.screenshotsRootDirectory);
    std::sort(sceneDirectoryList.begin(), sceneDirectoryList.end());
    for (auto it = sceneDirectoryList.rbegin(); it != sceneDirectoryList.rend(); it++) {
        if (it->rfind("scene", 0) == 0) {
            scene_idx = fromString<int>(it->substr(5).c_str()) + 1;
            break;
        }
    }
}

static void onDestroyDevice(device *device)
{
    grabber_context& grabber_state = device->get_user_data<grabber_context>(grabber_context::GUID);

    if (grabber_state.stagingTexture != 0) {
        device->destroy_resource(grabber_state.stagingTexture);
    }

    device->destroy_user_data<grabber_context>(grabber_context::GUID);
}

/**
 * Copies the depth data from the staging texture to CPU memory and returns the memory pointer.
 * @param device The device on which the depth-stencil texture is stored.
 * @param staging_texture The depth-stencil texture resource.
 * @param staging_texture_desc The resource descriptor of the selected depth-stencil texture.
 * @param depth_stencil_desc The resource descriptor of the selected depth-stencil texture.
 * @return
 *
 * NOTE: The user needs to free the returned memory using 'delete[]'.
 */
static float* copyDepthDataToCpu(
        device* const device, resource staging_texture, resource_desc staging_texture_desc,
        resource_desc depth_stencil_desc)
{
    void* depthBufferPtr = nullptr;
    uint32_t row_pitch = 0;
    device->map_resource_pitch(staging_texture, 0, map_access::read_only, &depthBufferPtr, &row_pitch);
    if (!depthBufferPtr) {
        Logfile::get()->fatalError("Failed to map resource to CPU memory!");
    }

    float* depthData = new float[depth_stencil_desc.texture.width * depth_stencil_desc.texture.height];
    if (staging_texture_desc.texture.format == format::r32_float) {
        if (row_pitch == depth_stencil_desc.texture.width * sizeof(float) || row_pitch == 0) {
            std::memcpy(
                    depthData, depthBufferPtr,
                    depth_stencil_desc.texture.width * depth_stencil_desc.texture.height * sizeof(float));
        } else {
            uint8_t* depthDataPtrByte = reinterpret_cast<uint8_t*>(depthData);
            uint8_t* depthBufferPtrByte = reinterpret_cast<uint8_t*>(depthBufferPtr);
            for (uint32_t y = 0; y < depth_stencil_desc.texture.height; y++) {
                std::memcpy(
                        depthDataPtrByte + y * depth_stencil_desc.texture.width * sizeof(float),
                        depthBufferPtrByte + y * row_pitch,
                        depth_stencil_desc.texture.width * sizeof(float));
            }
        }
    } else if (staging_texture_desc.texture.format == format::r24_unorm_x8_uint) {
        uint32_t* u32Data = new uint32_t[depth_stencil_desc.texture.width * depth_stencil_desc.texture.height];
		if (row_pitch == depth_stencil_desc.texture.width * sizeof(float) || row_pitch == 0) {
			std::memcpy(
				u32Data, depthBufferPtr,
				depth_stencil_desc.texture.width * depth_stencil_desc.texture.height * sizeof(uint32_t));
		} else {
			uint8_t* u32DataPtrByte = reinterpret_cast<uint8_t*>(u32Data);
			uint8_t* depthBufferPtrByte = reinterpret_cast<uint8_t*>(depthBufferPtr);
			for (uint32_t y = 0; y < depth_stencil_desc.texture.height; y++) {
				std::memcpy(
					u32DataPtrByte + y * depth_stencil_desc.texture.width * sizeof(uint32_t),
					depthBufferPtrByte + y * row_pitch,
					depth_stencil_desc.texture.width * sizeof(uint32_t));
			}
		}
        for (uint32_t y = 0; y < depth_stencil_desc.texture.height; y++) {
            for (uint32_t x = 0; x < depth_stencil_desc.texture.width; x++) {
                uint32_t i = x + y * depth_stencil_desc.texture.width;
                depthData[i] = float(u32Data[i] & 0xFFFFFFul) / 16777215.0f;
            }
        }
        delete[] u32Data;
    } else if (staging_texture_desc.texture.format == format::r16_unorm) {
        uint16_t* u16Data = new uint16_t[depth_stencil_desc.texture.width * depth_stencil_desc.texture.height];
		if (row_pitch == depth_stencil_desc.texture.width * sizeof(float) || row_pitch == 0) {
			std::memcpy(
				u16Data, depthBufferPtr,
				depth_stencil_desc.texture.width * depth_stencil_desc.texture.height * sizeof(uint16_t));
		} else {
			uint8_t* u16DataPtrByte = reinterpret_cast<uint8_t*>(u16Data);
			uint8_t* depthBufferPtrByte = reinterpret_cast<uint8_t*>(depthBufferPtr);
			for (uint32_t y = 0; y < depth_stencil_desc.texture.height; y++) {
				std::memcpy(
					u16DataPtrByte + y * depth_stencil_desc.texture.width * sizeof(uint16_t),
					depthBufferPtrByte + y * row_pitch,
					depth_stencil_desc.texture.width * sizeof(uint16_t));
			}
		}
		for (uint32_t y = 0; y < depth_stencil_desc.texture.height; y++) {
            for (uint32_t x = 0; x < depth_stencil_desc.texture.width; x++) {
                uint32_t i = x + y * depth_stencil_desc.texture.width;
                depthData[i] = u16Data[i] / 65535.0f;
            }
        }
        delete[] u16Data;
    } else {
        Logfile::get()->fatalError(
                "Error: Invalid texture format (" + std::to_string(int(staging_texture_desc.texture.format)) + ")");
    }

    device->unmap_resource(staging_texture, 0);

    return depthData;
}

inline int clamp(int x, int a, int b) {
    if (x <= a) {
        return a;
    } else if (x >= b) {
        return b;
    } else {
        return x;
    }
}

inline vec3 getPoint(int x, int y, int width, int height, const float* depthData) {
    float depth = depthData[clamp(x, 0, width - 1) + clamp(y, 0, height - 1) * width];
    float xf = (float(x) - float(width - 1) / 2.0f) / float(height);
    float yf = float(y) / float(height) - 0.5f;
    return vec3(xf * depth, yf * depth, depth);

    // The code below corresponds to what the built-in ReShade generic depth add-on uses, but does not correctly
    // handle aspect ratios != 1.
    //return vec3((x / float(width) - 0.5f) * depth, (y / float(height) - 0.5f) * depth, depth);
}

/**
 * Computes the normal map for the passed depth data.
 * @param width The frame width.
 * @param height The frame height.
 * @param depthData The input linearized depth data of size width x height.
 * @param normalData The output normal data as 3-channel 8-bit RGB data of size width x height.
 */
static void computeNormalMap(int width, int height, const float* depthData, uint8_t* normalData) {
    #pragma omp parallel for
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Central difference.
            //vec3 leftPoint   = getPoint(x - 1, y, width, height, depthData);
            //vec3 rightPoint  = getPoint(x + 1, y, width, height, depthData);
            //vec3 topPoint    = getPoint(x, y - 1, width, height, depthData);
            //vec3 bottomPoint = getPoint(x, y + 1, width, height, depthData);

            //vec3 dddx = rightPoint - leftPoint;
            //vec3 dddy = bottomPoint - topPoint;
            //vec3 normal = normalize(cross(dddx, dddy));

            // Forward difference (like done in ReShade generic depth plugin).
            vec3 centerPoint = getPoint(x, y, width, height, depthData);
            vec3 rightPoint  = getPoint(x + 1, y, width, height, depthData);
            vec3 topPoint    = getPoint(x, y - 1, width, height, depthData);

            vec3 dddx = rightPoint - centerPoint;
            vec3 dddy = topPoint - centerPoint;
            vec3 normal = normalize(cross(dddy, dddx));

            for (int c = 0; c < 3; c++) {
                normalData[(x + y * width) * 3 + c] = uint8_t(std::round(std::clamp(
                        normal.data[c] * 0.5f + 0.5f, 0.0f, 1.0f) * 255.0f));
            }
        }
    }
}

/**
 * Linearizes the depth data (cf. DisplayDepth.fx from the ReShade shader repository).
 * @param width The frame width.
 * @param height The frame height.
 * @param depthData The depth data of size width x height.
 * @param u16Data The depth data compressed to 16-bit values.
 */
static void linearizeDepthBuffer(int width, int height, float* depthData, uint16_t* u16Data) {
    // Linearize the depth data (cf. DisplayDepth.fx from the ReShade shader repository).
    int numPixels = width * height;
    #pragma omp parallel for
    for (int i = 0; i < numPixels; i++) {
        float depthValue = depthData[i];
        if (isDepthLogarithmic) {
            const float C = 0.01f;
            depthValue = (std::exp(depthValue * std::log(C + 1.0f)) - 1.0f) / C;
        }
        if (isDepthReversed) {
            depthValue = 1.0f - depthValue;
        }

        depthValue /= farPlaneDist - depthValue * (farPlaneDist - nearPlaneDist);
        u16Data[i] = uint16_t(std::round(std::clamp(depthValue, 0.0f, 1.0f) * 65535.0f));
        depthData[i] = depthValue;
    }
}

/**
 * Saves the data of the current frame to a set of files.
 * @param runtime The ReShade effect runtime object.
 * @param grabber_state The current add-on state.
 */
static void saveFrameData(reshade::api::effect_runtime* runtime, grabber_context& grabber_state) {
    device* const device = runtime->get_device();
    command_queue* const queue = runtime->get_command_queue();

    void* state_tracking_context_ptr = nullptr;
    if (!device->get_user_data(state_tracking_context::GUID, &state_tracking_context_ptr)) {
        Logfile::get()->writeError("Error: Built-in depth add-on not loaded!");
        return;
    }
    state_tracking_context &device_state = device->get_user_data<state_tracking_context>(state_tracking_context::GUID);

    if (device_state.selected_depth_stencil == 0) {
        Logfile::get()->writeError("Error: device_state.selected_depth_stencil");
        grabber_state.record = false;
        return;
    }

    TimeMeasurement updateStagingTime("update staging");
    const resource depth_stencil_resource = device_state.selected_depth_stencil;
    const resource_desc depth_stencil_desc = device->get_resource_desc(device_state.selected_depth_stencil);
    if (depth_stencil_desc.texture.format != grabber_state.lastDepthStencilDesc.texture.format
            || depth_stencil_desc.texture.width != grabber_state.lastDepthStencilDesc.texture.width
            || depth_stencil_desc.texture.height != grabber_state.lastDepthStencilDesc.texture.height) {
        grabber_state.updateStagingTexture(device, depth_stencil_desc);
    }
    updateStagingTime.stop();

    // Create a staging texture for copying the depth-stencil data to the CPU.
    TimeMeasurement copyStagingTimeGpu("copy staging (GPU)");
    if (grabber_state.stagingTexture != 0)
    {
        if ((depth_stencil_desc.usage & resource_usage::copy_source) == resource_usage::undefined) {
            Logfile::get()->fatalError("Error: Depth-stencil texture does not allow resource_usage::copy_source.");
        }

        command_list* const cmd_list = queue->get_immediate_command_list();

        cmd_list->barrier(
                grabber_state.stagingTexture,
                resource_usage::cpu_access,
                resource_usage::copy_dest);
        cmd_list->barrier(
                depth_stencil_resource,
                resource_usage::depth_stencil | resource_usage::shader_resource,
                resource_usage::copy_source);

        cmd_list->copy_resource(depth_stencil_resource, grabber_state.stagingTexture);

        cmd_list->barrier(
                depth_stencil_resource,
                resource_usage::copy_source,
                resource_usage::depth_stencil | resource_usage::shader_resource);
        cmd_list->barrier(
                grabber_state.stagingTexture,
                resource_usage::copy_dest,
                resource_usage::cpu_access);
    }
    copyStagingTimeGpu.stop();

    TimeMeasurement copyStagingTimeCpu("copy staging (CPU)");
    float* depthData = copyDepthDataToCpu(
            device, grabber_state.stagingTexture, grabber_state.stagingTextureDesc, depth_stencil_desc);
    copyStagingTimeCpu.stop();

    TimeMeasurement linearizeDepthTime("linearize depth");
    uint16_t* u16Data = new uint16_t[depth_stencil_desc.texture.width * depth_stencil_desc.texture.height];
    linearizeDepthBuffer(depth_stencil_desc.texture.width, depth_stencil_desc.texture.height, depthData, u16Data);
    linearizeDepthTime.stop();

    TimeMeasurement computeNormalMapTime("compute normal map");
    uint8_t* normalDataUint = new uint8_t[depth_stencil_desc.texture.width * depth_stencil_desc.texture.height * 3];
    computeNormalMap(depth_stencil_desc.texture.width, depth_stencil_desc.texture.height, depthData, normalDataUint);
    computeNormalMapTime.stop();

    // Get the color data and convert it from RGBA to RGB.
    TimeMeasurement captureColorFrameTime("capture color frame");
    uint32_t frameWidth = 0, frameHeight = 0;
    runtime->get_frame_width_and_height(&frameWidth, &frameHeight);
    uint8_t* colorDataRgba = new uint8_t[frameWidth * frameHeight * 4];
    uint8_t* colorData = new uint8_t[frameWidth * frameHeight * 3];
    runtime->capture_screenshot(colorDataRgba);
    int frameSize = static_cast<int>(frameWidth * frameHeight);
    #pragma omp parallel for
    for (int i = 0; i < frameSize; i++) {
        colorData[i*3] = colorDataRgba[i*4];
        colorData[i*3+1] = colorDataRgba[i*4+1];
        colorData[i*3+2] = colorDataRgba[i*4+2];
    }
    delete[] colorDataRgba;
    captureColorFrameTime.stop();

    // Save the frame data to a set of files.
    TimeMeasurement saveImagesTime("save images");
    std::string filenameColor, filenameDepth, filenameNormal;
    std::string frameIdxStr = toString(frame_idx);
#if defined(WRITE_PNG) && WRITE_PNG != 0
    const std::string fileExtension = ".png";
#else
    const std::string fileExtension = ".raw";
#endif
    if (grabber_state.onlyOneScreenshot) {
        filenameColor = grabber_state.screenshotsDirectory + "color_" + frameIdxStr + fileExtension;
        filenameDepth = grabber_state.screenshotsDirectory + "depth_" + frameIdxStr + fileExtension;
        filenameNormal = grabber_state.screenshotsDirectory + "normal_" + frameIdxStr + fileExtension;
    } else {
        filenameColor = grabber_state.screenshotsColorDirectory + frameIdxStr + fileExtension;
        filenameDepth = grabber_state.screenshotsDepthDirectory + frameIdxStr + fileExtension;
        filenameNormal = grabber_state.screenshotsNormalDirectory + frameIdxStr + fileExtension;
    }
    // Save the images to the disk in parallel (this can bring a huge speed-up due to the PNG encoding step being slow).
    #pragma omp parallel sections num_threads(3)
    {
        #pragma omp section
        {
            TimeMeasurement saveDepthImageTime("save depth image");
            saveImage16Bit(
                    filenameDepth, depth_stencil_desc.texture.width, depth_stencil_desc.texture.height,
                    u16Data, 1);
            saveDepthImageTime.stop();
        }

        #pragma omp section
        {
            TimeMeasurement saveNormalImageTime("save normal image");
            saveImage8Bit(
                    filenameNormal, depth_stencil_desc.texture.width, depth_stencil_desc.texture.height,
                    normalDataUint, 3);
            saveNormalImageTime.stop();
        }

        #pragma omp section
        {
            TimeMeasurement saveColorImageTime("save color image");
            saveImage8Bit(
                    filenameColor, frameWidth, frameHeight, colorData, 3);
            saveColorImageTime.stop();
        }
    }
    saveImagesTime.stop();

    // Delete all auxiliary CPU buffers.
    TimeMeasurement deleteAuxDataTime("delete aux data");
    delete[] u16Data;
    delete[] depthData;
    delete[] normalDataUint;
    delete[] colorData;
    deleteAuxDataTime.stop();

    frame_idx++;
}

static void onPresent(reshade::api::command_queue*, reshade::api::effect_runtime* runtime) {
    device* const device = runtime->get_device();
    grabber_context& grabber_state = device->get_user_data<grabber_context>(grabber_context::GUID);
    if (!grabber_state.record) {
        return;
    }

    // Make a screenshot.
    TimeMeasurement totalTime("total time");
    saveFrameData(runtime, grabber_state);
    totalTime.stop();

    // Stop making further screenshots?
    if (grabber_state.onlyOneScreenshot) {
        grabber_state.record = false;
        grabber_state.onlyOneScreenshot = false;
    }
}

void writeSceneInfo(const std::string& sceneName, const std::string& sceneDirectory) {
    std::string sceneInfoPath = sceneDirectory + sceneName + ".txt";
    std::ofstream sceneInfoFile(sceneInfoPath.c_str());
    sceneInfoFile << "appName = " << appName << "\n";
    sceneInfoFile << "nearPlaneDist = " << nearPlaneDist << "\n";
    sceneInfoFile << "farPlaneDist = " << farPlaneDist << "\n";
    sceneInfoFile.close();
}

static void drawDebugMenu(effect_runtime *runtime, void *) {
    device* const device = runtime->get_device();
    grabber_context& grabber_state = device->get_user_data<grabber_context>(grabber_context::GUID);

    g_imgui_function_table.Separator();

    // Let the user choose the output directory for the screenshots.
    const std::string screenshotsDirText = "Screenshots directory:";
    g_imgui_function_table.TextUnformatted(
            screenshotsDirText.c_str(), screenshotsDirText.c_str() + screenshotsDirText.size());
    g_imgui_function_table.PushItemWidth(g_imgui_function_table.GetWindowContentRegionWidth());
    ImGui::InputText("##screenshots-dir", &grabber_state.screenshotsRootDirectory);
    g_imgui_function_table.PopItemWidth();

    // Start recording from the UI.
    if (!grabber_state.record) {
        if (g_imgui_function_table.Button("Save One", ImVec2(0, 0))) {
            grabber_state.record = true;
            grabber_state.onlyOneScreenshot = true;
            grabber_state.screenshotsDirectory = grabber_state.screenshotsRootDirectory;
        }
        g_imgui_function_table.SameLine(0.0f, -1.0f);
        if (g_imgui_function_table.Button("Record Series", ImVec2(0, 0))) {
            frame_idx = 0;
            grabber_state.record = true;
            grabber_state.onlyOneScreenshot = false;

            std::string sceneName = "scene" + intToFixedLengthString(scene_idx, 4);
            grabber_state.screenshotsDirectory = grabber_state.screenshotsRootDirectory + sceneName + "/";
            grabber_state.screenshotsColorDirectory = grabber_state.screenshotsDirectory + "color/";
            grabber_state.screenshotsDepthDirectory = grabber_state.screenshotsDirectory + "depth/";
            grabber_state.screenshotsNormalDirectory = grabber_state.screenshotsDirectory + "normal/";
            if (!std::filesystem::is_directory(grabber_state.screenshotsDirectory)) {
                std::filesystem::create_directory(grabber_state.screenshotsDirectory);
            }
            if (!std::filesystem::is_directory(grabber_state.screenshotsColorDirectory)) {
                std::filesystem::create_directory(grabber_state.screenshotsColorDirectory);
            }
            if (!std::filesystem::is_directory(grabber_state.screenshotsDepthDirectory)) {
                std::filesystem::create_directory(grabber_state.screenshotsDepthDirectory);
            }
            if (!std::filesystem::is_directory(grabber_state.screenshotsNormalDirectory)) {
                std::filesystem::create_directory(grabber_state.screenshotsNormalDirectory);
            }
            writeSceneInfo(sceneName, grabber_state.screenshotsDirectory);
        }
    } else {
        if (g_imgui_function_table.Button("Stop Recording", ImVec2(0, 0))) {
            scene_idx++;
            grabber_state.record = false;
        }
    }

    g_imgui_function_table.Separator();

    g_imgui_function_table.Checkbox("Color", &grabber_state.outputColor);
    g_imgui_function_table.SameLine(0.0f, -1.0f);
    g_imgui_function_table.Checkbox("Depth", &grabber_state.outputDepth);
    g_imgui_function_table.SameLine(0.0f, -1.0f);
    g_imgui_function_table.Checkbox("Normals", &grabber_state.outputNormals);
}

// The following data is used by the ReShade add-on manager.
extern "C" {
__declspec(dllexport) const char* NAME = "reshade-grabber";
__declspec(dllexport) const char* DESCRIPTION =
        "An add-on for ReShade that acts as a frame grabber extracting both color frames and auxiliary G-buffer data.";
}

// Definition for dll_config.cpp (source file from ReShade).
std::filesystem::path g_reshade_dll_path;

/**
 * Returns the path to the passed module (i.e., a DLL file).
 * @param module The module to return the path for.
 * @return The path to the module.
 */
static inline std::filesystem::path getModulePath(HMODULE module) {
    WCHAR buf[4096];
    return GetModuleFileNameW(module, buf, ARRAYSIZE(buf)) ? buf : std::filesystem::path();
}

/**
 * Loads the ReShade config from <reshade-module>.ini.
 */
static void loadConfig() {
    g_reshade_dll_path = getModulePath(g_reshade_module_handle);
    Logfile::get()->writeInfo(std::string() + "g_reshade_dll_path:" + g_reshade_dll_path.generic_u8string());

    // Parse the ReShade config file and get the used settings for the generic depth plugin.
    std::vector<std::string> preprocessorDefinitions;
    reshade::global_config().get("GENERAL", "PreprocessorDefinitions", preprocessorDefinitions);
    for (const std::string& preprocessorDefinition : preprocessorDefinitions) {
        auto equalSignPos = preprocessorDefinition.find("=");
        if (equalSignPos == std::string::npos) {
            continue;
        }
        std::string key = preprocessorDefinition.substr(0, equalSignPos);
        std::string value = preprocessorDefinition.substr(equalSignPos + 1);
        Logfile::get()->writeInfo("key: " + key + ", value: " + value);

        if (key == "RESHADE_DEPTH_INPUT_IS_UPSIDE_DOWN") {
            isDepthUpsideDown = fromString<int>(value.c_str());
        } else if (key == "RESHADE_DEPTH_INPUT_IS_REVERSED") {
            isDepthReversed = fromString<int>(value.c_str());
        } else if (key == "RESHADE_DEPTH_INPUT_IS_LOGARITHMIC") {
            isDepthLogarithmic = fromString<int>(value.c_str());
        } else if (key == "RESHADE_DEPTH_LINEARIZATION_FAR_PLANE") {
            farPlaneDist = fromString<float>(value.c_str());
        }
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        if (!reshade::init_addon()) {
            return FALSE;
        }

        // Create the log file in the directory "C:/Users/<user-name>/AppData/Roaming/reshade-grabber/".
        char userDataPath[MAX_PATH];
        SHGetSpecialFolderPathA(NULL, userDataPath, CSIDL_APPDATA, true);
        std::string appDataDirectory = std::string() + userDataPath + "/reshade-grabber/";
        std::string logfilePath = appDataDirectory + "logfile.html";
        for (std::string::iterator it = logfilePath.begin(); it != logfilePath.end(); ++it) {
            if (*it == '\\') *it = '/';
        }
        if (!std::filesystem::is_directory(appDataDirectory)) {
            std::filesystem::create_directory(appDataDirectory);
        }
        Logfile::get()->createLogfile(logfilePath.c_str(), "reshade-grabber");

        std::string programName = getProgramName();
        Logfile::get()->writeInfo(std::string() + "Application name: " + programName);

        appName = getProgramName();
        loadConfig();

        reshade::register_overlay("reshade-grabber", drawDebugMenu);
        reshade::register_event<reshade::addon_event::init_device>(onInitDevice);
        reshade::register_event<reshade::addon_event::destroy_device>(onDestroyDevice);
        reshade::register_event<reshade::addon_event::present>(onPresent);
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        reshade::unregister_overlay("reshade-grabber");
        reshade::unregister_event<reshade::addon_event::init_device>(onInitDevice);
        reshade::unregister_event<reshade::addon_event::destroy_device>(onDestroyDevice);
        reshade::unregister_event<reshade::addon_event::present>(onPresent);
    }

    return TRUE;
}
