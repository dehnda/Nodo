#include "SceneFileManager.h"
#include "NodeGraphWidget.h"

#include <nodo/graph/graph_serializer.hpp>
#include <nodo/graph/node_graph.hpp>
#include <nodo/graph/execution_engine.hpp>
#include <nodo/io/obj_exporter.hpp>

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>

SceneFileManager::SceneFileManager(QWidget* parent)
    : parent_(parent)
    , node_graph_(nullptr)
    , execution_engine_(nullptr)
    , node_graph_widget_(nullptr)
    , is_modified_(false) {
}

void SceneFileManager::newScene() {
    // This method signals MainWindow to handle the full new scene creation
    // We only manage file tracking here
    current_file_path_.clear();
    is_modified_ = false;
}

void SceneFileManager::openScene() {
    using nodo::graph::GraphSerializer;

    QString file_path = QFileDialog::getOpenFileName(
        parent_, "Open Node Graph", "", "NodeFlux Graph (*.nfg);;All Files (*)");

    if (file_path.isEmpty()) {
        return; // User cancelled
    }

    auto loaded_graph = GraphSerializer::load_from_file(file_path.toStdString());

    if (loaded_graph.has_value()) {
        // Store the loaded graph temporarily
        // MainWindow will handle the actual replacement
        current_file_path_ = file_path;
        is_modified_ = false;
        
        // Add to recent files
        addToRecentFiles(file_path);
    } else {
        QMessageBox::warning(parent_, "Load Failed",
                           "Failed to load node graph from file.");
    }
}

void SceneFileManager::saveScene() {
    if (!node_graph_) return;
    
    // If we have a current file, save to it directly
    if (!current_file_path_.isEmpty()) {
        using nodo::graph::GraphSerializer;
        bool success = GraphSerializer::save_to_file(
            *node_graph_, current_file_path_.toStdString());
        if (success) {
            is_modified_ = false;
            addToRecentFiles(current_file_path_);
        } else {
            QMessageBox::warning(parent_, "Save Failed",
                               "Failed to save node graph to file.");
        }
    } else {
        // No current file, prompt for Save As
        saveSceneAs();
    }
}

void SceneFileManager::saveSceneAs() {
    if (!node_graph_) return;
    
    using nodo::graph::GraphSerializer;

    QString file_path = QFileDialog::getSaveFileName(
        parent_, "Save Node Graph As", "", "NodeFlux Graph (*.nfg);;All Files (*)");

    if (file_path.isEmpty()) {
        return; // User cancelled
    }

    // Add .nfg extension if not present
    if (!file_path.endsWith(".nfg", Qt::CaseInsensitive)) {
        file_path += ".nfg";
    }

    bool success =
        GraphSerializer::save_to_file(*node_graph_, file_path.toStdString());

    if (success) {
        current_file_path_ = file_path;
        is_modified_ = false;
        addToRecentFiles(file_path);
    } else {
        QMessageBox::warning(parent_, "Save Failed",
                           "Failed to save node graph to file.");
    }
}

void SceneFileManager::revertToSaved() {
    if (current_file_path_.isEmpty()) {
        return;
    }

    auto reply = QMessageBox::question(
        parent_, "Revert to Saved", "Discard all changes and reload from disk?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        using nodo::graph::GraphSerializer;
        auto loaded_graph =
            GraphSerializer::load_from_file(current_file_path_.toStdString());

        if (loaded_graph.has_value()) {
            is_modified_ = false;
            
            // Show message that the graph needs to be reloaded
            QMessageBox::information(parent_, "Revert Complete",
                                   "Graph reverted. Please reload the file using "
                                   "File → Open for full functionality.\n"
                                   "Full revert support coming in v1.1.");
        } else {
            QMessageBox::warning(parent_, "Revert Failed",
                               "Failed to reload graph from file.");
        }
    }
}

void SceneFileManager::importGeometry() {
    // TODO: Implement geometry import
    QMessageBox::information(parent_, "Import Geometry",
                           "Geometry import coming in v1.1!\n\n"
                           "For now, use the File node in the node graph.");
}

void SceneFileManager::importGraph() {
    // TODO: Implement graph merging
    QMessageBox::information(parent_, "Import Graph",
                           "Graph import/merge coming in v1.1!\n\n"
                           "For now, use File → Open to load a graph.");
}

void SceneFileManager::exportGeometry() {
    if (!node_graph_ || !execution_engine_) return;
    
    using nodo::io::ObjExporter;

    // Get the display node
    int display_node_id = node_graph_->get_display_node();

    if (display_node_id < 0) {
        QMessageBox::information(
            parent_, "No Mesh to Export",
            "Please set a display flag on a node first.\n\n"
            "Right-click a node in the graph and select 'Set Display' to mark it "
            "for export.");
        return;
    }

    // Get the geometry result for the display node
    auto geometry = execution_engine_->get_node_geometry(display_node_id);

    if (!geometry) {
        QMessageBox::warning(parent_, "Export Failed",
                           "The display node has no geometry output.\n"
                           "Please execute the graph first.");
        return;
    }

    // Check if geometry has points
    if (geometry->point_count() == 0) {
        QMessageBox::warning(parent_, "Export Failed",
                           "The display node's geometry is empty.\n"
                           "Cannot export geometry with no points.");
        return;
    }

    // Open file dialog for export location
    QString file_path = QFileDialog::getSaveFileName(
        parent_, "Export Mesh", "", "Wavefront OBJ (*.obj);;All Files (*)");

    if (file_path.isEmpty()) {
        return; // User cancelled
    }

    // Add .obj extension if not present
    if (!file_path.endsWith(".obj", Qt::CaseInsensitive)) {
        file_path += ".obj";
    }

    // Export the geometry
    bool success =
        ObjExporter::export_geometry(*geometry, file_path.toStdString());

    if (success) {
        int point_count = static_cast<int>(geometry->point_count());
        int prim_count = static_cast<int>(geometry->primitive_count());
        QString message =
            QString("Geometry exported successfully\n%1 points, %2 primitives")
                .arg(point_count)
                .arg(prim_count);
        QMessageBox::information(parent_, "Export Successful", message);
    } else {
        QMessageBox::critical(parent_, "Export Failed",
                            "Failed to write geometry to file.\n"
                            "Check file permissions and disk space.");
    }
}

void SceneFileManager::exportGraph() {
    // Export graph is the same as Save As
    saveSceneAs();
}

void SceneFileManager::exportSelection() {
    // TODO: Export only the selected node's geometry
    QMessageBox::information(parent_, "Export Selection",
                           "Export selected node coming in v1.1!\n\n"
                           "For now, set the display flag on the node "
                           "and use Export → Current Output.");
}

QStringList SceneFileManager::getRecentFiles() const {
    QSettings settings("Nodo", "NodoStudio");
    return settings.value("recentFiles").toStringList();
}

void SceneFileManager::setRecentFiles(const QStringList& files) {
    QSettings settings("Nodo", "NodoStudio");
    settings.setValue("recentFiles", files);
}

void SceneFileManager::addToRecentFiles(const QString& filename) {
    static constexpr int MaxRecentFiles = 10;
    
    QStringList files = getRecentFiles();
    files.removeAll(filename); // Remove if already exists
    files.prepend(filename);   // Add to front
    while (files.size() > MaxRecentFiles) {
        files.removeLast();
    }
    setRecentFiles(files);
}
