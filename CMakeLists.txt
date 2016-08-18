cmake_minimum_required(VERSION 3.5)
project(sticker3d)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp estimate.cpp setup_gl.cpp Shader.h)

set(DLIB_ROOT "/Users/npbool/Projects/dlib-18.18")
include_directories("${DLIB_ROOT}")
link_directories("${DLIB_ROOT}/dlib/build")
add_executable(sticker3d ${SOURCE_FILES})

FIND_LIBRARY(OPENGL_FRAMEWORK OpenGL)
target_link_libraries(sticker3d -ldlib -lopencv_calib3d -lopencv_core -lopencv_highgui -lopencv_imgproc ${OPENGL_FRAMEWORK} glew glfw3)