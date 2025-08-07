#include "games_listener.h"

GamesListener::GamesListener(std::string games_path,
                             moodycamel::ReaderWriterQueue<std::string>& file_watcher_queue)
    : games_path(std::filesystem::absolute(games_path).string()),
      file_watcher_queue(file_watcher_queue) {
    if (!games_path.empty() && games_path.back() != std::filesystem::path::preferred_separator) {
        games_path += std::filesystem::path::preferred_separator;
    }
}

bool GamesListener::file_changed(const std::string& path) {
    auto fpath = std::filesystem::path(path);
    if (!std::filesystem::exists(fpath)) return false;

    auto last_write = std::filesystem::last_write_time(fpath);
    auto it = file_times.find(path);
    if (it == file_times.end() || it->second != last_write) {
        file_times[path] = last_write;
        return true;
    }
    return false;
}

void GamesListener::handleFileAction(efsw::WatchID watchid, const std::string& dir,
                                     const std::string& filename, efsw::Action action,
                                     std::string oldFilename) {
    if (!file_changed(dir + filename)) {
        return;
    }
    std::string full_path = std::filesystem::absolute(dir + filename).string();
    if (full_path.find(games_path) != 0) {
        return;
    }

    std::filesystem::path relative = std::filesystem::relative(full_path, games_path);
    auto it = relative.begin();
    if (it == relative.end()) return;

    std::string folder_name = it->generic_string();
    it++;
    if (it == relative.end()) return;  // Not a folder

    update_game(folder_name, file_times[dir + filename]);
}

void GamesListener::update_game(const std::string& game_folder,
                                std::filesystem::file_time_type& last_write) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        last_write - std::filesystem::file_time_type::clock::now() +
        std::chrono::system_clock::now());
    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);

    std::cout << "Game folder changed: " << game_folder << " at "
              << std::asctime(std::localtime(&cftime));
    file_watcher_queue.emplace(game_folder);
}
