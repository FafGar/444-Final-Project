cmake_minimum_required (VERSION 3.0)

set (CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

set (glm_DIR C:/GLM/share/glm)
set (glfw3_DIR C:/GLFW/lib/cmake/glfw3)

project (finalProject)

if(MSVC)
  add_definitions(/DNOMINMAX)
endif()

if(UNIX)
    message(STATUS "Running Linux compiling. Options [GLVND] for modern OpenGL or [LEGACY] for historical OpenGl.")
    message(STATUS "Setting GLVND option.")
    set(OpenGL_GL_PREFERENCE GLVND)
endif()

find_package( glm CONFIG REQUIRED )
find_package( glfw3 CONFIG REQUIRED )
find_package( OpenGL REQUIRED )

include_directories( ingredients )

add_subdirectory( ingredients )
add_subdirectory( src )
add_subdirectory( src/shader )
