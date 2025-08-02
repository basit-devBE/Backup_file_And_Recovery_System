#include "BackupManager.h"
#include "Scheduler.h"
#include "Utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>

void printUsage(const std::string& programName) {
    std::cout << "Backup and Recovery System\n";
    std::cout << "Usage: " << programName << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --backup              Create a full backup\n";
    std::cout << "  --incremental         Create an incremental backup\n";
    std::cout << "  --restore             Restore from backup\n";
    std::cout << "  --verify              Verify backup integrity\n";
    std::cout << "  --schedule            Schedule automatic backups\n";
    std::cout << "  --list                List available backups\n";
    std::cout << "\n";
    std::cout << "Parameters:\n";
    std::cout << "  --source PATH         Source directory to backup\n";
    std::cout << "  --dest PATH           Destination directory for backup\n";
    std::cout << "  --backup-path PATH    Path to backup for restore/verify\n";
    std::cout << "  --restore-path PATH   Path to restore files to\n";
    std::cout << "  --compress            Enable compression (default: enabled)\n";
    std::cout << "  --no-compress         Disable compression\n";
    std::cout << "  --encrypt             Enable encryption\n";
    std::cout << "  --key KEY             Encryption key\n";
    std::cout << "  --level LEVEL         Compression level (1-9, default: 6)\n";
    std::cout << "  --interval SECONDS    Schedule interval in seconds\n";
    std::cout << "  --help                Show this help message\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " --backup --source /home/user/docs --dest /backup\n";
    std::cout << "  " << programName << " --incremental --source /home/user/docs --dest /backup\n";
    std::cout << "  " << programName << " --restore --backup-path /backup/backup_20250801_120000 --restore-path /restore\n";
    std::cout << "  " << programName << " --verify --backup-path /backup/backup_20250801_120000\n";
    std::cout << "  " << programName << " --schedule --source /home/user/docs --dest /backup --interval 3600\n";
}

void progressCallback(const std::string& operation, float percentage) {
    std::cout << "\r" << operation << ": " << std::fixed << std::setprecision(1) 
              << percentage << "%" << std::flush;
    if (percentage >= 100.0f) {
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::vector<std::string> args(argv + 1, argv + argc);
    
    // Parse command line arguments
    std::string operation;
    std::string sourcePath;
    std::string destPath;
    std::string backupPath;
    std::string restorePath;
    std::string encryptionKey;
    bool enableCompression = true;
    bool enableEncryption = false;
    int compressionLevel = 6;
    int scheduleInterval = 0;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (args[i] == "--backup") {
            operation = "backup";
        } else if (args[i] == "--incremental") {
            operation = "incremental";
        } else if (args[i] == "--restore") {
            operation = "restore";
        } else if (args[i] == "--verify") {
            operation = "verify";
        } else if (args[i] == "--schedule") {
            operation = "schedule";
        } else if (args[i] == "--list") {
            operation = "list";
        } else if (args[i] == "--source" && i + 1 < args.size()) {
            sourcePath = args[++i];
        } else if (args[i] == "--dest" && i + 1 < args.size()) {
            destPath = args[++i];
        } else if (args[i] == "--backup-path" && i + 1 < args.size()) {
            backupPath = args[++i];
        } else if (args[i] == "--restore-path" && i + 1 < args.size()) {
            restorePath = args[++i];
        } else if (args[i] == "--key" && i + 1 < args.size()) {
            encryptionKey = args[++i];
            enableEncryption = true;
        } else if (args[i] == "--compress") {
            enableCompression = true;
        } else if (args[i] == "--no-compress") {
            enableCompression = false;
        } else if (args[i] == "--encrypt") {
            enableEncryption = true;
        } else if (args[i] == "--level" && i + 1 < args.size()) {
            compressionLevel = std::stoi(args[++i]);
        } else if (args[i] == "--interval" && i + 1 < args.size()) {
            scheduleInterval = std::stoi(args[++i]);
        }
    }

    // Validate arguments
    if (operation.empty()) {
        std::cerr << "Error: No operation specified. Use --help for usage information.\n";
        return 1;
    }

    BackupManager backupManager;
    backupManager.setProgressCallback(progressCallback);

    try {
        if (operation == "backup" || operation == "incremental") {
            if (sourcePath.empty() || destPath.empty()) {
                std::cerr << "Error: Source and destination paths are required for backup operations.\n";
                return 1;
            }

            BackupManager::BackupOptions options;
            options.sourcePath = sourcePath;
            options.destPath = destPath;
            options.enableCompression = enableCompression;
            options.enableEncryption = enableEncryption;
            options.encryptionKey = encryptionKey;
            options.incremental = (operation == "incremental");
            options.compressionLevel = compressionLevel;

            std::cout << "Starting " << (options.incremental ? "incremental" : "full") << " backup...\n";
            std::cout << "Source: " << sourcePath << "\n";
            std::cout << "Destination: " << destPath << "\n";
            
            auto startTime = std::chrono::high_resolution_clock::now();
            bool success = options.incremental ? 
                backupManager.createIncrementalBackup(options) : 
                backupManager.createBackup(options);
            auto endTime = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
            
            if (success) {
                std::cout << "Backup completed successfully in " << Utils::formatDuration(duration) << "\n";
            } else {
                std::cerr << "Backup failed!\n";
                return 1;
            }

        } else if (operation == "restore") {
            if (backupPath.empty() || restorePath.empty()) {
                std::cerr << "Error: Backup path and restore path are required for restore operations.\n";
                return 1;
            }

            std::cout << "Starting restore...\n";
            std::cout << "Backup: " << backupPath << "\n";
            std::cout << "Restore to: " << restorePath << "\n";
            
            auto startTime = std::chrono::high_resolution_clock::now();
            bool success = backupManager.restoreBackup(backupPath, restorePath);
            auto endTime = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
            
            if (success) {
                std::cout << "Restore completed successfully in " << Utils::formatDuration(duration) << "\n";
            } else {
                std::cerr << "Restore failed!\n";
                return 1;
            }

        } else if (operation == "verify") {
            if (backupPath.empty()) {
                std::cerr << "Error: Backup path is required for verify operations.\n";
                return 1;
            }

            std::cout << "Verifying backup: " << backupPath << "\n";
            
            auto startTime = std::chrono::high_resolution_clock::now();
            bool success = backupManager.verifyBackup(backupPath);
            auto endTime = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
            
            if (success) {
                std::cout << "Backup verification successful in " << Utils::formatDuration(duration) << "\n";
            } else {
                std::cerr << "Backup verification failed!\n";
                return 1;
            }

        } else if (operation == "list") {
            if (destPath.empty()) {
                std::cerr << "Error: Destination path is required to list backups.\n";
                return 1;
            }

            std::cout << "Available backups in " << destPath << ":\n";
            auto backups = backupManager.listBackups(destPath);
            
            if (backups.empty()) {
                std::cout << "No backups found.\n";
            } else {
                for (const auto& backup : backups) {
                    auto timestamp = backupManager.getBackupTimestamp(backup);
                    auto size = backupManager.getBackupSize(backup);
                    
                    std::cout << "  " << Utils::getFileName(backup) 
                              << " - " << Utils::formatTimestamp(timestamp)
                              << " - " << Utils::formatBytes(size) << "\n";
                }
            }

        } else if (operation == "schedule") {
            if (sourcePath.empty() || destPath.empty() || scheduleInterval <= 0) {
                std::cerr << "Error: Source path, destination path, and interval are required for scheduling.\n";
                return 1;
            }

            Scheduler scheduler;
            
            // Set up the backup callback
            scheduler.setBackupCallback([&](const std::string& name) -> bool {
                BackupManager::BackupOptions options;
                options.sourcePath = sourcePath;
                options.destPath = destPath;
                options.enableCompression = enableCompression;
                options.enableEncryption = enableEncryption;
                options.encryptionKey = encryptionKey;
                options.incremental = true; // Use incremental for scheduled backups
                options.compressionLevel = compressionLevel;

                std::cout << "Executing scheduled backup: " << name << "\n";
                return backupManager.createIncrementalBackup(options);
            });

            // Schedule the backup
            std::string scheduleName = "auto_backup_" + Utils::formatTimestamp(std::chrono::system_clock::now());
            scheduler.scheduleBackup(scheduleName, Scheduler::ScheduleType::CUSTOM_INTERVAL, 
                                   std::chrono::seconds(scheduleInterval));

            std::cout << "Scheduled backup every " << scheduleInterval << " seconds\n";
            std::cout << "Press Ctrl+C to stop...\n";

            scheduler.start();
            
            // Keep the program running
            std::string input;
            std::getline(std::cin, input);
            
            scheduler.stop();
            std::cout << "Scheduler stopped.\n";

        } else {
            std::cerr << "Error: Unknown operation '" << operation << "'\n";
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
