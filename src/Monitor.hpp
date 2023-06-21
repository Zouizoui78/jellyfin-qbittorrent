#ifndef MONITOR_HPP
#define MONITOR_HPP

#include "httplib.h"
#include "nlohmann/json.hpp"

class Monitor {
public:
    Monitor();
    ~Monitor();

    bool is_ok() const;

private:
    bool init();
    bool _is_ok = false;

    std::atomic<bool> _exiting = false;
    void stop();

    httplib::Server _server;
    std::jthread _server_thread;
    void register_server_routes();


    // qbittorrent related members
    void pause_torrents();
    void resume_torrents();
    void manage_torrents(const std::string &method);
    std::mutex _qbittorrent_mutex;

#ifdef DEBUG
    std::string _qbittorrent_addr = "http://puti:8112";
#else
    std::string _qbittorrent_addr = "http://localhost:8112";
#endif
    httplib::Client _qbittorrent_client;


    // jellyfin related members
    void start_jellyfin_monitoring();
    void stop_jellyfin_monitoring();
    void monitor_jellyfin();
    nlohmann::json get_jellyfin_sessions();

    std::atomic<bool> _jellyfin_sessions_active = false;
    std::jthread _jellyfin_monitor_thread;
    std::condition_variable _jellyfin_monitor_cv;
    std::mutex _jellyfin_monitor_mutex;

#ifdef DEBUG
    std::string _jellyfin_addr = "http://puti:8096";
#else
    std::string _jellyfin_addr = "http://localhost:8096";
#endif
    httplib::Client _jellyfin_client;
    std::string _jellyfin_api_key; // from environement

    // http members
};

#endif // MONITOR_HPP