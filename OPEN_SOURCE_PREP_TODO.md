# Open Source Preparation TODO

This checklist tracks all items to complete before publishing Nodo as open source.

---

## 🚨 CRITICAL - Must Complete Before Publishing

### [X] 1. Change LICENSE from Proprietary to Open Source
- **Current Status**: ✅ COMPLETE - MIT License applied
- **Action**: Choose and apply an open source license
- **Options**:
  - [X] MIT License (most permissive, simple) ✅ SELECTED
  - [ ] Apache 2.0 (includes patent grant)
  - [ ] GPL v3 (copyleft, requires derivatives to be open source)
  - [ ] LGPL v3 (like GPL but allows linking from proprietary software)
- **Files to Update**:
  - [X] `/LICENSE` ✅ UPDATED
  - Update copyright notices if needed
- **Priority**: BLOCKING - Cannot publish without this

### [X] 2. Handle Personal Email Address
- **Current**: ✅ COMPLETE - Email addresses removed
- **Locations**:
  - [X] `/LICENSE` (removed in step 1) ✅
  - [X] `/THIRD_PARTY_LICENSES.txt` (replaced with GitHub issues) ✅
  - [X] `/LICENSING_STRATEGY.md` (replaced with GitHub issues) ✅
- **Options**:
  - Keep as-is (accept potential spam)
  - Change to GitHub contact: `@dehnda` ✅ SELECTED
  - Create project email
  - Remove entirely
- **Priority**: HIGH

### [X] 3. Update .gitignore for Missing Files
- **Action**: ✅ COMPLETE - Added patterns to .gitignore
  - [X] `*.log` (to catch build.log, studio_debug.log) ✅
  - [X] `*.csv` (for benchmark_results.csv) ✅
  - [X] `venv/` (Python virtual environment - 149 MB) ✅
  - [X] `out/` (output directory - 612 KB) ✅
- **Priority**: HIGH

### [X] 4. Clean Up Repository Root Directory
- **Files to Delete/Clean**: ✅ COMPLETE - All files deleted
  - [X] `build.log` ✅
  - [X] `studio_debug.log` ✅
  - [X] `boolean_result.obj` ✅
  - [X] `benchmark_results.csv` ✅
  - [X] `CLAUDE.md.backup` ✅
- **Projects directory cleanup**:
  - [X] Review `projects/*.obj` files - keep or delete? ✅ DELETED (9 files)
- **Priority**: HIGH

---

## ⚠️ RECOMMENDED - Should Complete

### [X] 5. Review Vercel Configuration
- **Files**: `.vercel/`, `.vercelignore`, `build-vercel.sh`
- **Decision**: ✅ Remove - Using GitHub Pages instead
- [X] If keeping: Ensure no sensitive config
- [X] If removing: Delete `.vercel/`, `.vercelignore`, `build-vercel.sh` ✅ DELETED
- **Priority**: MEDIUM

### [X] 6. Review .vscode Directory
- **Current**: Directory exists with personal settings
- **Already in .gitignore**: ✅ Yes
- **Decision**: ✅ Keep (shows recommended settings to contributors)
- [X] Option A: Keep (shows recommended settings to contributors) ✅ SELECTED
- [ ] Option B: Delete and let contributors use their own
- **Priority**: LOW

### [X] 7. Review TODO/FIXME Comments in Code
- **Found**: 44 instances in source code
- **Action**: ✅ COMPLETE - All reviewed and appropriate for public release
- [X] Run review of all TODOs ✅
- [X] Clean up or reword sensitive ones ✅ None found
- **Summary**: All TODOs are normal development markers:
  - Feature placeholders (disabled menu items)
  - Algorithm improvements (weights, sorting)
  - Future enhancements (proper implementations)
- **Result**: Safe to publish - TODOs are helpful for contributors
- **Priority**: MEDIUM

### [X] 8. Update Documentation References
- **Action**: ✅ COMPLETE - Updated references to reflect open source status
- [X] Review mentions of "CLAUDE.md" in docs ✅ Changed to CONTRIBUTING.md
- [X] Update any internal-only references ✅
- [X] Ensure ROADMAP.md reflects public roadmap ✅
- [X] Delete internal development markdown files ✅
- [X] Remove HTML concept/design files ✅
- **Files Updated**:
  - README.md (removed CLAUDE.md refs, updated license section)
  - docs/index.md (changed proprietary to MIT License)
  - docs/NAVIGATION.md (updated all CLAUDE.md references)
- **Files Deleted**:
  - 26 internal milestone/planning docs from docs/ (M1.x, M2.x, M3.x files)
  - 6 internal planning docs from root (COVERAGE_REPORT, DOCUMENTATION_AUDIT, etc.)
  - 13 HTML concept/design files (node_properties_*.html, context_menu_concept.html, etc.)
  - Node_design.jpg, widget_usage_examples.cpp
  - architecture/ directory (PMP planning docs)
- **Remaining User Docs**:
  - docs/*.md - Core documentation (index, CLI guide, attributes, etc.)
  - docs/concepts/ - Conceptual guides (attributes, groups, node-graph, etc.)
  - docs/getting-started/ - Installation and quick start
  - docs/nodes/ - Node documentation by category
  - docs/expressions/ - Expression system reference
  - docs/workflows/ - Usage patterns and workflows
  - docs/reference/ - FAQ and shortcuts
- **Priority**: LOW

### [X] 9. Clean Large Directories
- **Verify these are excluded** (already in .gitignore ✅):
- [X] `build/` - 1.3 GB ✅ (in .gitignore)
- [X] `site/` - 11 MB ✅ (in .gitignore)
- [X] `venv/` - 149 MB ✅ (added to .gitignore)
- [X] `out/` - 612 KB ✅ (added to .gitignore)
- **Note**: No need to delete - .gitignore prevents them from being tracked
- **Priority**: MEDIUM

---

## 📝 OPTIONAL - Nice to Have

### [ ] 10. Add AI Development Acknowledgment
- **Location**: README.md
- **Action**: Add section acknowledging AI-assisted development
- **Example**:
  ```markdown
  ## Development Notes
  This project was developed with assistance from AI coding tools
  (GitHub Copilot, Claude). All code has been reviewed and tested.
  ```
- **Priority**: LOW

### [ ] 11. Add Community Files
- [ ] `CODE_OF_CONDUCT.md` - For community guidelines
- [ ] `SECURITY.md` - For vulnerability reporting
- [ ] `.github/ISSUE_TEMPLATE/bug_report.md`
- [ ] `.github/ISSUE_TEMPLATE/feature_request.md`
- [ ] `.github/PULL_REQUEST_TEMPLATE.md`
- **Priority**: LOW (can add later)

### [ ] 12. Verify Copyright Year Consistency
- **Current**: "2025" in most files
- [ ] Scan all copyright notices
- [ ] Ensure consistency across files
- **Priority**: LOW

### [ ] 13. Review GitHub Actions Workflows
- **Files**: `.github/workflows/*.yml`
- [ ] Verify no hardcoded secrets (uses `${{ secrets.X }}` ✅)
- [ ] Test workflows will work for public repo
- [ ] Update any personal references
- **Priority**: LOW

---

## ✅ ALREADY DONE - No Action Needed

- ✅ Third-party licenses documented in `THIRD_PARTY_LICENSES.txt`
- ✅ No API keys or credentials in code
- ✅ Comprehensive README.md
- ✅ CONTRIBUTING.md present
- ✅ Good .gitignore foundation (just needs additions)
- ✅ Clean build system (CMake + Conan)

---

## 🎯 Recommended Workflow

1. **Complete CRITICAL section** (items 1-4)
2. **Make commit**: "Prepare repository for open source release"
3. **Complete RECOMMENDED section** (items 5-9)
4. **Make commit**: "Clean up repository structure"
5. **Complete OPTIONAL section** as desired
6. **Final review**: Check git status, test build
7. **Push to GitHub** and make repository public

---

## 📋 Pre-Publish Verification Checklist

Before making repository public, verify:

- [ ] LICENSE is open source (MIT/Apache/GPL/LGPL)
- [ ] No personal email or contact info (or acceptable level)
- [ ] No .log, .obj, .csv files in root
- [ ] No venv/ or out/ directories tracked
- [ ] git status is clean
- [ ] Build works: `cmake --preset=conan-debug && cmake --build --preset=conan-debug`
- [ ] Tests pass: `ctest --preset=conan-debug`
- [ ] README accurately describes project
- [ ] All commits are appropriate for public viewing

---

## 📞 Questions to Decide

1. **Which open source license?** (MIT recommended for max adoption)
2. **Email handling?** (Keep, change, or remove?)
3. **Vercel deployment?** (Keep or remove config?)
4. **Projects .obj files?** (Test outputs - keep or delete?)
5. **.vscode settings?** (Share or let contributors use their own?)

---

**Created**: 2025-11-14
**Status**: TODO - Not Started
**Next Action**: Choose open source license (#1)
