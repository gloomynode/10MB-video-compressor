/*
 * Copyright (c) 2026 gloomynode
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <array>
#include <charconv>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <sstream>

#include <vulkan/vulkan.h>

#include <Windows.h>

//thank you code from the internet
void getGpuVendor(uint32_t &vendor_id) {
    VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    VkInstance instance;
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) return;

    VkPhysicalDevice physicalDevice;
    uint32_t count = 1;
    vkEnumeratePhysicalDevices(instance, &count, &physicalDevice);

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice, &props);

    std::cout << "GPU Name: " << props.deviceName << "\n";

    vendor_id = props.vendorID;

    vkDestroyInstance(instance, nullptr);
}

//thank you code from the internet
bool hasAV1EncodeSupport() {
    VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    VkInstance instance;
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) return false;

    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
    if (gpuCount == 0) return false;

    std::vector<VkPhysicalDevice> gpus(gpuCount);
    vkEnumeratePhysicalDevices(instance, &gpuCount, gpus.data());

    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(gpus[0], nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extCount);
    vkEnumerateDeviceExtensionProperties(gpus[0], nullptr, &extCount, extensions.data());

    bool av1Supported = false;
    for (const auto& ext : extensions) {
        if (std::strcmp(ext.extensionName, "VK_KHR_video_encode_av1") == 0) {
            av1Supported = true;
            break;
        }
    }

    vkDestroyInstance(instance, nullptr);
    return av1Supported;
}

void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), s.end());
}

struct encoder_vars{
    std::string video_bitrate_string = "0";
    uint32_t video_bitrate = 0;
    uint32_t target_video_bitrate = 0;
    uint32_t audio_bitrate = 0;
    std::string audio_bitrate_string = "0";
    uint64_t total_bitrate = 0;
    uint32_t video_length_seconds = 0;
    std::string video_length_seconds_string = "0";
};

std::string exec(const std::string& cmd) {
    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return "";
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    std::string result;
    if (CreateProcessA(NULL, const_cast<char*>(cmd.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hWrite);

        std::array<char, 256> buffer;
        DWORD bytesRead;
        while (ReadFile(hRead, buffer.data(), static_cast<DWORD>(buffer.size() - 1), &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            result += buffer.data();
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        CloseHandle(hWrite);
    }

    CloseHandle(hRead);
    return result;
}

int main(int argc, char* argv[1])
{
    std::string nothing;
    //used for debugging

    if (argc < 2) {
        return 2;
    }

    encoder_vars ev;
    uint32_t gpu_vendor = 0x0;
    std::string arg1 = argv[1];

    //for debugging
/*
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stdin);
*/
    std::string command = "cmd.exe /c \"where ffmpeg >nul 2>&1 && echo FFmpeg exists\"";
    std::string ffmpeg_check = exec(command.c_str());
    rtrim(ffmpeg_check);
    std::cout<<"ffmpeg_check: " << ffmpeg_check << " \n";

    if(ffmpeg_check != "FFmpeg exists"){
        return 4;
    }

    std::string video_out = exec(("ffprobe -v error -select_streams v:0 -show_entries stream=bit_rate -of default=noprint_wrappers=1:nokey=1 \"" + arg1 + "\"").c_str());
    rtrim(video_out);
    std::from_chars(video_out.data(), video_out.data() + video_out.size(), ev.video_bitrate);
    std::cout<<"video: " << ev.video_bitrate << " \n";

    std::string audio_out = exec(("ffprobe -v error -select_streams a:0 -show_entries stream=bit_rate -of default=noprint_wrappers=1:nokey=1 \"" + arg1 + "\"").c_str());
    rtrim(audio_out);
    std::from_chars(audio_out.data(), audio_out.data() + audio_out.size(), ev.audio_bitrate);
    std::cout<<"audio: " << ev.audio_bitrate << " \n";

    std::string seconds_out = exec(("ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"" + arg1 + "\"").c_str());
    rtrim(seconds_out);
    std::from_chars(seconds_out.data(), seconds_out.data() + seconds_out.size(), ev.video_length_seconds);
    std::cout<<"length: " << ev.video_length_seconds << " \n";


    ev.total_bitrate = 76000000 / ev.video_length_seconds;
    std::cout<<"finnished length calculation \n";
    ev.video_bitrate = ev.total_bitrate - ev.audio_bitrate;
    ev.video_bitrate_string = std::to_string(ev.video_bitrate);

    std::cout<<"total_bitrate_is: " << ev.total_bitrate << " \n";

    getGpuVendor(gpu_vendor);
    std::string encoder_type = "libx265";

    if (hasAV1EncodeSupport()){
    switch (gpu_vendor) {
    case 0x10DE:
        encoder_type = "av1_nvenc";
        break;
    case 0x1002:
        encoder_type = "av1_amf";
        break;
    case 0x8086:
        encoder_type = "av1_qsv";
        break;
    default: return 5; break;
    } }
    else {
//        return 3;
    }


    std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch());

    std::string ms_string = std::to_string(ms.count());

    std::string output_file_name;

    output_file_name = "output_" + ms_string + ".mp4";

    std::string technical_debt_out =
    exec(("ffmpeg -i \"" + arg1 + "\" -c:v " + encoder_type + " -b:v " + ev.video_bitrate_string + " -maxrate " + ev.video_bitrate_string + " -bufsize " + ev.video_bitrate_string + " -c:a copy " + output_file_name).c_str());

    return 0;
}


