#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <shared_mutex>   // C++17 read-write lock
#include <future>         // std::future / std::async
#include <chrono>
#include <optional>
#include <thread>
#include <cstdio>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif
#include "../third_party/json.hpp"

using json = nlohmann::json;

// Forward declarations for file I/O
#include <fstream>
#include <filesystem>

// ---------------------------------------------------------------------------
// ArgDef
// ---------------------------------------------------------------------------
struct ArgDef
{
    std::string name;
    std::string desc;
    bool        required    = false;
    json        default_val = nullptr;  // pre-fills GUI template if set
};

// ---------------------------------------------------------------------------
// CmdDef
// ---------------------------------------------------------------------------
struct CmdDef
{
    std::string         name;
    std::string         desc;
    std::vector<ArgDef> args;
    bool                is_async = false; // mark slow commands for UI hint
};

// ---------------------------------------------------------------------------
// Result - Unified response format
// ---------------------------------------------------------------------------
struct Result
{
    int         code    = 0;        // 0 = success, non-zero = error
    std::string output;             // result data or message
    std::string error;              // error message (empty if code == 0)

    json to_json() const
    {
        json result;
        result["code"] = code;
        
        // Try to parse output as JSON; if successful, embed as object
        // This makes read_json and similar commands return clean JSON objects
        if (!output.empty() && (output[0] == '{' || output[0] == '['))
        {
            try {
                result["output"] = json::parse(output);
            } catch (...) {
                // If parse fails, keep as string
                result["output"] = output;
            }
        }
        else
        {
            result["output"] = output;
        }
        
        result["error"] = error;
        return result;
    }
};

// ---------------------------------------------------------------------------
// Handler types
//
//   SyncHandler  - returns Result directly (fast commands)
//   AsyncHandler - returns std::future<Result> (slow / IO-bound commands)
//
// Both are stored as AsyncHandler internally; SyncHandler is auto-wrapped.
// ---------------------------------------------------------------------------
using SyncHandler  = std::function<Result(const json& args)>;
using AsyncHandler = std::function<std::future<Result>(const json& args)>;

// ---------------------------------------------------------------------------
// Engine
//
// Thread-safety model:
//   - reg()    acquires unique_lock  (write) - typically only at startup
//   - run()    acquires shared_lock  (read)  - concurrent calls are safe
//   - schema() acquires shared_lock  (read)
//
// Handler execution happens OUTSIDE the lock, so long-running handlers
// do not block other requests from dispatching.
// ---------------------------------------------------------------------------
class Engine
{
public:
    // Constructor - load global JSON on startup
    Engine()
    {
        load_global_json();
    }

    // Register a synchronous (fast) command
    void reg(const CmdDef& def, SyncHandler handler)
    {
        // Wrap sync handler in a future so storage is uniform
        AsyncHandler wrapped = [h = std::move(handler)](const json& args)
        {
            return std::async(std::launch::deferred, h, args);
        };
        _reg(def, std::move(wrapped));
    }

    // Register an asynchronous (slow / IO-bound) command
    void reg_async(const CmdDef& def, AsyncHandler handler)
    {
        CmdDef d = def;
        d.is_async = true;
        _reg(d, std::move(handler));
    }

    // Execute a command.
    // For sync handlers: blocks until done (fast, no overhead).
    // For async handlers: launches on std::async(launch::async) thread and
    //   waits with an optional timeout.
    Result run(const std::string& cmd,
               const json&        args,
               std::chrono::milliseconds timeout = std::chrono::milliseconds(30000)) const
    {
        AsyncHandler handler;
        bool is_async = false;
        {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            auto it = handlers_.find(cmd);
            if (it == handlers_.end())
                return { 1, "", "unknown command: " + cmd };
            handler  = it->second;
            auto dit = def_map_.find(cmd);
            if (dit != def_map_.end()) is_async = dit->second.is_async;
        }
        // Handler runs OUTSIDE the lock
        try
        {
            std::future<Result> fut = handler(args);

            if (is_async)
            {
                // Real async: respect timeout
                auto status = fut.wait_for(timeout);
                if (status == std::future_status::timeout)
                    return { 2, "", "command timed out after " +
                             std::to_string(timeout.count()) + "ms" };
            }
            // Deferred (sync-wrapped) or ready async
            return fut.get();
        }
        catch (const std::exception& ex)
        {
            return { 3, "", std::string("exception: ") + ex.what() };
        }
    }

    // Return full command schema as JSON array
    json schema() const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        json out = json::array();
        for (auto& d : defs_)
        {
            json cmd;
            cmd["name"]     = d.name;
            cmd["desc"]     = d.desc;
            cmd["is_async"] = d.is_async;
            cmd["args"]     = json::array();
            for (auto& a : d.args)
            {
                json arg = { {"name", a.name}, {"desc", a.desc},
                             {"required", a.required} };
                if (!a.default_val.is_null()) arg["default"] = a.default_val;
                cmd["args"].push_back(arg);
            }
            out.push_back(cmd);
        }
        return out;
    }

    // ---------------------------------------------------------------------------
    // Global JSON Management
    // ---------------------------------------------------------------------------
    
    // Get entire global JSON
    json get_global_json() const
    {
        std::shared_lock<std::shared_mutex> lock(global_json_mutex_);
        return global_json_;
    }
    
    // Get value at JSON pointer path (RFC 6901)
    json get_global_json_path(const std::string& path) const
    {
        std::shared_lock<std::shared_mutex> lock(global_json_mutex_);
        try {
            return global_json_.at(json::json_pointer(path));
        } catch (...) {
            return nullptr;
        }
    }
    
    // Set entire global JSON (deferred persistence)
    void set_global_json(const json& new_val)
    {
        {
            std::unique_lock<std::shared_mutex> lock(global_json_mutex_);
            global_json_ = new_val;
        }
        schedule_save_global_json();
    }
    
    // Apply merge patch (RFC 7386) and return JSON Patch diff (RFC 6902)
    json patch_global_json(const json& patch)
    {
        json before, after;
        {
            std::unique_lock<std::shared_mutex> lock(global_json_mutex_);
            before = global_json_;
            global_json_.merge_patch(patch);
            after = global_json_;
        }
        schedule_save_global_json();
        
        // Generate JSON Patch (RFC 6902) diff
        json diff = json::diff(before, after);
        return diff;
    }
    
    // Save global JSON to disk
    void save_global_json()
    {
        std::shared_lock<std::shared_mutex> lock(global_json_mutex_);
        try {
            // Ensure data directory exists
            std::filesystem::create_directories("data");
            
            std::ofstream file(global_json_path_);
            if (file.is_open()) {
                file << global_json_.dump(2) << std::endl;
                file.close();
            }
        } catch (...) {
            // Silently fail - persistence is best-effort
        }
    }
    
    // Get CPU usage percentage (0-100)
    double get_cpu_usage()
    {
        #ifdef _WIN32
            // Windows: Try GetSystemTimes for CPU usage
            // Fallback: assume CPU is spare (< 50%)
            FILETIME idleTime, kernelTime, userTime;
            if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
                // Simple heuristic: if we can get system times, assume CPU is reasonably available
                // For simplified purposes, return a conservative estimate
                return 25.0;  // Conservative estimate for Windows
            }
            return 0.0;  // Fallback: assume spare CPU
        #else
            // Linux: read /proc/loadavg and estimate CPU usage
            // loadavg is normalized by number of CPUs
            FILE* fp = fopen("/proc/loadavg", "r");
            if (!fp) return 0.0;
            
            double load1, load5, load15;
            int result = fscanf(fp, "%lf %lf %lf", &load1, &load5, &load15);
            fclose(fp);
            
            if (result == 3) {
                // Get number of CPUs
                long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
                if (nprocs <= 0) nprocs = 1;
                
                // Estimate CPU usage from 1-minute load average
                double estimated_cpu = (load1 / nprocs) * 100.0;
                return std::min(100.0, estimated_cpu);  // Cap at 100%
            }
            return 0.0;
        #endif
    }
    
    // Schedule deferred save: wait 15min then write when CPU < 50%
    // Longer delay allows intensive write operations to batch together
    void schedule_save_global_json()
    {
        // Cancel previous pending save
        if (save_task_.valid()) {
            save_task_.wait();  // Wait for completion
        }
        
        // Schedule new async save task
        save_task_ = std::async(std::launch::async, [this]() {
            // Wait 15 minutes (900 seconds) before attempting disk write
            // This batches multiple edits into a single I/O operation
            std::this_thread::sleep_for(std::chrono::seconds(900));
            
            // Wait for CPU to drop below 50%, max 20 minutes total wait
            auto start = std::chrono::steady_clock::now();
            auto timeout = std::chrono::seconds(1200);  // 20 minutes max
            
            while (std::chrono::steady_clock::now() - start < timeout) {
                double cpu_usage = get_cpu_usage();
                if (cpu_usage < 50.0) {
                    // CPU is spare, safe to write
                    save_global_json();
                    return;
                }
                // CPU busy, wait 100ms and check again
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            // Timeout: force write anyway (20 min total wait)
            save_global_json();
        });
    }
    
    // Load global JSON from disk
    void load_global_json()
    {
        std::unique_lock<std::shared_mutex> lock(global_json_mutex_);
        try {
            std::ifstream file(global_json_path_);
            if (file.is_open()) {
                file >> global_json_;
                file.close();
            } else {
                // File doesn't exist - initialize as empty object
                global_json_ = json::object();
            }
        } catch (...) {
            // Parse error or file not found - initialize as empty object
            global_json_ = json::object();
        }
    }

private:
    void _reg(const CmdDef& def, AsyncHandler handler)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        defs_.push_back(def);
        handlers_[def.name] = std::move(handler);
        def_map_[def.name]  = def;
    }

    mutable std::shared_mutex                          mutex_;
    std::vector<CmdDef>                                defs_;
    std::unordered_map<std::string, AsyncHandler>      handlers_;
    std::unordered_map<std::string, CmdDef>            def_map_;
    
    // Global JSON state
    mutable std::shared_mutex                          global_json_mutex_;
    json                                               global_json_;
    std::string                                        global_json_path_ = "data/GLOBAL_JSON.json";
    
    // Deferred persistence task
    std::future<void>                                  save_task_;
};
