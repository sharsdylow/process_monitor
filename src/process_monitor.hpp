#pragma once
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

struct ProcessInfo {
    int pid;
    std::string name;
    double cpu_usage;
    size_t memory_usage;
    std::string status;
};

class ProcessMonitor {
public:
    ProcessMonitor();
    ~ProcessMonitor();
    
    // Start monitoring
    void start();
    // Stop monitoring
    void stop();
    
    // Process control methods
    bool terminateProcess(int pid);
    bool suspendProcess(int pid);
    bool resumeProcess(int pid);
    
    // Get current process list
    std::vector<ProcessInfo> getProcessList() const;

private:
    // Data collection thread function
    void collectData();
    // UI update thread function
    void updateUI();
    
    std::vector<ProcessInfo> processes_;
    mutable std::mutex processes_mutex_;
    std::atomic<bool> running_{false};
    
    std::thread data_thread_;
    std::thread ui_thread_;
    
    std::condition_variable cv_;
};
