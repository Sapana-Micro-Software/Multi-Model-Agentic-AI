'use client';

import { motion } from 'framer-motion';
import { ArrowDown, Github, FileText, Code } from 'lucide-react';
import Link from 'next/link';

export default function Hero() {
  return (
    <section className="relative min-h-screen flex items-center justify-center overflow-hidden">
      {/* Animated background gradient */}
      <div className="absolute inset-0 bg-gradient-to-br from-purple-600 via-blue-600 to-indigo-800">
        <div className="absolute inset-0 bg-[url('/grid.svg')] bg-center [mask-image:linear-gradient(180deg,white,rgba(255,255,255,0))]"></div>
      </div>

      {/* Floating particles */}
      <div className="absolute inset-0 overflow-hidden">
        {[...Array(20)].map((_, i) => (
          <motion.div
            key={i}
            className="absolute w-2 h-2 bg-white/30 rounded-full"
            initial={{
              x: Math.random() * 1200,
              y: Math.random() * 800,
            }}
            animate={{
              y: [null, Math.random() * 800],
              x: [null, Math.random() * 1200],
            }}
            transition={{
              duration: Math.random() * 10 + 10,
              repeat: Infinity,
              repeatType: 'reverse',
            }}
          />
        ))}
      </div>

      <div className="relative z-10 container mx-auto px-4 text-center">
        <motion.h1
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.8 }}
          className="text-6xl md:text-8xl font-bold text-white mb-6"
        >
          Multi-Model Agentic AI
        </motion.h1>
        
        <motion.p
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.8, delay: 0.2 }}
          className="text-xl md:text-2xl text-white/90 mb-8 max-w-3xl mx-auto"
        >
          A Comprehensive, Fault-Tolerant, Distributed Multi-Agent Architecture
        </motion.p>

        <motion.div
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.8, delay: 0.4 }}
          className="flex flex-wrap justify-center gap-4 mb-12"
        >
          <Link
            href="#paper"
            className="flex items-center gap-2 px-6 py-3 bg-white text-purple-600 rounded-full font-semibold hover:bg-purple-50 transition-all shadow-lg hover:shadow-xl"
          >
            <FileText className="w-5 h-5" />
            Paper
          </Link>
          <Link
            href="https://github.com/Sapana-Micro-Software/Multi-Model-Agentic-AI"
            target="_blank"
            className="flex items-center gap-2 px-6 py-3 bg-white/10 backdrop-blur-sm text-white border-2 border-white/30 rounded-full font-semibold hover:bg-white/20 transition-all"
          >
            <Github className="w-5 h-5" />
            Code
          </Link>
          <Link
            href="#benchmarks"
            className="flex items-center gap-2 px-6 py-3 bg-white/10 backdrop-blur-sm text-white border-2 border-white/30 rounded-full font-semibold hover:bg-white/20 transition-all"
          >
            <Code className="w-5 h-5" />
            Benchmarks
          </Link>
        </motion.div>

        <motion.div
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          transition={{ duration: 0.8, delay: 0.6 }}
          className="flex justify-center"
        >
          <motion.a
            href="#features"
            animate={{ y: [0, 10, 0] }}
            transition={{ duration: 2, repeat: Infinity }}
            className="text-white/80 hover:text-white transition-colors"
          >
            <ArrowDown className="w-8 h-8" />
          </motion.a>
        </motion.div>
      </div>
    </section>
  );
}

