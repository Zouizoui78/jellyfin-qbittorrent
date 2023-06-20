#ifndef MONITOR_HPP
#define MONITOR_HPP

#include "httplib.h"

class Monitor {
public:
    Monitor();
    ~Monitor();

private:
    void pause_torrents();
    void resume_torrents();
    void manage_torrents(const std::string &method);

    bool _sessions_running = false;
    std::mutex _sessions_mutex;

    // http members
    httplib::Server _server;
    std::thread _server_thread;
    void register_server_routes();

#ifdef DEBUG
    std::string _qbittorrent_addr = "http://puti:8112";
#else
    std::string _qbittorrent_addr = "http://localhost:8112";
#endif
    httplib::Client _qbittorrent_client;
};

#endif // MONITOR_HPP