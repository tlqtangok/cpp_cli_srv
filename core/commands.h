#pragma once
#include "engine.h"
#include <thread>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <array>
#include <sstream>
#include <fstream>  // For file I/O operations

// Register all commands here.
// Both CLI and Web Server call register_all() - zero duplication.
//
// Sync  command: e.reg(def, SyncHandler)
// Async command: e.reg_async(def, AsyncHandler)  <- returns std::future<Result>

inline void register_all(Engine& e, const std::string& token = "")
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
    // -------------------------------------------------------------------------
    // call_shell - ASYNC command execution (TOKEN REQUIRED)
    //
    // Executes shell commands on the host system:
    //   - Windows: runs as CMD command (e.g., "dir", "type file.txt")
    //   - Linux:   runs as bash command (e.g., "ls", "cat file.txt")
    //
    // SECURITY: Requires token authentication.
    //
    // Arguments:
    //   - command: Shell command to execute (required)
    //   - token: Security token (required if server has token enabled)
    //
    // Examples:
    //   Windows: {"cmd":"call_shell","args":{"command":"where cmd","token":"jd"}}
    //   Linux:   {"cmd":"call_shell","args":{"command":"ls -la","token":"jd"}}
    // -------------------------------------------------------------------------
    
    // Set platform-specific default example command
    #ifdef _WIN32
        std::string default_shell_cmd = "where cmd";
    #else
        std::string default_shell_cmd = "ls -la";
    #endif
    
    e.reg_async(
        { "call_shell", "Execute shell command (requires token)",
          { {"command", "shell command to execute", true, default_shell_cmd},
            {"token", "Security token", false, ""} },
          /* is_async = */ true },
        [token](const json& args) -> std::future<Result>
        {
            // Token authentication
            if (!token.empty()) {
                std::string req_token = args.value("token", "");
                if (req_token != token) {
                    auto fut = std::async(std::launch::deferred, []() -> Result {
                        return { 6, "", "call_shell requires valid token" };
                    });
                    return fut;
                }
            }
            
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

    // -------------------------------------------------------------------------
    // write_json - Write JSON content to file
    //
    // Writes a JSON object to a file with pretty formatting.
    //
    // Arguments:
    //   - path: File path to write to (required)
    //   - json_content: JSON object to write (required)
    //
    // Returns:
    //   - Success: {"code":0, "output":"File written: <path>", "error":""}
    //   - Error: {"code":1, "output":"", "error":"error message"}
    //
    // Examples:
    //   {"cmd":"write_json","args":{"path":"./data/test.json","json_content":{"k0":"v0","k1":"v1"}}}
    // -------------------------------------------------------------------------
    e.reg(
        { "write_json", "Write JSON content to a file",
          { {"path", "file path to write", true, std::string("./data/test.json")},
            {"json_content", "JSON object to write", true, json::object({{"k1","v1"},{"k2","v2"},{"k3",33}})} } },
        [](const json& args) -> Result
        {
            std::string path = args.value("path", "");
            
            if (path.empty()) {
                return { 1, "", "path is required" };
            }
            
            if (!args.contains("json_content")) {
                return { 1, "", "json_content is required" };
            }
            
            try {
                json content = args["json_content"];
                
                // Open file for writing
                std::ofstream file(path);
                if (!file.is_open()) {
                    return { 1, "", "failed to open file for writing: " + path };
                }
                
                // Write JSON with pretty formatting (indent = 2 spaces)
                file << content.dump(2) << std::endl;
                file.close();
                
                return { 0, "File written: " + path, "" };
                
            } catch (const json::exception& e) {
                return { 4, "", std::string("JSON error: ") + e.what() };
            } catch (const std::exception& e) {
                return { 3, "", std::string("exception: ") + e.what() };
            }
        }
    );

    // -------------------------------------------------------------------------
    // read_json - Read JSON content from file
    //
    // Reads a JSON file and returns its content as a JSON object.
    //
    // Arguments:
    //   - path: File path to read from (required)
    //
    // Returns:
    //   - Success: {"code":0, "output":{...json object...}, "error":""}
    //   - Error: {"code":1, "output":"", "error":"error message"}
    //
    // Note: The output field is automatically parsed as JSON object, not a string.
    //
    // Examples:
    //   {"cmd":"read_json","args":{"path":"./data/test.json"}}
    // -------------------------------------------------------------------------
    e.reg(
        { "read_json", "Read JSON content from a file",
          { {"path", "file path to read", true, std::string("./data/test.json")} } },
        [](const json& args) -> Result
        {
            std::string path = args.value("path", "");
            
            if (path.empty()) {
                return { 1, "", "path is required" };
            }
            
            try {
                // Open file for reading
                std::ifstream file(path);
                if (!file.is_open()) {
                    return { 1, "", "failed to open file for reading: " + path };
                }
                
                // Parse JSON from file
                json content;
                file >> content;
                file.close();
                
                // Return the JSON content as compact string (will be auto-parsed to object in output)
                return { 0, content.dump(), "" };
                
            } catch (const json::exception& e) {
                return { 4, "", std::string("JSON parse error: ") + e.what() };
            } catch (const std::exception& e) {
                return { 3, "", std::string("exception: ") + e.what() };
            }
        }
    );

    // Add your own commands below...
    // Sync:  e.reg(       {name, desc, {args...}},        [](const json& args) -> Result { ... });
    // Async: e.reg_async( {name, desc, {args...}, true},  [](const json& args) -> std::future<Result> { ... });

    // -------------------------------------------------------------------------
    // get_global_json - Get global JSON (TOKEN REQUIRED)
    //
    // Retrieves the global JSON variable that is stored in memory.
    //
    // SECURITY: Requires token authentication.
    //
    // Arguments:
    //   - path: Optional JSON pointer path (RFC 6901), e.g., "/a/b/c"
    //   - token: Security token (required if server has token enabled)
    //
    // Returns:
    //   - Success: {"code":0, "output":{...json...}, "error":""}
    //   - Error: {"code":6, "output":"", "error":"authentication error"}
    //
    // Examples:
    //   {"cmd":"get_global_json","args":{"token":"jd"}}
    //   {"cmd":"get_global_json","args":{"path":"/user/name","token":"jd"}}
    // -------------------------------------------------------------------------
    e.reg(
        { "get_global_json", "Get global JSON (requires token)",
          { {"path", "JSON pointer path (optional, e.g., /a/b/c)", false, ""},
            {"token", "Security token", false, ""} } },
        [&e, token](const json& args) -> Result
        {
            try {
                // Token authentication
                if (!token.empty()) {
                    std::string req_token = args.value("token", "");
                    if (req_token != token) {
                        return { 6, "", "get_global_json requires valid token" };
                    }
                }
                
                std::string path = args.value("path", "");
                
                json result;
                if (path.empty()) {
                    result = e.get_global_json();
                } else {
                    result = e.get_global_json_path(path);
                    if (result.is_null()) {
                        return { 1, "", "path not found: " + path };
                    }
                }
                
                return { 0, result.dump(), "" };
            } catch (const std::exception& ex) {
                return { 3, "", std::string("exception: ") + ex.what() };
            }
        }
    );

    // -------------------------------------------------------------------------
    // set_global_json - Replace entire global JSON (TOKEN REQUIRED)
    //
    // Sets the global JSON variable to a new value.
    // SECURITY: Requires token authentication.
    //
    // Arguments:
    //   - value: New JSON value (required)
    //   - token: Security token (required if server has token enabled)
    //
    // Returns:
    //   - Success: {"code":0, "output":"Global JSON updated", "error":""}
    //   - Error: {"code":6, "output":"", "error":"authentication error"}
    //
    // Examples:
    //   {"cmd":"set_global_json","args":{"value":{"key":"value"},"token":"jd"}}
    // -------------------------------------------------------------------------
    e.reg(
        { "set_global_json", "Replace entire global JSON (requires token)",
          { {"value", "New JSON value", true, json::object({{"example","value"}})},
            {"token", "Security token", false, ""} } },
        [&e, token](const json& args) -> Result
        {
            try {
                // Token authentication (same as call_shell)
                if (!token.empty()) {
                    std::string req_token = args.value("token", "");
                    if (req_token != token) {
                        return { 6, "", "set_global_json requires valid token" };
                    }
                }
                
                if (!args.contains("value")) {
                    return { 1, "", "value is required" };
                }
                
                e.set_global_json(args["value"]);
                return { 0, "Global JSON updated", "" };
            } catch (const std::exception& ex) {
                return { 3, "", std::string("exception: ") + ex.what() };
            }
        }
    );

    // -------------------------------------------------------------------------
    // patch_global_json - Apply JSON merge patch (RFC 7386)
    //
    // Applies a merge patch to the global JSON and returns a JSON Patch diff (RFC 6902).
    // The entire args object is used as the patch (simplified API).
    //
    // Merge Patch Rules (RFC 7386):
    //   - If patch value is object, recursively merge
    //   - If patch value is null, delete the key
    //   - Otherwise, replace the value
    //
    // Returns:
    //   - Success: {"code":0, "output":[...], "error":""}
    //     where output is a JSON Patch (RFC 6902) array of operations:
    //     [{"op":"replace","path":"/key","value":"new"}, {"op":"add","path":"/new","value":123}]
    //
    // Examples:
    //   Current: {"a":1, "b":{"c":2}}
    //   Args:    {"b":{"c":999}, "d":4}
    //   Diff:    [{"op":"replace","path":"/b/c","value":999}, {"op":"add","path":"/d","value":4}]
    //
    //   {"cmd":"patch_global_json","args":{"age":31,"city":"NYC"}}
    // -------------------------------------------------------------------------
    e.reg(
        { "patch_global_json", "Apply JSON merge patch and return diff",
          { } },  // No specific args - entire args object is the patch
        [&e](const json& args) -> Result
        {
            try {
                // Use the entire args object as the patch
                json diff = e.patch_global_json(args);
                return { 0, diff.dump(), "" };
            } catch (const std::exception& ex) {
                return { 3, "", std::string("exception: ") + ex.what() };
            }
        }
    );

    // -------------------------------------------------------------------------
    // persist_global_json - Force save global JSON to disk
    //
    // Manually triggers a save of the global JSON to data/GLOBAL_JSON.json.
    // Note: The global JSON is automatically saved after each modification,
    // so this command is typically only needed for manual checkpoints.
    //
    // Arguments: None
    //
    // Returns:
    //   - Success: {"code":0, "output":"Global JSON persisted to disk", "error":""}
    //
    // Examples:
    //   {"cmd":"persist_global_json","args":{}}
    // -------------------------------------------------------------------------
    e.reg(
        { "persist_global_json", "Force save global JSON to disk",
          { } },
        [&e](const json& /*args*/) -> Result
        {
            try {
                e.save_global_json();
                return { 0, "Global JSON persisted to disk", "" };
            } catch (const std::exception& ex) {
                return { 3, "", std::string("exception: ") + ex.what() };
            }
        }
    );
}
