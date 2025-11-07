#!/usr/bin/env python3
"""
Script to refactor MainWindow.cpp by splitting setupMenuBar() into smaller methods.
This is Phase 1 of the refactoring plan.
"""

import re

def split_setup_menubar():
    """Split the massive setupMenuBar() method into focused methods."""
    
    # Read the original file
    with open('nodo_studio/src/MainWindow.cpp', 'r') as f:
        content = f.read()
    
    # Find the setupMenuBar method
    pattern = r'(auto MainWindow::setupMenuBar\(\) -> void \{.*?(?=\n(?:auto|void|QWidget) MainWindow::))'
    match = re.search(pattern, content, re.DOTALL)
    
    if not match:
        print("Could not find setupMenuBar method!")
        return False
    
    original_method = match.group(1)
    print(f"Found setupMenuBar method ({len(original_method)} characters)")
    
    # Extract each menu section
    file_menu_match = re.search(r'(// =+\s*\n\s*// File Menu\s*\n\s*// =+.*?connect\(exitAction.*?;)', original_method, re.DOTALL)
    edit_menu_match = re.search(r'(// =+\s*\n\s*// Edit Menu\s*\n\s*// =+.*?&MainWindow::onInvertSelection\);)', original_method, re.DOTALL)
    view_menu_match = re.search(r'(// =+\s*\n\s*// View Menu\s*\n\s*// =+.*?// widget creation)', original_method, re.DOTALL)
    graph_menu_match = re.search(r'(// =+\s*\n\s*// Graph Menu\s*\n\s*// =+.*?&MainWindow::onDisconnectSelected\);)', original_method, re.DOTALL)
    help_menu_match = re.search(r'(// =+\s*\n\s*// Help Menu\s*\n\s*// =+.*?&MainWindow::onShowKeyboardShortcuts\);)', original_method, re.DOTALL)
    toolbar_match = re.search(r'(// =+\s*\n\s*// Icon Toolbar.*?setCornerWidget\(icon_toolbar, Qt::TopRightCorner\);)', original_method, re.DOTALL)
    
    if not all([file_menu_match, edit_menu_match, view_menu_match, graph_menu_match, help_menu_match, toolbar_match]):
        print("Could not find all menu sections!")
        print(f"File: {bool(file_menu_match)}, Edit: {bool(edit_menu_match)}, View: {bool(view_menu_match)}")
        print(f"Graph: {bool(graph_menu_match)}, Help: {bool(help_menu_match)}, Toolbar: {bool(toolbar_match)}")
        return False
    
    # Create new methods
    new_setup_menubar = """auto MainWindow::setupMenuBar() -> void {
  setupFileMenu();
  setupEditMenu();
  setupViewMenu();
  setupGraphMenu();
  setupHelpMenu();
  setupIconToolbar();
}

"""
    
    file_menu_method = f"""auto MainWindow::setupFileMenu() -> void {{
  QMenuBar *menuBar = this->menuBar();

{file_menu_match.group(1)}
}}

"""
    
    edit_menu_method = f"""auto MainWindow::setupEditMenu() -> void {{
  QMenuBar *menuBar = this->menuBar();

{edit_menu_match.group(1)}
}}

"""
    
    view_menu_method = f"""auto MainWindow::setupViewMenu() -> void {{
  QMenuBar *menuBar = this->menuBar();

{view_menu_match.group(1)}
}}

"""
    
    graph_menu_method = f"""auto MainWindow::setupGraphMenu() -> void {{
  QMenuBar *menuBar = this->menuBar();

{graph_menu_match.group(1)}
}}

"""
    
    help_menu_method = f"""auto MainWindow::setupHelpMenu() -> void {{
  QMenuBar *menuBar = this->menuBar();

{help_menu_match.group(1)}
}}

"""
    
    toolbar_method = f"""auto MainWindow::setupIconToolbar() -> void {{
  QMenuBar *menuBar = this->menuBar();

{toolbar_match.group(1)}
}}

"""
    
    # Replace the old method with all new methods
    new_methods = (new_setup_menubar + file_menu_method + edit_menu_method + 
                   view_menu_method + graph_menu_method + help_menu_method + toolbar_method)
    
    new_content = content.replace(original_method, new_methods)
    
    # Write the modified content
    with open('nodo_studio/src/MainWindow.cpp', 'w') as f:
        f.write(new_content)
    
    print("✓ Successfully refactored setupMenuBar() into 6 methods")
    print(f"  - setupMenuBar() (coordinator)")
    print(f"  - setupFileMenu()")
    print(f"  - setupEditMenu()")
    print(f"  - setupViewMenu()")
    print(f"  - setupGraphMenu()")
    print(f"  - setupHelpMenu()")
    print(f"  - setupIconToolbar()")
    
    return True

def update_header():
    """Add method declarations to MainWindow.h"""
    
    with open('nodo_studio/src/MainWindow.h', 'r') as f:
        content = f.read()
    
    # Find the setupMenuBar declaration
    pattern = r'(\s+void setupMenuBar\(\);)'
    match = re.search(pattern, content)
    
    if not match:
        print("Could not find setupMenuBar() declaration!")
        return False
    
    # Replace with all declarations
    new_declarations = """  void setupMenuBar();
  void setupFileMenu();
  void setupEditMenu();
  void setupViewMenu();
  void setupGraphMenu();
  void setupHelpMenu();
  void setupIconToolbar();"""
    
    new_content = content.replace(match.group(1), new_declarations)
    
    with open('nodo_studio/src/MainWindow.h', 'w') as f:
        f.write(new_content)
    
    print("✓ Updated MainWindow.h with new method declarations")
    
    return True

if __name__ == "__main__":
    print("=== MainWindow.cpp Refactoring - Phase 1 ===\n")
    print("Splitting setupMenuBar() into smaller methods...\n")
    
    if split_setup_menubar():
        if update_header():
            print("\n✓ Refactoring completed successfully!")
            print("\nNext steps:")
            print("1. Build the project: cmake --build --preset=conan-debug")
            print("2. Test all menu functionality")
            print("3. Commit changes")
        else:
            print("\n✗ Failed to update header file")
    else:
        print("\n✗ Failed to refactor source file")
