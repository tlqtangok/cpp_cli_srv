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
        return "jd";  // Default token if no server info
    }
    
    try {
        std::ifstream file(srv_file);
        if (!file.is_open()) {
            return "jd";
        }
        
        int pid = -1;
        std::string token = "jd";
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
            return "jd";  // Use default
        }
    } catch (...) {
        return "jd";  // Default on any error
    }
}

int main(int argc, char* argv[])
{
    Engine e(true);  // true = immediate persist mode for CLI
    // Try to get token from running server, otherwise use default "jd"
    std::string token = get_server_token();
    register_all(e, token);

    std::string cmd, args_str;
    bool schema_mode = false;
    bool human_mode  = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if      (a == "--schema")             schema_mode = true;
        else if (a == "--human")              human_mode  = true;
        else if (a == "--cmd"  && i+1 < argc) cmd         = argv[++i];
        else if (a == "--args" && i+1 < argc) args_str    = argv[++i];
        else if (a == "-h" || a == "--help")
        {
            std::cout << "cpp_cli - Command Line Interface\n\n"
                      << "Usage: cpp_cli --cmd <name> --args '{...}' [--human]\n"
                      << "       cpp_cli --schema\n"
                      << "       cpp_cli --help\n\n"
                      << "Options:\n"
                      << "  --cmd <name>      Command to execute\n"
                      << "  --args '{...}'    JSON arguments for the command\n"
                      << "  --human           Human-friendly output format\n"
                      << "  --schema          Show all available commands and their schemas\n"
                      << "  -h, --help        Show this help message\n\n"
                      << "Available Commands and Examples:\n\n"
                      << "  echo - Echo back text\n"
                      << "    cpp_cli --cmd echo --args '{\"text\":\"hello world\"}'\n\n"
                      << "  add - Add two numbers\n"
                      << "    cpp_cli --cmd add --args '{\"a\":10,\"b\":32}'\n\n"
                      << "  upper - Convert text to uppercase\n"
                      << "    cpp_cli --cmd upper --args '{\"text\":\"hello\"}'\n\n"
                      << "  slow_task - Simulate slow operation (async)\n"
                      << "    cpp_cli --cmd slow_task --args '{\"ms\":2000}'\n\n"
#ifdef _WIN32
                      << "  call_shell - Execute shell command (requires token)\n"
                      << "    cpp_cli --cmd call_shell --args '{\"command\":\"where cmd\",\"token\":\"jd\"}'\n"
                      << "    cpp_cli --cmd call_shell --args '{\"command\":\"echo Hello\",\"token\":\"jd\"}'\n\n"
#else
                      << "  call_shell - Execute shell command (requires token)\n"
                      << "    cpp_cli --cmd call_shell --args '{\"command\":\"ls -la\",\"token\":\"jd\"}'\n"
                      << "    cpp_cli --cmd call_shell --args '{\"command\":\"pwd\",\"token\":\"jd\"}'\n\n"
#endif
                      << "  write_json - Write JSON content to file\n"
                      << "    cpp_cli --cmd write_json --args '{\"path\":\"./data/config.json\",\"json_content\":{\"host\":\"localhost\",\"port\":8080}}'\n\n"
                      << "  read_json - Read JSON content from file\n"
                      << "    cpp_cli --cmd read_json --args '{\"path\":\"./data/config.json\"}'\n\n"
                      << "  get_global_json - Get global JSON (requires token)\n"
                      << "    cpp_cli --cmd get_global_json --args '{\"token\":\"jd\"}'\n"
                      << "    cpp_cli --cmd get_global_json --args '{\"path\":\"/user/name\",\"token\":\"jd\"}'\n\n"
                      << "  set_global_json - Replace entire global JSON (requires token)\n"
                      << "    cpp_cli --cmd set_global_json --args '{\"value\":{\"name\":\"Alice\"},\"token\":\"jd\"}'\n\n"
                      << "  patch_global_json - Apply JSON merge patch (RFC 7386)\n"
                      << "    cpp_cli --cmd patch_global_json --args '{\"age\":31,\"city\":\"NYC\"}'\n\n"
                      << "  persist_global_json - Force save global JSON to disk\n"
                      << "    cpp_cli --cmd persist_global_json --args '{}'\n\n"
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

    if (cmd.empty())
    {
        std::cerr << "Usage: cpp_cli --cmd <name> --args '{...}' [--human]\n"
                  << "       cpp_cli --schema\n"
                  << "       cpp_cli --help\n";
        return 1;
    }

    json args;
    if (!args_str.empty())
    {
        try { args = json::parse(args_str); }
        catch (...)
        {
            std::cerr << "{\"code\":1,\"output\":\"\",\"error\":\"invalid JSON in --args\"}\n";
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
