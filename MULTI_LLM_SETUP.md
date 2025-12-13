# Multi-LLM Agent Configuration Guide

This guide explains how to configure multiple agents with different LLM models to explore diverse behaviors, reasoning patterns, and "hallucination" characteristics.

## Quick Start

1. **Install Ollama** (if not already installed):
   ```bash
   # macOS
   brew install ollama
   
   # Linux
   curl -fsSL https://ollama.ai/install.sh | sh
   ```

2. **Pull the models you want to use**:
   ```bash
   ollama pull tinyllama
   ollama pull llama3.2:3b
   ollama pull gemma
   ollama pull mistral
   ollama pull llama2
   ollama pull codellama
   ollama pull neural-chat
   ```

3. **Start Ollama server**:
   ```bash
   ollama serve
   # Or just run: ollama run <model> (this starts the server automatically)
   ```

4. **Configure your agents** in `config/agents_multi_llm.yaml` (see example below)

5. **Run with the configuration**:
   ```bash
   ./multi_agent_llm --config config/agents_multi_llm.yaml --task "your task" --verbose
   ```

## Model Selection Methods

### Method 1: Simple Model Name (Recommended)
```yaml
agents:
  - id: agent1
    model_path: "gemma"  # System auto-detects as Ollama model
    trace_limit: 30
    memory_size: 6144
```

### Method 2: Explicit Ollama Prefix
```yaml
agents:
  - id: agent2
    model_path: "ollama:mistral"  # Explicit Ollama model
    trace_limit: 30
    memory_size: 6144
```

### Method 3: Model with Tag/Version
```yaml
agents:
  - id: agent3
    model_path: "llama3.2:3b"  # Model with specific tag
    trace_limit: 25
    memory_size: 4096
```

### Method 4: File-Based Models
```yaml
agents:
  - id: agent4
    model_path: "models/local_model.bin"  # Local file path
    trace_limit: 20
    memory_size: 4096
```

## How Model Detection Works

The system automatically detects Ollama models if:
- The `model_path` starts with `"ollama:"`, OR
- The `model_path` doesn't contain path separators (`/` or `\`) AND doesn't contain a dot (`.`)

Examples:
- `"gemma"` → Detected as Ollama model
- `"llama3.2:3b"` → Detected as Ollama model (colon is OK for tags)
- `"ollama:mistral"` → Detected as Ollama model
- `"models/gemma.bin"` → Detected as file-based model
- `"./model.bin"` → Detected as file-based model

## Available Ollama Models

Here are some popular models you can use:

### Small & Fast Models
- `tinyllama` - Very fast, 1.1B parameters
- `phi` - Microsoft's small model
- `llama3.2:1b` - Smallest Llama 3.2 variant

### Medium Models
- `llama3.2:3b` - Balanced Llama 3.2
- `gemma:2b` - Google's 2B model
- `mistral:7b` - Mistral 7B

### Large Models
- `llama2:13b` - Llama 2 13B
- `gemma:7b` - Google's 7B model
- `mistral:7b` - Mistral 7B
- `neural-chat:7b` - Neural Chat 7B

### Specialized Models
- `codellama` - Code-focused Llama
- `llama2-uncensored` - Uncensored variant
- `wizardcoder` - Code generation specialist

## Configuration Example: Hallucination Study

To study how different models "hallucinate" or diverge in their responses:

```yaml
agents:
  # Fast model - may hallucinate more
  - id: fast_agent
    model_path: "tinyllama"
    trace_limit: 20
    memory_size: 2048
    
  # Balanced model
  - id: balanced_agent
    model_path: "llama3.2:3b"
    trace_limit: 25
    memory_size: 4096
    
  # High-quality model - fewer hallucinations
  - id: quality_agent
    model_path: "mistral"
    trace_limit: 35
    memory_size: 8192
```

Then run the same task with different agents:
```bash
# Task with fast agent
./multi_agent_llm --config config/agents_multi_llm.yaml --task "explain quantum computing" --agent fast_agent

# Same task with quality agent
./multi_agent_llm --config config/agents_multi_llm.yaml --task "explain quantum computing" --agent quality_agent
```

## Running Multiple Agents Concurrently

The system supports multiple agents running concurrently, each with their own model:

```yaml
agents:
  - id: agent_gemma
    model_path: "gemma"
    trace_limit: 30
    memory_size: 6144
    
  - id: agent_mistral
    model_path: "mistral"
    trace_limit: 30
    memory_size: 6144
    
  - id: agent_llama
    model_path: "llama3.2:3b"
    trace_limit: 30
    memory_size: 6144
```

All agents will be created and can process tasks independently. Each agent maintains its own:
- LLM instance (different model)
- Memory and trace history
- World model
- Message queue

## Tips for Hallucination Studies

1. **Use the same prompt** across different models to compare responses
2. **Enable verbose mode** (`--verbose`) to see detailed reasoning steps
3. **Compare trace histories** - different models will have different reasoning patterns
4. **Monitor memory usage** - larger models may need more memory
5. **Use different trace limits** - some models benefit from longer context

## Troubleshooting

### Model Not Found
If you get an error about a model not being found:
1. Check that Ollama is running: `ollama list`
2. Pull the model: `ollama pull <model_name>`
3. Verify the model name matches exactly (case-sensitive)

### Connection Errors
If you get connection errors:
1. Ensure Ollama server is running: `ollama serve`
2. Check the default URL is correct: `http://localhost:11434`
3. For remote Ollama, modify the URL in the code or config

### Performance Issues
- Smaller models (tinyllama, phi) are faster but may have more hallucinations
- Larger models (mistral, llama2:13b) are slower but more accurate
- Adjust `trace_limit` and `memory_size` based on your needs

## Advanced: Custom Ollama URL

To use a remote Ollama server, you would need to modify the code to pass a custom URL. Currently, the default is `http://localhost:11434`.

## See Also

- `config/agents_ollama.yaml` - Basic Ollama configuration example
- `config/agents_multi_llm.yaml` - Multi-model configuration example
- `OLLAMA_INTEGRATION.md` - Detailed Ollama integration guide

