#include "Monitor.hpp"
#include "spdlog/spdlog.h"

using namespace httplib;
using json = nlohmann::json;
using namespace std::chrono_literals;

constexpr uint8_t jellyfin_sessions_threshold = 2;

Monitor::Monitor() {
    _is_ok = init();
}

Monitor::~Monitor() {
    stop();
}

bool Monitor::init() {
    auto env = std::getenv("QBITTORRENT_ADDRESS");
    if (env == nullptr) {
        spdlog::error("Failed to read env var QBITTORRENT_ADDRESS. Make sure it's defined.");
        return false;
    }
    _qbittorrent_addr = std::string(env);
    _qbittorrent_client = std::make_shared<Client>(_qbittorrent_addr);
    spdlog::info("qbittorrent address : {}", _qbittorrent_addr);

    env = std::getenv("JELLYFIN_ADDRESS");
    if (env == nullptr) {
        spdlog::error("Failed to read env var JELLYFIN_ADDRESS. Make sure it's defined.");
        return false;
    }
    _jellyfin_addr = std::string(env);
    _jellyfin_client = std::make_shared<Client>(_jellyfin_addr);
    spdlog::info("Jellyfin address : {}", _jellyfin_addr);

    env = std::getenv("JELLYFIN_API_KEY");
    if (env == nullptr) {
        spdlog::error("Failed to read env var JELLYFIN_API_KEY. Make sure it's defined.");
        return false;
    }
    _jellyfin_api_key = std::string(env);
    spdlog::info("Jellyfin API key : {}", _jellyfin_api_key);

    register_server_routes();
    _server_thread = std::jthread([this] {
        _server.listen("0.0.0.0", 4001);
    });

    return true;
}

void Monitor::stop() {
    _exiting = true;
    stop_jellyfin_monitoring();
    _server.stop();
}

bool Monitor::is_ok() const {
    return _is_ok;
}

void Monitor::pause_torrents() {
    spdlog::info("Pausing torrents");
    manage_torrents("pause");
    _torrents_paused = true;
}

void Monitor::resume_torrents() {
    spdlog::info("Resuming torrents");
    manage_torrents("resume");
    _torrents_paused = false;
}

void Monitor::manage_torrents(const std::string &method) {
#ifdef DEBUG
    spdlog::debug("Sending POST to /api/v2/torrents/" + method);
    return;
#endif
    Params p;
    p.emplace("hashes", "all");
    auto res = _qbittorrent_client->Post("/api/v2/torrents/" + method, p);

    if (auto err = res.error(); err != Error::Success) {
        spdlog::error(to_string(err));
    }
    else {
        spdlog::debug("qbitorrent : {} {}", res.value().status, res.value().body);
    }
}

void Monitor::start_jellyfin_monitoring() {
    spdlog::debug("Starting jellyfin monitoring");
    _jellyfin_monitor_thread = std::jthread([this] {
        while (!_exiting && _jellyfin_sessions_active) {
            std::unique_lock lock(_monitor_mutex);
            _jellyfin_monitor_cv.wait_for(lock, 2s, [this] { return _exiting.load(); });

            if (_exiting) {
                spdlog::debug("Breaking from jellyfin monitor loop");
                break;
            }

            monitor_jellyfin();
        }
        spdlog::debug("Stopped jellyfin monitoring");
    });
    spdlog::debug("Started jellyfin monitoring");
}

void Monitor::stop_jellyfin_monitoring() {
    spdlog::debug("Stopping jellyfin monitor");
    _jellyfin_monitor_cv.notify_one();
}

void Monitor::monitor_jellyfin() {
    // _monitor_mutex is locked here
    auto sessions = get_jellyfin_sessions();
    if (sessions.is_null()) {
        return;
    }

    auto n_sessions = sessions.size();
    spdlog::debug("{} active sessions", n_sessions);

    if (_torrents_paused && n_sessions < jellyfin_sessions_threshold) {
        resume_torrents();
    }
    else if (!_torrents_paused && n_sessions >= jellyfin_sessions_threshold) {
        pause_torrents();
    }

    _jellyfin_sessions_active = n_sessions > 0;
}

json Monitor::get_jellyfin_sessions() {
    auto res = _jellyfin_client->Get("/Sessions?api_key=" + _jellyfin_api_key);
    if (auto err = res.error(); err != Error::Success) {
        spdlog::error("Failed to get jellyfin sessions : {}", to_string(err));
        return json();
    }
    else if (res.value().status == 401) {
        spdlog::error("Failed to authenticate to jellyfin API, check API key");
        return json();
    }

    try {
        return json::parse(res.value().body);
    }
    catch (json::parse_error &e) {
        spdlog::error("Failed to parse json from jellyfin : {}", e.what());
        return json();
    }
}

void Monitor::register_server_routes() {
    _server.set_pre_routing_handler([this](const Request &req, Response &res) {
        spdlog::debug("{} {}", req.method, req.path);
        return Server::HandlerResponse::Unhandled;
    });

    _server.Post("/api/session_start", [this](const Request &req, Response &res) {
        post_session_start(req, res);
    });

    spdlog::info("Registered HTTP server routes");
}

void Monitor::post_session_start(const Request &req, Response &res) {
    std::lock_guard lock(_monitor_mutex);

    if (!_jellyfin_sessions_active) {
        spdlog::debug("Jellyfin session start");
        _jellyfin_sessions_active = true;
        start_jellyfin_monitoring();
    }
}