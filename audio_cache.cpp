#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <fstream>
#include <vector>
#include <atomic>
#include "audio_cache.h"
#include <iostream>


    AudioCache::AudioCache(const std::string& filename)
        : output_file(filename, std::ios::binary), running(true) {
        writer_thread = std::thread(&AudioCache::writer_loop, this);
    };

    AudioCache::~AudioCache() {
        running = false;
        cond_var.notify_all();
        if (writer_thread.joinable()) {
            writer_thread.join();
        }
        output_file.close();
    }

    void AudioCache::push_data(const std::vector<uint8_t>& chunk) {
        {
            std::cout << "pushed";
            std::lock_guard<std::mutex> lock(mutex);
            buffer_queue.push(chunk);
        }
        cond_var.notify_one();
    }


    void AudioCache::writer_loop() {
        while (running || !buffer_queue.empty()) {
            std::cout << "ciao";
            std::unique_lock<std::mutex> lock(mutex);
            cond_var.wait(lock, [&] { return !buffer_queue.empty() || !running; });

            while (!buffer_queue.empty()) {
                auto chunk = buffer_queue.front();
                buffer_queue.pop();
                lock.unlock();

                std::cout << chunk.data();

                output_file.write(reinterpret_cast<const char*>(chunk.data()), chunk.size());

                lock.lock();
            }
        }
    }
