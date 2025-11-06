# Installation Guide

Get Nodo up and running on your system.

## System Requirements

### Minimum Requirements

- **OS**: Windows 10/11 or Linux (Ubuntu 20.04+)
- **CPU**: Dual-core processor (2 GHz+)
- **RAM**: 4 GB
- **GPU**: OpenGL 3.3 compatible graphics card
- **Storage**: 200 MB free space

### Recommended Requirements

- **OS**: Windows 11 or Linux (Ubuntu 22.04+)
- **CPU**: Quad-core processor (3 GHz+)
- **RAM**: 8 GB or more
- **GPU**: Dedicated graphics card with 2 GB VRAM
- **Storage**: 500 MB free space

---

## Download Nodo

Download the latest version from the official website: **[nodo3d.com](https://nodo3d.com)**

Choose your platform:
- **Windows**: `Nodo-Windows-x64.zip`
- **Linux**: `Nodo-Linux-x64.AppImage`

---

## Installation Instructions

### Windows

1. **Download** the Windows ZIP file
2. **Extract** to a folder (e.g., `C:\Program Files\Nodo`)
3. **Run** `nodo_studio.exe`
4. *Optional*: Create a desktop shortcut

!!! note "Windows Defender"
    Windows may show a SmartScreen warning for unsigned applications. Click "More info" → "Run anyway".

### Linux

1. **Download** the AppImage file
2. **Make it executable**:
   ```bash
   chmod +x Nodo-Linux-x64.AppImage
   ```
3. **Run** the AppImage:
   ```bash
   ./Nodo-Linux-x64.AppImage
   ```

!!! tip "Desktop Integration"
    Most Linux systems will prompt to integrate the AppImage into your application menu.

---

## First Launch

### What to Expect

On first launch, Nodo will:
1. Create a settings directory in your home folder
2. Open with a default empty scene
3. Display the node graph, viewport, and panels

### Default Layout

- **Left**: Node Library panel
- **Center**: Node Graph (bottom) and Viewport (top)
- **Right**: Property Panel

You can rearrange panels by dragging their title bars.

---

## Verify Installation

### Quick Test

1. **Open** Nodo
2. **Add a Sphere node** from Node Library → Generator
3. You should see a sphere in the viewport

If you see the sphere, Nodo is working correctly! ✅

### Troubleshooting

#### "Failed to initialize OpenGL"

Your graphics driver may be outdated.

**Solution:**
- Update your graphics drivers
- For Intel integrated graphics, ensure OpenGL 3.3+ support

#### "Missing DLL" (Windows)

**Solution:**
- Download and install [Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)

#### Crash on Startup

**Solution:**
1. Check system requirements (OpenGL 3.3+)
2. Update graphics drivers
3. Try running from terminal to see error messages

#### Blank Viewport

**Solution:**
- Check that a node is connected to Output
- Press ++f++ to frame geometry
- Toggle wireframe/shading modes

---

## Configuration

### Settings Location

Nodo stores settings in:

- **Windows**: `%APPDATA%/Nodo/NodoStudio/`
- **Linux**: `~/.config/Nodo/NodoStudio/`

### Recent Files

Recent projects are tracked automatically in settings. Access via **File → Recent Projects**.

### Reset Settings

To reset to defaults, delete the settings directory (application must be closed).

---

## Uninstallation

### Windows
1. Delete the Nodo folder
2. Optionally delete settings: `%APPDATA%/Nodo/`

### Linux
1. Delete the AppImage file
2. Optionally delete settings: `~/.config/Nodo/`

---

## Next Steps

Now that Nodo is installed:

1. **[Quick Start Guide](quick-start.md)** - Build your first model in 30 minutes
2. **[Interface Overview](interface.md)** - Learn the UI layout
3. **[Your First Project](first-project.md)** - Guided tutorial

---

## Getting Help

### Resources

- **Documentation**: [Full docs](../index.md)
- **Support**: Contact support via the website
- **Feature Requests**: Submit via the website

Happy modeling! 🚀
