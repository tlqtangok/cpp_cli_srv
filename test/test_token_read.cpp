#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <unistd.h>
#include <signal.h>

bool is_process_running(int pid)
{
    return kill(pid, 0) == 0;
}

std::string get_server_token()
{
    const std::string srv_file = "data/srv_argc_argv.txt";
    
    std::cout << "Checking file: " << srv_file << std::endl;
    std::cout << "File exists: " << std::filesystem::exists(srv_file) << std::endl;
    
    if (!std::filesystem::exists(srv_file)) {
        return "jd";
    }
    
    std::ifstream file(srv_file);
    if (!file.is_open()) {
        std::cout << "Cannot open file" << std::endl;
        return "jd";
    }
    
    int pid = -1;
    std::string token = "jd";
    std::string line;
    
    while (std::getline(file, line)) {
        std::cout << "Read line: " << line << std::endl;
        if (line.substr(0, 4) == "PID=") {
            pid = std::stoi(line.substr(4));
            std::cout << "Parsed PID: " << pid << std::endl;
        } else if (line.substr(0, 6) == "TOKEN=") {
            token = line.substr(6);
            std::cout << "Parsed TOKEN: " << token << std::endl;
        }
    }
    file.close();
    
    if (pid > 0) {
        bool running = is_process_running(pid);
        std::cout << "Process " << pid << " running: " << running << std::endl;
        if (running) {
            return token;
        } else {
            std::filesystem::remove(srv_file);
            return "jd";
        }
    }
    
    return "jd";
}

int main()
{
    std::string token = get_server_token();
    std::cout << "Final token: " << token << std::endl;
    return 0;
}
