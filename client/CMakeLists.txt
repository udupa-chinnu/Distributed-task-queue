cmake_minimum_required(VERSION 3.10)
project(TestClient)

# Find Poco libraries
find_package(Poco REQUIRED COMPONENTS Net JSON)

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(test_client 
    test_client.cpp
)

# Link libraries
target_link_libraries(test_client
    Poco::Net
    Poco::JSON
)

# Install for Docker
install(TARGETS test_client DESTINATION /app)