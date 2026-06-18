cmake_minimum_required(VERSION 3.20)
project(bankara_backend CXX)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Drogon package

# Find Drogon package
find_package(Drogon CONFIG REQUIRED)

# Add source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Set include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/third_party/jwt-cpp/include
    ${PROJECT_SOURCE_DIR}/third_party/picojson
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)

# Copy static files directory (if needed for serving static content)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/static
)

# Copy config.json to the output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/config.json
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json
)