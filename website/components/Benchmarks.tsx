'use client';

import { BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer, LineChart, Line } from 'recharts';
import { motion } from 'framer-motion';

const performanceData = [
  { metric: 'Input Validation', latency: 85, throughput: 1000 },
  { metric: 'Concurrent Ops', latency: 10, throughput: 1200 },
  { metric: 'Memory Efficiency', latency: 5, throughput: 2000 },
  { metric: 'Cache Coherence', latency: 8, throughput: 1500 },
  { metric: 'Fault Recovery', latency: 450, throughput: 800 },
  { metric: 'Encryption', latency: 20, throughput: 50000 },
  { metric: 'Distributed', latency: 8, throughput: 1000 },
];

const scalabilityData = [
  { agents: 1, throughput: 100, latency: 10 },
  { agents: 10, throughput: 950, latency: 12 },
  { agents: 50, throughput: 4500, latency: 15 },
  { agents: 100, throughput: 8500, latency: 18 },
];

export default function Benchmarks() {
  return (
    <section id="benchmarks" className="py-20 bg-white">
      <div className="container mx-auto px-4">
        <motion.h2
          initial={{ opacity: 0, y: 20 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true }}
          className="text-4xl font-bold text-center mb-12 text-gray-900"
        >
          Performance Benchmarks
        </motion.h2>

        <div className="grid md:grid-cols-2 gap-8 mb-12">
          <motion.div
            initial={{ opacity: 0, x: -20 }}
            whileInView={{ opacity: 1, x: 0 }}
            viewport={{ once: true }}
            className="bg-gradient-to-br from-purple-50 to-blue-50 rounded-2xl p-6 shadow-lg"
          >
            <h3 className="text-2xl font-bold mb-4 text-gray-900">Latency (ms)</h3>
            <ResponsiveContainer width="100%" height={300}>
              <BarChart data={performanceData}>
                <CartesianGrid strokeDasharray="3 3" />
                <XAxis dataKey="metric" angle={-45} textAnchor="end" height={100} />
                <YAxis />
                <Tooltip />
                <Legend />
                <Bar dataKey="latency" fill="#8b5cf6" />
              </BarChart>
            </ResponsiveContainer>
          </motion.div>

          <motion.div
            initial={{ opacity: 0, x: 20 }}
            whileInView={{ opacity: 1, x: 0 }}
            viewport={{ once: true }}
            className="bg-gradient-to-br from-blue-50 to-indigo-50 rounded-2xl p-6 shadow-lg"
          >
            <h3 className="text-2xl font-bold mb-4 text-gray-900">Throughput (ops/sec)</h3>
            <ResponsiveContainer width="100%" height={300}>
              <BarChart data={performanceData}>
                <CartesianGrid strokeDasharray="3 3" />
                <XAxis dataKey="metric" angle={-45} textAnchor="end" height={100} />
                <YAxis />
                <Tooltip />
                <Legend />
                <Bar dataKey="throughput" fill="#6366f1" />
              </BarChart>
            </ResponsiveContainer>
          </motion.div>
        </div>

        <motion.div
          initial={{ opacity: 0, y: 20 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true }}
          className="bg-gradient-to-br from-indigo-50 to-purple-50 rounded-2xl p-6 shadow-lg"
        >
          <h3 className="text-2xl font-bold mb-4 text-gray-900">Scalability Analysis</h3>
          <ResponsiveContainer width="100%" height={400}>
            <LineChart data={scalabilityData}>
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis dataKey="agents" label={{ value: 'Number of Agents', position: 'insideBottom', offset: -5 }} />
              <YAxis yAxisId="left" label={{ value: 'Throughput (ops/sec)', angle: -90, position: 'insideLeft' }} />
              <YAxis yAxisId="right" orientation="right" label={{ value: 'Latency (ms)', angle: 90, position: 'insideRight' }} />
              <Tooltip />
              <Legend />
              <Line yAxisId="left" type="monotone" dataKey="throughput" stroke="#8b5cf6" strokeWidth={3} name="Throughput" />
              <Line yAxisId="right" type="monotone" dataKey="latency" stroke="#ec4899" strokeWidth={3} name="Latency" />
            </LineChart>
          </ResponsiveContainer>
        </motion.div>
      </div>
    </section>
  );
}

