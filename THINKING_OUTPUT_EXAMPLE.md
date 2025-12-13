# Agent Thinking Process Output

When you run an agent with `--verbose`, you'll now see the agent's thinking process in real-time as it processes your task.

## Example Output Structure

When you run:
```bash
./build/multi_agent_llm \
  --config config/agents_multi_llm.yaml \
  --task "explain quantum computing" \
  --agent tinyllama_agent \
  --verbose
```

You'll see output like this:

```
======================================================================
🤖 AGENT: tinyllama_agent | TASK: explain quantum computing
======================================================================

[VERBOSE] ========== PHASE: RUMINATION: tinyllama_agent - Task: explain quantum computing ==========
[VERBOSE] [Rumination] [Step 1] Generating initial reasoning steps

[THINKING] tinyllama_agent is generating reasoning steps...

[THINKING OUTPUT - Reasoning Steps]
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
1. Understand what quantum computing is
2. Explain quantum bits (qubits) vs classical bits
3. Describe superposition and entanglement
4. Discuss potential applications
5. Mention current limitations
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[VERBOSE] Generated 5 reasoning steps

[VERBOSE] [Rumination] [Step 2] Reflecting on reasoning steps
[THINKING] tinyllama_agent is reflecting on: Understand what quantum computing is...

[REFLECTION OUTPUT]
  This step is valid and foundational. Understanding quantum computing requires
  grasping its fundamental principles. The implications include establishing
  a base for further explanation...

[THINKING] tinyllama_agent is reflecting on: Explain quantum bits (qubits) vs classical bits...

[REFLECTION OUTPUT]
  This is a crucial distinction. Qubits can exist in superposition, unlike
  classical bits which are either 0 or 1. This opens up exponential
  computational possibilities...

[... more reflections ...]

[VERBOSE] [Rumination] [Step 3] Synthesizing findings
[THINKING] tinyllama_agent is synthesizing findings from 10 reasoning steps...

[SYNTHESIS OUTPUT - Findings]
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Quantum computing represents a paradigm shift from classical computing.
Unlike classical bits that exist in definite states (0 or 1), quantum bits
(qubits) can exist in superposition, allowing them to be in multiple states
simultaneously. This property, combined with entanglement, enables quantum
computers to process information in ways that would be impossible for
classical computers...

Key applications include cryptography, drug discovery, optimization problems,
and machine learning. However, current quantum computers face challenges
including decoherence, error rates, and the need for extremely low
temperatures...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[VERBOSE] [Rumination] [Step 4] Extracting key insights
[THINKING] tinyllama_agent is extracting key insights...

[INSIGHTS OUTPUT]
  • Quantum computing uses qubits in superposition
  • Entanglement enables parallel processing
  • Potential for exponential speedup in certain problems
  • Current limitations include decoherence and error rates
  • Applications in cryptography, drug discovery, and optimization

======================================================================
📊 FINAL REPORT - Agent: tinyllama_agent
======================================================================

Task: explain quantum computing

Reasoning Steps (10 total):
----------------------------------------------------------------------
1. Understand what quantum computing is
2. Explain quantum bits (qubits) vs classical bits
3. Describe superposition and entanglement
4. Discuss potential applications
5. Mention current limitations
Reflection: This step is valid and foundational...
[... more steps ...]
----------------------------------------------------------------------

Findings:
----------------------------------------------------------------------
Quantum computing represents a paradigm shift from classical computing...
[... full findings ...]
----------------------------------------------------------------------

Key Insights (5 total):
----------------------------------------------------------------------
• Quantum computing uses qubits in superposition
• Entanglement enables parallel processing
• Potential for exponential speedup in certain problems
• Current limitations include decoherence and error rates
• Applications in cryptography, drug discovery, and optimization
----------------------------------------------------------------------
======================================================================
```

## What You'll See

### 1. **Initial Task Header**
Shows which agent is processing which task.

### 2. **Reasoning Steps Generation**
- Shows the raw LLM output as it generates reasoning steps
- Displays the parsed steps in a formatted box

### 3. **Reflection Phase**
- Shows each reasoning step being reflected upon
- Displays the reflection output for each step
- Shows the agent's critical thinking about its own reasoning

### 4. **Synthesis Phase**
- Shows the agent synthesizing all reasoning steps
- Displays the comprehensive findings

### 5. **Insight Extraction**
- Shows key insights being extracted
- Displays the final insights list

### 6. **Final Report**
- Formatted summary with all reasoning steps
- Complete findings
- Key insights

## Real-Time Thinking Indicators

The `[THINKING]` prefix shows when the agent is actively processing:
- `[THINKING] agent_name is generating reasoning steps...`
- `[THINKING] agent_name is reflecting on: ...`
- `[THINKING] agent_name is synthesizing findings...`
- `[THINKING] agent_name is extracting key insights...`

## Comparison Between Models

To compare how different models think about the same task:

```bash
# tinyllama - fast, may have more hallucinations
./build/multi_agent_llm --config config/agents_multi_llm.yaml \
  --task "explain dark matter" --agent tinyllama_agent --verbose

# mistral - stronger reasoning
./build/multi_agent_llm --config config/agents_multi_llm.yaml \
  --task "explain dark matter" --agent mistral_agent --verbose

# gemma - Google's model
./build/multi_agent_llm --config config/agents_multi_llm.yaml \
  --task "explain dark matter" --agent gemma_agent --verbose
```

Each will show different:
- Reasoning step structures
- Reflection depth
- Synthesis quality
- Insight extraction

This allows you to observe how different models "think" and where they might diverge or "hallucinate" differently.

