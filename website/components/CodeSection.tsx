'use client';

import { motion } from 'framer-motion';
import { Github, Code2, Terminal } from 'lucide-react';
import Link from 'next/link';

export default function CodeSection() {
  return (
    <section id="code" className="py-20 bg-gradient-to-br from-gray-50 to-gray-100">
      <div className="container mx-auto px-4">
        <motion.div
          initial={{ opacity: 0, y: 20 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true }}
          className="max-w-4xl mx-auto"
        >
          <h2 className="text-4xl font-bold text-center mb-12 text-gray-900">Code</h2>

          <div className="bg-white rounded-2xl p-8 shadow-xl mb-8">
            <div className="flex items-center gap-4 mb-6">
              <Github className="w-8 h-8 text-gray-900" />
              <div>
                <h3 className="text-2xl font-bold text-gray-900">Repository</h3>
                <p className="text-gray-600">Complete source code with documentation</p>
              </div>
            </div>

            <div className="flex flex-wrap gap-4 mb-6">
              <Link
                href="https://github.com/Sapana-Micro-Software/Multi-Model-Agentic-AI"
                target="_blank"
                className="flex items-center gap-2 px-6 py-3 bg-gray-900 text-white rounded-full font-semibold hover:bg-gray-800 transition-all"
              >
                <Github className="w-5 h-5" />
                View on GitHub
              </Link>
              <Link
                href="https://github.com/Sapana-Micro-Software/Multi-Model-Agentic-AI/archive/refs/heads/main.zip"
                className="flex items-center gap-2 px-6 py-3 bg-white text-gray-900 border-2 border-gray-300 rounded-full font-semibold hover:bg-gray-50 transition-all"
              >
                <Code2 className="w-5 h-5" />
                Download ZIP
              </Link>
            </div>

            <div className="bg-gray-900 rounded-xl p-6 text-green-400 font-mono text-sm overflow-x-auto">
              <div className="flex items-center gap-2 mb-4">
                <Terminal className="w-4 h-4" />
                <span className="text-gray-400">Quick Start</span>
              </div>
              <pre className="whitespace-pre-wrap">
{`# Build
mkdir build && cd build
cmake ..
make

# Run
./multi_agent_llm --task "research topic" --agent agent1`}
              </pre>
            </div>
          </div>
        </motion.div>
      </div>
    </section>
  );
}

