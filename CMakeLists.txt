cmake_minimum_required(VERSION 3.10)
project(spotify-cli)

set(CMAKE_CXX_STANDARD 17)

find_package(cpr REQUIRED)
find_package(nlohmann_json REQUIRED)

# Add executable with source files
add_executable(spotify-cli
    main.cpp
    spotify_api.cpp
    login.cpp
    token.cpp
    spotify_api.h
    base64.h
)

# Include directories for headers
target_include_directories(spotify-cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(spotify-cli PRIVATE cpr::cpr nlohmann_json::nlohmann_json)



# cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/home/monolith/vcpkg/scripts/buildsystems/vcpkg.cmake
# cmake --build build



#code=AQCGgjt2Lv8Takz6HODddumKy2dtomP6eP6ZzZceTKHZo-uwCn6eRhDPLYzq_r5d11WHY9TtZLBJaIlGnWPS_sB1xzE2wUbVqpb_Hl0uC8F_xaP3ZmExWFDQdp1zUih69gb3-Geeah3QgDTMdbtgcrWGBiOu3WKYfkbv7NoyE3NwOH9xjsfi8GRXlHzCuaOjAqWHis5CPw5bSvOSixUjVSDM6o-BT4zaUwcr_rmw6IvwcjIBzIIkJKLBEUsIvOODoG4Neqk7y2ynbu2r_0L5xvKVxLOmFHMXFsISLQ&state=pwXnNZjOm5u9OIy7