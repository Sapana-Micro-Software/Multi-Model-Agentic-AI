'use client';

import { useEffect, useRef } from 'react';
import { motion } from 'framer-motion';

export default function ArchitectureDiagram() {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    // Set canvas size
    canvas.width = 1200;
    canvas.height = 800;

    // Clear canvas
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Draw architecture diagram
    const drawNode = (x: number, y: number, width: number, height: number, label: string, color: string) => {
      // Gradient fill
      const gradient = ctx.createLinearGradient(x, y, x + width, y + height);
      gradient.addColorStop(0, color);
      gradient.addColorStop(1, color + '80');
      
      ctx.fillStyle = gradient;
      ctx.fillRect(x, y, width, height);
      
      // Border
      ctx.strokeStyle = color;
      ctx.lineWidth = 2;
      ctx.strokeRect(x, y, width, height);
      
      // Label
      ctx.fillStyle = '#1f2937';
      ctx.font = 'bold 16px sans-serif';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText(label, x + width / 2, y + height / 2);
    };

    const drawArrow = (x1: number, y1: number, x2: number, y2: number) => {
      ctx.strokeStyle = '#6366f1';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.moveTo(x1, y1);
      ctx.lineTo(x2, y2);
      ctx.stroke();
      
      // Arrowhead
      const angle = Math.atan2(y2 - y1, x2 - x1);
      ctx.beginPath();
      ctx.moveTo(x2, y2);
      ctx.lineTo(x2 - 10 * Math.cos(angle - Math.PI / 6), y2 - 10 * Math.sin(angle - Math.PI / 6));
      ctx.lineTo(x2 - 10 * Math.cos(angle + Math.PI / 6), y2 - 10 * Math.sin(angle + Math.PI / 6));
      ctx.closePath();
      ctx.fillStyle = '#6366f1';
      ctx.fill();
    };

    // Agent Manager (top center)
    drawNode(450, 50, 300, 80, 'Agent Manager', '#8b5cf6');
    
    // Agents (middle row)
    drawNode(100, 250, 200, 120, 'Agent 1\n(LLM + Memory)', '#6366f1');
    drawNode(450, 250, 200, 120, 'Agent 2\n(LLM + Memory)', '#6366f1');
    drawNode(800, 250, 200, 120, 'Agent 3\n(LLM + Memory)', '#6366f1');
    
    // Communication Layer
    drawNode(200, 450, 800, 100, 'Communication Layer (Message Queue + Router)', '#ec4899');
    
    // Security & Fault Tolerance
    drawNode(50, 600, 250, 100, 'Security Layer', '#10b981');
    drawNode(450, 600, 250, 100, 'Fault Tolerance', '#f59e0b');
    drawNode(850, 600, 250, 100, 'Cache Coherence', '#06b6d4');
    
    // Arrows
    drawArrow(600, 130, 600, 250); // Manager to Agent 2
    drawArrow(200, 310, 200, 450); // Agent 1 to Communication
    drawArrow(550, 310, 550, 450); // Agent 2 to Communication
    drawArrow(900, 310, 900, 450); // Agent 3 to Communication
    drawArrow(175, 550, 175, 600); // Communication to Security
    drawArrow(575, 550, 575, 600); // Communication to Fault Tolerance
    drawArrow(975, 550, 975, 600); // Communication to Cache
  }, []);

  return (
    <section id="architecture" className="py-20 bg-gradient-to-br from-gray-50 to-gray-100">
      <div className="container mx-auto px-4">
        <motion.h2
          initial={{ opacity: 0, y: 20 }}
          whileInView={{ opacity: 1, y: 0 }}
          viewport={{ once: true }}
          className="text-4xl font-bold text-center mb-12 text-gray-900"
        >
          System Architecture
        </motion.h2>
        <div className="bg-white rounded-2xl shadow-xl p-8 overflow-x-auto">
          <canvas
            ref={canvasRef}
            className="w-full h-auto max-w-full"
            style={{ maxHeight: '800px' }}
          />
        </div>
      </div>
    </section>
  );
}

