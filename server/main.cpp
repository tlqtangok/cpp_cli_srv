// server/main.cpp
// API:
//   GET  /get/schema   -> command list JSON
//   GET  /get/status   -> health check + concurrency info
//   POST /post/run     -> body: {"cmd":"...", "args":{...}, "timeout_ms": 5000}
//   GET  /             -> web/index.html
//
// Concurrency model:
//   - Two Server instances: IPv4 (0.0.0.0) + IPv6 (::), each on its own thread
//   - Both share the same Engine (thread-safe via shared_mutex)
//   - Each server has its own ThreadPool (default: CPU cores, max 4x cores)
//   - Slow (async) handlers run on std::async threads, waited with per-request timeout
//
// Note: do NOT define CPPHTTPLIB_OPENSSL_SUPPORT (uses #ifdef, not #if)
#include <iostream>
#include <fstream>
#include <sstream>
#include <atomic>
#include <thread>
#include <filesystem>
#include <csignal>
#include <algorithm>
#include <vector>
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif
#include "../third_party/httplib.h"
#include "../core/engine.h"
#include "../core/commands.h"
#include "../core/logger.h"

#ifndef BUILD_TIME
#define BUILD_TIME "unknown"
#endif
#ifndef GIT_COMMIT_ID
#define GIT_COMMIT_ID "unknown"
#endif
static const std::string VERSION = std::string(BUILD_TIME) + "-" + GIT_COMMIT_ID;

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------
static constexpr size_t MAX_BODY_BYTES     = 2*1024 * 1024;  // 2M 
static constexpr int    DEFAULT_PORT       = 8080;
static constexpr size_t THREAD_POOL_SIZE   = 0;           // 0 = auto (hw threads)
static constexpr int    KEEP_ALIVE_TIMEOUT = 10;          // seconds
static constexpr int    READ_TIMEOUT       = 10;          // seconds
static constexpr int    WRITE_TIMEOUT      = 10;          // seconds
static constexpr int    DEFAULT_TIMEOUT_MS = 30000;       // per-command default

// ---------------------------------------------------------------------------
// Signal handler for cleanup
// ---------------------------------------------------------------------------
static Engine* g_engine_ptr = nullptr;  // Global pointer for signal handler

static void cleanup_server_info()
{
    try {
        std::filesystem::remove("data/srv_argc_argv.txt");
    } catch (...) {
        // Ignore errors during cleanup
    }
}

static void signal_handler(int signum)
{
    // Save global JSON before exit (prevents data loss on Ctrl+C)
    if (g_engine_ptr) {
        try {
            g_engine_ptr->save_global_json();
        } catch (...) {
            // Ignore errors during shutdown
        }
    }
    
    cleanup_server_info();
    std::exit(signum);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string load_file(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void cors(httplib::Response& res)
{
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

static void json_resp(httplib::Response& res, const json& body, int status = 200)
{
    cors(res);
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

static std::string get_client_ip(const httplib::Request& req)
{
    // Try X-Forwarded-For header first (for proxies)
    if (req.has_header("X-Forwarded-For"))
        return req.get_header_value("X-Forwarded-For");
    
    // Fall back to remote address
    return req.remote_addr;
}

// ---------------------------------------------------------------------------
// mount_routes
// Attach all route handlers to a Server instance.
// Engine& and active counter are captured by reference - both are safe
// because they outlive both server threads.
// ---------------------------------------------------------------------------
static void mount_routes(httplib::Server& svr,
                         Engine&          e,
                         std::atomic<int>& active,
                         size_t           threads,
                         Logger&          logger,
                         const std::string& token)
{
    // -------------------------------------------------------------------------
    // GET /get/schema
    // -------------------------------------------------------------------------
    svr.Get("/get/schema", [&e, &logger](const httplib::Request& req, httplib::Response& res)
    {
        std::string client_ip = get_client_ip(req);
        json body = e.schema();
        json_resp(res, body);
        logger.log_request("GET", "/get/schema", client_ip, "", 200, body.dump());
    });

    // -------------------------------------------------------------------------
    // GET /get/token
    // Returns server token for Web GUI authentication
    // Security: Should only be accessible from same origin (CORS protected)
    // -------------------------------------------------------------------------
    svr.Get("/get/token", [&logger, &token](const httplib::Request& req, httplib::Response& res)
    {
        std::string client_ip = get_client_ip(req);
        json body = { {"token", token} };
        json_resp(res, body);
        logger.log_request("GET", "/get/token", client_ip, "", 200, body.dump());
    });

    // -------------------------------------------------------------------------
    // POST /post/run
    // Body: { "cmd": "...", "args": {...}, "timeout_ms": 5000 }
    // For authenticated commands, args must include: { "token": "..." }
    // -------------------------------------------------------------------------
    svr.Post("/post/run", [&e, &active, &logger, &token](const httplib::Request& req, httplib::Response& res)
    {
        std::string client_ip = get_client_ip(req);
        std::string resp_body;
        int status = 200;

        if (req.body.size() > MAX_BODY_BYTES)
        {
            json err = { {"ok",false},
                        {"message","request body too large"},
                        {"data",nullptr} };
            resp_body = err.dump();
            status = 413;
            json_resp(res, err, status);
            logger.log_request("POST", "/post/run", client_ip, req.body, status, resp_body);
            return;
        }

        ++active;
        struct Guard { std::atomic<int>& c; ~Guard(){--c;} } g{active};

        try
        {
            auto        body = json::parse(req.body);
            std::string cmd  = body.value("cmd", "");
            json        args = body.value("args", json::object());
            int         tms  = body.value("timeout_ms", DEFAULT_TIMEOUT_MS);

            if (cmd.empty())
            {
                json err = { {"code", 1}, {"output", ""}, {"error", "missing 'cmd' field"} };
                resp_body = err.dump();
                status = 400;
                json_resp(res, err, status);
                logger.log_request("POST", "/post/run", client_ip, req.body, status, resp_body);
                return;
            }

            Result r = e.run(cmd, args, std::chrono::milliseconds(tms));
            resp_body = r.to_json().dump();
            status = r.code == 0 ? 200 : 400;
            json_resp(res, r.to_json(), status);
            logger.log_request("POST", "/post/run", client_ip, req.body, status, resp_body);
        }
        catch (const json::exception& ex)
        {
            json err = { {"code", 4}, {"output", ""}, {"error", std::string("JSON parse error: ") + ex.what()} };
            resp_body = err.dump();
            status = 400;
            json_resp(res, err, status);
            logger.log_request("POST", "/post/run", client_ip, req.body, status, resp_body);
        }
        catch (const std::exception& ex)
        {
            json err = { {"code", 5}, {"output", ""}, {"error", ex.what()} };
            resp_body = err.dump();
            status = 500;
            json_resp(res, err, status);
            logger.log_request("POST", "/post/run", client_ip, req.body, status, resp_body);
        }
    });

    // -------------------------------------------------------------------------
    // GET /get/status
    // -------------------------------------------------------------------------
    svr.Get("/get/status", [&active, threads, &logger](const httplib::Request& req, httplib::Response& res)
    {
        std::string client_ip = get_client_ip(req);
        json status_data = {
            {"threads", (int)threads},
            {"active",  active.load()},
            {"status", "running"}
        };
        json body = { {"code", 0}, {"output", status_data.dump()}, {"error", ""} };
        json_resp(res, body);
        logger.log_request("GET", "/get/status", client_ip, "", 200, body.dump());
    });

    // -------------------------------------------------------------------------
    // GET /get/global - Get entire global JSON
    // -------------------------------------------------------------------------
    svr.Get("/get/global", [&e, &logger](const httplib::Request& req, httplib::Response& res)
    {
        std::string client_ip = get_client_ip(req);
        try {
            json global_json = e.get_global_json();
            json body = { {"code", 0}, {"output", global_json}, {"error", ""} };
            json_resp(res, body);
            logger.log_request("GET", "/get/global", client_ip, "", 200, body.dump());
        } catch (const std::exception& ex) {
            json err = { {"code", 5}, {"output", ""}, {"error", ex.what()} };
            json_resp(res, err, 500);
            logger.log_request("GET", "/get/global", client_ip, "", 500, err.dump());
        }
    });

    // -------------------------------------------------------------------------
    // GET /get/global/path - Get value at JSON pointer path
    // Query parameter: ?path=/a/b/c
    // -------------------------------------------------------------------------
    svr.Get("/get/global/path", [&e, &logger](const httplib::Request& req, httplib::Response& res)
    {
        std::string client_ip = get_client_ip(req);
        try {
            std::string path = req.get_param_value("path");
            if (path.empty()) {
                json err = { {"code", 1}, {"output", ""}, {"error", "path parameter is required"} };
                json_resp(res, err, 400);
                logger.log_request("GET", "/get/global/path", client_ip, "", 400, err.dump());
                return;
            }
            
            json value = e.get_global_json_path(path);
            if (value.is_null()) {
                json err = { {"code", 1}, {"output", ""}, {"error", "path not found: " + path} };
                json_resp(res, err, 404);
                logger.log_request("GET", "/get/global/path", client_ip, "?path=" + path, 404, err.dump());
                return;
            }
            
            json body = { {"code", 0}, {"output", value}, {"error", ""} };
            json_resp(res, body);
            logger.log_request("GET", "/get/global/path", client_ip, "?path=" + path, 200, body.dump());
        } catch (const std::exception& ex) {
            json err = { {"code", 5}, {"output", ""}, {"error", ex.what()} };
            json_resp(res, err, 500);
            logger.log_request("GET", "/get/global/path", client_ip, "", 500, err.dump());
        }
    });

    // -------------------------------------------------------------------------
    // POST /post/global - Set entire global JSON (TOKEN REQUIRED)
    // Body: { "value": {...}, "token": "..." }
    // -------------------------------------------------------------------------
    svr.Post("/post/global", [&e, &logger, &token](const httplib::Request& req, httplib::Response& res)
    {
        std::string client_ip = get_client_ip(req);
        std::string resp_body;
        int status = 200;
        
        try {
            auto body = json::parse(req.body);
            
            // Token authentication (same as call_shell)
            if (!token.empty()) {
                std::string req_token = body.value("token", "");
                if (req_token != token) {
                    json err = { {"code", 6}, {"output", ""}, {"error", "set_global_json requires valid token"} };
                    resp_body = err.dump();
                    status = 403;
                    json_resp(res, err, status);
                    logger.log_request("POST", "/post/global", client_ip, req.body, status, resp_body);
                    return;
                }
            }
            
            if (!body.contains("value")) {
                json err = { {"code", 1}, {"output", ""}, {"error", "value field is required"} };
                resp_body = err.dump();
                status = 400;
                json_resp(res, err, status);
                logger.log_request("POST", "/post/global", client_ip, req.body, status, resp_body);
                return;
            }
            
            e.set_global_json(body["value"]);
            json result = { {"code", 0}, {"output", "Global JSON updated"}, {"error", ""} };
            resp_body = result.dump();
            json_resp(res, result);
            logger.log_request("POST", "/post/global", client_ip, req.body, status, resp_body);
        } catch (const json::exception& ex) {
            json err = { {"code", 4}, {"output", ""}, {"error", std::string("JSON parse error: ") + ex.what()} };
            resp_body = err.dump();
            status = 400;
            json_resp(res, err, status);
            logger.log_request("POST", "/post/global", client_ip, req.body, status, resp_body);
        } catch (const std::exception& ex) {
            json err = { {"code", 5}, {"output", ""}, {"error", ex.what()} };
            resp_body = err.dump();
            status = 500;
            json_resp(res, err, status);
            logger.log_request("POST", "/post/global", client_ip, req.body, status, resp_body);
        }
    });

    // -------------------------------------------------------------------------
    // POST /post/global/patch - Apply JSON merge patch
    // Body: { "key": "value", ... } - entire body is the patch
    // Returns: { "code": 0, "output": {"before":{...}, "after":{...}, "patch_applied":{...}}, "error": "" }
    // -------------------------------------------------------------------------
    svr.Post("/post/global/patch", [&e, &logger](const httplib::Request& req, httplib::Response& res)
    {
        std::string client_ip = get_client_ip(req);
        std::string resp_body;
        int status = 200;
        
        try {
            auto body = json::parse(req.body);
            
            // Use entire body as patch (simplified API)
            json diff = e.patch_global_json(body);
            json result = { {"code", 0}, {"output", diff}, {"error", ""} };
            resp_body = result.dump();
            json_resp(res, result);
            logger.log_request("POST", "/post/global/patch", client_ip, req.body, status, resp_body);
        } catch (const json::exception& ex) {
            json err = { {"code", 4}, {"output", ""}, {"error", std::string("JSON parse error: ") + ex.what()} };
            resp_body = err.dump();
            status = 400;
            json_resp(res, err, status);
            logger.log_request("POST", "/post/global/patch", client_ip, req.body, status, resp_body);
        } catch (const std::exception& ex) {
            json err = { {"code", 5}, {"output", ""}, {"error", ex.what()} };
            resp_body = err.dump();
            status = 500;
            json_resp(res, err, status);
            logger.log_request("POST", "/post/global/patch", client_ip, req.body, status, resp_body);
        }
    });

    // -------------------------------------------------------------------------
    // POST /post/global/persist - Force save to disk
    // -------------------------------------------------------------------------
    svr.Post("/post/global/persist", [&e, &logger](const httplib::Request& req, httplib::Response& res)
    {
        std::string client_ip = get_client_ip(req);
        try {
            e.save_global_json();
            json result = { {"code", 0}, {"output", "Global JSON persisted to disk"}, {"error", ""} };
            json_resp(res, result);
            logger.log_request("POST", "/post/global/persist", client_ip, "", 200, result.dump());
        } catch (const std::exception& ex) {
            json err = { {"code", 5}, {"output", ""}, {"error", ex.what()} };
            json_resp(res, err, 500);
            logger.log_request("POST", "/post/global/persist", client_ip, "", 500, err.dump());
        }
    });

    // -------------------------------------------------------------------------
    // OPTIONS (CORS preflight)
    // -------------------------------------------------------------------------
    svr.Options(".*", [&logger](const httplib::Request& req, httplib::Response& res)
    {
        std::string client_ip = get_client_ip(req);
        cors(res);
        res.status = 204;
        logger.log_request("OPTIONS", req.path, client_ip, "", 204, "");
    });

    // -------------------------------------------------------------------------
    // GET /  (static GUI)
    // -------------------------------------------------------------------------
    svr.Get("/", [&logger](const httplib::Request& req, httplib::Response& res)
    {
        std::string client_ip = get_client_ip(req);
        std::string html = load_file("web/index.html");
        int status = 200;
        if (html.empty()) 
        {
            html = "<h1>web/index.html not found</h1>";
            status = 404;
        }
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(html, "text/html; charset=utf-8");
        res.status = status;
        logger.log_request("GET", "/", client_ip, "", status, "[HTML " + std::to_string(html.size()) + " bytes]");
    });
}

// ---------------------------------------------------------------------------
// Token validation helper
// ---------------------------------------------------------------------------
static bool is_valid_token(const std::string& token)
{
    if (token.length() < 2) return false;
    for (char c : token) {
        if (!std::isalnum(c) && c != '_') return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// make_server
// Create and configure a server instance (thread pool + timeouts).
// ---------------------------------------------------------------------------
static httplib::Server* make_server(size_t threads)
{
    auto* svr = new httplib::Server();

    svr->new_task_queue = [threads]
    {
        return new httplib::ThreadPool(threads, threads * 4);
    };

    svr->set_keep_alive_max_count(100);
    svr->set_keep_alive_timeout(KEEP_ALIVE_TIMEOUT);
    svr->set_read_timeout(READ_TIMEOUT,  0);
    svr->set_write_timeout(WRITE_TIMEOUT, 0);

    return svr;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Register signal handlers for cleanup
    std::signal(SIGINT, signal_handler);   // Ctrl+C
    std::signal(SIGTERM, signal_handler);  // kill command
    
    int         port        = DEFAULT_PORT;
    int         port_https  = 0;              // 0 = HTTPS disabled
    size_t      threads     = THREAD_POOL_SIZE;
    bool        no_ipv6     = false;
    std::string logfile     = "";
    std::string token       = "jd";           // Default token for security
    std::string ssl_dir     = "";             // SSL certificate directory

    for (int i = 1; i < argc; ++i)
    {
        std::string flag = argv[i];
        if      (flag == "--port"       && i+1 < argc) port       = std::stoi(argv[++i]);
        else if (flag == "--port_https" && i+1 < argc) port_https = std::stoi(argv[++i]);
        else if (flag == "--threads"    && i+1 < argc) threads    = (size_t)std::stoi(argv[++i]);
        else if (flag == "--no-ipv6")                  no_ipv6    = true;
        else if (flag == "--log"        && i+1 < argc) logfile    = argv[++i];
        else if (flag == "--token"      && i+1 < argc) token      = argv[++i];
        else if (flag == "--ssl"        && i+1 < argc) ssl_dir    = argv[++i];
        else if (flag == "-v" || flag == "--version")
        {
            std::cout << "cpp_srv " << VERSION << "\n";
            return 0;
        }
        else if (flag == "-h" || flag == "--help")
        {
            std::cout << "cpp_srv - C++ CLI/HTTP Server\n\n"
                      << "Usage: cpp_srv [options]\n\n"
                      << "Options:\n"
                      << "  --port <number>       HTTP server port (default: 8080)\n"
                      << "  --port_https <number> HTTPS server port (requires --ssl, default: disabled)\n"
                      << "  --ssl <directory>     SSL certificate directory (must contain server.crt and server.key)\n"
                      << "  --threads <number>    Thread pool size (default: hardware_concurrency)\n"
                      << "  --no-ipv6             Disable IPv6 server (IPv4 only)\n"
                      << "  --log <file>          Enable logging to file\n"
                      << "  --token <string>      Security token for call_shell command (min 2 chars, alphanumeric + underscore)\n"
                      << "  -v, --version         Show version (build time + git commit)\n"
                      << "  -h, --help            Show this help message\n\n"
                      << "Examples:\n"
                      << "  cpp_srv\n"
                      << "  cpp_srv --port 8080 --port_https 8443 --ssl /path/to/ssl\n"
                      << "  cpp_srv --port 9090 --threads 8\n"
                      << "  cpp_srv --port 8080 --log server.log --token mytoken123\n"
                      << "  cpp_srv --no-ipv6 --port 8080\n\n"
                      << "Once started, access:\n"
                      << "  Web GUI (HTTP):  http://localhost:8080/\n"
                      << "  Web GUI (HTTPS): https://localhost:8443/\n"
                      << "  Schema:  http://localhost:8080/get/schema\n"
                      << "  Status:  http://localhost:8080/get/status\n";
            return 0;
        }
    }

    // Validate token if provided
    if (!token.empty() && !is_valid_token(token))
    {
        std::cerr << "Error: token must be at least 2 characters and contain only letters, numbers, or underscore\n";
        return 1;
    }

    // Validate HTTPS configuration
    if (port_https > 0 && ssl_dir.empty())
    {
        std::cerr << "Error: --port_https requires --ssl <directory>\n";
        return 1;
    }
    
    if (!ssl_dir.empty() && port_https == 0)
    {
        std::cerr << "Error: --ssl requires --port_https <port>\n";
        return 1;
    }

    // Validate SSL files exist
    if (port_https > 0)
    {
        std::string cert_file = ssl_dir + "/server.crt";
        std::string key_file  = ssl_dir + "/server.key";
        
        std::ifstream cert(cert_file);
        std::ifstream key(key_file);
        
        if (!cert.good())
        {
            std::cerr << "Error: SSL certificate not found: " << cert_file << "\n";
            return 1;
        }
        if (!key.good())
        {
            std::cerr << "Error: SSL key not found: " << key_file << "\n";
            return 1;
        }
        
        cert.close();
        key.close();
    }

    if (threads == 0)
        threads = std::max(1u, std::thread::hardware_concurrency());

    Engine           e;
    g_engine_ptr = &e;  // Set global pointer for signal handler
    std::atomic<int> active{0};
    Logger           logger(logfile);
    register_all(e, token);  // Pass token to register commands
    
    logger.log("Server starting...");

    // --- IPv4 server --------------------------------------------------------
    httplib::Server* svr4 = make_server(threads);
    mount_routes(*svr4, e, active, threads, logger, token);

    if (!svr4->bind_to_port("0.0.0.0", port))
    {
        logger.log("ERROR: Cannot bind IPv4 0.0.0.0:" + std::to_string(port));
        std::cerr << "[ERROR] Cannot bind IPv4 0.0.0.0:" << port << "\n";
        delete svr4;
        return 1;
    }
    logger.log("IPv4 server bound to 0.0.0.0:" + std::to_string(port));

    std::thread t4([svr4]()
    {
        svr4->listen_after_bind();
    });

    // --- IPv6 server (optional) ---------------------------------------------
    httplib::Server* svr6 = nullptr;
    std::thread      t6;

    if (!no_ipv6)
    {
        svr6 = make_server(threads);
        // IPV6_V6ONLY = true: bind only IPv6, don't overlap with IPv4
        svr6->set_ipv6_v6only(true);
        mount_routes(*svr6, e, active, threads, logger, token);

        if (!svr6->bind_to_port("::", port))
        {
            logger.log("WARN: Cannot bind IPv6 [::]:" + std::to_string(port) + " - running IPv4 only");
            std::cerr << "[WARN] Cannot bind IPv6 [::]:" << port
                      << " - running IPv4 only\n";
            delete svr6;
            svr6 = nullptr;
        }
        else
        {
            logger.log("IPv6 server bound to [::]:" + std::to_string(port));
            t6 = std::thread([svr6]()
            {
                svr6->listen_after_bind();
            });
        }
    }

    // --- HTTPS server (optional) --------------------------------------------
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLServer* svr_https4 = nullptr;
    httplib::SSLServer* svr_https6 = nullptr;
    std::thread         t_https4;
    std::thread         t_https6;

    if (port_https > 0)
    {
        std::string cert_file = ssl_dir + "/server.crt";
        std::string key_file  = ssl_dir + "/server.key";
        
        logger.log("Starting HTTPS server on port " + std::to_string(port_https));
        
        // HTTPS IPv4
        svr_https4 = new httplib::SSLServer(cert_file.c_str(), key_file.c_str());
        svr_https4->new_task_queue = [threads]()
        {
            return new httplib::ThreadPool(threads, threads * 4);
        };
        svr_https4->set_read_timeout(5, 0);
        svr_https4->set_write_timeout(5, 0);
        svr_https4->set_payload_max_length(MAX_BODY_BYTES);
        mount_routes(*svr_https4, e, active, threads, logger, token);
        
        if (!svr_https4->bind_to_port("0.0.0.0", port_https))
        {
            logger.log("ERROR: Cannot bind HTTPS IPv4 0.0.0.0:" + std::to_string(port_https));
            std::cerr << "[ERROR] Cannot bind HTTPS IPv4 0.0.0.0:" << port_https << "\n";
            delete svr_https4;
            svr_https4 = nullptr;
        }
        else
        {
            logger.log("HTTPS IPv4 server bound to 0.0.0.0:" + std::to_string(port_https));
            t_https4 = std::thread([svr_https4]()
            {
                svr_https4->listen_after_bind();
            });
        }
        
        // HTTPS IPv6 (if not disabled)
        if (!no_ipv6 && svr_https4)
        {
            svr_https6 = new httplib::SSLServer(cert_file.c_str(), key_file.c_str());
            svr_https6->new_task_queue = [threads]()
            {
                return new httplib::ThreadPool(threads, threads * 4);
            };
            svr_https6->set_read_timeout(5, 0);
            svr_https6->set_write_timeout(5, 0);
            svr_https6->set_payload_max_length(MAX_BODY_BYTES);
            svr_https6->set_ipv6_v6only(true);
            mount_routes(*svr_https6, e, active, threads, logger, token);
            
            if (!svr_https6->bind_to_port("::", port_https))
            {
                logger.log("WARN: Cannot bind HTTPS IPv6 [::]:" + std::to_string(port_https) + " - running IPv4 only");
                std::cerr << "[WARN] Cannot bind HTTPS IPv6 [::]:" << port_https
                          << " - running IPv4 only\n";
                delete svr_https6;
                svr_https6 = nullptr;
            }
            else
            {
                logger.log("HTTPS IPv6 server bound to [::]:" + std::to_string(port_https));
                t_https6 = std::thread([svr_https6]()
                {
                    svr_https6->listen_after_bind();
                });
            }
        }
    }
#else
    if (port_https > 0)
    {
        std::cerr << "[ERROR] HTTPS requested but OpenSSL support not compiled in\n";
        std::cerr << "        Rebuild with -DCPPHTTPLIB_OPENSSL_SUPPORT and link OpenSSL\n";
        logger.log("ERROR: HTTPS requested but OpenSSL support not available");
        return 1;
    }
#endif

    // --- Wait until all servers are up, then print banner ------------------
    svr4->wait_until_ready();
    if (svr6) svr6->wait_until_ready();
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    if (svr_https4) svr_https4->wait_until_ready();
    if (svr_https6) svr_https6->wait_until_ready();
#endif

    logger.log("Server ready - accepting connections");
    
    // Write server startup info to file for CLI token sync
    try {
        std::filesystem::create_directories("data");
        std::ofstream srv_info("data/srv_argc_argv.txt");
        if (srv_info.is_open()) {
            // Write PID for verification
            srv_info << "PID=" << getpid() << "\n";
            // Write token
            srv_info << "TOKEN=" << token << "\n";
            // Write command line
            srv_info << "CMDLINE=";
            for (int i = 0; i < argc; ++i) {
                if (i > 0) srv_info << " ";
                srv_info << argv[i];
            }
            srv_info << "\n";
            srv_info.close();
        }
    } catch (...) {
        // Ignore errors - this is not critical
    }
    
    std::cout << "=== cpp_srv started ===\n"
              << "  IPv4 (HTTP)  : http://0.0.0.0:"     << port << "\n";
    if (svr6)
        std::cout << "  IPv6 (HTTP)  : http://[::]:" << port << "\n";
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    if (svr_https4)
        std::cout << "  IPv4 (HTTPS) : https://0.0.0.0:" << port_https << "\n";
    if (svr_https6)
        std::cout << "  IPv6 (HTTPS) : https://[::]:" << port_https << "\n";
#endif
    std::cout << "  Threads : " << threads << " per server\n"
              << "  Schema  : GET  /get/schema\n"
              << "  Status  : GET  /get/status\n"
              << "  Run     : POST /post/run\n"
              << "  GUI     : GET  /\n";
    if (!logfile.empty())
        std::cout << "  Log     : " << logfile << "\n";
    if (port_https > 0)
        std::cout << "  SSL     : " << ssl_dir << "\n";
    std::cout << "  Press Ctrl+C to stop.\n";

    // --- Block main thread until servers exit ------------------------------
    t4.join();
    if (t6.joinable()) t6.join();
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    if (t_https4.joinable()) t_https4.join();
    if (t_https6.joinable()) t_https6.join();
#endif

    logger.log("Server shutting down");
    
    // Clean up server info file
    cleanup_server_info();
    
    delete svr4;
    if (svr6) delete svr6;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    if (svr_https4) delete svr_https4;
    if (svr_https6) delete svr_https6;
#endif
    return 0;
}
