#!/bin/bash
# Quick test script to show agent thinking process

echo "Testing agent thinking output..."
echo "Make sure Ollama is running and tinyllama is pulled:"
echo "  ollama pull tinyllama"
echo ""
echo "Running tinyllama_agent with a simple task..."
echo ""

./build/multi_agent_llm \
  --config config/agents_multi_llm.yaml \
  --task "what is a catalyst" \
  --agent tinyllama_agent \
  --verbose
