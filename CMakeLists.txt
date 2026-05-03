cmake_minimum_required(VERSION 3.20)

project(fromville VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FROMVILLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(FROMVILLE_USE_FETCHCONTENT "Use FetchContent fallback for selected libraries" ON)

if(MSVC)
    add_compile_options(/W4 /permissive-)
    if(FROMVILLE_WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(FROMVILLE_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
endif()

include(FetchContent)

find_package(OpenGL REQUIRED)

find_package(glfw3 QUIET)
if(NOT TARGET glfw)
    if(FROMVILLE_USE_FETCHCONTENT)
        message(STATUS "glfw3 not found; fetching GLFW")
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
        )
        FetchContent_MakeAvailable(glfw)
    else()
        message(FATAL_ERROR "glfw3 not found")
    endif()
endif()

find_package(glm CONFIG QUIET)
if(NOT TARGET glm::glm)
    if(FROMVILLE_USE_FETCHCONTENT)
        message(STATUS "glm not found; fetching GLM")
        FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG 1.0.1
        )
        FetchContent_MakeAvailable(glm)
    else()
        message(FATAL_ERROR "glm not found")
    endif()
endif()

find_package(assimp CONFIG QUIET)
if(NOT TARGET assimp::assimp)
    find_package(assimp REQUIRED)
endif()

find_package(OpenAL CONFIG REQUIRED)

find_package(SndFile QUIET)
if(NOT SndFile_FOUND)
    find_package(sndfile REQUIRED)
endif()

find_package(Freetype REQUIRED)

find_package(nlohmann_json CONFIG QUIET)
if(NOT TARGET nlohmann_json::nlohmann_json)
    if(FROMVILLE_USE_FETCHCONTENT)
        message(STATUS "nlohmann_json not found; fetching")
        FetchContent_Declare(
            nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.11.3
        )
        FetchContent_MakeAvailable(nlohmann_json)
    else()
        message(FATAL_ERROR "nlohmann_json not found")
    endif()
endif()

find_package(glad CONFIG QUIET)
if(NOT TARGET glad::glad)
    if(FROMVILLE_USE_FETCHCONTENT)
        message(STATUS "glad package not found. Provide glad::glad via package manager or vendor source in include/")
    endif()
endif()

file(GLOB_RECURSE FROMVILLE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(fromville ${FROMVILLE_SOURCES})

target_include_directories(fromville
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)

target_link_libraries(fromville
    PRIVATE
        OpenGL::GL
        glfw
        glm::glm
        Freetype::Freetype
        OpenAL::OpenAL
        nlohmann_json::nlohmann_json
)

if(TARGET assimp::assimp)
    target_link_libraries(fromville PRIVATE assimp::assimp)
else()
    target_link_libraries(fromville PRIVATE assimp)
endif()

if(TARGET SndFile::sndfile)
    target_link_libraries(fromville PRIVATE SndFile::sndfile)
elseif(TARGET sndfile::sndfile)
    target_link_libraries(fromville PRIVATE sndfile::sndfile)
elseif(TARGET sndfile)
    target_link_libraries(fromville PRIVATE sndfile)
endif()

if(TARGET glad::glad)
    target_link_libraries(fromville PRIVATE glad::glad)
else()
    target_compile_definitions(fromville PRIVATE FROMVILLE_NEEDS_GLAD=1)
endif()

if(MSVC)
    target_compile_definitions(fromville PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

add_custom_command(TARGET fromville POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:fromville>/assets
    COMMENT "Copying assets directory to runtime output"
)

# Force rebuild
