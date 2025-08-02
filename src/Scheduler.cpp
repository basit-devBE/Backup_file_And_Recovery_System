#include "Scheduler.h"
#include "Utils.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

Scheduler::Scheduler() 
    : running_(false)
    , maxConcurrentBackups_(1)
    , retryAttempts_(3)
    , retryDelay_(std::chrono::seconds(60)) {
}

Scheduler::~Scheduler() {
    stop();
}

bool Scheduler::scheduleBackup(const std::string& name, ScheduleType type, std::chrono::seconds customInterval) {
    ScheduleInfo schedule;
    schedule.type = type;
    schedule.backupName = name;
    schedule.enabled = true;
    
    // Calculate interval based on type
    switch (type) {
        case ScheduleType::ONCE:
            schedule.interval = std::chrono::seconds(0);
            schedule.nextRun = std::chrono::system_clock::now();
            break;
        case ScheduleType::HOURLY:
            schedule.interval = std::chrono::hours(1);
            break;
        case ScheduleType::DAILY:
            schedule.interval = std::chrono::hours(24);
            break;
        case ScheduleType::WEEKLY:
            schedule.interval = std::chrono::hours(24 * 7);
            break;
        case ScheduleType::MONTHLY:
            schedule.interval = std::chrono::hours(24 * 30); // Approximate
            break;
        case ScheduleType::CUSTOM_INTERVAL:
            schedule.interval = customInterval;
            break;
    }
    
    // Calculate next run time
    if (type != ScheduleType::ONCE) {
        schedule.nextRun = std::chrono::system_clock::now() + schedule.interval;
    }
    
    schedules_[name] = schedule;
    
    std::cout << "Scheduled backup '" << name << "' with interval " 
              << Utils::formatDuration(schedule.interval) << std::endl;
    
    return true;
}

bool Scheduler::scheduleBackupAt(const std::string& name, const std::chrono::system_clock::time_point& when) {
    ScheduleInfo schedule;
    schedule.type = ScheduleType::ONCE;
    schedule.interval = std::chrono::seconds(0);
    schedule.nextRun = when;
    schedule.backupName = name;
    schedule.enabled = true;
    
    schedules_[name] = schedule;
    
    std::cout << "Scheduled backup '" << name << "' at " 
              << Utils::formatTimestamp(when) << std::endl;
    
    return true;
}

bool Scheduler::cancelScheduledBackup(const std::string& name) {
    auto it = schedules_.find(name);
    if (it != schedules_.end()) {
        schedules_.erase(it);
        std::cout << "Cancelled scheduled backup: " << name << std::endl;
        return true;
    }
    return false;
}

bool Scheduler::pauseScheduledBackup(const std::string& name) {
    auto it = schedules_.find(name);
    if (it != schedules_.end()) {
        it->second.enabled = false;
        std::cout << "Paused scheduled backup: " << name << std::endl;
        return true;
    }
    return false;
}

bool Scheduler::resumeScheduledBackup(const std::string& name) {
    auto it = schedules_.find(name);
    if (it != schedules_.end()) {
        it->second.enabled = true;
        std::cout << "Resumed scheduled backup: " << name << std::endl;
        return true;
    }
    return false;
}

void Scheduler::start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    schedulerThread_ = std::thread(&Scheduler::schedulerLoop, this);
    
    std::cout << "Backup scheduler started" << std::endl;
}

void Scheduler::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (schedulerThread_.joinable()) {
        schedulerThread_.join();
    }
    
    std::cout << "Backup scheduler stopped" << std::endl;
}

bool Scheduler::isRunning() const {
    return running_;
}

void Scheduler::setBackupCallback(std::function<bool(const std::string&)> callback) {
    backupCallback_ = callback;
}

void Scheduler::setErrorCallback(std::function<void(const std::string&, const std::string&)> callback) {
    errorCallback_ = callback;
}

std::vector<Scheduler::ScheduleInfo> Scheduler::getScheduledBackups() const {
    std::vector<ScheduleInfo> schedules;
    
    for (const auto& pair : schedules_) {
        schedules.push_back(pair.second);
    }
    
    return schedules;
}

std::chrono::system_clock::time_point Scheduler::getNextScheduledTime() const {
    auto earliest = std::chrono::system_clock::time_point::max();
    
    for (const auto& pair : schedules_) {
        const ScheduleInfo& schedule = pair.second;
        if (schedule.enabled && schedule.nextRun < earliest) {
            earliest = schedule.nextRun;
        }
    }
    
    return earliest;
}

size_t Scheduler::getActiveSchedulesCount() const {
    return std::count_if(schedules_.begin(), schedules_.end(),
                        [](const auto& pair) { return pair.second.enabled; });
}

void Scheduler::setMaxConcurrentBackups(size_t maxConcurrent) {
    maxConcurrentBackups_ = maxConcurrent;
}

void Scheduler::setRetryAttempts(int attempts) {
    retryAttempts_ = attempts;
}

void Scheduler::setRetryDelay(std::chrono::seconds delay) {
    retryDelay_ = delay;
}

bool Scheduler::saveSchedulesToFile(const std::string& filename) {
    try {
        json j;
        j["version"] = "1.0";
        j["schedules"] = json::array();
        
        for (const auto& pair : schedules_) {
            const std::string& name = pair.first;
            const ScheduleInfo& schedule = pair.second;
            
            json scheduleJson;
            scheduleJson["name"] = name;
            scheduleJson["type"] = static_cast<int>(schedule.type);
            scheduleJson["interval"] = schedule.interval.count();
            scheduleJson["nextRun"] = Utils::formatTimestamp(schedule.nextRun);
            scheduleJson["backupName"] = schedule.backupName;
            scheduleJson["enabled"] = schedule.enabled;
            
            j["schedules"].push_back(scheduleJson);
        }
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        file << j.dump(2);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving schedules: " << e.what() << std::endl;
        return false;
    }
}

bool Scheduler::loadSchedulesFromFile(const std::string& filename) {
    try {
        if (!Utils::pathExists(filename)) {
            return true; // No schedules file is OK
        }
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        json j;
        file >> j;
        
        schedules_.clear();
        
        for (const auto& scheduleJson : j["schedules"]) {
            std::string name = scheduleJson["name"];
            
            ScheduleInfo schedule;
            schedule.type = static_cast<ScheduleType>(scheduleJson["type"]);
            schedule.interval = std::chrono::seconds(scheduleJson["interval"]);
            schedule.nextRun = Utils::parseTimestamp(scheduleJson["nextRun"]);
            schedule.backupName = scheduleJson["backupName"];
            schedule.enabled = scheduleJson["enabled"];
            
            schedules_[name] = schedule;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading schedules: " << e.what() << std::endl;
        return false;
    }
}

void Scheduler::schedulerLoop() {
    while (running_) {
        try {
            // Check for backups that need to run
            for (auto& pair : schedules_) {
                const std::string& name = pair.first;
                ScheduleInfo& schedule = pair.second;
                
                if (shouldExecuteBackup(schedule)) {
                    executeScheduledBackup(name);
                    updateNextRunTime(name);
                }
            }
            
            // Sleep for a short interval before checking again
            std::this_thread::sleep_for(std::chrono::seconds(10));
            
        } catch (const std::exception& e) {
            std::cerr << "Error in scheduler loop: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
    }
}

std::chrono::system_clock::time_point Scheduler::calculateNextRun(const ScheduleInfo& schedule) {
    auto now = std::chrono::system_clock::now();
    
    switch (schedule.type) {
        case ScheduleType::ONCE:
            return std::chrono::system_clock::time_point::max(); // Never again
            
        case ScheduleType::HOURLY:
        case ScheduleType::DAILY:
        case ScheduleType::WEEKLY:
        case ScheduleType::MONTHLY:
        case ScheduleType::CUSTOM_INTERVAL:
            return now + schedule.interval;
            
        default:
            return now + std::chrono::hours(24); // Default to daily
    }
}

void Scheduler::executeScheduledBackup(const std::string& name) {
    std::cout << "Executing scheduled backup: " << name << std::endl;
    
    if (!backupCallback_) {
        std::cerr << "Error: No backup callback set" << std::endl;
        return;
    }
    
    // Execute backup with retry logic
    int attempts = 0;
    bool success = false;
    
    while (attempts < retryAttempts_ && !success) {
        attempts++;
        
        try {
            success = backupCallback_(name);
            
            if (success) {
                std::cout << "Scheduled backup completed successfully: " << name << std::endl;
            } else {
                std::cerr << "Scheduled backup failed (attempt " << attempts << "/" 
                         << retryAttempts_ << "): " << name << std::endl;
                
                if (attempts < retryAttempts_) {
                    std::cout << "Retrying in " << Utils::formatDuration(retryDelay_) << "..." << std::endl;
                    std::this_thread::sleep_for(retryDelay_);
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Exception during scheduled backup " << name << ": " << e.what() << std::endl;
            
            if (errorCallback_) {
                errorCallback_(name, e.what());
            }
            
            if (attempts < retryAttempts_) {
                std::this_thread::sleep_for(retryDelay_);
            }
        }
    }
    
    if (!success) {
        std::cerr << "Scheduled backup failed after " << retryAttempts_ << " attempts: " << name << std::endl;
        
        if (errorCallback_) {
            errorCallback_(name, "Backup failed after all retry attempts");
        }
    }
}

bool Scheduler::shouldExecuteBackup(const ScheduleInfo& schedule) {
    if (!schedule.enabled) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    return now >= schedule.nextRun;
}

void Scheduler::updateNextRunTime(const std::string& name) {
    auto it = schedules_.find(name);
    if (it != schedules_.end()) {
        it->second.nextRun = calculateNextRun(it->second);
        
        // For one-time schedules, disable them after execution
        if (it->second.type == ScheduleType::ONCE) {
            it->second.enabled = false;
        }
    }
}
