'use client';

import { motion } from 'framer-motion';
import { FileText, Download } from 'lucide-react';
import Link from 'next/link';

export default function PaperSection() {
  return (
    <section id="paper" className="py-20 bg-white">
      <div className="container mx-auto px-4">
        <motion.div
          initial={{ opacity: 0, y: 20 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true }}
          className="max-w-4xl mx-auto"
        >
          <div className="bg-gradient-to-br from-purple-50 to-blue-50 rounded-2xl p-8 shadow-xl">
            <div className="flex items-center gap-4 mb-6">
              <div className="w-16 h-16 rounded-full bg-gradient-to-br from-purple-600 to-blue-600 flex items-center justify-center">
                <FileText className="w-8 h-8 text-white" />
              </div>
              <div>
                <h2 className="text-3xl font-bold text-gray-900">Research Paper</h2>
                <p className="text-gray-600">Complete and unabridged documentation</p>
              </div>
            </div>

            <div className="bg-white rounded-xl p-6 mb-6">
              <h3 className="text-xl font-bold mb-4 text-gray-900">
                Multi-Model Agentic AI System: A Comprehensive Architecture
              </h3>
              <p className="text-gray-700 mb-4">
                <strong>Author:</strong> Shyamal Chandra<br />
                <strong>Institution:</strong> Sapana Micro Software<br />
                <strong>Year:</strong> 2025
              </p>
              <p className="text-gray-600">
                This paper presents a complete and unabridged documentation of the Multi-Model Agentic AI system, 
                including all implementation details, architecture decisions, security mechanisms, fault tolerance 
                strategies, distributed system design, and comprehensive evaluation results.
              </p>
            </div>

            <div className="flex flex-wrap gap-4">
              <Link
                href="/doc/paper.pdf"
                target="_blank"
                className="flex items-center gap-2 px-6 py-3 bg-gradient-to-r from-purple-600 to-blue-600 text-white rounded-full font-semibold hover:from-purple-700 hover:to-blue-700 transition-all shadow-lg hover:shadow-xl"
              >
                <Download className="w-5 h-5" />
                Download Paper (PDF)
              </Link>
              <Link
                href="/doc/presentation.pdf"
                target="_blank"
                className="flex items-center gap-2 px-6 py-3 bg-white text-purple-600 border-2 border-purple-600 rounded-full font-semibold hover:bg-purple-50 transition-all"
              >
                <Download className="w-5 h-5" />
                Download Presentation
              </Link>
            </div>

            <div className="mt-6 bg-gray-50 rounded-lg p-4 font-mono text-sm">
              <strong>BibTeX:</strong><br />
              @article&#123;chandra2025multimodel,<br />
              &nbsp;&nbsp;title=&#123;Multi-Model Agentic AI System...&#125;,<br />
              &nbsp;&nbsp;author=&#123;Chandra, Shyamal&#125;,<br />
              &nbsp;&nbsp;year=&#123;2025&#125;<br />
              &#125;
            </div>
          </div>
        </motion.div>
      </div>
    </section>
  );
}

