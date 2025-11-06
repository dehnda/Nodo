# Nodo Documentation System - Auto-Generation Guide

This guide explains how automatic documentation generation works for Nodo.

## 🤖 How Auto-Generation Works

Node reference documentation is automatically generated from your C++ source code, eliminating duplication and ensuring docs stay in sync with implementation.

## 📋 What Gets Auto-Generated

### From `SOPFactory::get_all_available_nodes()`
```cpp
{NodeType::Sphere, "Sphere", "Generator", "Create a UV sphere primitive"}
```

Extracts:
- ✅ Node type enum
- ✅ Display name
- ✅ Category
- ✅ Description

### From Individual SOP Headers (e.g., `sphere_sop.hpp`)

```cpp
register_parameter(define_float_parameter("radius", DEFAULT_RADIUS)
    .label("Radius")
    .range(0.01, 100.0)
    .category("Size")
    .description("Radius of the sphere")
    .build());
```

Extracts:
- ✅ Parameter name (`radius`)
- ✅ Parameter type (`float`, `int`, `bool`, `string`)
- ✅ Label (`Radius`)
- ✅ Default value (`1.0`)
- ✅ Range constraints (`0.01` to `100.0`)
- ✅ Category (`Size`)
- ✅ Description tooltip

### From Input Configuration

```cpp
InputConfig get_input_config() const override {
    return InputConfig(InputType::NONE, 0, 0, 0);
}
```

Extracts:
- ✅ Input type (NONE/SINGLE/MULTIPLE)
- ✅ Min/max input count
- ✅ Required inputs

## 🔧 Using the Generator

### Generate All Node Docs

```bash
cd /home/daniel/projects/Nodo
python tools/generate_node_docs.py
```

Output:
```
=== Nodo Node Documentation Generator ===

✓ Found 44 nodes in SOPFactory

Generating documentation:
  • Sphere (Generator)
  • Box (Generator)
  • Cylinder (Generator)
  ...
  • Sort (Utility)

✓ Generated 44 node documentation pages
✓ Output: docs/nodes/
```

### Generated File Structure

```
docs/nodes/
├── index.md                    # Overview with all nodes categorized
├── generator/
│   ├── sphere.md              # Auto-generated
│   ├── box.md
│   ├── cylinder.md
│   └── ...
├── modifier/
│   ├── transform.md
│   ├── extrude.md
│   └── ...
├── boolean/
│   ├── boolean.md
│   └── merge.md
└── ...
```

## 📄 Generated Markdown Format

Each node page includes:

1. **Header** - Node name and category
2. **Description** - From metadata
3. **Inputs** - Based on InputConfig (NONE/SINGLE/MULTIPLE)
4. **Parameters** - Grouped by category with:
   - Type, label, description
   - Default values
   - Range constraints
   - Options for combo boxes
5. **Example Usage** - Template text (can be manually enhanced)
6. **See Also** - Related nodes (can be manually enhanced)

### Example Generated Page

See: `docs/nodes/generator/sphere.md`

## ✏️ Manual Enhancements

After auto-generation, you can manually add:

### Expression Examples
```markdown
## Expression Examples

**Dynamic radius:**
\`\`\`
ch("../radius_control") * 2
\`\`\`

**Animated:**
\`\`\`
sin($F * 0.1) + 1
\`\`\`
```

### Screenshots
```markdown
![Sphere Parameters](../images/sphere_params.png)
```

### Tips & Best Practices
```markdown
!!! tip "Performance"
    Use lower segment counts for real-time editing.

!!! warning "Avoid Zero Radius"
    Radius must be greater than 0.01.
```

## 🔄 Workflow

### Initial Setup
1. Run generator once: `python tools/generate_node_docs.py`
2. Review generated files
3. Add manual enhancements (examples, tips, screenshots)
4. Commit to git

### When Adding New Nodes
1. Implement new SOP with proper metadata
2. Re-run generator: `python tools/generate_node_docs.py`
3. Only new/changed nodes are regenerated
4. Add manual enhancements to new node docs

### When Changing Parameters
1. Update C++ parameter definitions
2. Re-run generator
3. Parameter sections auto-update
4. Manual sections (examples, tips) are preserved

## 🎨 Customization

### Modify Templates

Edit `tools/generate_node_docs.py`:

```python
def generate_node_doc(self, node: NodeMetadata) -> str:
    # Customize the markdown template here
    md.append(f"# {node.name}")
    md.append(f"**Category:** {node.category}")
    # Add your custom sections...
```

### Add New Sections

```python
# Add workflow examples
md.append("## Common Workflows")
md.append("")
md.append(self._generate_workflows(node))
```

### Custom Example Generation

```python
def _generate_example(self, node: NodeMetadata, parameters: List[Parameter]) -> str:
    if node.name == "Sphere":
        return "Perfect for creating planets, bubbles, or organic shapes."
    # ... custom examples per node
```

## 📊 Benefits

### Why Auto-Generate?

1. **Single Source of Truth** - Code is documentation
2. **Always Accurate** - Docs auto-update with code changes
3. **Saves Time** - No manual duplication for 44+ nodes
4. **Consistency** - All nodes documented identically
5. **Scalability** - Adding nodes scales to docs automatically

### What to Keep Manual

- Tutorial workflows
- Conceptual explanations
- Tips and best practices
- Screenshots and videos
- Expression examples
- Design rationale

## 🚀 Integration with MkDocs

The generated Markdown files work seamlessly with MkDocs:

```yaml
# mkdocs.yml
nav:
  - Node Reference:
    - Overview: nodes/index.md
    - Generator:
      - Sphere: nodes/generator/sphere.md
      - Box: nodes/generator/box.md
```

Build and serve:
```bash
mkdocs serve    # Preview at localhost:8000
mkdocs build    # Generate static site
mkdocs gh-deploy # Deploy to GitHub Pages
```

## 🔍 Advanced Features

### Extract Expression Functions

Could extend to auto-generate expression function reference:

```python
# Parse ExpressionEvaluator.cpp
functions = extract_expression_functions()
# Generate expressions/function-reference.md
```

### Extract Keyboard Shortcuts

```python
# Parse MainWindow QActions
shortcuts = extract_shortcuts()
# Generate reference/keyboard-shortcuts.md
```

### Generate Change Log

```python
# Compare old vs new metadata
changes = diff_node_versions()
# Generate changelog entries
```

## 📝 Example Output

See the example generated file:
- `docs/nodes/generator/sphere.md`

This shows what auto-generation produces, with sections for manual enhancement clearly marked.
