#pragma once
#include "engine.h"
#include <thread>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <array>
#include <sstream>

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
            return { 0, text, "" };
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
            double result = a + b;
            return { 0, std::to_string(result), "" };
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
            return { 0, s, "" };
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
                json result;
                result["slept_ms"] = ms;
                result["status"] = "done";
                return { 0, result.dump(), "" };
            });
        }
    );

    // -------------------------------------------------------------------------
    // call_shell - ASYNC command execution
    //
    // Executes shell commands on the host system:
    //   - Windows: runs as CMD command (e.g., "dir", "type file.txt")
    //   - Linux:   runs as bash command (e.g., "ls", "cat file.txt")
    //
    // Security Warning: This command executes arbitrary shell commands.
    // In production, consider:
    //   - Whitelisting allowed commands
    //   - Input sanitization
    //   - Running with restricted user permissions
    //   - Adding authentication/authorization
    //
    // Examples:
    //   Windows: {"cmd":"call_shell","args":{"command":"where cmd"}}
    //   Linux:   {"cmd":"call_shell","args":{"command":"cat /etc/*release"}}
    // -------------------------------------------------------------------------
    
    // Set platform-specific default example command
    #ifdef _WIN32
        std::string default_shell_cmd = "where cmd";
    #else
        std::string default_shell_cmd = "cat /etc/*release";
    #endif
    
    e.reg_async(
        { "call_shell", "Execute a shell command (Windows: CMD, Linux: bash)",
          { {"command", "shell command to execute", true, default_shell_cmd} },
          /* is_async = */ true },
        [](const json& args) -> std::future<Result>
        {
            std::string command = args.value("command", "");
            
            return std::async(std::launch::async, [command]() -> Result
            {
                if (command.empty()) {
                    return { 1, "", "command argument is required" };
                }

                // Platform-specific command execution
                std::string full_command;
                #ifdef _WIN32
                    // Windows: use cmd.exe
                    full_command = "cmd.exe /c " + command + " 2>&1";
                #else
                    // Linux/Unix: use bash
                    full_command = command + " 2>&1";
                #endif

                // Execute command and capture output
                std::array<char, 128> buffer;
                std::string output;
                int exit_code = 0;
                
                try {
                    #ifdef _WIN32
                        FILE* pipe = _popen(full_command.c_str(), "r");
                    #else
                        FILE* pipe = popen(full_command.c_str(), "r");
                    #endif
                    
                    if (!pipe) {
                        return { 2, "", "failed to execute command" };
                    }

                    // Read command output
                    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
                        output += buffer.data();
                    }

                    // Get exit code
                    #ifdef _WIN32
                        exit_code = _pclose(pipe);
                    #else
                        exit_code = pclose(pipe);
                    #endif

                    // Return result
                    json result_data;
                    result_data["command"] = command;
                    result_data["exit_code"] = exit_code;
                    result_data["stdout"] = output;
                    
                    if (exit_code == 0) {
                        return { 0, result_data.dump(), "" };
                    } else {
                        return { exit_code, result_data.dump(), "command failed with exit code " + std::to_string(exit_code) };
                    }
                    
                } catch (const std::exception& e) {
                    return { 3, "", std::string("exception: ") + e.what() };
                }
            });
        }
    );

    // Add your own commands below...
    // Sync:  e.reg(       {name, desc, {args...}},        [](const json& args) -> Result { ... });
    // Async: e.reg_async( {name, desc, {args...}, true},  [](const json& args) -> std::future<Result> { ... });
}
