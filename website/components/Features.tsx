'use client';

import { motion } from 'framer-motion';
import { Shield, Zap, Network, Lock, RefreshCw, Database, Code, Rocket } from 'lucide-react';

const features = [
  {
    icon: Shield,
    title: 'Security',
    description: 'Input validation with recursive retry, encryption, secure communication',
    color: 'from-red-500 to-pink-500',
  },
  {
    icon: RefreshCw,
    title: 'Fault Tolerance',
    description: 'Retry executors, circuit breakers, graceful degradation',
    color: 'from-orange-500 to-yellow-500',
  },
  {
    icon: Network,
    title: 'Distributed',
    description: 'Network communication, cache coherence, agent registry',
    color: 'from-blue-500 to-cyan-500',
  },
  {
    icon: Database,
    title: 'Memory System',
    description: 'MDL-normalized context, trace management, key insights',
    color: 'from-purple-500 to-indigo-500',
  },
  {
    icon: Code,
    title: 'Protocol-Driven',
    description: 'Formal message protocols, version management, validation',
    color: 'from-green-500 to-emerald-500',
  },
  {
    icon: Rocket,
    title: 'Performance',
    description: 'Thread pooling, lock-free structures, optimized caching',
    color: 'from-indigo-500 to-purple-500',
  },
];

export default function Features() {
  return (
    <section id="features" className="py-20 bg-white">
      <div className="container mx-auto px-4">
        <motion.h2
          initial={{ opacity: 0, y: 20 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true }}
          className="text-4xl font-bold text-center mb-12 text-gray-900"
        >
          Key Features
        </motion.h2>

        <div className="grid md:grid-cols-2 lg:grid-cols-3 gap-8">
          {features.map((feature, index) => (
            <motion.div
              key={index}
              initial={{ opacity: 0, y: 20 }}
              whileInView={{ opacity: 1, y: 0 }}
              viewport={{ once: true }}
              transition={{ delay: index * 0.1 }}
              className="group relative bg-white rounded-2xl p-6 shadow-lg hover:shadow-2xl transition-all duration-300 border-2 border-transparent hover:border-purple-200"
            >
              <div className={`inline-flex items-center justify-center w-16 h-16 rounded-2xl bg-gradient-to-br ${feature.color} mb-4 transform group-hover:scale-110 transition-transform`}>
                <feature.icon className="w-8 h-8 text-white" />
              </div>
              <h3 className="text-2xl font-bold mb-2 text-gray-900">{feature.title}</h3>
              <p className="text-gray-600">{feature.description}</p>
            </motion.div>
          ))}
        </div>
      </div>
    </section>
  );
}

