cmake_minimum_required(VERSION 3.10)
project(TaskQueueServer)

# Find PostgreSQL (alternative method)
find_path(POSTGRESQL_INCLUDE_DIR libpq-fe.h
          HINTS "/usr/include/postgresql" "/usr/local/include/postgresql")
find_library(POSTGRESQL_LIBRARY pq
             HINTS "/usr/lib" "/usr/local/lib" "/usr/lib/x86_64-linux-gnu")

if(POSTGRESQL_INCLUDE_DIR AND POSTGRESQL_LIBRARY)
    add_library(PostgreSQL::client SHARED IMPORTED)
    set_target_properties(PostgreSQL::client PROPERTIES
        IMPORTED_LOCATION "${POSTGRESQL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${POSTGRESQL_INCLUDE_DIR}"
    )
    message(STATUS "Found PostgreSQL: ${POSTGRESQL_LIBRARY}")
else()
    message(FATAL_ERROR "PostgreSQL not found")
endif()

# Find Poco components
find_package(Poco REQUIRED COMPONENTS Net Util Foundation DataPostgreSQL)

# Add executable
add_executable(task_queue_server task_queue_server.cpp)

# Link libraries
target_link_libraries(task_queue_server
    Poco::Net
    Poco::Util
    Poco::Foundation
    Poco::DataPostgreSQL
    PostgreSQL::client
)