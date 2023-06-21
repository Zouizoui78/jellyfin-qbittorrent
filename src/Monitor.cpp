#include "Monitor.hpp"
#include "spdlog/spdlog.h"

using namespace httplib;
using json = nlohmann::json;
using namespace std::chrono_literals;

Monitor::Monitor() :
    _qbittorrent_client(_qbittorrent_addr),
    _jellyfin_client(_jellyfin_addr)
{
    _is_ok = init();
}

Monitor::~Monitor() {
    stop();
}

bool Monitor::init() {
    auto api_key = std::getenv("JELLYFIN_API_KEY");
    if (api_key == nullptr) {
        spdlog::error("Failed to read env var JELLYFIN_API_KEY. Make sure it's defined.");
        return false;
    }

    _jellyfin_api_key = std::string(api_key);

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
}

void Monitor::resume_torrents() {
    spdlog::info("Resuming torrents");
    manage_torrents("resume");
}

void Monitor::manage_torrents(const std::string &method) {
    Params p;
    p.emplace("hashes", "all");
    auto res = _qbittorrent_client.Post("/api/v2/torrents/" + method, p);

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
        while (_jellyfin_sessions_active && !_exiting) {
            std::unique_lock lock(_jellyfin_monitor_mutex);
            _jellyfin_monitor_cv.wait_for(lock, 5s, [this] { return _exiting.load(); });

            if (_exiting) {
                break;
            }

            monitor_jellyfin();
        }
        spdlog::debug("Stopped jellyfin monitoring");
    });
    spdlog::debug("Started jellyfin monitoring");
}

void Monitor::stop_jellyfin_monitoring() {
    _jellyfin_monitor_cv.notify_one();
}

void Monitor::monitor_jellyfin() {
    spdlog::debug("Monitoring jellyfin");
    auto sessions = get_jellyfin_sessions();
    if (sessions.is_discarded()) {
        spdlog::error("Failed to parse json from jellyfin");
        return;
    }

    if (sessions.empty()) {
        _jellyfin_sessions_active = false;
        {
            std::lock_guard lock(_qbittorrent_mutex);
            resume_torrents();
        }
    }
}

json Monitor::get_jellyfin_sessions() {
    auto res = _jellyfin_client.Get("/Sessions?api_key=" + _jellyfin_api_key);
    if (auto err = res.error(); err != Error::Success) {
        spdlog::error("Failed to get jellyfin sessions : {}", to_string(err));
        return json::value_t::discarded;
    }
    else {
        return json::parse(res.value().body, nullptr, false);
    }
}

void Monitor::register_server_routes() {
    _server.set_pre_routing_handler([this](const Request &req, Response &res) {
        spdlog::debug("{} {}", req.method, req.path);
        return Server::HandlerResponse::Unhandled;
    });

    _server.Post("/api/session_start", [this](const Request &req, Response &res) {
        if (_jellyfin_sessions_active.load()) {
            return;
        }

        spdlog::info("Jellyfin session start");
        _jellyfin_sessions_active = true;
        {
            std::lock_guard lock(_qbittorrent_mutex);
            pause_torrents();
            start_jellyfin_monitoring();
        }
    });

    spdlog::info("Registered HTTP server routes");
}