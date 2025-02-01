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
    
    // Setup terminal for raw input mode
    struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);  // Get current terminal settings
    new_tio = old_tio;                  // Make a copy to modify

    // Modify terminal settings
    new_tio.c_lflag &= (~ICANON & ~ECHO);  // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);  // Apply new settings

    
    std::string current_line;
    size_t cursor_pos = 0;
    char ch;

    while (true) {
        if(read(STDIN_FILENO, &ch, 1) > 0){
            if (ch == 127 || ch == '\b') {  // Backspace
                if (cursor_pos > 0) {
                    current_line.erase(cursor_pos - 1, 1);
                    cursor_pos--;
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
                            output << (success ? "Success " : "Failed ") << command << " " << pid;
                        } else {
                            output << "Invalid PID";
                        }
                    } else {
                        output << "Unknown command. Type 'help' for available commands.";
                    }
                    monitor.current_output = output.str();
                    current_line.clear();
                    cursor_pos = 0;
                }
            }
            else if (ch == 27) {  // Escape sequence
                char seq[3];
                if (read(STDIN_FILENO, &seq[0], 1) > 0) {
                    if (seq[0] == '[') {
                        if (read(STDIN_FILENO, &seq[1], 1) > 0) {
                            switch(seq[1]) {
                                case 'C':  // Right arrow
                                    if (cursor_pos < current_line.length()) {
                                        cursor_pos++;
                                    }
                                    break;
                                case 'D':  // Left arrow
                                    if (cursor_pos > 0) {
                                        cursor_pos--;
                                    }
                                    break;
                            }
                        }
                    }
                }
            }
            else {  // Regular character
                current_line.insert(cursor_pos, 1, ch);
                cursor_pos++;
            }
            monitor.current_command = current_line;
            monitor.cursor_pos = cursor_pos;
        }
    }
    
    // Restore terminal settings
    std::cout << '\n';
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    monitor.stop();
    return 0;
}