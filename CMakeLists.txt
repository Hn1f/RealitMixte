cmake_minimum_required(VERSION 3.16)
project(RenduARCube CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

# GLM header-only
find_path(GLM_INCLUDE_DIR glm/glm.hpp)
if(NOT GLM_INCLUDE_DIR)
  message(FATAL_ERROR "GLM not found. sudo apt install -y libglm-dev")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
foreach(cfg IN LISTS CMAKE_CONFIGURATION_TYPES)
  string(TOUPPER "${cfg}" cfgU)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfgU} "${CMAKE_CURRENT_SOURCE_DIR}")
endforeach()

add_executable(arcube
  main.cpp
  SceneObjects.cpp              # ✅ AJOUT ICI
  UtilsOpenCV/opencv_utils.cpp
  Shaders/shaders.cpp
  GLUtils/gl_utils.cpp
  ARMatrices/ar_matrices.cpp
  Geometries/geometries.cpp
  Texture/texture.cpp
  Smoothing/smoothing.cpp
)

target_include_directories(arcube PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/UtilsOpenCV
  ${CMAKE_CURRENT_SOURCE_DIR}/Shaders
  ${CMAKE_CURRENT_SOURCE_DIR}/GLUtils
  ${CMAKE_CURRENT_SOURCE_DIR}/ARMatrices
  ${CMAKE_CURRENT_SOURCE_DIR}/Geometries
  ${CMAKE_CURRENT_SOURCE_DIR}/Texture
  ${CMAKE_CURRENT_SOURCE_DIR}/Smoothing
  ${OpenCV_INCLUDE_DIRS}
  ${GLM_INCLUDE_DIR}
)

target_link_libraries(arcube PRIVATE
  ${OpenCV_LIBS}
  OpenGL::GL
  GLEW::GLEW
  glfw
)
