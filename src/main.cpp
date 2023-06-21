#ifdef DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif
#include "spdlog/spdlog.h"

#include "Monitor.hpp"

using namespace std::chrono_literals;

std::atomic<bool> exiting = false;
std::condition_variable cv;
std::mutex cv_mutex;

void signal_handler(int signal) {
    std::cout << "Received signal " << strsignal(signal) << std::endl;
    if (signal == SIGINT || signal == SIGTERM) {
        exiting = true;
        cv.notify_one();
    }
}

int main() {
    #ifdef DEBUG
    spdlog::set_level(spdlog::level::debug);
    #endif

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    Monitor m;
    if (!m.is_ok()) {
        spdlog::error("Monitor initialization failed");
        return 1;
    }

    spdlog::info("Waiting for requests...");

    {
        std::unique_lock lock(cv_mutex);
        cv.wait(lock, [] { return exiting.load(); });
    }

    spdlog::info("Exiting");
}