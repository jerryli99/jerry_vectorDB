#pragma once

#include <chrono>
#include <thread>
#include <filesystem>
#include <fstream>
#include <atomic>
#include <iostream>

/*
Main idea is to do periodict backups for my vector index files, say every 5 hours or so, and then sleep and then continue.

*/
namespace vectordb {

class BackupTimer {
public:
    BackupTimer() = default;

    //careful, need to close the thread if db ends.
    ~BackupTimer() {
        stop();
    }

    void start(const std::string& source_dir, const std::string& backup_dir, int interval_hrs = 5) {
        stop();//stop if already running
        m_running = true;
        m_source_dir = source_dir;
        m_backup_dir = backup_dir;
        m_interval_hrs = interval_hrs;

        //start backup thread
        m_backup_thread = std::thread(&BackupTimer::backup_loop, this);
    }

    void stop() {
        m_running = false;
        if (m_backup_thread.joinable()) {
            m_backup_thread.join();
        }
    }
    
private:
    std::atomic<bool> m_running{false};
    std::thread m_backup_thread;
    std::string m_source_dir;
    std::string m_backup_dir;
    int m_interval_hrs{5};

    void backup_loop() {
        do_backup();
        while (m_running) {
            //sleep for specified interval
            std::this_thread::sleep_for(std::chrono::hours(m_interval_hrs));

            if (m_running) {
                do_backup();
            }
        }
    }

    void do_backup() {
        try {
            namespace fs = std::filesystem;
            
            // Check if source directory exists
            if (!fs::exists(m_source_dir)) {
                std::cout << "Backup: Source directory does not exist: " << m_source_dir << std::endl;
                return;
            }

            // Create backup directory if it doesn't exist
            if (!fs::exists(m_backup_dir)) {
                fs::create_directories(m_backup_dir);
            }

            // Get current timestamp for backup folder
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::string timestamp = std::to_string(time_t);
            
            std::string backup_path = m_backup_dir + "/backup_" + timestamp;
            fs::create_directories(backup_path);

            int files_copied = 0;
            
            // Copy all files from source to backup directory
            for (const auto& entry : fs::directory_iterator(m_source_dir)) {
                if (!m_running) break; // Stop if shutdown requested
                
                if (entry.is_regular_file()) {
                    const auto& path = entry.path();
                    std::string filename = path.filename().string();
                    std::string dest_path = backup_path + "/" + filename;
                    
                    try {
                        fs::copy_file(path, dest_path, fs::copy_options::overwrite_existing);
                        files_copied++;
                        std::cout << "Backup: Copied " << filename << std::endl;
                    } catch (const std::exception& e) {
                        std::cout << "Backup: Failed to copy " << filename << ": " << e.what() << std::endl;
                    }
                }
            }
            
            std::cout << "Backup: Completed. " << files_copied << " files copied to " << backup_path << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Backup: Error during backup: " << e.what() << std::endl;
        }
    }

};

}//namespace vectordb