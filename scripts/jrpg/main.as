#include "vector2i.as"
#include "player.as"
#include "map.as"

namespace main {
    // key pressed and release and direction isn't same
    void turn(int64 dir) {
        Lobby@ l = lobby::get();
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        if (dir < 0 || dir > 3) {
            throw("invalid dir");
        }
        peer.public_data.set("dir", dir);
    }
    // key pressed and released
    void move_press(int64 dir) {
        Lobby@ l = lobby::get();
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        if (dir < 0 || dir > 3) {
            throw("invalid dir");
        }

        auto move_start_ms = peer.public_data.get_int("move_start");
        auto move_time_ms = peer.public_data.get_int("move_time");
        auto current_time_ms = lobby::get_ticks_ms();
        // Not enough time passed to finish movement
        if (move_start_ms + move_time_ms > current_time_ms) {
            return;
        }
        // same direction walking already
        if (peer.public_data.get_bool("is_moving") && dir == peer.public_data.get_int("dir")) {
            return;
        }
        _move_peer(peer, l, dir, current_time_ms);
    }
    // key released
    void move_release() {
        Lobby@ l = lobby::get();
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        peer.public_data.set("is_moving", false);
    }
    void _can_create() {
        Lobby@ l = lobby::get();
        if (l.max_players != 1000) {
            throw("max players needs to be 1000");
        }
    }
    void _on_create() {
        map::_read_map();
        player::_init_peer();
    }
    void _on_join() {
        player::_init_peer();
    }
    void _can_chat(string message) {}
    void _can_tags(dictionary tags) {}
    void _can_kick(string kicked_peer_id) {}
    void _can_ready(bool ready) {}
    void _can_seal(bool seal) {}
    void _on_left() {}
    Vector2i _dir_code_to_vector(int64 dir_code) {
        switch(dir_code) {
            case player::PLAYER_DIR::DIR_LEFT:
                return Vector2i(-1, 0);
            case player::PLAYER_DIR::DIR_UP:
                return Vector2i(0, -1);
            case player::PLAYER_DIR::DIR_RIGHT:
                return Vector2i(1, 0);
            case player::PLAYER_DIR::DIR_DOWN:
                return Vector2i(0, 1);
            default:
                return Vector2i(0, 0);
        }
    }
    bool _check_peer_collision(Lobby@ l, Vector2i pos) {
        auto peerKeys = l.peers.getKeys();
        for (uint64 i=0; i < peerKeys.length(); i++) {
            auto checking_peer = cast<LobbyPeer@>(l.peers[peerKeys[i]]);
            auto checking_pos = Vector2i(checking_peer.public_data.get_int("pos_x"), checking_peer.public_data.get_int("pos_y"));
            if (checking_pos == pos) {
                // another peer is in the way
                return true;
            }
        }
        return false;
    }
    bool _move_peer(LobbyPeer@ moving_peer, Lobby@ l, int64 dir_code, int64 move_start) {
        auto pos = Vector2i(moving_peer.public_data.get_int("pos_x"), moving_peer.public_data.get_int("pos_y"));
        auto dir = _dir_code_to_vector(dir_code);
        auto current_id = l.private_data.get_int(pos.ToString());
        auto new_id = l.private_data.get_int((pos + dir).ToString());
        moving_peer.public_data.set("is_interactable", map::_is_interactable(new_id));
        moving_peer.public_data.set("dir", dir_code);
        if (!map::_is_walkable(new_id) || _check_peer_collision(l, pos + dir)) {
            moving_peer.public_data.set("is_moving", false);
            return false;
        }
        auto speed_ms = map::_tile_speed(current_id) + map::_tile_speed(new_id);
        moving_peer.public_data.set("move_time", speed_ms);
        moving_peer.public_data.set("pos_x", (pos + dir).x);
        moving_peer.public_data.set("pos_y", (pos + dir).y);
        moving_peer.public_data.set("move_start", move_start);
        moving_peer.public_data.set("is_moving", true);
        return true;
    }
    void _on_tick(int64 tickrate) {
        auto l = lobby::get();
        auto current_time_ms = lobby::get_ticks_ms();
        auto val = peer.public_data.get_int("val");
        auto peerKeys = l.peers.getKeys();
        for (uint64 i=0; i < peerKeys.length(); i++) {
            auto peer = cast<LobbyPeer@>(l.peers[peerKeys[i]]);
            if (peer.public_data.get_bool("is_moving")) {
                auto move_start_ms = int64(peer.public_data.get_int("move_start"));
                auto move_time_ms = peer.public_data.get_int("move_time");
                // if next tick would expire our movement, send new move
                if (move_start_ms + move_time_ms < current_time_ms + tickrate) {
                    _move_peer(peer, l, peer.public_data.get_int("dir"), move_start_ms + move_time_ms);
                }
            }
        }
    }
}
