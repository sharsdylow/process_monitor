// main.cpp
#include "process_monitor.hpp"
#include <iostream>
#include <thread>
#include <sstream>
#include <sys/ioctl.h>  // For ioctl and winsize
#include <unistd.h>
#include <termios.h>

// In main.cpp:
int main() {
    ProcessMonitor monitor;
    monitor.start();
    
    // Setup terminal for raw input
    struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
    
    std::string current_line;
    char ch;

    while (true) {
        if(read(STDIN_FILENO, &ch, 1) > 0){
            if (ch == 127 || ch == '\b') {  // Backspace
                if (!current_line.empty()) {
                    current_line.pop_back();
                }
            }
            else if (ch == '\n') {
                if (!current_line.empty()) {
                    std::stringstream output;
                    std::istringstream iss(current_line);
                    std::string command;
                    iss >> command;
                    
                    if (command == "quit") {
                        break;
                    } else if (command == "help") {
                        output  << "Available commands:\n"
                                << "  help           - Show this help message\n"
                                << "  quit           - Exit the program\n"
                                << "  kill <pid>     - Terminate a process\n"
                                << "  suspend <pid>  - Suspend a process\n"
                                << "  resume <pid>   - Resume a suspended process\n";
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
                    monitor.current_output = output.str();
                    current_line = "";
                }
            }
            else{
                current_line += ch;
            }
            monitor.current_command = current_line;
        }
    }
    
    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    monitor.stop();
    return 0;
}