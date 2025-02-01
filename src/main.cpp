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

int main() {
    ProcessMonitor monitor;
    monitor.start();
    
    // Move cursor to the last line
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    std::cout << "\033[" << w.ws_row << ";1H";
    
    std::cout << "Command > " << std::flush;
    
    std::string line;
    while (std::getline(std::cin, line)) {
        // Process commands
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
        } else if (!command.empty()) {
            std::cout << "Unknown command. Type 'help' for available commands.\n";
        }
        
        // Show prompt again
        std::cout << "Command > " << std::flush;
    }
    
    monitor.stop();
    return 0;
}