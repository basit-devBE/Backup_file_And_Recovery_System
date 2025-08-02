#pragma once

#include <string>
#include <chrono>
#include <functional>
#include <thread>
#include <atomic>
#include <unordered_map>

/**
 * Handles automatic backup scheduling
 */
class Scheduler {
public:
    enum class ScheduleType {
        ONCE,
        HOURLY,
        DAILY,
        WEEKLY,
        MONTHLY,
        CUSTOM_INTERVAL
    };

    struct ScheduleInfo {
        ScheduleType type;
        std::chrono::seconds interval;
        std::chrono::system_clock::time_point nextRun;
        std::string backupName;
        bool enabled;
    };

    Scheduler();
    ~Scheduler();

    // Schedule management
    bool scheduleBackup(const std::string& name, ScheduleType type, 
                       std::chrono::seconds customInterval = std::chrono::seconds(0));
    bool scheduleBackupAt(const std::string& name, const std::chrono::system_clock::time_point& when);
    bool cancelScheduledBackup(const std::string& name);
    bool pauseScheduledBackup(const std::string& name);
    bool resumeScheduledBackup(const std::string& name);
    
    // Scheduler control
    void start();
    void stop();
    bool isRunning() const;
    
    // Callback management
    void setBackupCallback(std::function<bool(const std::string&)> callback);
    void setErrorCallback(std::function<void(const std::string&, const std::string&)> callback);
    
    // Information
    std::vector<ScheduleInfo> getScheduledBackups() const;
    std::chrono::system_clock::time_point getNextScheduledTime() const;
    size_t getActiveSchedulesCount() const;
    
    // Configuration
    void setMaxConcurrentBackups(size_t maxConcurrent);
    void setRetryAttempts(int attempts);
    void setRetryDelay(std::chrono::seconds delay);
    
    // Persistence
    bool saveSchedulesToFile(const std::string& filename);
    bool loadSchedulesFromFile(const std::string& filename);

private:
    std::unordered_map<std::string, ScheduleInfo> schedules_;
    std::atomic<bool> running_;
    std::thread schedulerThread_;
    
    std::function<bool(const std::string&)> backupCallback_;
    std::function<void(const std::string&, const std::string&)> errorCallback_;
    
    size_t maxConcurrentBackups_;
    int retryAttempts_;
    std::chrono::seconds retryDelay_;
    
    // Internal methods
    void schedulerLoop();
    std::chrono::system_clock::time_point calculateNextRun(const ScheduleInfo& schedule);
    void executeScheduledBackup(const std::string& name);
    bool shouldExecuteBackup(const ScheduleInfo& schedule);
    void updateNextRunTime(const std::string& name);
};
