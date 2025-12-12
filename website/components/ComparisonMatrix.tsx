'use client';

import { motion } from 'framer-motion';
import { Check, X } from 'lucide-react';

const characteristics = [
  'Modular', 'Fault-Tolerant', 'Secure', 'Atomic', 'Concurrent',
  'Parallel', 'Distributed', 'Cache Coherent', 'Encrypted', 'Protocol-Driven',
  'Robust', 'Asynchronous', 'Producer-Consumer', 'Synchronized', 'Optimized', 'Lightweight'
];

const systems = [
  {
    name: 'Multi-Model Agentic AI',
    values: Array(16).fill(true),
    color: 'bg-green-500',
  },
  {
    name: 'Standard Multi-Agent',
    values: [true, false, false, false, true, false, false, false, false, false, true, false, false, true, false, false],
    color: 'bg-yellow-500',
  },
  {
    name: 'Basic LLM System',
    values: [true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true],
    color: 'bg-red-500',
  },
];

export default function ComparisonMatrix() {
  return (
    <section className="py-20 bg-gradient-to-br from-gray-50 to-gray-100">
      <div className="container mx-auto px-4">
        <motion.h2
          initial={{ opacity: 0, y: 20 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true }}
          className="text-4xl font-bold text-center mb-12 text-gray-900"
        >
          Feature Comparison Matrix
        </motion.h2>

        <div className="bg-white rounded-2xl shadow-xl overflow-hidden">
          <div className="overflow-x-auto">
            <table className="w-full">
              <thead>
                <tr className="bg-gradient-to-r from-purple-600 to-blue-600 text-white">
                  <th className="px-6 py-4 text-left font-bold sticky left-0 bg-gradient-to-r from-purple-600 to-blue-600 z-10">
                    System / Feature
                  </th>
                  {characteristics.map((char, idx) => (
                    <th key={idx} className="px-4 py-4 text-center font-semibold min-w-[120px]">
                      <div className="transform -rotate-45 origin-center whitespace-nowrap">
                        {char}
                      </div>
                    </th>
                  ))}
                </tr>
              </thead>
              <tbody>
                {systems.map((system, sysIdx) => (
                  <motion.tr
                    key={sysIdx}
                    initial={{ opacity: 0, x: -20 }}
                    whileInView={{ opacity: 1, x: 0 }}
                    viewport={{ once: true }}
                    transition={{ delay: sysIdx * 0.1 }}
                    className={sysIdx % 2 === 0 ? 'bg-gray-50' : 'bg-white'}
                  >
                    <td className="px-6 py-4 font-semibold text-gray-900 sticky left-0 bg-inherit z-10">
                      {system.name}
                    </td>
                    {system.values.map((value, valIdx) => (
                      <td key={valIdx} className="px-4 py-4 text-center">
                        <motion.div
                          initial={{ scale: 0 }}
                          whileInView={{ scale: 1 }}
                          viewport={{ once: true }}
                          transition={{ delay: sysIdx * 0.1 + valIdx * 0.01 }}
                          className={`inline-flex items-center justify-center w-8 h-8 rounded-full ${
                            value ? 'bg-green-100 text-green-600' : 'bg-red-100 text-red-600'
                          }`}
                        >
                          {value ? (
                            <Check className="w-5 h-5" />
                          ) : (
                            <X className="w-5 h-5" />
                          )}
                        </motion.div>
                      </td>
                    ))}
                  </motion.tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>

        <motion.div
          initial={{ opacity: 0, y: 20 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true }}
          className="mt-8 text-center text-gray-600"
        >
          <p className="text-lg">
            Our system implements <span className="font-bold text-purple-600">all 16 characteristics</span>, 
            providing comprehensive enterprise-grade capabilities.
          </p>
        </motion.div>
      </div>
    </section>
  );
}

