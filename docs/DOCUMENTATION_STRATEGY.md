# Nodo Documentation Strategy

Complete plan for documentation system before beta testing.

## 📋 Overview

Three-tier documentation approach:
1. **In-App Help** - Immediate access, contextual
2. **Web Documentation** - Comprehensive, searchable (MkDocs)
3. **Example Files** - Learn by doing

---

## 🌐 Web Documentation (MkDocs Material)

### Why MkDocs?
- ✅ Beautiful Material Design theme with dark mode
- ✅ Full-text search, mobile-friendly
- ✅ Simple Markdown files (easy to write)
- ✅ Free GitHub Pages hosting
- ✅ Used by major projects (FastAPI, SQLModel, etc.)

### Setup (5 minutes)

```bash
# Install
pip install mkdocs-material

# Preview locally
mkdocs serve    # http://localhost:8000

# Deploy to GitHub Pages
mkdocs gh-deploy  # https://dehnda.github.io/Nodo
```

### Documentation Structure

```
docs/
├── index.md                    # Home page
├── getting-started/
│   ├── installation.md         # Download & install
│   ├── quick-start.md          # 30min tutorial
│   ├── interface.md            # UI walkthrough
│   └── first-project.md        # Step-by-step example
├── concepts/
│   ├── procedural-modeling.md  # What is procedural modeling?
│   ├── node-graph.md           # Node graph basics
│   ├── geometry-types.md       # Meshes, points, etc.
│   ├── attributes.md           # Attribute system
│   └── groups.md               # Group system
├── nodes/                      # 🤖 AUTO-GENERATED
│   ├── index.md                # All nodes categorized
│   ├── generator/              # 6 nodes
│   ├── modifier/               # 16 nodes
│   ├── array/                  # 4 nodes
│   ├── boolean/                # 2 nodes
│   ├── attribute/              # 5 nodes
│   ├── group/                  # 6 nodes
│   ├── io/                     # 2 nodes
│   └── utility/                # 7 nodes
├── expressions/
│   ├── graph-parameters.md     # M3.2 system
│   ├── expression-syntax.md    # M3.3 syntax
│   ├── math-functions.md       # Available functions
│   └── ch-references.md        # Channel references
├── workflows/
│   ├── architectural.md        # Tutorial: Column
│   ├── game-assets.md          # Tutorial: Weapon
│   └── patterns.md             # Tutorial: Patterns
└── reference/
    ├── keyboard-shortcuts.md   # Quick reference
    ├── file-format.md          # .nfg format spec
    └── faq.md                  # Common questions
```

---

## 🤖 Auto-Generation System

### Node Reference Pages

**Script:** `tools/generate_node_docs.py`

**What it does:**
1. Parses `SOPFactory::get_all_available_nodes()` for node list
2. Reads each `*_sop.hpp` for parameter definitions
3. Generates `docs/nodes/{category}/{node}.md` files
4. Creates categorized index page

**What it extracts from C++:**
- ✅ Node name, category, description
- ✅ Input configuration (none/single/multiple)
- ✅ All parameters with types, labels, defaults
- ✅ Parameter ranges, options (for combos)
- ✅ Parameter categories and descriptions

**Example:**

```cpp
// C++ Input
register_parameter(define_float_parameter("radius", 1.0f)
    .label("Radius")
    .range(0.01, 100.0)
    .category("Size")
    .description("Radius of the sphere")
    .build());
```

```markdown
<!-- Markdown Output -->
### Size

**Radius** (`float`)

Radius of the sphere

- Default: `1.0` | Range: `0.01` to `100.0`
```

**Run it:**
```bash
python tools/generate_node_docs.py
# Generates all 44 node pages in seconds
```

**Manual Enhancements:**
After auto-generation, you can add:
- Expression examples
- Tips & best practices
- Screenshots
- Common use cases

These are preserved when you re-run the generator.

---

## 📱 In-App Help Integration

### Phase 1: Help Menu

Add to `MainWindow`:

```cpp
// Help menu
QMenu *help_menu = menuBar()->addMenu("&Help");

help_menu->addAction("📖 Documentation", []() {
    QDesktopServices::openUrl(QUrl("https://dehnda.github.io/Nodo"));
});

help_menu->addAction("⌨️ Keyboard Shortcuts", this, &MainWindow::showShortcuts);

help_menu->addSeparator();

help_menu->addAction("About Nodo", this, &MainWindow::showAbout);
```

### Phase 2: Context Help (F1)

```cpp
// In NodeGraphWidget::keyPressEvent
if (event->key() == Qt::Key_F1) {
    if (auto *node = get_selected_node()) {
        QString node_type = node->getType().toLower();
        QString url = QString("https://dehnda.github.io/Nodo/nodes/%1/%2/")
                          .arg(node->getCategory().toLower())
                          .arg(node_type);
        QDesktopServices::openUrl(QUrl(url));
    }
}
```

**User Experience:**
1. User selects Sphere node
2. Presses F1
3. Browser opens to `https://dehnda.github.io/Nodo/nodes/generator/sphere/`
4. Instant access to parameter docs, examples, tips

### Phase 3: Enhanced Tooltips

Already have good parameter tooltips. Could enhance with:
- Mini examples in tooltips
- Link to full docs

---

## 📂 Example Files Integration

### Ship Example Projects

**Location:** `examples/` directory (bundled with app)

**Categories:**
- `basics/` - Sphere, Box, Transform basics
- `intermediate/` - Boolean operations, Arrays
- `advanced/` - Complex workflows, Expressions

**Examples:**
- `basics/01_your_first_sphere.nfg`
- `basics/02_transform_and_array.nfg`
- `intermediate/03_boolean_subtract.nfg`
- `intermediate/04_copy_to_points.nfg`
- `advanced/05_procedural_column.nfg`
- `advanced/06_expression_driven.nfg`

**In-App Access:**

```cpp
// File menu
QMenu *examples_menu = file_menu->addMenu("Open Example");

// Populate from examples/ directory
QDir examples_dir(QCoreApplication::applicationDirPath() + "/examples");
for (const QString &category : examples_dir.entryList(QDir::Dirs)) {
    QMenu *category_menu = examples_menu->addMenu(category);
    // Add .nfg files...
}
```

---

## 📅 Implementation Timeline

### Week 1: Auto-Generation + Core Docs

**Days 1-2: Setup MkDocs**
- [ ] Install mkdocs-material
- [ ] Create `mkdocs.yml` (already done ✅)
- [ ] Create home page and basic structure
- [ ] Test local preview

**Days 3-4: Auto-Generate Node Docs**
- [ ] Test `generate_node_docs.py` script (already created ✅)
- [ ] Run generator for all 44 nodes
- [ ] Review generated docs for accuracy
- [ ] Add manual enhancements to 5-10 key nodes

**Days 5-7: Essential Guides**
- [ ] Quick Start Guide (30min to first model)
- [ ] Interface Overview (screenshot tour)
- [ ] Expression System Guide (M3.3 features)
- [ ] FAQ (known issues, limitations)

### Week 2: Tutorials + Integration

**Days 1-3: Workflow Tutorials**
- [ ] Tutorial 1: Architectural Column (Transform, Array, Boolean)
- [ ] Tutorial 2: Scatter Objects (Scatter, Copy to Points)
- [ ] Tutorial 3: Expression-Driven Design (Graph Parameters)

**Days 4-5: In-App Integration**
- [ ] Add Help menu with Documentation link
- [ ] Add About dialog with version info
- [ ] Add F1 context help for selected nodes

**Days 6-7: Example Files**
- [ ] Create 10-15 example .nfg files
- [ ] Add File → Open Example menu
- [ ] Test all examples work

### Week 3: Polish + Deploy

**Days 1-2: Screenshots & Media**
- [ ] Take screenshots of UI, nodes, workflows
- [ ] Add images to documentation
- [ ] Create workflow GIFs/videos (optional)

**Days 3-4: Review & Refinement**
- [ ] Proofread all docs
- [ ] Test all links
- [ ] Ensure consistency

**Day 5: Deploy**
- [ ] Deploy to GitHub Pages: `mkdocs gh-deploy`
- [ ] Test live site
- [ ] Share with beta testers

---

## 🎯 Beta Testing Priorities

For beta testers, they NEED:

1. ✅ **Quick Start** - Get productive in 30 minutes
2. ✅ **Node Reference** - What does each node do?
3. ✅ **Expression Guide** - How to use M3.3 system
4. ✅ **3 Workflow Tutorials** - Real examples
5. ✅ **FAQ** - Known issues, workarounds

You CAN DEFER:
- Video tutorials (post-launch)
- Advanced optimization guides
- API/developer docs
- Multi-language support

---

## 📊 Metrics for Success

### Documentation Quality Checklist

- [ ] Every node has documentation
- [ ] Quick Start gets user to first model in <30min
- [ ] Expression system is explained with examples
- [ ] 3+ workflow tutorials cover common use cases
- [ ] FAQ addresses known issues
- [ ] All docs have working links
- [ ] Mobile-friendly layout
- [ ] Search works correctly
- [ ] F1 help opens correct pages

### Beta Tester Feedback

Collect:
- "Was documentation helpful?" (1-5 scale)
- "What was confusing or missing?"
- "Which tutorials were most useful?"
- Documentation gap analysis

---

## 🔮 Future Enhancements

After launch, consider:

1. **Video Tutorials** - YouTube channel
2. **Interactive Examples** - Web-based node graph viewer
3. **Community Contributions** - User-submitted workflows
4. **API Documentation** - For plugin developers
5. **Multi-Language** - Translate to other languages
6. **Versioned Docs** - Per-release documentation

---

## 🛠️ Tools & Resources

**Created for you:**
- ✅ `mkdocs.yml` - MkDocs configuration
- ✅ `tools/generate_node_docs.py` - Auto-generation script
- ✅ `docs/nodes/generator/sphere.md` - Example generated doc
- ✅ `docs/AUTO_GENERATION_GUIDE.md` - How auto-gen works
- ✅ `docs/AUTO_GENERATION_DEMO.md` - Examples

**Next Steps:**
1. Review the auto-generation script
2. Test it: `python tools/generate_node_docs.py`
3. Install MkDocs: `pip install mkdocs-material`
4. Preview: `mkdocs serve`
5. Start writing Quick Start guide

**Questions?**
- How does auto-generation work? → See `AUTO_GENERATION_DEMO.md`
- What to write manually? → Tutorials, guides, concepts
- How to deploy? → `mkdocs gh-deploy` (one command)

---

## 💡 Key Benefits

### Auto-Generation
- ✅ Saves hours (44 nodes × 15min = 11 hours saved)
- ✅ Always accurate (code = docs)
- ✅ Consistent formatting
- ✅ Scalable to 100+ nodes

### MkDocs
- ✅ Professional appearance
- ✅ Easy to maintain (just Markdown)
- ✅ Free hosting (GitHub Pages)
- ✅ Search, mobile, dark mode built-in

### In-App Integration
- ✅ F1 help = instant documentation
- ✅ Example files = learn by doing
- ✅ No context switching = better UX

Ready to set up? Ask me anything about implementation!
