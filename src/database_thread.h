#pragma once
#include <readerwriterqueue.h>

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

// Define a struct for database tasks/messages
struct DatabaseReceivedMessage {
    enum class Type {
        SetScore,
    } type;
    // Arguments for each type
    std::string game_id;
    std::string peer_id;
    int64_t score = 0;
    std::string leaderboard_id;
    std::string leaderboard_type;
    int limit = 0;
    std::string query;
};

class DatabaseThread {
    moodycamel::BlockingReaderWriterQueue<DatabaseReceivedMessage> &receive_queue;
    std::atomic<bool> running{true};
    std::thread db_thread;

   public:
    DatabaseThread(moodycamel::BlockingReaderWriterQueue<DatabaseReceivedMessage> &queue,
                   bool db_enabled);
    ~DatabaseThread();
    void run();
    void stop();
};
