# 10MB-video-compressor for Discord V1.1
A little project I made in around 12 days that automatically compresses mp4 files given to it using hardware-accelerated AV1 encoding on supported vendors into the 10MB file limit for Discord users without Nitro.
It runs in the background without opening any terminal windows.

To use it you need to either drag and drop an MP4 file onto the executable in the Windows File Explorer or open the MP4 with the compiled executable.

OS: Windows 11
Dependencies: Vulkan, FFmpeg

Error code explanation: 

Code 0: No error.
Code 1: Generic error.
Code 2: No file argument was provided, check if you opened the MP4 with the executable.
Code 3: The GPU does not support AV1.
Code 4: FFmpeg is not in the system path or is not installed.
Code 5: Unknown GPU vendor.
