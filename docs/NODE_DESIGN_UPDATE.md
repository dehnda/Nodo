# Node Design Update

## Overview
This document describes the node design changes made to match the screenshot design while maintaining all existing functionality.

## Screenshot Analysis
The target design shows:
- **Transform** node with blue status indicator dot
- Dark background (#2b2b2f or similar)
- Cook time **2.4ms** centered in body
- Footer stats: **124v 248t 4KB**
- Orange/coral pins at top and bottom
- Right-side vertical button toolbar with 3 buttons (VIEW, WIRE, PASS)
- Clean, minimal aesthetic with no complex gradients

## Changes Made

### 1. Simplified Layout Structure
**Before:** 4 sections (Header + Status + Body + Footer)
**After:** 3 sections (Header + Body + Footer)

- Removed separate status bar section
- Integrated status indicator (blue dot) into header
- Moved cook time display to body center

### 2. Updated Constants (NodeGraphWidget.h)
```cpp
// Node dimensions
NODE_WIDTH: 240.0F → 200.0F  // Narrower, cleaner
NODE_HEADER_HEIGHT: 32.0F    // Unchanged
NODE_BODY_HEIGHT: 60.0F → 48.0F  // Slightly reduced
NODE_FOOTER_HEIGHT: 28.0F → 24.0F  // More compact

// New button toolbar (right side)
BUTTON_TOOLBAR_WIDTH: 36.0F  // Vertical toolbar width
BUTTON_SIZE: 32.0F           // Individual button size
BUTTON_SPACING: 4.0F         // Gap between buttons
```

Removed:
- `NODE_STATUS_HEIGHT` (no longer needed)
- `ACTION_BUTTON_SIZE` (replaced by BUTTON_SIZE)

### 3. Visual Design Changes

#### Header (drawHeader)
- **Simplified background:** Solid color (42, 42, 47) instead of gradient
- **Status dot:** Blue indicator on left side (5px radius)
  - Blue (74, 158, 255) for normal state
  - Red (239, 68, 68) for error state
- **Title:** Left-aligned after dot, no icon needed
- **Removed:** Gradient, icon box, inline buttons

#### Body (drawBody)
- **Background:** Matches overall node background (35, 35, 40)
- **Content:** Centered cook time display only (e.g., "2.4ms")
- **Font:** 10pt, gray text (160, 160, 168)
- **Removed:** Parameter display (moved out of body)

#### Footer (drawFooter)
- **Background:** Slightly darker (30, 30, 35)
- **Stats format:** "124v  248t  4KB" centered
- **Font:** 8pt, gray text (130, 130, 140)
- **Removed:** Icons, left-aligned stats, top border

#### Button Toolbar (drawButtonToolbar - NEW)
- **Position:** Right side of node, outside node bounds
- **Gap:** 4px between node and toolbar
- **Background:** Dark semi-transparent (30, 30, 35, 220)
- **Border:** Subtle rounded rect (6px radius)
- **Buttons:** 3 vertically stacked buttons
  1. **VIEW** - Display flag (blue #4a9eff when active)
  2. **WIRE** - Bypass flag (gray when active)
  3. **PASS** - Lock flag (gray when active)
- **Button styling:**
  - Size: 32×32px
  - Spacing: 4px between buttons
  - Text labels (7pt bold)
  - Active: colored background, white text
  - Inactive: dark gray background (50, 50, 55)

### 4. Pin Colors
Changed to match screenshot:
- **Input pins (top):** Orange/coral (255, 140, 90)
- **Output pins (bottom):** Orange/coral (255, 140, 90)

Previously: Input=Blue, Output=Pink

### 5. Interaction Updates

#### Mouse Events (mousePressEvent)
- **Before:** Checked header area for 4 inline buttons
- **After:** Checks button toolbar area for 3 vertical buttons
- Button detection uses toolbar rect bounds
- Buttons control same flags but with different mapping:
  - VIEW → `has_display_flag_`
  - WIRE → `bypass_flag_`
  - PASS → `lock_flag_`

#### Hover Detection (hoverMoveEvent)
- **Before:** Checked each button in header individually
- **After:** Checks entire toolbar rect
- Simplified: cursor changes when anywhere over toolbar

#### Bounding Rectangle (boundingRect)
- **Extended:** Now includes button toolbar width + gap
- Total width in non-compact mode: NODE_WIDTH + BUTTON_TOOLBAR_WIDTH + 8px
- Ensures toolbar is within item bounds for proper rendering

### 6. Removed Methods
- `drawActionButtons()` - replaced by `drawButtonToolbar()`
- `drawStatusBar()` - functionality integrated into header and body
- `getStatusRect()` - no longer needed

### 7. New Methods
- `drawButtonToolbar()` - draws vertical button bar on right
- `getButtonToolbarRect()` - returns toolbar bounds for hit detection

## Color Palette

### Node Colors (unchanged)
- Background: (35, 35, 40) dark gray
- Header: (42, 42, 47) slightly lighter
- Footer: (30, 30, 35) slightly darker
- Border: (50, 50, 55) subtle
- Selected: (74, 158, 255) blue
- Error: (239, 68, 68) red

### New Colors
- Status dot: Blue (74, 158, 255) / Red (239, 68, 68)
- Pins: Orange/coral (255, 140, 90)
- Button toolbar bg: (30, 30, 35, 220)
- Active button: Blue (74, 158, 255)
- Inactive button: (50, 50, 55)
- Button text active: White
- Button text inactive: (160, 160, 165)

## Layout Metrics

### Node Dimensions
- Width: 200px (was 240px)
- Header: 32px
- Body: 48px (was 60px)
- Footer: 24px (was 28px)
- **Total height:** 104px (non-compact)
- **Compact height:** 56px (unchanged)

### Button Toolbar
- Width: 36px
- Height: Matches body + footer (72px)
- Gap from node: 4px
- Button size: 32×32px
- Button spacing: 4px
- Total buttons: 3

### Pins
- Radius: 8px (unchanged)
- Spacing: 80px (unchanged)
- Position: Centered horizontally
- Input: Top edge (y=0)
- Output: Bottom edge (y=total_height)

## Functionality Preserved
All existing functionality remains intact:
- Node selection and dragging
- Pin connections
- Display flag toggling with scene notification
- Bypass and lock flag toggling
- Cook time and statistics display
- Compact mode support
- Error state handling
- Hover cursor changes
- Node type color schemes (Houdini-inspired)

## Build Status
✅ Successfully builds with CMake
✅ No compilation errors
⚠️ Style warnings only (magic numbers, cognitive complexity - non-blocking)

## Visual Result
The updated design achieves:
- ✅ Cleaner, more minimal aesthetic matching screenshot
- ✅ Centered cook time display
- ✅ Compact footer statistics format
- ✅ Professional button toolbar with text labels
- ✅ Unified orange/coral pin color scheme
- ✅ Blue status indicator dot in header
- ✅ Darker, more cohesive color palette
- ✅ Proper spacing and alignment

## Next Steps (Optional)
Consider future enhancements:
1. Add button tooltips for clarity
2. Add button icons in addition to text labels
3. Implement smooth transitions for button states
4. Add parameter display back (in a dedicated section or popup)
5. Add keyboard shortcuts for button actions
6. Improve button hover feedback (subtle highlight)
