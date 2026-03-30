#pragma once
#include "engine.h"
#include <thread>
#include <chrono>
#include <cmath>

// Register all commands here.
// Both CLI and Web Server call register_all() - zero duplication.
//
// Sync  command: e.reg(def, SyncHandler)
// Async command: e.reg_async(def, AsyncHandler)  <- returns std::future<Result>

inline void register_all(Engine& e)
{
    // -------------------------------------------------------------------------
    // echo - sync, fast
    // -------------------------------------------------------------------------
    e.reg(
        { "echo", "Return the text argument as-is",
          { {"text", "string to echo", true} } },
        [](const json& args) -> Result
        {
            std::string text = args.value("text", "");
            return { true, "ok", { {"result", text} } };
        }
    );

    // -------------------------------------------------------------------------
    // add - sync, fast
    // -------------------------------------------------------------------------
    e.reg(
        { "add", "Return a + b",
          { {"a", "number a", true, 22}, {"b", "number b", true, 100} } },
        [](const json& args) -> Result
        {
            double a = args.at("a").get<double>();
            double b = args.at("b").get<double>();
            return { true, "ok", { {"result", a + b} } };
        }
    );

    // -------------------------------------------------------------------------
    // upper - sync, fast
    // -------------------------------------------------------------------------
    e.reg(
        { "upper", "Convert string to uppercase",
          { {"text", "input string", true} } },
        [](const json& args) -> Result
        {
            std::string s = args.value("text", "");
            for (auto& c : s) c = (char)toupper((unsigned char)c);
            return { true, "ok", { {"result", s} } };
        }
    );

    // -------------------------------------------------------------------------
    // slow_task - ASYNC demo
    //
    // Simulates a slow operation (e.g. DB query, external API call).
    // Runs on its own std::async thread so it never blocks the HTTP thread pool.
    // The server will wait up to timeout_ms (default 30s) then return a timeout
    // error - the handler thread itself will finish eventually on its own.
    //
    // Try it:
    //   POST /post/run  { "cmd":"slow_task", "args":{"ms":3000}, "timeout_ms":5000 }
    //   POST /post/run  { "cmd":"slow_task", "args":{"ms":3000}, "timeout_ms":1000 }
    //     -> returns timeout error while the thread still runs in background
    // -------------------------------------------------------------------------
    e.reg_async(
        { "slow_task", "Simulate a slow async operation (ms = sleep duration)",
          { {"ms", "sleep milliseconds", true, 2000} },
          /* is_async = */ true },
        [](const json& args) -> std::future<Result>
        {
            int ms = args.value("ms", 1000);
            // Launch on a real OS thread so HTTP pool is not blocked
            return std::async(std::launch::async, [ms]() -> Result
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                return { true, "ok", { {"result", "done"}, {"slept_ms", ms} } };
            });
        }
    );

    // Add your own commands below...
    // Sync:  e.reg(       {name, desc, {args...}},        [](const json& args) -> Result { ... });
    // Async: e.reg_async( {name, desc, {args...}, true},  [](const json& args) -> std::future<Result> { ... });
}
