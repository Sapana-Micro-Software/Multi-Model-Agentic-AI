# Multi-Model Agentic AI

Copyright (C) 2025, Shyamal Suhana Chandra  

A comprehensive, production-ready C++ multi-agent system where each agent uses llm.c for LLM inference, maintains working memory with MDL-normalized context, ruminates on tasks using chain-of-thought reasoning, and can communicate via direct messaging.

## Key Characteristics

The system implements all of the following characteristics:
- ✅ **Modular**: Component-based architecture
- ✅ **Fault-Tolerant**: Retry mechanisms, circuit breakers, graceful degradation
- ✅ **Secure**: Input validation, encryption, secure communication
- ✅ **Atomic**: Transaction support, rollback capabilities
- ✅ **Concurrent**: Thread-safe operations, synchronized access
- ✅ **Parallel**: Thread pools, parallel execution
- ✅ **Distributed**: Network communication, remote agents
- ✅ **Cache Coherent**: MESI-like protocol, distributed caching
- ✅ **Encrypted**: Data at rest and in transit
- ✅ **Protocol-Driven**: Formal message protocols
- ✅ **Robust**: Comprehensive error handling
- ✅ **Asynchronous**: Non-blocking operations
- ✅ **Producer-Consumer**: Bounded queues, efficient signaling
- ✅ **Synchronized**: Proper synchronization primitives
- ✅ **Optimized**: Performance optimizations
- ✅ **Lightweight**: Minimal overhead

## Features

- **Multiple Agents**: Each agent has its own LLM instance and memory
- **MDL-Normalized Context**: Custom encoding scheme for efficient context storage
- **Chain-of-Thought Reasoning**: Agents ruminate on tasks with structured reasoning
- **Inter-Agent Communication**: Direct messaging between agents
- **Configurable Trace Limits**: Per-agent working memory with recursion limits
- **Multiple Interfaces**: CLI, config file, and C++ API

## Building

### Prerequisites

- C++17 compatible compiler
- CMake 3.15 or later
- Git (for submodules)

### Build Steps

```bash
# Clone repository with submodules
git submodule update --init --recursive

# Build
mkdir build && cd build
cmake ..
make

# Run
./multi_agent_llm --task "keyword" --agent agent1 --config ../config/agents.yaml
```

## Configuration

See `config/agents.yaml` for example agent configurations.

## Usage

### Command Line

```bash
# Basic usage with task and agent
./multi_agent_llm --task "research quantum computing" --agent agent1

# Using configuration file
./multi_agent_llm --config config/agents.yaml --task "analyze data"

# With specific agent from config
./multi_agent_llm --config config/agents.yaml --task "research topic" --agent research_agent
```

### C++ API

```cpp
#include "agent_manager.hpp"
#include "task_interface.hpp"
#include "reporting.hpp"

// Create agent manager
agent::AgentManager manager;

// Create agent from config
agent::AgentConfig config;
config.id = "my_agent";
config.model_path = "models/my_model.bin";
config.trace_limit = 20;
config.memory_size = 4096;
manager.createAgent(config);

// Create task API
task_interface::TaskAPI task_api(&manager);

// Submit task
std::string result = task_api.submitTask("research topic", "my_agent");

// Set up reporting
auto reporter = std::make_unique<reporting::ConsoleReporter>();
// Use reporter to output results
```

See `examples/example_usage.cpp` for a complete example.

## Architecture

- **Agent**: Individual agent with LLM, memory, and trace management
- **AgentManager**: Creates and manages multiple agents
- **Memory System**: MDL-normalized context with trace recursion limits
- **Communication**: Direct messaging between agents
- **Reporting**: Console, file, and callback outputs

## License

Copyright (C) 2025, Shyamal Suhana Chandra

