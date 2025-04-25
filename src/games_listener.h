#pragma once
#include <readerwriterqueue.h>

#include <efsw/efsw.hpp>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

class GameThread;

class GamesListener : public efsw::FileWatchListener {
    std::string games_path;
    moodycamel::ReaderWriterQueue<std::string>& file_watcher_queue;
    std::unordered_map<std::string, std::filesystem::file_time_type> file_times;
    bool file_changed(const std::string& path);

   public:
    GamesListener(std::string games_path,
                  moodycamel::ReaderWriterQueue<std::string>& file_watcher_queue);
    void handleFileAction(efsw::WatchID watchid, const std::string& dir,
                          const std::string& filename, efsw::Action action,
                          std::string oldFilename) override;
    void update_game(const std::string& game_folder, std::filesystem::file_time_type& last_write);
};
