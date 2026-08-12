cmake_minimum_required(VERSION 3.16)

project(discord-10MB-compressor LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
add_executable(discord-10MB-compressor WIN32
    main.cpp    
    resources.rc
)

find_package(Vulkan REQUIRED)

if(MINGW)
    target_link_options(discord-10MB-compressor PRIVATE
        -mwindows
        -static-libgcc
        -static-libstdc++
        -static     
        $<$<CONFIG:Release>:-s>
    )
    target_link_libraries(discord-10MB-compressor PRIVATE
        Vulkan::Vulkan
    )
endif()

include(GNUInstallDirs)
install(TARGETS discord-10MB-compressor
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
