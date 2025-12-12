'use client';

import { motion } from 'framer-motion';
import { Shield, Zap, Network, TestTube } from 'lucide-react';

const stats = [
  { icon: Shield, label: 'Core Characteristics', value: '16', color: 'text-purple-600' },
  { icon: TestTube, label: 'Test Cases', value: '160+', color: 'text-blue-600' },
  { icon: Zap, label: 'Lines of Code', value: '7K+', color: 'text-indigo-600' },
  { icon: Network, label: 'Production Ready', value: '100%', color: 'text-green-600' },
];

export default function Stats() {
  return (
    <section className="py-20 bg-white">
      <div className="container mx-auto px-4">
        <div className="grid grid-cols-2 md:grid-cols-4 gap-8">
          {stats.map((stat, index) => (
            <motion.div
              key={index}
              initial={{ opacity: 0, y: 20 }}
              whileInView={{ opacity: 1, y: 0 }}
              viewport={{ once: true }}
              transition={{ duration: 0.5, delay: index * 0.1 }}
              className="text-center"
            >
              <div className="inline-flex items-center justify-center w-16 h-16 rounded-full bg-gradient-to-br from-purple-100 to-blue-100 mb-4">
                <stat.icon className={`w-8 h-8 ${stat.color}`} />
              </div>
              <div className="text-4xl font-bold text-gray-900 mb-2">{stat.value}</div>
              <div className="text-sm text-gray-600">{stat.label}</div>
            </motion.div>
          ))}
        </div>
      </div>
    </section>
  );
}

