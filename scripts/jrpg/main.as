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
        _set_peer_direction(peer, dir);
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
        if (current_time_ms - move_start_ms < move_time_ms) {
            return;
        }
        // same direction walking already
        if (peer.public_data.get_bool("is_moving") && dir == peer.public_data.get_int("dir")) {
            return;
        }
        _set_peer_direction(peer, dir);
        if (_move_peer(peer, l, dir)) {
            peer.public_data.set("is_moving", true);
            peer.public_data.set("move_start", current_time_ms);
        }
    }
    // key released
    void move_release() {
        Lobby@ l = lobby::get();
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        peer.public_data.set("is_moving", false);
    }
    void _on_create() {
        Lobby@ l = lobby::get();
        if (l.max_players != 1000) {
            throw("max players needs to be 1000");
        }
        map::_read_map();
        player::_init_peer();
    }
    void _on_join() {
        player::_init_peer();
    }
    void _on_chat(string message) {}
    void _on_tags(dictionary tags) {}
    void _on_kick(string kicked_peer_id) {}
    void _on_ready(bool ready) {}
    void _on_seal(bool seal) {}
    void _on_left() {}
    void _set_peer_direction(LobbyPeer@ peer, int64 dir) {
        peer.public_data.set("dir", dir);
    }
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
    bool _move_peer(LobbyPeer@ moving_peer, Lobby@ l, int64 dir_code) {
        auto pos = Vector2i(moving_peer.public_data.get_int("pos_x"), moving_peer.public_data.get_int("pos_y"));
        auto dir = _dir_code_to_vector(dir_code);
        auto current_id = l.private_data.get_int(pos.ToString());
        auto new_id = l.private_data.get_int((pos + dir).ToString());
        moving_peer.public_data.set("is_interactable", map::_is_interactable(new_id));
        if (!map::_is_walkable(new_id)) {
            return false;
        }
        if (_check_peer_collision(l, pos + dir)) {
            return false;
        }
        auto speed_ms = map::_tile_speed(current_id) + map::_tile_speed(new_id);
        moving_peer.public_data.set("move_time", speed_ms);
        moving_peer.public_data.set("pos_x", (pos + dir).x);
        moving_peer.public_data.set("pos_y", (pos + dir).y);
        return true;
    }
    void _on_tick(int64 delta) {
        auto l = lobby::get();
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        if (peer.public_data.get_bool("is_moving")) {
            auto move_start_ms = int64(peer.public_data.get_int("move_start"));
            auto move_time_ms = peer.public_data.get_int("move_time");
            if (lobby::get_ticks_ms() - move_start_ms > move_time_ms) {
                _move_peer(peer, l, peer.public_data.get_int("dir"));
                peer.public_data.set("move_start", move_start_ms + move_time_ms);
            }
        }
    }
}
