# Keyboard Shortcuts

Quick reference for all keyboard shortcuts in Nodo.

## File Operations

| Action | Windows/Linux |
|--------|---------------|
| **New Scene** | ++ctrl+n++ |
| **Open Scene** | ++ctrl+o++ |
| **Save Scene** | ++ctrl+s++ |
| **Save Scene As** | ++ctrl+shift+s++ |
| **Exit** | ++ctrl+q++ |

---

## Edit Operations

| Action | Windows/Linux |
|--------|---------------|
| **Undo** | ++ctrl+z++ |
| **Redo** | ++ctrl+shift+z++ or ++ctrl+y++ |
| **Cut** | ++ctrl+x++ |
| **Copy** | ++ctrl+c++ |
| **Paste** | ++ctrl+v++ |
| **Delete** | ++delete++ or ++backspace++ |
| **Duplicate** | ++ctrl+d++ |
| **Select All** | ++ctrl+a++ |

---

## Node Graph

### Navigation

| Action | Windows/Linux |
|--------|---------------|
| **Pan Graph** | ++middle-button++ drag |
| | ++space++ + ++left-button++ drag |
| **Zoom In/Out** | ++scroll-wheel++ |
| **Frame All Nodes** | ++a++ or ++home++ |
| **Frame Selected** | ++f++ |

### Node Operations

| Action | Windows/Linux |
|--------|---------------|
| **Add Node** | ++tab++ (future) |
| **Delete Node** | ++delete++ or ++backspace++ |
| **Duplicate Node** | ++ctrl+d++ |
| **Rename Node** | ++f2++ or double-click title |
| **Select All Nodes** | ++ctrl+a++ |
| **Deselect All** | ++escape++ |

### Connections

| Action | Windows/Linux |
|--------|---------------|
| **Delete Connection** | Select + ++delete++ |
| **Create Connection** | Drag from output pin to input pin |

---

## Viewport

### Camera Navigation

| Action | Windows/Linux |
|--------|---------------|
| **Orbit/Rotate** | ++middle-button++ drag |
| | ++alt+left-button++ drag |
| **Pan** | ++shift+middle-button++ drag |
| | ++shift+alt+left-button++ drag |
| **Zoom** | ++scroll-wheel++ |
| | ++alt+right-button++ drag |
| **Frame All** | ++a++ or double-click background |
| **Frame Selected** | ++f++ |
| **Reset Camera** | ++home++ |

### Display Toggles

| Action | Windows/Linux |
|--------|---------------|
| **Toggle Wireframe** | ++1++ (future) |
| **Toggle Shading** | ++2++ (future) |
| **Toggle Grid** | ++g++ (future) |

*Note: Current version uses toolbar buttons for display toggles. Keyboard shortcuts coming soon.*

---

## Property Panel

### Parameter Editing

| Action | Windows/Linux |
|--------|---------------|
| **Next Parameter** | ++tab++ |
| **Previous Parameter** | ++shift+tab++ |
| **Confirm Edit** | ++enter++ |
| **Cancel Edit** | ++escape++ |
| **Toggle Expression Mode** | Click `≡` or `#` button |
| **Reset to Default** | Right-click → Reset to Default |

### Value Adjustment

| Action | Windows/Linux |
|--------|---------------|
| **Scrub Value** | Click + drag left/right |
| **Fine Adjust** | ++shift++ + drag (10x slower) |
| **Coarse Adjust** | ++ctrl++ + drag (10x faster) |

---

## Selection

### Node Graph Selection

| Action | Windows/Linux |
|--------|---------------|
| **Select Node** | ++left-button++ click |
| **Add to Selection** | ++ctrl+left-button++ click |
| **Toggle Selection** | ++ctrl+left-button++ click |
| **Box Select** | ++left-button++ drag (future) |
| **Select All** | ++ctrl+a++ |
| **Deselect All** | ++escape++ |

### Multi-Selection

| Action | Windows/Linux |
|--------|---------------|
| **Add to Selection** | ++ctrl+left-button++ |
| **Remove from Selection** | ++ctrl+left-button++ on selected |
| **Invert Selection** | ++ctrl+i++ (future) |

---

## Context Menus

### Node Graph

| Action | Windows/Linux |
|--------|---------------|
| **Node Context Menu** | ++right-button++ on node |
| **Connection Context Menu** | ++right-button++ on connection |
| **Graph Context Menu** | ++right-button++ on empty space |

### Property Panel

| Action | Windows/Linux |
|--------|---------------|
| **Parameter Context Menu** | ++right-button++ on parameter |

---

## Focus & Windows

| Action | Windows/Linux |
|--------|---------------|
| **Focus Node Graph** | Click in node graph area |
| **Focus Viewport** | Click in viewport |
| **Focus Property Panel** | Click in property panel |

---

## Expression Mode

When editing expressions in parameters:

| Action | Windows/Linux |
|--------|---------------|
| **Accept Expression** | ++enter++ |
| **Cancel Expression** | ++escape++ |
| **Toggle Expression Mode** | Click `≡` or `#` button |

---

## Quick Tips

### Viewport Navigation

!!! tip "Two-Button Mouse"
    If you don't have a middle mouse button:
    - Use ++alt+left-button++ for orbit
    - Use ++shift+alt+left-button++ for pan

!!! tip "Touchpad"
    - Two-finger drag for pan
    - Pinch for zoom
    - ++alt++ + drag for orbit

### Efficiency Tips

!!! tip "Frame Often"
    Press ++f++ to keep selected nodes in view while working. Press ++a++ to see the whole graph.

!!! tip "Duplicate + Modify"
    ++ctrl+d++ to duplicate a node with all its connections, then modify parameters.

!!! tip "Tab Through Parameters"
    Use ++tab++ to quickly move between parameter fields without clicking.

### Common Workflows

**Create and Connect:**
1. Add node from library
2. Drag from output to input
3. Adjust parameters
4. Press ++f++ to frame

**Iterate Quickly:**
1. Select node
2. ++ctrl+d++ to duplicate
3. Modify parameters
4. Compare results

**Organize Graph:**
1. Select nodes
2. Drag to reposition
3. Use Null nodes as anchors
4. Rename nodes (++f2++) for clarity

---

## Customization

### Future Features

The following shortcuts will be customizable in future versions:

- Display toggles (wireframe, grid, etc.)
- Node creation shortcuts
- Custom key bindings
- Hotkey editor

---

## Platform Differences

### Windows

All shortcuts use ++ctrl++ as the modifier key.

### Linux

All shortcuts use ++ctrl++ as the modifier key.

Some Linux desktop environments may intercept certain shortcuts. Check your system settings if a shortcut doesn't work.

---

## Mouse Controls Summary

### Node Graph

| Action | Control |
|--------|---------|
| **Pan** | ++middle-button++ drag or ++space++ + drag |
| **Zoom** | ++scroll-wheel++ |
| **Select Node** | ++left-button++ |
| **Multi-Select** | ++ctrl+left-button++ |
| **Context Menu** | ++right-button++ |
| **Create Connection** | Drag from pin |
| **Delete Connection** | ++right-button++ → Delete |

### Viewport

| Action | Control |
|--------|---------|
| **Orbit** | ++middle-button++ or ++alt+left-button++ |
| **Pan** | ++shift+middle-button++ or ++shift+alt+left-button++ |
| **Zoom** | ++scroll-wheel++ or ++alt+right-button++ |
| **Frame All** | ++a++ or double-click background |
| **Frame Selected** | ++f++ |

### Property Panel

| Action | Control |
|--------|---------|
| **Edit Value** | ++left-button++ click |
| **Scrub Value** | Click + drag |
| **Context Menu** | ++right-button++ |

---

## Printable Cheat Sheet

### Essential Shortcuts

**File:**
- Save: ++ctrl+s++
- Open: ++ctrl+o++

**Edit:**
- Undo: ++ctrl+z++
- Redo: ++ctrl+shift+z++
- Delete: ++delete++
- Duplicate: ++ctrl+d++

**View:**
- Frame All: ++a++
- Frame Selected: ++f++

**Navigation:**
- Orbit: ++middle-button++ or ++alt+left-button++
- Pan: ++shift+middle-button++
- Zoom: ++scroll-wheel++

---

**Pro Tip:** Print this page or bookmark it for quick reference while learning Nodo!

See the [Interface Overview](../getting-started/interface.md) for more details on UI navigation.
