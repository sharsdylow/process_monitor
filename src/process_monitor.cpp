// process_monitor.cpp
#include "process_monitor.hpp"
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <signal.h>
#include <sstream>
#include <iomanip>
#include <sys/ioctl.h>  // For ioctl and winsize
#include <unistd.h>

ProcessMonitor::ProcessMonitor() = default;

ProcessMonitor::~ProcessMonitor() {
    stop();
}

void ProcessMonitor::start() {
    if (!running_) {
        running_ = true;
        data_thread_ = std::thread(&ProcessMonitor::collectData, this);
        ui_thread_ = std::thread(&ProcessMonitor::updateUI, this);
    }
}

void ProcessMonitor::stop() {
    if (running_) {
        running_ = false;
        cv_.notify_all();
        if (data_thread_.joinable()) data_thread_.join();
        if (ui_thread_.joinable()) ui_thread_.join();
    }
}

ProcessInfo ProcessMonitor::readProcessInfo(int pid) {
    ProcessInfo info;
    info.pid = pid;
    info.name = readProcessName(pid);
    info.status = readProcessStatus(pid);
    info.memory_usage = readProcessMemory(pid);
    
    // Read CPU stats
    std::ifstream stat_file("/proc/" + std::to_string(pid) + "/stat");
    if (stat_file) {
        std::string line;
        std::getline(stat_file, line);
        std::istringstream iss(line);
        
        std::string unused;
        for (int i = 0; i < 13; ++i) iss >> unused;
        
        iss >> info.utime >> info.stime;
        for (int i = 0; i < 4; ++i) iss >> unused;
        iss >> info.starttime;
    }
    
    return info;
}

std::string ProcessMonitor::readProcessName(int pid) {
    std::ifstream comm_file("/proc/" + std::to_string(pid) + "/comm");
    std::string name;
    if (comm_file) {
        std::getline(comm_file, name);
    }
    return name;
}

std::string ProcessMonitor::readProcessStatus(int pid) {
    std::ifstream status_file("/proc/" + std::to_string(pid) + "/status");
    std::string line;
    while (std::getline(status_file, line)) {
        if (line.substr(0, 6) == "State:") {
            return line.substr(7);
        }
    }
    return "unknown";
}

size_t ProcessMonitor::readProcessMemory(int pid) {
    std::ifstream status_file("/proc/" + std::to_string(pid) + "/status");
    std::string line;
    while (std::getline(status_file, line)) {
        std::string name = line.substr(0, 6);
        if (name == "VmRSS:") {
            size_t kb;
            std::istringstream iss(line.substr(7));
            iss >> kb;
            return kb;
        }
    }
    return 0;
}

double ProcessMonitor::calculateCPUUsage(const ProcessInfo& current, const ProcessInfo& previous) {
    unsigned long total_time = total_time_current_ - total_time_prev_;
    if (total_time == 0) return 0.0;
    
    unsigned long process_time = (current.utime + current.stime) - 
                               (previous.utime + previous.stime);
    
    return (process_time * 100.0) / total_time;
}

void ProcessMonitor::collectData() {
    while (running_) {
        std::vector<ProcessInfo> new_processes;
        
        // Read system CPU time
        std::ifstream stat_file("/proc/stat");
        if (stat_file) {
            std::string line;
            std::getline(stat_file, line);
            std::istringstream iss(line);
            std::string cpu;
            unsigned long user, nice, system, idle, iowait, irq, softirq;
            iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
            
            total_time_prev_ = total_time_current_;
            total_time_current_ = user + nice + system + idle + iowait + irq + softirq;
        }
        
        DIR* proc_dir = opendir("/proc");
        if (proc_dir) {
            struct dirent* entry;
            while ((entry = readdir(proc_dir)) != nullptr) {
                if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
                    int pid = std::stoi(entry->d_name);
                    ProcessInfo info = readProcessInfo(pid);
                    
                    // Calculate CPU usage if we have previous data
                    auto it = previous_processes_.find(pid);
                    if (it != previous_processes_.end()) {
                        info.cpu_usage = calculateCPUUsage(info, it->second);
                    } else {
                        info.cpu_usage = 0.0;
                    }
                    
                    new_processes.push_back(info);
                }
            }
            closedir(proc_dir);
        }
        
        {
            std::lock_guard<std::mutex> lock(processes_mutex_);
            processes_ = std::move(new_processes);
            
            // Update previous processes map
            previous_processes_.clear();
            for (const auto& proc : processes_) {
                previous_processes_[proc.pid] = proc;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(RATE));
    }
}

void ProcessMonitor::updateUI() {
    bool first_run = true;
    int last_displayed_lines = 0;
    const int process_display_limit = 10;  // Limit process list height

    while (running_) {
        std::vector<ProcessInfo> current_processes;
        std::vector<CommandOutput> current_history;
        {
            std::lock_guard<std::mutex> lock(processes_mutex_);
            current_processes = processes_;
        }
        
        // If first run, clear screen
        if (first_run) {
            std::cout << "\033[2J\033[H";
            first_run = false;
        } else {
            // Move cursor to top
            std::cout << "\033[H";
        }
        
        // Display process list (top section)
        std::cout << "Process Monitor (Type 'help' for commands)\n";
        std::cout << std::setw(8) << "PID" 
                  << std::setw(20) << "NAME"
                  << std::setw(10) << "CPU%"
                  << std::setw(12) << "MEM(KB)"
                  << std::setw(20) << "STATUS\n";
        std::cout << std::string(70, '-') << "\n";
        
        // Display limited number of processes
        for (size_t i = 0; i < std::min(current_processes.size(), 
                                      static_cast<size_t>(process_display_limit)); ++i) {
            const auto& proc = current_processes[i];
            std::cout << "\033[K";  // Clear line
            std::cout << std::setw(8) << proc.pid
                      << std::setw(20) << proc.name
                      << std::setw(10) << std::fixed << std::setprecision(1) << proc.cpu_usage
                      << std::setw(12) << proc.memory_usage
                      << std::setw(20) << proc.status << "\n";
        }
        
        // Separator between process list and command history
        std::cout << std::string(70, '-') << "\n";
        
        // Display current command and output
        if (!current_command.empty()) {
            std::cout << "\033[K";  // Clear line
            std::cout << "Command > " << current_command << "\n";
            if (!current_output.empty()) {
                std::cout << current_output << "\n";
            }
        }
        
        // Display current prompt
        std::cout << "\033[K";  // Clear line
        std::cout << "Command > " << std::flush;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(RATE));
    }
}

bool ProcessMonitor::terminateProcess(int pid) {
    return kill(pid, SIGTERM) == 0;
}

bool ProcessMonitor::suspendProcess(int pid) {
    return kill(pid, SIGSTOP) == 0;
}

bool ProcessMonitor::resumeProcess(int pid) {
    return kill(pid, SIGCONT) == 0;
}

std::vector<ProcessInfo> ProcessMonitor::getProcessList() const {
    std::lock_guard<std::mutex> lock(processes_mutex_);
    return processes_;
}