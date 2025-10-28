#pragma once

#include <QColor>
#include <QHash>
#include <QIcon>
#include <QPixmap>
#include <QString>

namespace nodo_studio {

/**
 * @brief Centralized icon management using Iconoir icon library
 *
 * This class provides a single point of access for all application icons,
 * using the Iconoir icon library (https://iconoir.com/).
 * Icons are loaded as SVGs and can be colored/styled dynamically.
 */
class IconManager {
public:
  // Icon identifiers - centralized enum for type safety
  enum class Icon {
    // File operations
    FileNew,
    FileOpen,
    FileSave,
    FileExport,

    // Edit operations
    Undo,
    Redo,
    Delete,
    Copy,

    // View operations
    Wireframe,
    Shaded,
    ResetCamera,
    FitView,
    Eye,
    EyeClosed,

    // Node generators
    Sphere,
    Box,
    Cylinder,
    Plane,
    Torus,
    Line,

    // Node modifiers
    Smooth,
    Subdivide,
    Resample,
    Extrude,
    PolyExtrude,
    Normal,

    // Node arrays
    Array,
    Scatter,
    CopyToPoints,

    // Node boolean & transform
    BooleanUnion,
    Transform,
    Mirror,
    NoiseDisplacement,

    // Node utilities
    Merge,
    Group,
    Wrangle,
    UVUnwrap,

    // UI elements
    Add,
    Remove,
    Search,
    Settings,
    Info,
    Warning,
    Error,
    Success,

    // Misc
    GPU,
    Play,
    Pause,
    Stop
  };

  /**
   * @brief Get the singleton instance
   */
  static IconManager &instance();

  /**
   * @brief Get a QIcon for the specified icon type
   * @param icon The icon identifier
   * @param color Optional color override (uses theme default if not specified)
   * @return QIcon that can be used in Qt widgets
   */
  QIcon getIcon(Icon icon, const QColor &color = QColor());

  /**
   * @brief Get a QPixmap for the specified icon type
   * @param icon The icon identifier
   * @param size The desired size in pixels
   * @param color Optional color override
   * @return QPixmap that can be used in Qt widgets
   */
  QPixmap getPixmap(Icon icon, int size = 24, const QColor &color = QColor());

  /**
   * @brief Get the Unicode fallback character for an icon
   * Useful for text-only contexts or if SVG loading fails
   * @param icon The icon identifier
   * @return QString containing a Unicode character
   */
  QString getUnicodeFallback(Icon icon) const;

  /**
   * @brief Set the default icon color for the current theme
   * @param color The color to use for icons
   */
  void setDefaultColor(const QColor &color);

  /**
   * @brief Clear the icon cache (useful when theme changes)
   */
  void clearCache();

private:
  IconManager();
  ~IconManager() = default;
  IconManager(const IconManager &) = delete;
  IconManager &operator=(const IconManager &) = delete;

  /**
   * @brief Load an SVG icon from resources
   * @param iconName The base name of the icon file (without extension)
   * @param color The color to apply to the SVG
   * @param size The size to render
   * @return QPixmap of the rendered icon
   */
  QPixmap loadSvgIcon(const QString &iconName, const QColor &color, int size);

  /**
   * @brief Get the icon filename for an icon enum
   * @param icon The icon identifier
   * @return QString containing the icon filename (from Iconoir)
   */
  QString getIconFileName(Icon icon) const;

  // Cache for loaded icons (key = icon_size_color)
  QHash<QString, QPixmap> cache_;

  // Default color for icons
  QColor default_color_;
};

// Convenience functions for easy icon access
namespace Icons {
inline QIcon get(IconManager::Icon icon, const QColor &color = QColor()) {
  return IconManager::instance().getIcon(icon, color);
}

inline QPixmap getPixmap(IconManager::Icon icon, int size = 24,
                         const QColor &color = QColor()) {
  return IconManager::instance().getPixmap(icon, size, color);
}

inline QString getUnicode(IconManager::Icon icon) {
  return IconManager::instance().getUnicodeFallback(icon);
}
} // namespace Icons

} // namespace nodo_studio
