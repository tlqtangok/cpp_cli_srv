#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <shared_mutex>   // C++17 read-write lock
#include <future>         // std::future / std::async
#include <chrono>
#include <optional>
#include "../third_party/json.hpp"

using json = nlohmann::json;

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
};
