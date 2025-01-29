#include "process_monitor.hpp"
#include <iostream>
#include <thread>

int main() {
    ProcessMonitor monitor;
    
    // Start monitoring
    monitor.start();
    
    // Simple CLI
    std::string command;
    while (std::getline(std::cin, command)) {
        if (command == "quit") {
            break;
        }
        // Add command handling here
    }
    
    monitor.stop();
    return 0;
}