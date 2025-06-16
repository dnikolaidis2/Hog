# Hog

Hog was designed as a support library to simplify the process of writing Vulkan applications, allowing developers to dive directly into graphics programming. Its design was heavily inspired by the architecture of [TheCherno/Hazel](https://github.com/TheCherno/Hazel) and [TheCherno/OpenGL](https://github.com/TheCherno/OpenGL).

**⚠️ Disclaimer:** This project is no longer maintained or actively developed. It served as a testbed for early C++ concepts and as an introduction to complex framework and engine design, with mixed results. This repository is preserved as a personal archive to document past successes and mistakes. Readers exploring the code should be aware of its experimental nature.

-----

## Libraries Used

This project incorporates a number of external libraries to provide its functionality:

  - **[Boost](https://www.boost.org/)**: A collection of high-quality, peer-reviewed C++ libraries.
  - **[cgltf](https://github.com/jkuhlmann/cgltf)**: A single-file glTF 2.0 loader and writer.
  - **[GLFW](https://www.glfw.org/)**: A multi-platform windowing library for OpenGL, OpenGL ES, and Vulkan development.
  - **[glm](https://github.com/g-truc/glm)**: A header-only C++ mathematics library for graphics software.
  - **[ImGui](https://github.com/ocornut/imgui)**: A bloat-free graphical user interface library for C++.
  - **[Optick](https://github.com/bombomby/optick)**: A C++ profiler for games.
  - **[shaderc](https://github.com/google/shaderc)**: A collection of tools, libraries, and tests for shader compilation.
  - **[SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross)**: A tool for parsing and converting SPIR-V to other shader languages.
  - **[SPIRV-Reflect](https://github.com/KhronosGroup/SPIRV-Reflect)**: A lightweight library for SPIR-V reflection.
  - **[stb\_image](https://www.google.com/search?q=https://github.com/nothings/stb/blob/master/stb_image.h)**: A single-file library for loading images.
  - **[tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)**: A tiny, single-file C++ wavefront obj loader.
  - **[VMA (Vulkan Memory Allocator)](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)**: An easy-to-integrate Vulkan memory allocation library.
  - **[volk](https://github.com/zeux/volk)**: A meta-loader for Vulkan.
  - **[yaml-cpp](https://github.com/jbeder/yaml-cpp)**: A YAML parser and emitter in C++.

## Requirements

To build this project, you will need the following:

  - **Vulkan SDK**: Version 1.1 or newer.
  - **C++20 Compiler**: The project was originally built using MSVC with Visual Studio 2019.
  - **Premake5**: A build configuration tool.

## Installation

1.  **Clone the repository:**

    ```bash
    git clone https://github.com/dnikolaidis2/Hog
    ```

2.  **Navigate to the project directory:**

    ```bash
    cd Hog
    ```

3.  **Initialize and update the Git submodules:**

    ```bash
    git submodule init
    git submodule update
    ```

4.  **Generate the project files.** Use Premake to generate the solution/project files for your development environment. For example, to generate a Visual Studio 2022 solution, run the following command:

    ```bash
    premake5 vs2022
    ```