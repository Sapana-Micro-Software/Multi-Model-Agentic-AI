# GitHub Pages Setup Guide

## Enable GitHub Pages

To make the website live at `sapana-micro-software.github.io/multi-model-agentic-ai`, follow these steps:

1. Go to your repository: https://github.com/Sapana-Micro-Software/Multi-Model-Agentic-AI
2. Click on **Settings** (top menu)
3. Scroll down to **Pages** (left sidebar)
4. Under **Source**, select:
   - **Deploy from a branch**
   - **Branch**: `main`
   - **Folder**: `/docs`
5. Click **Save**

## Automatic Deployment

The GitHub Actions workflow (`.github/workflows/pages.yml`) will automatically deploy the website whenever you push to the `main` branch.

## Website URL

Once enabled, your website will be available at:
**https://sapana-micro-software.github.io/multi-model-agentic-ai**

## Features

The website includes:
- ✅ Beautiful, modern design inspired by New York Smells
- ✅ Sections for Paper, Code, Benchmarks, and Studies
- ✅ Reference to New York Smells research
- ✅ Responsive design for mobile and desktop
- ✅ Smooth animations and transitions
- ✅ GitHub Actions for automatic deployment

## Manual Deployment

If you need to manually trigger deployment:
1. Go to **Actions** tab in your repository
2. Select **Deploy GitHub Pages** workflow
3. Click **Run workflow**

