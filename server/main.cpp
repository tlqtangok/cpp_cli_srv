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
#include "../third_party/httplib.h"
#include "../core/engine.h"
#include "../core/commands.h"
#include "../core/logger.h"

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------
static constexpr size_t MAX_BODY_BYTES     = 1024 * 64;  // 64 KB
static constexpr int    DEFAULT_PORT       = 8080;
static constexpr size_t THREAD_POOL_SIZE   = 0;           // 0 = auto (hw threads)
static constexpr int    KEEP_ALIVE_TIMEOUT = 10;          // seconds
static constexpr int    READ_TIMEOUT       = 10;          // seconds
static constexpr int    WRITE_TIMEOUT      = 10;          // seconds
static constexpr int    DEFAULT_TIMEOUT_MS = 30000;       // per-command default

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
                         Logger&          logger)
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
    // POST /post/run
    // Body: { "cmd": "...", "args": {...}, "timeout_ms": 5000 }
    // -------------------------------------------------------------------------
    svr.Post("/post/run", [&e, &active, &logger](const httplib::Request& req, httplib::Response& res)
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
                json err = { {"ok",false},
                            {"message","missing 'cmd' field"},
                            {"data",nullptr} };
                resp_body = err.dump();
                status = 400;
                json_resp(res, err, status);
                logger.log_request("POST", "/post/run", client_ip, req.body, status, resp_body);
                return;
            }

            Result r = e.run(cmd, args, std::chrono::milliseconds(tms));
            resp_body = r.to_json().dump();
            status = r.ok ? 200 : 400;
            json_resp(res, r.to_json(), status);
            logger.log_request("POST", "/post/run", client_ip, req.body, status, resp_body);
        }
        catch (const json::exception& ex)
        {
            json err = { {"ok",false},
                        {"message", std::string("JSON parse error: ") + ex.what()},
                        {"data",nullptr} };
            resp_body = err.dump();
            status = 400;
            json_resp(res, err, status);
            logger.log_request("POST", "/post/run", client_ip, req.body, status, resp_body);
        }
        catch (const std::exception& ex)
        {
            json err = { {"ok",false},
                        {"message", ex.what()},
                        {"data",nullptr} };
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
        json body = {
            {"ok",      true},
            {"threads", (int)threads},
            {"active",  active.load()},
            {"message", "running"}
        };
        json_resp(res, body);
        logger.log_request("GET", "/get/status", client_ip, "", 200, body.dump());
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
    int         port    = DEFAULT_PORT;
    size_t      threads = THREAD_POOL_SIZE;
    bool        no_ipv6 = false;
    std::string logfile = "";

    for (int i = 1; i < argc; ++i)
    {
        std::string flag = argv[i];
        if      (flag == "--port"    && i+1 < argc) port    = std::stoi(argv[++i]);
        else if (flag == "--threads" && i+1 < argc) threads = (size_t)std::stoi(argv[++i]);
        else if (flag == "--no-ipv6")               no_ipv6 = true;
        else if (flag == "--log"     && i+1 < argc) logfile = argv[++i];
    }

    if (threads == 0)
        threads = std::max(1u, std::thread::hardware_concurrency());

    Engine           e;
    std::atomic<int> active{0};
    Logger           logger(logfile);
    register_all(e);
    
    logger.log("Server starting...");

    // --- IPv4 server --------------------------------------------------------
    httplib::Server* svr4 = make_server(threads);
    mount_routes(*svr4, e, active, threads, logger);

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
        mount_routes(*svr6, e, active, threads, logger);

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

    // --- Wait until both are up, then print banner -------------------------
    svr4->wait_until_ready();
    if (svr6) svr6->wait_until_ready();

    logger.log("Server ready - accepting connections");
    
    std::cout << "=== cpp_srv started ===\n"
              << "  IPv4 : http://0.0.0.0:"     << port << "\n";
    if (svr6)
        std::cout << "  IPv6 : http://[::]:" << port << "\n";
    std::cout << "  Threads : " << threads << " per server\n"
              << "  Schema  : GET  /get/schema\n"
              << "  Status  : GET  /get/status\n"
              << "  Run     : POST /post/run\n"
              << "  GUI     : GET  /\n";
    if (!logfile.empty())
        std::cout << "  Log     : " << logfile << "\n";
    std::cout << "  Press Ctrl+C to stop.\n";

    // --- Block main thread until servers exit ------------------------------
    t4.join();
    if (t6.joinable()) t6.join();

    logger.log("Server shutting down");
    delete svr4;
    delete svr6;
    return 0;
}
