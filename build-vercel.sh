#!/bin/bash

# Test Vercel build locally
echo "🔨 Building documentation..."
. venv/bin/activate
python -m mkdocs build

echo "📦 Creating deployment structure..."
mkdir -p public
cp -r website/* public/
cp -r site public/docs

echo ""
echo "✅ Build complete!"
echo ""
echo "📂 Directory structure:"
echo "   public/"
echo "   ├── index.html         (main website)"
echo "   ├── style.css"
echo "   ├── script.js"
echo "   └── docs/"
echo "       └── index.html     (documentation)"
echo ""
echo "🧪 To test locally, run:"
echo "   cd public && python3 -m http.server 8080"
echo "   Then visit:"
echo "   - http://localhost:8080/          (main site)"
echo "   - http://localhost:8080/docs/     (documentation)"
