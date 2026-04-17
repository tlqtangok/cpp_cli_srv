// cli/main.cpp
// Usage:
//   cpp_cli --schema
//   cpp_cli --cmd echo --args "{\"text\":\"hello\"}"
//   cpp_cli --cmd add  --args "{\"a\":3,\"b\":4}"
//   cpp_cli --cmd echo --args "{\"text\":\"hi\"}" --human
//
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <signal.h>
#endif
#include "../core/engine.h"
#include "../core/commands.h"

#ifndef BUILD_TIME
#define BUILD_TIME "unknown"
#endif
#ifndef GIT_COMMIT_ID
#define GIT_COMMIT_ID "unknown"
#endif
static const std::string VERSION = std::string(BUILD_TIME) + "-" + GIT_COMMIT_ID;

// Check if a process is running (cross-platform)
bool is_process_running(int pid)
{
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess == NULL) return false;
    DWORD exitCode;
    bool running = GetExitCodeProcess(hProcess, &exitCode) && exitCode == STILL_ACTIVE;
    CloseHandle(hProcess);
    return running;
#else
    // On Unix, kill(pid, 0) checks if process exists without sending a signal
    return kill(pid, 0) == 0;
#endif
}

// Read server token from data/srv_argc_argv.txt if server is running
std::string get_server_token()
{
    const std::string srv_file = "data/srv_argc_argv.txt";
    
    // Check if file exists
    if (!std::filesystem::exists(srv_file)) {
        return "your_token";  // Default token if no server info
    }
    
    try {
        std::ifstream file(srv_file);
        if (!file.is_open()) {
            return "your_token";
        }
        
        int pid = -1;
        std::string token = "your_token";
        std::string line;
        
        while (std::getline(file, line)) {
            if (line.substr(0, 4) == "PID=") {
                pid = std::stoi(line.substr(4));
            } else if (line.substr(0, 6) == "TOKEN=") {
                token = line.substr(6);
            }
        }
        file.close();
        
        // Verify the server process is still running
        if (pid > 0 && is_process_running(pid)) {
            return token;  // Use server's token
        } else {
            // Server not running, delete stale file
            std::filesystem::remove(srv_file);
            return "your_token";  // Use default
        }
    } catch (...) {
        return "your_token";  // Default on any error
    }
}

int main(int argc, char* argv[])
{
    Engine e(true);  // true = immediate persist mode for CLI
    // Try to get token from running server, otherwise use default "your_token"
    std::string token = get_server_token();
    register_all(e, token);

    std::string cmd, args_str, data_str;
    bool schema_mode = false;
    bool human_mode  = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if      (a == "--schema")             schema_mode = true;
        else if (a == "--human")              human_mode  = true;
        else if (a == "-v" || a == "--version")
        {
            std::cout << "cpp_cli " << VERSION << "\n";
            return 0;
        }
        // Old format: --cmd <name> --args '{...}'
        else if (a == "--cmd"  && i+1 < argc) cmd         = argv[++i];
        else if (a == "--args" && i+1 < argc) args_str    = argv[++i];
        // New format: --data '{...}' or -d '{...}'
        else if ((a == "--data" || a == "-d") && i+1 < argc) data_str    = argv[++i];
        else if (a == "-h" || a == "--help")
        {
            std::cout << "cpp_cli - Command Line Interface\n\n"
                      << "Usage (curl-style, recommended):\n"
                      << "  cpp_cli --data '{\"cmd\":\"<name>\",\"args\":{...}}'\n"
                      << "  cpp_cli -d '{\"cmd\":\"<name>\",\"args\":{...}}'\n\n"
                      << "Usage (legacy format):\n"
                      << "  cpp_cli --cmd <name> --args '{...}'\n\n"
                      << "Other options:\n"
                      << "  cpp_cli --schema              Show all available commands and their schemas\n"
                      << "  cpp_cli --human               Human-friendly output format\n"
                      << "  cpp_cli -v, --version         Show version (build time + git commit)\n"
                      << "  cpp_cli -h, --help            Show this help message\n\n"
                      << "Examples (curl-style format - matches curl POST):\n\n"
                      << "  echo - Echo back text\n"
                      << "    cpp_cli -d '{\"cmd\":\"echo\",\"args\":{\"text\":\"hello world\"}}'\n\n"
                      << "  add - Add two numbers\n"
                      << "    cpp_cli -d '{\"cmd\":\"add\",\"args\":{\"a\":10,\"b\":32}}'\n\n"
                      << "  upper - Convert text to uppercase\n"
                      << "    cpp_cli -d '{\"cmd\":\"upper\",\"args\":{\"text\":\"hello\"}}'\n\n"
                      << "  call_shell - Execute shell command (requires token)\n"
#ifdef _WIN32
                      << "    cpp_cli -d '{\"cmd\":\"call_shell\",\"args\":{\"command\":\"where cmd\",\"token\":\"your_token\"}}'\n\n"
#else
                      << "    cpp_cli -d '{\"cmd\":\"call_shell\",\"args\":{\"command\":\"ls -la\",\"token\":\"your_token\"}}'\n\n"
#endif
                      << "  get_global_json - Get global JSON (requires token)\n"
                      << "    cpp_cli -d '{\"cmd\":\"get_global_json\",\"args\":{\"token\":\"your_token\"}}'\n\n"
                      << "  set_global_json - Replace entire global JSON (requires token)\n"
                      << "    cpp_cli -d '{\"cmd\":\"set_global_json\",\"args\":{\"value\":{\"name\":\"Alice\"},\"token\":\"your_token\"}}'\n\n"
                      << "  patch_global_json - Apply JSON merge patch (requires token)\n"
                      << "    cpp_cli -d '{\"cmd\":\"patch_global_json\",\"args\":{\"age\":31,\"city\":\"NYC\",\"token\":\"your_token\"}}'\n\n"
                      << "Output Format:\n"
                      << "  Default (JSON): {\"code\":0,\"output\":\"...\",\"error\":\"\"}\n"
                      << "  With --human:   [OK] result\\n<output>\n\n"
                      << "For detailed schema: cpp_cli --schema\n";
            return 0;
        }
    }

    if (schema_mode)
    {
        std::cout << e.schema().dump(2) << "\n";
        return 0;
    }

    // Parse data from either new or old format
    json data;
    
    // If new format (--data or -d) is provided
    if (!data_str.empty())
    {
        try 
        { 
            data = json::parse(data_str);
            if (!data.contains("cmd"))
            {
                std::cerr << "{\"code\":1,\"output\":\"\",\"error\":\"missing 'cmd' field in --data JSON\"}\n";
                return 1;
            }
            cmd = data["cmd"];
            if (data.contains("args"))
            {
                args_str = data["args"].dump();
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "{\"code\":1,\"output\":\"\",\"error\":\"invalid JSON in --data: " << e.what() << "\"}\n";
            return 1;
        }
    }
    // Old format (--cmd and --args)
    else if (!cmd.empty())
    {
        // cmd already set, args_str already set
    }
    else
    {
        std::cerr << "Usage: cpp_cli -d '{\"cmd\":\"<name>\",\"args\":{...}}'\n"
                  << "       cpp_cli --schema\n"
                  << "       cpp_cli --help\n";
        return 1;
    }

    // Parse arguments
    json args;
    if (!args_str.empty())
    {
        try { args = json::parse(args_str); }
        catch (const std::exception& e)
        {
            std::cerr << "{\"code\":1,\"output\":\"\",\"error\":\"invalid JSON in args: " << e.what() << "\"}\n";
            return 1;
        }
    }

    Result r   = e.run(cmd, args);
    json   out = r.to_json();

    if (human_mode)
    {
        std::cout << (r.code == 0 ? "[OK] " : "[ERR] ") << (r.code == 0 ? r.output : r.error) << "\n";
        if (!r.output.empty() && r.code == 0) std::cout << r.output << "\n";
    }
    else
    {
        std::cout << out.dump() << "\n";
    }

    return r.code == 0 ? 0 : 1;
}
