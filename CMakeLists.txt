cmake_minimum_required(VERSION 3.16)
project(main)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(SDL EXCLUDE_FROM_ALL)
add_subdirectory(SDL_image EXCLUDE_FROM_ALL)
add_subdirectory(SDL_ttf EXCLUDE_FROM_ALL)

add_executable(main WIN32 main.c)

target_link_libraries(main PRIVATE SDL3::SDL3 SDL3_image-shared SDL3_ttf-shared)

target_include_directories(main PRIVATE
    SDL/include/SDL3
    SDL_image/include/SDL3_image
    SDL_ttf/include/SDL3_ttf
)