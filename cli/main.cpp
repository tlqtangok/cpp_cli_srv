// cli/main.cpp
// Usage:
//   mytool-cli --schema
//   mytool-cli --cmd echo --args "{\"text\":\"hello\"}"
//   mytool-cli --cmd add  --args "{\"a\":3,\"b\":4}"
//   mytool-cli --cmd echo --args "{\"text\":\"hi\"}" --human
//
#include <iostream>
#include <string>
#include "../core/engine.h"
#include "../core/commands.h"

int main(int argc, char* argv[])
{
    Engine e;
    register_all(e);

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
    }

    if (schema_mode)
    {
        std::cout << e.schema().dump(2) << "\n";
        return 0;
    }

    if (cmd.empty())
    {
        std::cerr << "Usage: mytool-cli --cmd <name> --args '{...}' [--human]\n"
                  << "       mytool-cli --schema\n";
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
