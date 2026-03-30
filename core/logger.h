// core/logger.h
// Thread-safe logger with [YYYYMMDD_HHMMSS] timestamp format
#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <ctime>

class Logger
{
public:
    Logger(const std::string& logfile = "") : logfile_(logfile)
    {
        if (!logfile_.empty())
        {
            ofs_.open(logfile_, std::ios::app);
        }
    }

    ~Logger()
    {
        if (ofs_.is_open())
            ofs_.close();
    }

    // Format: [20260330_111722]
    static std::string timestamp()
    {
        auto now   = std::chrono::system_clock::now();
        auto tt    = std::chrono::system_clock::to_time_t(now);
        auto ms    = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now.time_since_epoch()) % 1000;
        
        std::tm tm_local;
#ifdef _WIN32
        localtime_s(&tm_local, &tt);
#else
        localtime_r(&tt, &tm_local);
#endif
        
        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(4) << (tm_local.tm_year + 1900)
            << std::setw(2) << (tm_local.tm_mon + 1)
            << std::setw(2) << tm_local.tm_mday
            << "_"
            << std::setw(2) << tm_local.tm_hour
            << std::setw(2) << tm_local.tm_min
            << std::setw(2) << tm_local.tm_sec;
        
        return oss.str();
    }

    void log(const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string line = "[" + timestamp() + "] " + msg + "\n";
        
        std::cout << line;
        std::cout.flush();
        
        if (ofs_.is_open())
        {
            ofs_ << line;
            ofs_.flush();
        }
    }

    // Structured logging for HTTP requests
    void log_request(const std::string& method,
                     const std::string& path,
                     const std::string& client_ip,
                     const std::string& req_body,
                     int                status,
                     const std::string& resp_body)
    {
        std::ostringstream oss;
        oss << method << " " << path
            << " | IP=" << client_ip
            << " | Status=" << status;
        
        if (!req_body.empty() && req_body.size() <= 200)
            oss << " | In=" << req_body;
        else if (!req_body.empty())
            oss << " | In=[" << req_body.size() << " bytes]";
        
        if (!resp_body.empty() && resp_body.size() <= 200)
            oss << " | Out=" << resp_body;
        else if (!resp_body.empty())
            oss << " | Out=[" << resp_body.size() << " bytes]";
        
        log(oss.str());
    }

private:
    std::string   logfile_;
    std::ofstream ofs_;
    std::mutex    mutex_;
};
