# Ollama Integration Guide

This document explains how to use Ollama with the Multi-Model Agentic AI system.

## Overview

The system now supports **three backend types**:
1. **Ollama** - HTTP API to Ollama server (recommended for multi-agent systems)
2. **llm.c** - Local file-based models using Andrej Karpathy's llm.c
3. **Stub** - Testing mode without actual models

## Prerequisites

1. **Install Ollama**: https://ollama.ai
   ```bash
   # macOS/Linux
   curl -fsSL https://ollama.ai/install.sh | sh
   
   # Or download from https://ollama.ai/download
   ```

2. **Start Ollama server**:
   ```bash
   ollama serve
   ```
   The server runs on `http://localhost:11434` by default.

3. **Pull models** you want to use:
   ```bash
   ollama pull llama2
   ollama pull mistral
   ollama pull codellama
   ```

## Configuration

### Using Ollama Models

In your agent configuration file (YAML), specify Ollama model names instead of file paths:

```yaml
agents:
  - id: research_agent
    model_path: "llama2"  # Ollama model name
    trace_limit: 30
    memory_size: 8192
    
  - id: analysis_agent
    model_path: "mistral"  # Different Ollama model
    trace_limit: 25
    memory_size: 6144
```

### Explicit Ollama Prefix (Optional)

You can also use the `ollama:` prefix:

```yaml
agents:
  - id: research_agent
    model_path: "ollama:llama2"
    trace_limit: 30
    memory_size: 8192
```

### File-Based Models (llm.c)

For file-based models, use file paths:

```yaml
agents:
  - id: local_agent
    model_path: "models/my_model.bin"  # File path
    trace_limit: 20
    memory_size: 4096
```

## How It Works

1. **Automatic Detection**: The system automatically detects if `model_path` is:
   - An Ollama model name (no path separators, no file extension)
   - A file path (contains `/` or `\`, or has file extension)

2. **Multiple Instances**: Each agent can use a **different Ollama model**:
   - Agent 1 → `llama2`
   - Agent 2 → `mistral`
   - Agent 3 → `codellama`
   - All running concurrently via Ollama's HTTP API

3. **Ollama Server**: All agents connect to the same Ollama server instance, which manages:
   - Model loading/unloading
   - GPU memory allocation
   - Concurrent request handling

## Usage Examples

### Basic Usage

```bash
# Use Ollama models from config
./multi_agent_llm --config config/agents_ollama.yaml --task "research zumba" --verbose

# Use specific agent
./multi_agent_llm --config config/agents_ollama.yaml --task "analyze data" --agent research_agent
```

### Custom Ollama URL

The system defaults to `http://localhost:11434`. To use a different Ollama server, you would need to modify the code (future enhancement could add this to config).

## Benefits of Ollama Integration

1. **Multiple Models**: Each agent can use a different model simultaneously
2. **Easy Management**: Ollama handles model loading, memory, and GPU allocation
3. **Scalability**: Ollama server can handle many concurrent requests
4. **No File Management**: No need to manage model files directly
5. **Model Variety**: Easy to switch between different models

## Troubleshooting

### "Could not connect to Ollama"

- Make sure Ollama is running: `ollama serve`
- Check if Ollama is accessible: `curl http://localhost:11434/api/tags`
- Verify the model is pulled: `ollama list`

### Model Not Found

- Pull the model first: `ollama pull <model_name>`
- Check available models: `ollama list`
- Verify model name in config matches Ollama model name

### Compilation Issues

If curl is not found during compilation:
- macOS: `brew install curl`
- Linux: `sudo apt-get install libcurl4-openssl-dev` (Debian/Ubuntu)
- The system will fall back to stub mode if curl is unavailable

## Example Configuration Files

See `config/agents_ollama.yaml` for a complete example.

## Technical Details

- **HTTP Client**: Uses libcurl for HTTP requests
- **API Endpoint**: `POST http://localhost:11434/api/generate`
- **Request Format**: JSON with model name, prompt, and generation parameters
- **Response Format**: JSON with generated text in `response` field
- **Timeout**: 5 minutes per request

## Future Enhancements

- [ ] Configurable Ollama URL per agent
- [ ] Streaming support for real-time generation
- [ ] Model health checking
- [ ] Automatic model pulling if not available
- [ ] Connection pooling for better performance

