#!/bin/bash

echo "🎯 NodeFlux JSON Integration Demo"
echo "=================================="
echo

echo "📋 Available JSON Templates:"
echo "  • templates/basic_sphere.json"
echo "  • templates/boolean_union_template.json"
echo

echo "🚀 Starting Interactive Node Graph Editor..."
echo "📝 Instructions:"
echo "  1. Use File → Templates → Basic Sphere to load a template"
echo "  2. Modify parameters with the sliders"
echo "  3. Use File → Save Graph to save your changes"
echo "  4. Use View → JSON Preview to see live JSON"
echo "  5. Use Graph → Execute to generate meshes"
echo

echo "🎮 Controls:"
echo "  • Left click: Select/drag nodes"
echo "  • Right click in empty space: Add new nodes"
echo "  • Drag between pins: Create connections"
echo

echo "Starting application..."
./build/examples/node_graph_editor_app
