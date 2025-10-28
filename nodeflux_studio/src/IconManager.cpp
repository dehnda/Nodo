#include "IconManager.h"
#include <QDebug>
#include <QFile>
#include <QPainter>
#include <QSvgRenderer>

namespace nodeflux_studio {

IconManager &IconManager::instance() {
  static IconManager instance;
  return instance;
}

IconManager::IconManager()
    : default_color_(
          QColor(224, 224, 224)) { // Default light gray for dark theme
}

QIcon IconManager::getIcon(Icon icon, const QColor &color) {
  QColor icon_color = color.isValid() ? color : default_color_;

  // Create icon with multiple sizes for better scaling
  QIcon qicon;
  qicon.addPixmap(getPixmap(icon, 16, icon_color));
  qicon.addPixmap(getPixmap(icon, 24, icon_color));
  qicon.addPixmap(getPixmap(icon, 32, icon_color));
  qicon.addPixmap(getPixmap(icon, 48, icon_color));

  return qicon;
}

QPixmap IconManager::getPixmap(Icon icon, int size, const QColor &color) {
  QColor icon_color = color.isValid() ? color : default_color_;

  // Create cache key
  QString cache_key = QString("%1_%2_%3")
                          .arg(static_cast<int>(icon))
                          .arg(size)
                          .arg(icon_color.name());

  // Check cache first
  if (cache_.contains(cache_key)) {
    return cache_[cache_key];
  }

  // Load and cache the icon
  QString icon_file = getIconFileName(icon);
  QPixmap pixmap = loadSvgIcon(icon_file, icon_color, size);

  // If SVG loading failed, create a fallback using Unicode
  if (pixmap.isNull()) {
    qWarning() << "Failed to load icon:" << icon_file << "- using fallback";
    pixmap = QPixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(icon_color);
    QFont font = painter.font();
    font.setPixelSize(size - 4);
    painter.setFont(font);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, getUnicodeFallback(icon));
  }

  cache_[cache_key] = pixmap;
  return pixmap;
}

QString IconManager::getUnicodeFallback(Icon icon) const {
  // Fallback Unicode characters (same as currently used)
  switch (icon) {
  // File operations
  case Icon::FileNew:
    return "📄";
  case Icon::FileSave:
    return "💾";
  case Icon::FileOpen:
    return "📂";
  case Icon::FileExport:
    return "📤";

  // Edit operations
  case Icon::Undo:
    return "↶";
  case Icon::Redo:
    return "↷";
  case Icon::Delete:
    return "⌫";
  case Icon::Copy:
    return "⎘";

  // View operations
  case Icon::Wireframe:
    return "◫";
  case Icon::Shaded:
    return "●";
  case Icon::ResetCamera:
    return "⟲";
  case Icon::FitView:
    return "⊡";

  // Node generators
  case Icon::Sphere:
    return "●";
  case Icon::Box:
    return "■";
  case Icon::Cylinder:
    return "▮";
  case Icon::Plane:
    return "▬";
  case Icon::Torus:
    return "◯";
  case Icon::Line:
    return "─";

  // Node modifiers
  case Icon::Smooth:
    return "⚙";
  case Icon::Subdivide:
    return "◇";
  case Icon::Resample:
    return "◈";
  case Icon::Extrude:
    return "↑";
  case Icon::PolyExtrude:
    return "⇈";
  case Icon::Normal:
    return "⟂";

  // Node arrays
  case Icon::Array:
    return "⋮";
  case Icon::Scatter:
    return "∴";
  case Icon::CopyToPoints:
    return "⊕";

  // Node boolean & transform
  case Icon::BooleanUnion:
    return "∪";
  case Icon::Transform:
    return "↔";
  case Icon::Mirror:
    return "⇄";
  case Icon::NoiseDisplacement:
    return "≈";

  // Node utilities
  case Icon::Merge:
    return "⊞";
  case Icon::Group:
    return "◉";
  case Icon::Wrangle:
    return "✎";
  case Icon::UVUnwrap:
    return "▦";

  // UI elements
  case Icon::Add:
    return "➕";
  case Icon::Remove:
    return "➖";
  case Icon::Search:
    return "🔍";
  case Icon::Settings:
    return "⚙";
  case Icon::Info:
    return "ℹ";
  case Icon::Warning:
    return "⚠";
  case Icon::Error:
    return "✗";
  case Icon::Success:
    return "✓";

  // Misc
  case Icon::GPU:
    return "⚡";
  case Icon::Play:
    return "▶";
  case Icon::Pause:
    return "⏸";
  case Icon::Stop:
    return "⏹";

  default:
    return "?";
  }
}

QString IconManager::getIconFileName(Icon icon) const {
  // Map to Iconoir icon names
  // See: https://iconoir.com/
  switch (icon) {
  // File operations
  case Icon::FileNew:
    return "page-plus";
  case Icon::FileSave:
    return "floppy-disk";
  case Icon::FileOpen:
    return "folder";
  case Icon::FileExport:
    return "export";

  // Edit operations
  case Icon::Undo:
    return "undo";
  case Icon::Redo:
    return "redo";
  case Icon::Delete:
    return "bin";
  case Icon::Copy:
    return "copy";

  // View operations
  case Icon::Wireframe:
    return "view-grid";
  case Icon::Shaded:
    return "sphere";
  case Icon::ResetCamera:
    return "refresh-circle";
  case Icon::FitView:
    return "frame-simple";

  // Node generators
  case Icon::Sphere:
    return "sphere";
  case Icon::Box:
    return "cube";
  case Icon::Cylinder:
    return "cylinder";
  case Icon::Plane:
    return "square";
  case Icon::Torus:
    return "circle";
  case Icon::Line:
    return "line";

  // Node modifiers
  case Icon::Smooth:
    return "settings";
  case Icon::Subdivide:
    return "grid";
  case Icon::Resample:
    return "refresh-double";
  case Icon::Extrude:
    return "arrow-up";
  case Icon::PolyExtrude:
    return "arrow-up-circle";
  case Icon::Normal:
    return "arrow-separate-vertical";

  // Node arrays
  case Icon::Array:
    return "align-bottom-box";
  case Icon::Scatter:
    return "selection";
  case Icon::CopyToPoints:
    return "copy-plus";

  // Node boolean & transform
  case Icon::BooleanUnion:
    return "union";
  case Icon::Transform:
    return "move";
  case Icon::Mirror:
    return "flip";
  case Icon::NoiseDisplacement:
    return "signal";

  // Node utilities
  case Icon::Merge:
    return "merge";
  case Icon::Group:
    return "multi-bubble";
  case Icon::Wrangle:
    return "code";
  case Icon::UVUnwrap:
    return "grid-remove";

  // UI elements
  case Icon::Add:
    return "plus";
  case Icon::Remove:
    return "minus";
  case Icon::Search:
    return "search";
  case Icon::Settings:
    return "settings";
  case Icon::Info:
    return "info-circle";
  case Icon::Warning:
    return "warning-triangle";
  case Icon::Error:
    return "cancel";
  case Icon::Success:
    return "check";

  // Misc
  case Icon::GPU:
    return "flash";
  case Icon::Play:
    return "play";
  case Icon::Pause:
    return "pause";
  case Icon::Stop:
    return "square";

  default:
    return "circle";
  }
}

QPixmap IconManager::loadSvgIcon(const QString &iconName, const QColor &color,
                                 int size) {
  // Try to load from Qt resources first
  QString resource_path = QString(":/icons/iconoir/%1.svg").arg(iconName);

  // If not in resources, try filesystem path (for development)
  if (!QFile::exists(resource_path)) {
    // Path relative to build directory: external/iconoir/icons/regular/
    resource_path =
        QString("../external/iconoir/icons/regular/%1.svg").arg(iconName);
  }

  if (!QFile::exists(resource_path)) {
    qWarning() << "Icon file not found:" << iconName
               << "- tried resource and filesystem";
    return QPixmap();
  }

  QSvgRenderer renderer(resource_path);
  if (!renderer.isValid()) {
    qWarning() << "Failed to load SVG:" << resource_path;
    return QPixmap();
  }

  // Create pixmap with the icon rendered
  QPixmap pixmap(size, size);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  renderer.render(&painter);
  painter.end();

  // Apply color if specified (recolor the icon)
  if (color.isValid()) {
    QPixmap colored(size, size);
    colored.fill(Qt::transparent);
    QPainter colorPainter(&colored);
    colorPainter.setCompositionMode(QPainter::CompositionMode_Source);
    colorPainter.drawPixmap(0, 0, pixmap);
    colorPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    colorPainter.fillRect(colored.rect(), color);
    colorPainter.end();
    return colored;
  }

  return pixmap;
}

void IconManager::setDefaultColor(const QColor &color) {
  if (default_color_ != color) {
    default_color_ = color;
    clearCache();
  }
}

void IconManager::clearCache() { cache_.clear(); }

} // namespace nodeflux_studio
