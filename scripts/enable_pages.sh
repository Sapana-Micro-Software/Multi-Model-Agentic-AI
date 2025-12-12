#!/bin/bash
# Copyright (C) 2025, Shyamal Chandra
# Script to enable GitHub Pages via API

set -e

REPO="Sapana-Micro-Software/Multi-Model-Agentic-AI"

echo "Attempting to enable GitHub Pages for $REPO..."

# First, try to get current Pages status
echo "Checking current Pages status..."
STATUS=$(gh api repos/$REPO/pages 2>&1 || echo "NOT_ENABLED")

if [[ "$STATUS" == *"Not Found"* ]] || [[ "$STATUS" == *"NOT_ENABLED"* ]]; then
    echo "Pages is not enabled. Attempting to enable..."
    
    # Try to enable Pages
    gh api -X POST repos/$REPO/pages \
        -f 'source[branch]=main' \
        -f 'source[path]=/docs' 2>&1 || {
        echo ""
        echo "⚠️  Could not enable Pages via API."
        echo "   This is normal - Pages must be enabled via web UI first."
        echo ""
        echo "📝 Please enable Pages manually:"
        echo "   1. Visit: https://github.com/$REPO/settings/pages"
        echo "   2. Select: Deploy from a branch"
        echo "   3. Branch: main"
        echo "   4. Folder: /docs"
        echo "   5. Click Save"
        echo ""
        exit 0
    }
    
    echo "✅ Pages enabled successfully!"
else
    echo "Pages is already enabled or configured."
    echo "Current status:"
    echo "$STATUS" | jq '.' 2>/dev/null || echo "$STATUS"
fi

echo ""
echo "🔍 Checking Pages configuration..."
gh api repos/$REPO/pages 2>&1 | jq '.' 2>/dev/null || gh api repos/$REPO/pages 2>&1

echo ""
echo "✅ Setup complete! Your site will be available at:"
echo "   https://sapana-micro-software.github.io/multi-model-agentic-ai"
echo ""
echo "📊 Monitor deployment at:"
echo "   https://github.com/$REPO/actions"

