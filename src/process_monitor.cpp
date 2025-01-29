#include "process_monitor.hpp"
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <signal.h>

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

void ProcessMonitor::collectData() {
    while (running_) {
        std::vector<ProcessInfo> new_processes;
        DIR* proc_dir = opendir("/proc");
        if (proc_dir) {
            struct dirent* entry;
            while ((entry = readdir(proc_dir)) != nullptr) {
                // Check if entry is a process (numeric directory)
                if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
                    ProcessInfo info;
                    info.pid = std::stoi(entry->d_name);
                    // Read process info from /proc/[pid]/...
                    // TODO: Implement full process info reading
                    new_processes.push_back(info);
                }
            }
            closedir(proc_dir);
        }
        
        {
            std::lock_guard<std::mutex> lock(processes_mutex_);
            processes_ = std::move(new_processes);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10Hz refresh rate
    }
}

void ProcessMonitor::updateUI() {
    while (running_) {
        // Get current process list
        std::vector<ProcessInfo> current_processes;
        {
            std::lock_guard<std::mutex> lock(processes_mutex_);
            current_processes = processes_;
        }
        
        // Update UI (basic console output for now)
        system("clear");
        std::cout << "Process Monitor\n";
        std::cout << "PID\tNAME\tCPU%\tMEM\tSTATUS\n";
        for (const auto& proc : current_processes) {
            std::cout << proc.pid << "\t" 
                      << proc.name << "\t"
                      << proc.cpu_usage << "\t"
                      << proc.memory_usage << "\t"
                      << proc.status << "\n";
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
