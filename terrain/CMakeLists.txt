cmake_minimum_required(VERSION 3.20)
project(fromville_terrain VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
find_package(OpenGL REQUIRED)

# ── GLFW ──
find_package(glfw3 QUIET)
if(NOT TARGET glfw)
    message(STATUS "Fetching GLFW...")
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4)
    FetchContent_MakeAvailable(glfw)
endif()

# ── GLM ──
find_package(glm CONFIG QUIET)
if(NOT TARGET glm::glm)
    message(STATUS "Fetching GLM...")
    FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1)
    FetchContent_MakeAvailable(glm)
endif()

# ── GLAD (generate from source) ──
find_package(glad CONFIG QUIET)
if(NOT TARGET glad::glad)
    # Use vendored glad source
    set(GLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include")
    add_library(glad_terrain STATIC "${CMAKE_CURRENT_SOURCE_DIR}/glad.cpp")
    target_include_directories(glad_terrain PUBLIC "${GLAD_DIR}")
    add_library(glad::glad ALIAS glad_terrain)
endif()

# ── Executable ──
add_executable(fromville_terrain main.cpp)

target_include_directories(fromville_terrain PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(fromville_terrain PRIVATE
    OpenGL::GL
    glfw
    glm::glm
    glad::glad
)

if(MSVC)
    target_compile_definitions(fromville_terrain PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Copy assets to build output
add_custom_command(TARGET fromville_terrain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:fromville_terrain>/shaders"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:fromville_terrain>/textures"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/models/textures/coast_sand_rocks_02_diff_2k.jpg"
        "$<TARGET_FILE_DIR:fromville_terrain>/textures/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/models/textures/coast_sand_rocks_02_nor_gl_2k.png"
        "$<TARGET_FILE_DIR:fromville_terrain>/textures/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/models/textures/coast_sand_rocks_02_rough_2k.png"
        "$<TARGET_FILE_DIR:fromville_terrain>/textures/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../assets/models/vegetation_grass_card_03.png"
        "$<TARGET_FILE_DIR:fromville_terrain>/textures/"
    COMMENT "Copying terrain assets to build output"
)
