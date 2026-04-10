#include "third_party/json.hpp"
#include <iostream>
using json = nlohmann::json;

int main() {
    // Simulate what read_json returns
    std::string output = "{\"test\":\"value\"}";
    
    // Test to_json() logic
    json result;
    result["code"] = 0;
    
    if (!output.empty() && (output[0] == '{' || output[0] == '[')) {
        try {
            result["output"] = json::parse(output);
            std::cout << "Parsed as JSON object\n";
        } catch (...) {
            result["output"] = output;
            std::cout << "Kept as string\n";
        }
    } else {
        result["output"] = output;
    }
    
    result["error"] = "";
    
    std::cout << "Result: " << result.dump() << "\n";
    std::cout << "Pretty: " << result.dump(2) << "\n";
    
    return 0;
}
