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
    void post_session_start(const httplib::Request &req, httplib::Response &res);


    // qbittorrent related members
    void pause_torrents();
    void resume_torrents();
    void manage_torrents(const std::string &method);

    std::atomic<bool> _torrents_paused = false;
    std::string _qbittorrent_addr; // from environement
    std::shared_ptr<httplib::Client> _qbittorrent_client;


    // jellyfin related members
    void start_jellyfin_monitoring();
    void stop_jellyfin_monitoring();
    void loop_jellyfin_monitoring();
    void monitor_jellyfin();
    nlohmann::json get_jellyfin_sessions();

    std::atomic<bool> _jellyfin_sessions_active = false;
    std::jthread _jellyfin_monitor_thread;
    std::condition_variable _jellyfin_monitor_cv;
    std::mutex _monitor_mutex;

    std::string _jellyfin_addr; // from environement
    std::shared_ptr<httplib::Client> _jellyfin_client;
    std::string _jellyfin_api_key; // from environement
};

#endif // MONITOR_HPP