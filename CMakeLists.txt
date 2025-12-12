cmake_minimum_required(VERSION 3.15)
project(MultiModelAgenticAI VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTS "Build tests" OFF)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Find packages
find_package(Threads REQUIRED)

# llm.c submodule
set(LLM_C_DIR ${CMAKE_SOURCE_DIR}/third_party/llm.c)
if(EXISTS ${LLM_C_DIR})
    add_subdirectory(${LLM_C_DIR} EXCLUDE_FROM_ALL)
    include_directories(${LLM_C_DIR})
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/agent.cpp
    src/agent_manager.cpp
    src/memory.cpp
    src/llm_wrapper.cpp
    src/communication.cpp
    src/task_interface.cpp
    src/reporting.cpp
    src/security/security.cpp
    src/fault_tolerance/retry.cpp
    src/utils/thread_pool.cpp
    tests/test_framework.cpp
)

set(HEADERS
    src/agent.hpp
    src/agent_manager.hpp
    src/memory.hpp
    src/llm_wrapper.hpp
    src/communication.hpp
    src/task_interface.hpp
    src/reporting.hpp
    src/security/security.hpp
    src/fault_tolerance/retry.hpp
    src/distributed/network.hpp
    src/cache/cache_coherent.hpp
    src/protocol/protocol.hpp
    src/utils/atomicity.hpp
    src/utils/thread_pool.hpp
    tests/test_framework.hpp
)

# Executable
add_executable(multi_agent_llm ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(multi_agent_llm
    PRIVATE
    Threads::Threads
    crypto
    ssl
)

# If llm.c is available, link it
if(EXISTS ${LLM_C_DIR}/CMakeLists.txt)
    target_link_libraries(multi_agent_llm PRIVATE llm)
endif()

# Compiler flags
if(MSVC)
    target_compile_options(multi_agent_llm PRIVATE /W4)
else()
    target_compile_options(multi_agent_llm PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install
install(TARGETS multi_agent_llm
    RUNTIME DESTINATION bin
)

