#include "Monitor.hpp"
#include "spdlog/spdlog.h"

using namespace httplib;

Monitor::Monitor() :
    _qbittorrent_client(_qbittorrent_addr)
{
    register_server_routes();
    _server_thread = std::thread([this] {
        _server.listen("0.0.0.0", 4001);
    });
}

Monitor::~Monitor() {
    _server.stop();
    _server_thread.join();
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
        spdlog::info("qbitorrent : {} {}", res.value().status, res.value().body);
    }
}

void Monitor::register_server_routes() {
    _server.set_pre_routing_handler([this](const Request &req, Response &res) {
        spdlog::debug("{} {}", req.method, req.path);
        return Server::HandlerResponse::Unhandled;
    });

    _server.Post("/api/session_start", [this](const Request &req, Response &res) {
        if (_sessions_running) {
            return;
        }

        spdlog::info("session start");
        std::lock_guard lock(_sessions_mutex);
        _sessions_running = true;
        pause_torrents();
    });

    spdlog::info("Registered HTTP server routes");
}