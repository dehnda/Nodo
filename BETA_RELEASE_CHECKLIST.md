# Beta Release Checklist for Nodo

**Target**: Beta testers (5-20 trusted users)
**Timeline Estimate**: 2-3 weeks of focused work
**Current Status**: 251/260 tests passing (96.5%)

---

## 🚨 **CRITICAL BLOCKERS** (Must Fix Before ANY Release)

### 1. Fix All 9 Failing Tests ⚠️ **PRIORITY 1**
**Status**: ❌ Not Started
**Estimated Time**: 3-5 days
**Impact**: HIGH - These indicate core instability

**Failing Tests**:
- `MeshGeneratorTest.BoxGeneration` - Basic box primitive broken (expects 12 faces, getting 6)
- `MeshValidatorTest.ClosedMeshChecking` - Validation system unreliable
- `MeshRepairerTest.CleanMeshUnchanged` - Eigen assertion failure (out of bounds)
- `ArraySOPTest.LinearArrayCreation` - Input port nullptr
- `ArraySOPTest.RadialArrayCreation` - Disabled
- `ArraySOPTest.GridArrayCreation` - Disabled
- `ArraySOPTest.CachingWorks` - Input port nullptr
- `ArraySOPTest.MarkDirtyInvalidatesCache` - Input port nullptr
- `NoiseDisplacementTest.GraphExecutionIntegration` - Node execution failure

**Why Critical**: Beta testers WILL encounter these bugs. Shipping with failing tests destroys credibility.

**Action Items**:
- [ ] Debug BoxGeneration: Why only 6 faces instead of 12? (Quads vs triangles issue?)
- [ ] Fix MeshValidator::is_closed() - sphere should be closed
- [ ] Fix MeshRepairer Eigen bounds error
- [ ] Fix ArraySOP input port registration (4 related failures)
- [ ] Debug NoiseDisplacementTest execution failure
- [ ] Re-enable disabled array tests or remove them

---

### 2. Remove False GPU Acceleration Claims ⚠️ **PRIORITY 1**
**Status**: ❌ Not Started
**Estimated Time**: 30 minutes
**Impact**: CRITICAL - Credibility/trust issue

**Current README Claims**:
> "GPU-native procedural mesh generation system"
> "Complete GPU acceleration for real-time performance"
> "GPU Mesh Generation: All primitives working with 10-100x speedups"

**Reality**: NO GPU compute shaders implemented. Everything is CPU-based.

**Action Items**:
- [ ] Remove "GPU-native" and "GPU-accelerated" from title/tagline
- [ ] Remove GPU claims from Current Status section
- [ ] Update to: "CPU-based with GPU acceleration planned for v2.0"
- [ ] Be honest: "BVH spatial acceleration provides 45x boolean speedup" (this is TRUE)
- [ ] Reposition as "Houdini-inspired procedural modeling for C++"

**Updated Positioning**:
```markdown
# Nodo - Procedural Mesh Generation

A modern C++20 **node-based procedural mesh generation system** with
Houdini-inspired SOP workflows and intelligent caching.

**Features**:
- 🔥 28 working SOP nodes with visual node editor
- 🧠 Smart caching and dependency resolution
- ⚡ BVH spatial acceleration (45x boolean speedup)
- 🎯 Modern C++20 architecture
```

---

## 📋 **ESSENTIAL for Beta** (Must Have)

### 3. Create Screenshots & Demo Video 📸 **PRIORITY 2**
**Status**: ❌ Not Started
**Estimated Time**: 2-3 hours
**Impact**: HIGH - First impression matters

**What to Capture**:
- [ ] Node editor with medium complexity scene (8-12 nodes)
- [ ] 3D viewport showing resulting mesh (shaded mode)
- [ ] Property panel with parameters visible
- [ ] Example of boolean operation result
- [ ] Example of array/scatter procedural geometry

**Screenshot Specs**:
- 1920x1080 or higher resolution
- Good lighting in viewport (not default dark)
- Clean scene (not "Untitled-1.nfg")
- Professional color scheme visible

**Optional Video** (2-5 minutes):
- Screen recording showing: Create sphere → Transform → Array → Export
- Upload to YouTube (unlisted link for beta testers)
- Shows UI responsiveness and workflow

**Where to Add**:
- Update README with `![Screenshot](docs/screenshots/node_editor.png)`
- Create `docs/screenshots/` folder
- Add to beta tester email

---

### 4. Write User Documentation 📚 **PRIORITY 2**
**Status**: ❌ Not Started
**Estimated Time**: 1-2 days
**Impact**: HIGH - Beta testers need guidance

**Create: USER_GUIDE.md**

**Table of Contents**:
```markdown
# Nodo User Guide

## Getting Started
- Installation (Windows/Linux)
- First Launch
- Interface Overview

## Tutorial 1: Your First Scene
- Creating a sphere node
- Adjusting parameters
- Adding a transform
- Exporting to OBJ

## Tutorial 2: Boolean Operations
- Creating multiple primitives
- Using BooleanSOP
- Understanding manifold meshes

## Tutorial 3: Procedural Patterns
- Using ArraySOP for duplication
- ScatterSOP for point distribution
- CopyToPointsSOP for instancing

## Node Reference
- Generators (Sphere, Box, Cylinder, Plane, Torus, Line)
- Modifiers (Transform, Extrude, Subdivision, Laplacian)
- Utilities (Merge, Delete, Group, Switch)
- Export (ExportSOP)

## Keyboard Shortcuts
- Node creation: Tab menu
- Delete: Delete/Backspace
- Frame All: F
- (etc.)

## FAQ & Troubleshooting
- Why isn't my boolean working? (manifold mesh requirement)
- How do I export my model? (ExportSOP node)
- App won't launch (Qt dependency issues)
```

**Action Items**:
- [ ] Create USER_GUIDE.md
- [ ] Write 3 basic tutorials with screenshots
- [ ] Document all 28 SOP nodes (brief descriptions)
- [ ] Add troubleshooting section
- [ ] Link from README

---

### 5. Create Example Project Files 🎨 **PRIORITY 2**
**Status**: ❌ Not Started
**Estimated Time**: 3-4 hours
**Impact**: MEDIUM - Shows capabilities

**Projects to Create** (in `projects/` folder):

**Simple Examples**:
- [ ] `01_basic_sphere.nfg` - Single sphere with transform
- [ ] `02_boolean_union.nfg` - Two primitives with union
- [ ] `03_linear_array.nfg` - Box duplicated in line

**Intermediate Examples**:
- [ ] `04_procedural_tower.nfg` - Multiple cylinders, arrays, transforms
- [ ] `05_scatter_pattern.nfg` - Scatter → CopyToPoints workflow
- [ ] `06_subdivision_smooth.nfg` - Rough mesh → Subdivision smooth

**Create: projects/README.md**:
```markdown
# Example Projects

## Basic Examples
- `01_basic_sphere.nfg` - Introduction to node creation and parameters
- `02_boolean_union.nfg` - Combining meshes with boolean operations
- `03_linear_array.nfg` - Duplicating geometry in patterns

## Intermediate Examples
- `04_procedural_tower.nfg` - Multi-node procedural structure
- `05_scatter_pattern.nfg` - Instancing workflow demonstration
- `06_subdivision_smooth.nfg` - Mesh smoothing techniques

Load these files via: File → Open Scene
```

---

### 6. Binary Packaging Script 📦 **PRIORITY 2**
**Status**: ❌ Not Started
**Estimated Time**: 1 day
**Impact**: HIGH - Testers need easy installation

**Create: `scripts/package_release.sh` (Linux)**:
```bash
#!/bin/bash
# Package Nodo for distribution

VERSION="0.9.0-beta"
BUILD_DIR="build/Debug"
PACKAGE_DIR="nodo-${VERSION}-linux-x64"

# Create package directory
mkdir -p "${PACKAGE_DIR}"/{bin,examples,docs}

# Copy executable
cp "${BUILD_DIR}/nodo_studio/nodo_studio" "${PACKAGE_DIR}/bin/"

# Copy Qt libraries (from Conan)
# Find and copy libQt6*.so files
cp ~/.conan2/p/*/p/lib/libQt6*.so.6 "${PACKAGE_DIR}/bin/"

# Copy licenses
cp LICENSE "${PACKAGE_DIR}/"
cp THIRD_PARTY_LICENSES.txt "${PACKAGE_DIR}/"

# Copy documentation
cp USER_GUIDE.md "${PACKAGE_DIR}/docs/"
cp README.md "${PACKAGE_DIR}/docs/"

# Copy examples
cp projects/*.nfg "${PACKAGE_DIR}/examples/"
cp projects/README.md "${PACKAGE_DIR}/examples/"

# Create launcher script
cat > "${PACKAGE_DIR}/nodo.sh" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export LD_LIBRARY_PATH="${SCRIPT_DIR}/bin:${LD_LIBRARY_PATH}"
"${SCRIPT_DIR}/bin/nodo_studio" "$@"
EOF
chmod +x "${PACKAGE_DIR}/nodo.sh"

# Create archive
tar -czf "nodo-${VERSION}-linux-x64.tar.gz" "${PACKAGE_DIR}"

echo "Package created: nodo-${VERSION}-linux-x64.tar.gz"
```

**Create: `scripts/package_release.bat` (Windows)**:
```batch
@echo off
set VERSION=0.9.0-beta
set BUILD_DIR=build\Debug
set PACKAGE_DIR=nodo-%VERSION%-windows-x64

mkdir "%PACKAGE_DIR%\bin"
mkdir "%PACKAGE_DIR%\examples"
mkdir "%PACKAGE_DIR%\docs"

copy "%BUILD_DIR%\nodo_studio\nodo_studio.exe" "%PACKAGE_DIR%\bin\"
copy LICENSE "%PACKAGE_DIR%\"
copy THIRD_PARTY_LICENSES.txt "%PACKAGE_DIR%\"
copy USER_GUIDE.md "%PACKAGE_DIR%\docs\"
copy projects\*.nfg "%PACKAGE_DIR%\examples\"

REM Use windeployqt to copy Qt DLLs
windeployqt "%PACKAGE_DIR%\bin\nodo_studio.exe"

echo Package created in %PACKAGE_DIR%
```

**Action Items**:
- [ ] Create packaging scripts
- [ ] Test on your machine
- [ ] Verify Qt libraries are included
- [ ] Test extracted package on clean VM

---

## 🔧 **IMPORTANT for Beta** (Should Have)

### 7. Beta Tester Agreement 📄 **PRIORITY 3**
**Status**: ❌ Not Started
**Estimated Time**: 1 hour
**Impact**: MEDIUM - Legal protection

**Create: BETA_AGREEMENT.txt**:
```
Nodo Beta Testing Agreement

Thank you for participating in the Nodo beta program!

BY USING THIS BETA SOFTWARE, YOU AGREE TO:

1. NON-DISCLOSURE
   - This software is confidential and proprietary
   - Do not share the software binaries with others
   - Do not publicly discuss features without permission
   - Screenshots/videos for bug reports are OK

2. FEEDBACK
   - Provide feedback on bugs, features, usability
   - Response is appreciated but not required
   - Feedback helps improve the final product

3. NO WARRANTY
   - Beta software is provided "as is"
   - May contain bugs or cause data loss
   - Always backup your work
   - Use at your own risk

4. RESTRICTIONS
   - Do not reverse engineer or decompile
   - Do not use for commercial projects (yet)
   - Beta license expires: [Date - e.g., Jan 31, 2026]

5. REPORTING BUGS
   - Email: your-email@example.com
   - Include: steps to reproduce, screenshots, .nfg file
   - Check Discord/GitHub Issues first

BETA PERIOD: [Start Date] to [End Date]

By downloading and using this beta software, you acknowledge
that you have read and agree to these terms.

Questions? Contact: your-email@example.com
```

**Distribution Method**:
- Option 1: Google Form with agreement checkbox
- Option 2: Email acceptance ("Reply YES to accept")
- Option 3: Click-through in installer (more complex)

---

### 8. Set Up Feedback Channels 💬 **PRIORITY 3**
**Status**: ❌ Not Started
**Estimated Time**: 2-3 hours
**Impact**: MEDIUM - Need to collect feedback

**Options** (pick 1-2):

**Option A: GitHub Issues** (Recommended if < 20 testers)
- [ ] Keep repo private
- [ ] Invite beta testers as collaborators (read-only)
- [ ] Create issue template for bug reports
- [ ] Create issue template for feature requests

**Option B: Discord Server**
- [ ] Create private Discord server
- [ ] Channels: #announcements, #bug-reports, #feedback, #general
- [ ] Invite beta testers
- [ ] Pin beta agreement in #announcements

**Option C: Email Only**
- [ ] Create dedicated email: nodo-beta@yourdomain.com
- [ ] Template for bug report emails
- [ ] Slower but simpler

**Recommended**: GitHub Issues + Discord for discussion

---

### 9. Test on Clean Machines 🖥️ **PRIORITY 3**
**Status**: ❌ Not Started
**Estimated Time**: 1 day
**Impact**: HIGH - Find packaging issues

**Test Platforms**:
- [ ] Fresh Ubuntu 22.04 VM (VirtualBox/VMware)
- [ ] Fresh Windows 11 VM
- [ ] Verify Qt dependencies install correctly
- [ ] App launches without errors
- [ ] Can create scene, manipulate nodes
- [ ] Can save/load .nfg files
- [ ] Can export OBJ files
- [ ] Example projects load correctly

**Common Issues to Check**:
- Missing Qt libraries
- Missing Manifold/CGAL dependencies
- File path issues (hardcoded paths)
- Font rendering problems
- OpenGL driver issues

---

## 🎯 **NICE TO HAVE for Beta** (Optional)

### 10. Clean Up Compiler Warnings 🧹 **PRIORITY 4**
**Status**: ❌ Not Started
**Estimated Time**: 2-3 hours
**Impact**: LOW - But makes project look professional

**Current**: 328 warnings (mostly in nodo_studio)

**Focus on**:
- Unused includes
- Magic numbers in UI code (define constants)
- Redundant access specifiers

**Can Ignore**:
- "Parameter name too short" style warnings (not breaking)

---

### 11. Add Error Logging 📝 **PRIORITY 4**
**Status**: ❌ Not Started
**Estimated Time**: 3-4 hours
**Impact**: MEDIUM - Helps debug user issues

**Create Simple Logger**:
```cpp
// nodo_studio/src/Logger.hpp
class Logger {
public:
    static void log_error(const std::string& message) {
        std::ofstream file("nodo_errors.log", std::ios::app);
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        file << "[" << std::ctime(&time) << "] ERROR: " << message << "\n";
    }
};
```

**Use in catch blocks**:
```cpp
try {
    // ... code ...
} catch (const std::exception& e) {
    Logger::log_error(e.what());
    QMessageBox::critical(this, "Error", e.what());
}
```

**Benefit**: Beta testers can send you `nodo_errors.log` with bug reports

---

### 12. Beta Announcement Materials 📧 **PRIORITY 4**
**Status**: ❌ Not Started
**Estimated Time**: 1-2 hours
**Impact**: MEDIUM - First contact with testers

**Create Email Template**:
```
Subject: Nodo Beta Testing Invitation

Hi [Name],

I'm excited to invite you to beta test Nodo - a new procedural mesh
generation tool with Houdini-inspired workflows!

WHAT IS NODO?
Nodo is a node-based 3D modeling tool with 28 procedural operations,
visual node editor, and real-time 3D viewport. Think "Houdini SOP
workflow" but standalone and C++.

WHAT I NEED FROM YOU:
- Test core features (node creation, mesh export, boolean operations)
- Report bugs via [GitHub Issues / Discord / Email]
- Optional feedback on UI/UX
- 2-week beta period: [Start] to [End]

BETA PACKAGE INCLUDES:
- Nodo v0.9.0-beta (Windows/Linux)
- User guide & tutorials
- 6 example project files
- Beta agreement (NDA - please read)

DOWNLOAD LINK:
[Google Drive / Dropbox link]

GETTING STARTED:
1. Download package
2. Read BETA_AGREEMENT.txt
3. Follow USER_GUIDE.md
4. Open example files in examples/ folder
5. Report bugs!

SUPPORT:
- Discord: [invite link]
- GitHub: [repo link - private]
- Email: [your email]

Thank you for helping make Nodo better!

Screenshots attached - see what you'll be testing!

Best,
Daniel

---
Attachment: screenshot1.png (node editor)
Attachment: screenshot2.png (3D viewport)
```

---

## ⏱️ **Timeline Estimation**

### Week 1: Critical Fixes
- **Mon-Wed**: Fix all 9 failing tests (PRIORITY 1)
- **Thu**: Remove GPU claims from README (PRIORITY 1)
- **Fri**: Create screenshots (PRIORITY 2)

### Week 2: Documentation & Packaging
- **Mon-Tue**: Write USER_GUIDE.md (PRIORITY 2)
- **Wed**: Create example .nfg files (PRIORITY 2)
- **Thu**: Create packaging scripts (PRIORITY 2)
- **Fri**: Test on clean VMs (PRIORITY 3)

### Week 3: Final Prep & Launch
- **Mon**: Set up GitHub Issues + Discord (PRIORITY 3)
- **Tue**: Create beta agreement (PRIORITY 3)
- **Wed**: Write announcement email (PRIORITY 4)
- **Thu**: Send to 3-5 trusted testers (soft launch)
- **Fri**: Gather feedback, fix critical issues

### Week 4: Expand Beta
- **Mon**: Send to 10-15 more testers if Week 3 went well
- Monitor feedback and iterate

---

## ✅ **ABSOLUTE MINIMUM for Beta** (If Rushed)

If you only have 1 week, focus on:

1. ✅ Fix ArraySOP tests (4 failures - related issue)
2. ✅ Fix BoxGeneration test
3. ✅ Remove GPU claims from README
4. ✅ Create 3 screenshots
5. ✅ Write 1-page quick start guide
6. ✅ Create 2 example .nfg files
7. ✅ Package binary with Qt libraries
8. ✅ Test on one clean VM
9. ✅ Send to 3-5 close friends/colleagues

**This is RISKY** but technically possible. Recommend full 2-3 week timeline.

---

## 📊 **Success Metrics for Beta**

**Week 1-2 Goals**:
- [ ] 5-10 beta testers actively using
- [ ] <5 critical bugs reported
- [ ] All testers can create basic scene and export

**Week 3-4 Goals**:
- [ ] 10-20 total beta testers
- [ ] Positive feedback on core workflow
- [ ] UI/UX improvements identified
- [ ] Decision: go public or iterate more?

**Red Flags** (Stop and fix):
- Crashes on launch for >50% of testers
- Cannot export OBJ files reliably
- Major feature completely broken
- Overwhelming negative feedback

---

## 🎯 **Next Steps**

**TODAY**:
1. Read this checklist thoroughly
2. Decide on timeline (1 week minimal vs 2-3 weeks proper)
3. Start with PRIORITY 1 items (tests + README)

**THIS WEEK**:
1. Fix failing tests
2. Update README with honest positioning
3. Create screenshots

**NEXT WEEK**:
1. Write user documentation
2. Create example files
3. Package and test

**Questions?**
- Prioritization: Focus on PRIORITY 1-2 items first
- Timeline: 2-3 weeks recommended, 1 week possible but risky
- Beta size: Start small (5 testers), expand if successful

---

**Remember**: Beta is about finding issues BEFORE public launch.
It's OK (expected!) that it's not perfect. But basic stability is critical.
