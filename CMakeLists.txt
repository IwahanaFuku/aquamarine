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

target_link_libraries(aquamarine PRIVATE
  glfw
  glad_local
  imgui_lib
)
