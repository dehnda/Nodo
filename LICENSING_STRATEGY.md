# Nodo - Proprietary Licensing Strategy

**Status**: Proprietary/Closed Source
**Date**: October 29, 2025
**Author**: Daniel Dehne

## 🎯 Licensing Overview

Nodo is proprietary software with plans to potentially open-source in the future.
Current strategy allows for commercial development while keeping options open.

## 📋 Current License Structure

### Your Code (Proprietary)
- **All original Nodo code**: Fully proprietary, all rights reserved
- **Distribution**: Binary-only, no source code access
- **Commercial use**: Permitted under paid license terms (when you define them)
- **Source code**: Remains private, not distributed

### Third-Party Dependencies (Open Source)
All dependencies are **commercial-friendly**:
- ✅ Qt (LGPL v3) - dynamically linked, allows proprietary apps
- ✅ Eigen (MPL 2.0) - allows proprietary use
- ✅ Manifold (Apache 2.0) - allows proprietary use
- ✅ xatlas (MIT) - allows proprietary use
- ✅ Iconoir (MIT) - allows proprietary use
- ✅ exprtk (MIT) - allows proprietary use
- ✅ fmt (MIT) - allows proprietary use
- ✅ nlohmann_json (MIT) - allows proprietary use
- ✅ Google Test (BSD-3) - allows proprietary use

**No GPL dependencies** - you're free to keep your code proprietary!

## ✅ Legal Compliance Checklist

### Qt LGPL Compliance (Critical)
- [x] Dynamically link to Qt libraries (already doing this via Conan)
- [x] Include THIRD_PARTY_LICENSES.txt with all distributions
- [x] Don't modify Qt source code (you haven't)
- [x] Inform users they can replace Qt libraries
- [ ] If distributing binaries: include note about Qt availability

### General Compliance
- [x] LICENSE file created (proprietary EULA)
- [x] THIRD_PARTY_LICENSES.txt created
- [x] Copyright notices in place
- [ ] Add copyright header to your source files (optional but recommended)
- [ ] Distribution package includes both LICENSE files

## 💰 Monetization Options

### Option 1: Free Beta / Paid Full Version
- Release limited beta for free (watermarked exports, node limit, etc.)
- Sell full license: $99-299 one-time or $19-49/month subscription
- Keep source closed
- Works perfectly with current license structure

### Option 2: Freemium Model
- Free version: limited nodes, basic features
- Pro version: $199-499/year
- Enterprise: $999+/year with support
- All binaries only, no source access

### Option 3: Pure Commercial
- No free version
- $299-799 perpetual license
- Optional maintenance: $99/year
- Position as professional tool

### Option 4: Future Open Source
- Start proprietary to establish market
- Once profitable, open-source under MIT/Apache
- Offer commercial support contracts
- This strategy is proven (GitLab, Redis, etc.)

## 🚀 Distribution Strategy

### What You Can Distribute Now
1. **Compiled binaries** (Windows/Linux executables)
2. **LICENSE** (proprietary EULA)
3. **THIRD_PARTY_LICENSES.txt**
4. **User documentation** (PDF/HTML)
5. **Example project files** (.nfg files)
6. **Qt runtime libraries** (bundled or separate installer)

### What You CANNOT Distribute
- Source code (.cpp, .hpp files)
- Internal development documentation (CLAUDE.md)
- CMake build files
- Test suite
- Git history

### Packaging Checklist
```
nodo-installer/
├── bin/
│   ├── nodo_studio[.exe]          # Your executable
│   └── Qt6*.dll / libQt6*.so      # Qt libraries (LGPL)
├── LICENSE.txt                     # Your proprietary license
├── THIRD_PARTY_LICENSES.txt        # Required for LGPL compliance
├── README.txt                      # User guide
└── examples/
    └── *.nfg                       # Example scenes
```

## 🔒 Repository Management

### Private Repository (Current)
- Keep GitHub repo **private**
- Only you have access
- All source code safe
- Can share with trusted collaborators via private access

### Public Repository (If/When Open Sourcing)
Before making public:
1. Replace LICENSE with MIT or Apache 2.0
2. Remove proprietary EULA
3. Clean git history of sensitive commits
4. Update README to remove commercial language
5. Add CONTRIBUTING.md
6. Decide on CLA (Contributor License Agreement) if needed

## ⚖️ Future Open Source Path

### When to Consider Open Sourcing:
✅ **Good reasons to open source:**
- You've made enough money and want community growth
- You pivot to support/service business model
- You want to build ecosystem/plugins
- Marketing benefit outweighs proprietary value
- You have 1000+ users and want contributions

❌ **Bad reasons to open source:**
- You're struggling to get users (won't magically fix this)
- You think it'll make development easier (more review burden)
- Competitors are open source (different business models)

### Transition Strategy:
1. **Hybrid approach**: Core library open source, Studio app proprietary
   - `nodo_core` → MIT/Apache (open)
   - `nodo_studio` → Proprietary (closed)
   - Allows ecosystem while protecting UI/UX investment

2. **Delayed open source**:
   - Release v1.0 as proprietary
   - Open source v1.0 when you release v2.0
   - Always one major version behind

3. **Dual licensing**:
   - GPL for free users (must share modifications)
   - Commercial license for proprietary use
   - Complex but maximizes revenue (Qt's model)

## 📝 Recommended Next Steps

### Immediate (This Week)
- [x] Add LICENSE file (done)
- [x] Add THIRD_PARTY_LICENSES.txt (done)
- [ ] Update README to remove open-source language
- [ ] Add copyright headers to major source files (optional)
- [ ] Update CLAUDE.md to note proprietary status

### Short Term (Before Distribution)
- [ ] Decide on pricing model
- [ ] Create user EULA (can customize LICENSE file)
- [ ] Set up payment processing (Gumroad, Stripe, etc.)
- [ ] Create binary packaging script
- [ ] Test LGPL compliance (Qt dynamic linking)
- [ ] Consider trademark for "Nodo" name

### Medium Term (Business Development)
- [ ] Decide on trial/demo version strategy
- [ ] Create licensing server (if using subscriptions)
- [ ] Set up support channels (email, Discord, etc.)
- [ ] Consider business entity (LLC, corporation)
- [ ] Consult lawyer for EULA review ($500-1500 one-time)

## 🤝 Working with Others

### Hiring Developers
- Have them sign NDA + Work-for-Hire agreement
- All code they write belongs to you
- Standard industry practice

### Beta Testers
- Beta EULA with NDA clause
- "Don't share binaries or reverse engineer"
- Can use free tools like Google Forms for agreement

### Contractors/Consultants
- Work-for-Hire clause in contract
- IP assignment to you
- Get legal template ($200-500 from lawyer)

## 📧 Contact & Questions

For legal questions: Consult a software licensing attorney
For business questions: Daniel Dehne - [danny.dehne@gmail.com]

## 📚 Resources

- Qt LGPL FAQ: https://www.qt.io/licensing/open-source-lgpl-obligations
- LGPL v3 Full Text: https://www.gnu.org/licenses/lgpl-3.0.html
- Software licensing guide: https://choosealicense.com/
- Dual licensing strategy: https://en.wikipedia.org/wiki/Multi-licensing

---

**Remember**: You can always go from proprietary → open source, but you can NEVER go from open source → proprietary. Starting proprietary gives you maximum flexibility.
