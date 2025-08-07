#include "database_thread.h"

#include <iostream>
#include <pqxx/pqxx>

#include "database.h"

DatabaseThread::DatabaseThread(
    moodycamel::BlockingReaderWriterQueue<DatabaseReceivedMessage> &queue, bool db_enabled)
    : receive_queue(queue), running(db_enabled), db_thread(&DatabaseThread::run, this) {}

DatabaseThread::~DatabaseThread() {
    stop();
    if (db_thread.joinable()) db_thread.join();
}

void DatabaseThread::stop() { running = false; }

void DatabaseThread::run() {
    Database db;
    if (running) {
        db.connect_to_db();
    }
    while (running) {
        DatabaseReceivedMessage msg;
        if (receive_queue.try_dequeue(msg)) {
            switch (msg.type) {
                case DatabaseReceivedMessage::Type::SetScore: {
                    db.leaderboard_set_score(msg.leaderboard_id, msg.game_id, msg.peer_id,
                                             msg.score, msg.leaderboard_type);
                    break;
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    db.close_connection();
}
