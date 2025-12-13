cmake_minimum_required(VERSION 3.15)
project(MultiModelAgenticAI VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimizations
if(CMAKE_BUILD_TYPE STREQUAL "" OR CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_BUILD_TYPE Release)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall -Wextra")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g -Wall -Wextra")
endif()

# Options
option(BUILD_TESTS "Build tests" OFF)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Find packages
find_package(Threads REQUIRED)
find_package(CURL)

# If curl is not found, try pkg-config
if(NOT CURL_FOUND)
    find_program(PKG_CONFIG_EXECUTABLE pkg-config)
    if(PKG_CONFIG_EXECUTABLE)
        execute_process(
            COMMAND ${PKG_CONFIG_EXECUTABLE} --libs libcurl
            OUTPUT_VARIABLE CURL_LIBRARIES
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(CURL_LIBRARIES)
            set(CURL_FOUND TRUE)
        endif()
    endif()
endif()

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
    src/verbose.cpp
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
    src/verbose.hpp
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

# Link curl if available
if(CURL_FOUND)
    if(TARGET CURL::libcurl)
        target_link_libraries(multi_agent_llm PRIVATE CURL::libcurl)
    else()
        target_link_libraries(multi_agent_llm PRIVATE ${CURL_LIBRARIES})
    endif()
    target_compile_definitions(multi_agent_llm PRIVATE OLLAMA_SUPPORT)
else()
    # Try to find curl via system
    find_library(CURL_LIB curl)
    if(CURL_LIB)
        target_link_libraries(multi_agent_llm PRIVATE ${CURL_LIB})
        target_compile_definitions(multi_agent_llm PRIVATE OLLAMA_SUPPORT)
    endif()
endif()

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

