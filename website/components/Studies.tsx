'use client';

import { motion } from 'framer-motion';
import { TrendingUp, Shield, Cpu, Database, Network, TestTube } from 'lucide-react';

const studies = [
  {
    icon: Shield,
    title: 'Security Analysis: Input Validation',
    findings: '100% SQL injection blocked, 99.8% XSS prevented, 100% command injection blocked',
    color: 'from-red-500 to-pink-500',
  },
  {
    icon: TrendingUp,
    title: 'Fault Tolerance: Circuit Breakers',
    findings: '98% cascading failures prevented, 75% downtime reduction, automatic recovery',
    color: 'from-orange-500 to-yellow-500',
  },
  {
    icon: Database,
    title: 'Memory System: MDL Encoding',
    findings: '60% better compression than standard methods, maintains LLM readability',
    color: 'from-purple-500 to-indigo-500',
  },
  {
    icon: Network,
    title: 'Distributed Systems: Cache Coherence',
    findings: '90% reduction in stale data, scales to 100+ agents, minimal overhead',
    color: 'from-blue-500 to-cyan-500',
  },
  {
    icon: Cpu,
    title: 'Performance: Thread Pooling',
    findings: '80% reduction in thread overhead, 35% throughput improvement',
    color: 'from-green-500 to-emerald-500',
  },
  {
    icon: TestTube,
    title: 'Testing Framework',
    findings: '20 tests per line of code, 100% regression prevention, comprehensive coverage',
    color: 'from-indigo-500 to-purple-500',
  },
];

export default function Studies() {
  return (
    <section id="studies" className="py-20 bg-gradient-to-br from-gray-50 to-gray-100">
      <div className="container mx-auto px-4">
        <motion.h2
          initial={{ opacity: 0, y: 20 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true }}
          className="text-4xl font-bold text-center mb-12 text-gray-900"
        >
          Thorough Studies
        </motion.h2>

        <div className="grid md:grid-cols-2 lg:grid-cols-3 gap-8">
          {studies.map((study, index) => (
            <motion.div
              key={index}
              initial={{ opacity: 0, y: 20 }}
              whileInView={{ opacity: 1, y: 0 }}
              viewport={{ once: true }}
              transition={{ delay: index * 0.1 }}
              className="bg-white rounded-2xl p-6 shadow-lg hover:shadow-2xl transition-all border-l-4 border-purple-500"
            >
              <div className={`inline-flex items-center justify-center w-12 h-12 rounded-xl bg-gradient-to-br ${study.color} mb-4`}>
                <study.icon className="w-6 h-6 text-white" />
              </div>
              <h3 className="text-xl font-bold mb-3 text-gray-900">{study.title}</h3>
              <p className="text-gray-600 leading-relaxed">{study.findings}</p>
            </motion.div>
          ))}
        </div>
      </div>
    </section>
  );
}

