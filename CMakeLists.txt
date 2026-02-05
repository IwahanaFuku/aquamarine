cmake_minimum_required(VERSION 3.20)
project(MyModeler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# -----------------------------
# GLFW
# -----------------------------
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.9
)
FetchContent_MakeAvailable(glfw)

# -----------------------------
# glad (OpenGL loader) - vendored
# -----------------------------
add_library(glad_local STATIC
  external/glad/src/glad.c
)

target_include_directories(glad_local PUBLIC
  external/glad/include
)

target_link_libraries(glad_local PUBLIC opengl32)

# -----------------------------
# glm
# -----------------------------
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# -----------------------------
# Dear ImGui
# -----------------------------
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.90.1
)
FetchContent_MakeAvailable(imgui)

# ImGui はCMakeターゲットを提供しないので、自前でライブラリ化
add_library(imgui_lib STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  # backends
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui_lib PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui_lib PUBLIC glfw)


# -----------------------------
# Executable
# -----------------------------
add_executable(aquamarine
  src/main.cpp
)

target_include_directories(aquamarine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_sources(aquamarine PRIVATE
    src/app/app.cpp
    src/app/app.h
    src/camera/orbit_camera.cpp
    src/camera/orbit_camera.h
    src/platform/glfw_system.cpp
    src/platform/glfw_system.h
    src/platform/imgui_context_guard.cpp
    src/platform/imgui_context_guard.h
    src/platform/input.cpp
    src/platform/input.h
    src/platform/platform.cpp
    src/platform/platform.h
    src/platform/window.h
    src/render/geometry_gen.cpp
    src/render/geometry_gen.h
    src/render/line_mesh.cpp
    src/render/line_mesh.h
    src/render/line_program.cpp
    src/render/line_program.h
    src/render/picker.cpp
    src/render/picker.h
    src/render/renderer.cpp
    src/render/renderer.h
    src/render/shader_utils.cpp
    src/render/shader_utils.h
    src/render/vertex.h
)

add_custom_command(
    TARGET aquamarine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:aquamarine>/assets
)

target_link_libraries(aquamarine PRIVATE
  glfw
  glad_local
  imgui_lib
  glm::glm
)

target_compile_definitions(aquamarine PRIVATE GLFW_INCLUDE_NONE)