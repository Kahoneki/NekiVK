cmake_minimum_required(VERSION 3.28)
project(NekiVK LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#Define library target
file(GLOB_RECURSE NEKIVK_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_library(NekiVK STATIC ${NEKIVK_SOURCES})
target_include_directories(NekiVK PUBLIC include)



#Propagate public dependencies
include(FetchContent)

#Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(NekiVK PUBLIC Vulkan::Vulkan)

#GLM
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 0.9.9.8)
FetchContent_MakeAvailable(glm)
target_link_libraries(NekiVK PUBLIC glm)

#GLFW
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG "3.4")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)
target_link_libraries(NekiVK PUBLIC glfw)



#Handle private dependencies

#stb_image
FetchContent_Declare(stb_image GIT_REPOSITORY https://github.com/nothings/stb.git GIT_TAG master)
FetchContent_MakeAvailable(stb_image)
target_include_directories(NekiVK PRIVATE ${stb_image_SOURCE_DIR})



#Test executable - check only passes if building the NekiVK library directly, not when it's being  included by another project via FetchContent
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_executable(NekiVK_Test "LibraryTest.cpp")
    target_link_libraries(NekiVK_Test PRIVATE NekiVK)
endif()