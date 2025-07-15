#include <iostream>
#include <nodeflux/nodes/torus_node.hpp>
#include <nodeflux/io/obj_exporter.hpp>

using namespace nodeflux;

int main() {
    std::cout << "=== NodeFlux Torus Generator Demo ===" << std::endl;
    
    // Create different torus configurations
    std::cout << "\n1. Default Torus:" << std::endl;
    nodes::TorusNode default_torus;
    auto default_mesh = default_torus.generate();
    
    if (default_mesh) {
        std::cout << "   ✓ Generated torus with " << default_mesh->vertices().rows() 
                  << " vertices and " << default_mesh->faces().rows() << " faces" << std::endl;
        
        // Export to OBJ
        io::ObjExporter exporter;
        if (exporter.export_mesh(*default_mesh, "torus_default.obj")) {
            std::cout << "   ✓ Exported to torus_default.obj" << std::endl;
        }
    }
    
    std::cout << "\n2. Large Torus (major=3.0, minor=0.8):" << std::endl;
    nodes::TorusNode large_torus(3.0, 0.8, 36, 18);
    auto large_mesh = large_torus.generate();
    
    if (large_mesh) {
        std::cout << "   ✓ Generated large torus with " << large_mesh->vertices().rows() 
                  << " vertices and " << large_mesh->faces().rows() << " faces" << std::endl;
        
        io::ObjExporter exporter;
        if (exporter.export_mesh(*large_mesh, "torus_large.obj")) {
            std::cout << "   ✓ Exported to torus_large.obj" << std::endl;
        }
    }
    
    std::cout << "\n3. Thin Ring (major=2.0, minor=0.1):" << std::endl;
    nodes::TorusNode thin_ring(2.0, 0.1, 48, 8);
    auto thin_mesh = thin_ring.generate();
    
    if (thin_mesh) {
        std::cout << "   ✓ Generated thin ring with " << thin_mesh->vertices().rows() 
                  << " vertices and " << thin_mesh->faces().rows() << " faces" << std::endl;
        
        io::ObjExporter exporter;
        if (exporter.export_mesh(*thin_mesh, "torus_thin_ring.obj")) {
            std::cout << "   ✓ Exported to torus_thin_ring.obj" << std::endl;
        }
    }
    
    std::cout << "\n4. High Resolution Torus (72 x 24 segments):" << std::endl;
    nodes::TorusNode hires_torus(1.5, 0.4, 72, 24);
    auto hires_mesh = hires_torus.generate();
    
    if (hires_mesh) {
        std::cout << "   ✓ Generated high-res torus with " << hires_mesh->vertices().rows() 
                  << " vertices and " << hires_mesh->faces().rows() << " faces" << std::endl;
        
        io::ObjExporter exporter;
        if (exporter.export_mesh(*hires_mesh, "torus_hires.obj")) {
            std::cout << "   ✓ Exported to torus_hires.obj" << std::endl;
        }
    }
    
    // Demonstrate parameter modification
    std::cout << "\n5. Parameter Modification Demo:" << std::endl;
    nodes::TorusNode configurable_torus;
    
    // Show initial parameters
    std::cout << "   Initial parameters:" << std::endl;
    std::cout << "     Major radius: " << configurable_torus.major_radius() << std::endl;
    std::cout << "     Minor radius: " << configurable_torus.minor_radius() << std::endl;
    std::cout << "     Major segments: " << configurable_torus.major_segments() << std::endl;
    std::cout << "     Minor segments: " << configurable_torus.minor_segments() << std::endl;
    
    // Modify parameters
    configurable_torus.set_major_radius(2.5);
    configurable_torus.set_minor_radius(0.6);
    configurable_torus.set_major_segments(60);
    configurable_torus.set_minor_segments(20);
    
    std::cout << "   Modified parameters:" << std::endl;
    std::cout << "     Major radius: " << configurable_torus.major_radius() << std::endl;
    std::cout << "     Minor radius: " << configurable_torus.minor_radius() << std::endl;
    std::cout << "     Major segments: " << configurable_torus.major_segments() << std::endl;
    std::cout << "     Minor segments: " << configurable_torus.minor_segments() << std::endl;
    
    auto modified_mesh = configurable_torus.generate();
    if (modified_mesh) {
        std::cout << "   ✓ Generated modified torus with " << modified_mesh->vertices().rows() 
                  << " vertices and " << modified_mesh->faces().rows() << " faces" << std::endl;
        
        io::ObjExporter exporter;
        if (exporter.export_mesh(*modified_mesh, "torus_modified.obj")) {
            std::cout << "   ✓ Exported to torus_modified.obj" << std::endl;
        }
    }
    
    std::cout << "\n=== Demo Complete ===" << std::endl;
    std::cout << "Generated torus meshes saved as OBJ files for visualization." << std::endl;
    
    return 0;
}
