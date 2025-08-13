cmake_minimum_required(VERSION 3.28)
project(NekiVK LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#Define library target
file(GLOB_RECURSE NEKIVK_SOURCES "src/*.cpp")
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
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
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

#Assimp
FetchContent_Declare(assimp GIT_REPOSITORY "https://github.com/assimp/assimp.git" GIT_TAG v5.4.1)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)
target_link_libraries(NekiVK PRIVATE assimp)


#Copy all NekiVK resource files to the output directory, maintaining subdirectory structure
file(COPY "${CMAKE_SOURCE_DIR}/NekiVK Resource Files/" DESTINATION "${CMAKE_BINARY_DIR}/NekiVK Resource Files/")








#Test executables - check only passes if building the NekiVK library directly, not when it's being included by another project via FetchContent
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    
    #Shader compilation
    find_program(
            GLSLC_EXECUTABLE
            NAMES glslc
            HINTS "$ENV{VULKAN_SDK}/bin"
            REQUIRED
            DOC "Path to the GLSL to SPIR-V compiler"
    )
    message(STATUS "Found glslc: ${GLSLC_EXECUTABLE}")

    #Define shader files
    file(GLOB_RECURSE SHADER_SOURCES
            "${CMAKE_SOURCE_DIR}/Tests/Shaders/*.vert"
            "${CMAKE_SOURCE_DIR}/Tests/Shaders/*.frag"
    )
    message(STATUS "---- Found Shader Files ----")
    foreach(FILE ${SHADER_SOURCES})
        message(STATUS "  ${FILE}")
    endforeach()
    message(STATUS "--------------------------------")

    #Get list of all the full paths of all the compiled .spv files and compile them
    set(SPIRV_OUTPUTS "")
    foreach(SHADER_SRC ${SHADER_SOURCES})
        get_filename_component(SHADER_FILENAME ${SHADER_SRC} NAME)
        set(SHADER_SPV "${PROJECT_BINARY_DIR}/Tests/Shaders/${SHADER_FILENAME}.spv")
        list(APPEND SPIRV_OUTPUTS ${SHADER_SPV})

        #Compile shader
        add_custom_command(
                OUTPUT ${SHADER_SPV}
                COMMAND ${GLSLC_EXECUTABLE} ${SHADER_SRC} -o ${SHADER_SPV}
                DEPENDS ${SHADER_SRC}
                COMMENT "Compiling ${SHADER_SRC} to ${SHADER_SPV}"
        )
    endforeach()

    #Create phony target that depends on all the compiled shaders
    add_custom_target(
            Shaders
            DEPENDS ${SPIRV_OUTPUTS}
    )
    
    #Copy all resource files to the output directory, maintaining subdirectory structure
    file(COPY "${CMAKE_SOURCE_DIR}/Tests/Resource Files/" DESTINATION "${CMAKE_BINARY_DIR}/Tests/Resource Files/")

    
    
    #Create executables
    add_executable(NekiVK_LibraryTest "Tests/LibraryTest.cpp")
    target_link_libraries(NekiVK_LibraryTest PRIVATE NekiVK)
    add_dependencies(NekiVK_LibraryTest Shaders)

    add_executable(NekiVK_ModelTest "Tests/ModelTest.h" "Tests/ModelTest.cpp")
    target_link_libraries(NekiVK_ModelTest PRIVATE NekiVK)
    add_dependencies(NekiVK_ModelTest Shaders)
    
    
endif()
