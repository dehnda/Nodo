/**
 * NodeFluxStudio MVP - Main Application Entry Point
 * Professional procedural modeling desktop application
 */

#include "studio_app.hpp"
#include <iostream>

int main() {
    try {
        NodeFluxStudio::StudioApp app;
        
        if (!app.initialize()) {
            std::cerr << "Failed to initialize NodeFluxStudio\n";
            return -1;
        }
        
        std::cout << "🎯 NodeFluxStudio MVP Starting...\n";
        app.run();
        
        app.shutdown();
        std::cout << "👋 NodeFluxStudio MVP Shutdown Complete\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "NodeFluxStudio Error: " << e.what() << "\n";
        return -1;
    }
}
