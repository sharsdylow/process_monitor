// process_monitor.hpp
#pragma once
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <unordered_map>

#define RATE 100

// In process_monitor.hpp, add:
struct CommandOutput {
    std::string command;
    std::string output;
};

struct ProcessInfo {
    int pid;
    std::string name;
    std::string user;
    double cpu_usage;
    size_t memory_usage;  // in KB
    std::string status;
    unsigned long utime;  // User time
    unsigned long stime;  // System time
    unsigned long starttime;  // Start time
};

class ProcessMonitor {
public:
    ProcessMonitor();
    ~ProcessMonitor();
    
    void start();
    void stop();
    void setCurrentCommand(const std::string& command, const std::string& output);
    
    bool terminateProcess(int pid);
    bool suspendProcess(int pid);
    bool resumeProcess(int pid);
    
    std::vector<ProcessInfo> getProcessList() const;
    std::string current_command;
    std::string current_output;
    size_t cursor_pos{0};

private:
    void collectData();
    void updateUI();
    
    // Helper methods for data collection
    ProcessInfo readProcessInfo(int pid);
    std::string readProcessName(int pid);
    std::string readProcessStatus(int pid);
    std::string readProcessUser(int pid);
    size_t readProcessMemory(int pid);
    double calculateCPUUsage(const ProcessInfo& current, const ProcessInfo& previous);
    
    std::vector<ProcessInfo> processes_;
    std::unordered_map<int, ProcessInfo> previous_processes_;
    mutable std::mutex processes_mutex_;
    std::atomic<bool> running_{false};
    
    unsigned long total_time_prev_{0};
    unsigned long total_time_current_{0};
    
    std::thread data_thread_;
    std::thread ui_thread_;
    std::condition_variable cv_;
};