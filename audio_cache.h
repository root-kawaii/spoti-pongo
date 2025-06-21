#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <fstream>
#include <vector>
#include <atomic>

class AudioCache {
public:
    AudioCache(const std::string& filename);
    ~AudioCache();
    void push_data(const std::vector<uint8_t>& chunk);

private:
    std::ofstream output_file;
    std::queue<std::vector<uint8_t>> buffer_queue;
    std::mutex mutex;
    std::condition_variable cond_var;
    std::thread writer_thread;
    std::atomic<bool> running;

    void writer_loop();
};
