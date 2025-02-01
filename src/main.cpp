// main.cpp
#include "process_monitor.hpp"
#include <iostream>
#include <thread>
#include <sstream>
#include <sys/ioctl.h>  // For ioctl and winsize
#include <unistd.h>

void printHelp() {
    std::cout << "Available commands:\n"
              << "  help           - Show this help message\n"
              << "  quit           - Exit the program\n"
              << "  kill <pid>     - Terminate a process\n"
              << "  suspend <pid>  - Suspend a process\n"
              << "  resume <pid>   - Resume a suspended process\n";
}

// In main.cpp:
int main() {
    ProcessMonitor monitor;
    monitor.start();
    
    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty()) {
            std::stringstream output;
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
                    output << (success ? "Success" : "Failed");
                } else {
                    output << "Invalid PID";
                }
            } else {
                output << "Unknown command. Type 'help' for available commands.";
            }
            
            monitor.addCommandHistory(line, output.str());
        }
    }
    
    monitor.stop();
    return 0;
}