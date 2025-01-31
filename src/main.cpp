// main.cpp
#include "process_monitor.hpp"
#include <iostream>
#include <thread>
#include <sstream>

void printHelp() {
    std::cout << "Available commands:\n"
              << "  help           - Show this help message\n"
              << "  quit           - Exit the program\n"
              << "  kill <pid>     - Terminate a process\n"
              << "  suspend <pid>  - Suspend a process\n"
              << "  resume <pid>   - Resume a suspended process\n";
}

int main() {
    ProcessMonitor monitor;
    monitor.start();
    
    std::cout << "Process Monitor started. Type 'help' for commands.\n";
    
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "quit") {
            break;
        } else if (command == "help") {
            printHelp();
        } else if (command == "kill" || command == "suspend" || command == "resume") {
            int pid;
            if (iss >> pid) {
                bool success = false;
                if (command == "kill") {
                    success = monitor.terminateProcess(pid);
                } else if (command == "suspend") {
                    success = monitor.suspendProcess(pid);
                } else if (command == "resume") {
                    success = monitor.resumeProcess(pid);
                }
                std::cout << (success ? "Success" : "Failed") << "\n";
            } else {
                std::cout << "Invalid PID\n";
            }
        } else {
            std::cout << "Unknown command. Type 'help' for available commands.\n";
        }
    }
    
    monitor.stop();
    return 0;
}