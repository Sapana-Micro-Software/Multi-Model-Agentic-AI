'use client';

import { useState } from 'react';
import Hero from '@/components/Hero';
import Features from '@/components/Features';
import ArchitectureDiagram from '@/components/ArchitectureDiagram';
import Benchmarks from '@/components/Benchmarks';
import ComparisonMatrix from '@/components/ComparisonMatrix';
import Studies from '@/components/Studies';
import PaperSection from '@/components/PaperSection';
import CodeSection from '@/components/CodeSection';
import Stats from '@/components/Stats';

export default function Home() {
  return (
    <main className="min-h-screen bg-gradient-to-br from-purple-50 via-white to-blue-50">
      <Hero />
      <Stats />
      <Features />
      <ArchitectureDiagram />
      <Benchmarks />
      <ComparisonMatrix />
      <Studies />
      <PaperSection />
      <CodeSection />
    </main>
  );
}
