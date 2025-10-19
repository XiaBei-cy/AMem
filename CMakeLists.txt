cmake_minimum_required(VERSION 3.16)
project(ImGuiProject)

set(CMAKE_CXX_STANDARD 17)

# 选项：选择渲染后端
option(USE_DX11 "Use DirectX 11 backend" OFF)
option(USE_DX12 "Use DirectX 12 backend" ON)
#option(USE_VULKAN "Use Vulkan backend" OFF)

message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")


#设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# 查找 Capstone 库
set(CAPSTONE_ROOT "C:/Program Files/capstone" CACHE PATH "Capstone installation directory")

find_package(capstone CONFIG QUIET)
if(capstone_FOUND)
    message(STATUS "Capstone found")
    # 直接手动查找库文件和头文件，添加搜索路径
    find_library(CAPSTONE_LIBRARY 
        NAMES capstone capstone_static
        HINTS "${CAPSTONE_ROOT}/lib" "${CAPSTONE_ROOT}/build" "${CAPSTONE_ROOT}"
        PATH_SUFFIXES lib lib64)
    find_path(CAPSTONE_INCLUDE_DIR 
        NAMES capstone/capstone.h
        HINTS "${CAPSTONE_ROOT}/include" "${CAPSTONE_ROOT}"
        PATH_SUFFIXES include)
    message(STATUS "Capstone library: ${CAPSTONE_LIBRARY}")
    message(STATUS "Capstone include: ${CAPSTONE_INCLUDE_DIR}")
        
        if(CAPSTONE_LIBRARY AND CAPSTONE_INCLUDE_DIR)
            set(CAPSTONE_LIBRARIES ${CAPSTONE_LIBRARY})
            set(CAPSTONE_INCLUDE_DIRS ${CAPSTONE_INCLUDE_DIR})
            set(HAVE_CAPSTONE TRUE)
            message(STATUS "Capstone library: ${CAPSTONE_LIBRARY}")
            message(STATUS "Capstone include: ${CAPSTONE_INCLUDE_DIR}")
        else()
            message(WARNING "Capstone package found but files not accessible")
            set(HAVE_CAPSTONE FALSE)
        endif()
    else()
        # 手动查找 Capstone
        find_path(CAPSTONE_INCLUDE_DIR capstone/capstone.h)
        find_library(CAPSTONE_LIBRARY NAMES capstone)
        if(CAPSTONE_INCLUDE_DIR AND CAPSTONE_LIBRARY)
            message(STATUS "Capstone found manually")
            message(STATUS "Capstone include: ${CAPSTONE_INCLUDE_DIR}")
            message(STATUS "Capstone library: ${CAPSTONE_LIBRARY}")
            set(CAPSTONE_LIBRARIES ${CAPSTONE_LIBRARY})
            set(CAPSTONE_INCLUDE_DIRS ${CAPSTONE_INCLUDE_DIR})
            set(HAVE_CAPSTONE TRUE)
        else()
            message(WARNING "Capstone not found! Disassembly features will be disabled.")
            message(STATUS "To install Capstone, run: vcpkg install capstone:x64-windows")
            set(HAVE_CAPSTONE FALSE)
        endif()
    endif()


include_directories(
    imgui 
    imgui/backends
    gui
    PointerScan
    )

# 添加 Capstone 包含目录（如果需要）
if(HAVE_CAPSTONE)
    if(CAPSTONE_INCLUDE_DIRS)
        include_directories(${CAPSTONE_INCLUDE_DIRS})
        message(STATUS "Added Capstone include directory: ${CAPSTONE_INCLUDE_DIRS}")
    else()
        message(STATUS "Capstone include directories will be handled by target link")
    endif()
endif()

set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
    imgui/backends/imgui_impl_win32.cpp
)

set(GUI_SOURCES
    gui/Gui.cpp
    gui/Window.cpp
    gui/CEWindow.cpp
    gui/ServerConnectWindow.cpp
    gui/ModulesWindow.cpp
    gui/ScanWindow.cpp
    gui/MemoryViewerWindow.cpp
    gui/BreakpointWindow.cpp
    gui/ProcessListWindow.cpp
    gui/VersionWindow.cpp
    gui/PointerChainWindow.cpp
    gui/DisassemblyHelper.cpp
)

set(SOCKET_SOURCES
    socket/client_singleton.cpp
)

set(POINTERSCAN_SOURCES
    PointerScan/formatter.cpp
    PointerScan/scanner.cpp
 
)

if (USE_DX11)
    add_definitions(-DUSE_DX11)
    list(APPEND IMGUI_SOURCES imgui/backends/imgui_impl_dx11.cpp)
elseif (USE_DX12)
    add_definitions(-DUSE_DX12)
    list(APPEND IMGUI_SOURCES imgui/backends/imgui_impl_dx12.cpp)
endif()

#取消imgui的调试信息
add_definitions(-DIMGUI_DISABLE_DEBUG_TOOLS)
add_executable(ImGuiProject 
    main.cpp 
    PointerChainExample.cpp
    ${IMGUI_SOURCES} 
    ${GUI_SOURCES} 
    ${SOCKET_SOURCES} 
    ${POINTERSCAN_SOURCES}
)

# 链接 Windows 系统库
if (USE_DX11)
    target_link_libraries(ImGuiProject d3d11 dxgi d3dcompiler)
elseif (USE_DX12)
    target_link_libraries(ImGuiProject d3d12 dxgi d3dcompiler ws2_32)
endif()

# 链接 Capstone 库
if(HAVE_CAPSTONE)
    target_link_libraries(ImGuiProject ${CAPSTONE_LIBRARIES})
    target_compile_definitions(ImGuiProject PRIVATE HAVE_CAPSTONE)
    message(STATUS "Capstone will be linked to the project: ${CAPSTONE_LIBRARIES}")
else()
    message(STATUS "Building without Capstone support")
endif()
