cmake_minimum_required(VERSION 3.10)
project(WorkerNode)

# Find Poco libraries
find_package(Poco REQUIRED COMPONENTS Net JSON)

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(worker_node 
    worker_node.cpp
)

# Link libraries
target_link_libraries(worker_node
    Poco::Net
    Poco::JSON
)

# Install for Docker
install(TARGETS worker_node DESTINATION /app)