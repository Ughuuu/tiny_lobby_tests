#include "vector2.as"
#include "player.as"
#include "math.as"
namespace main {
    void _on_init() {
        print("init");
    }
    void _on_reload() {
        print("reload");
    }
    const int64 MAX_PREV_TICKS = 5;
    // input_dir between -1 and 1
    void send_move_input(double input_dir, bool input_jump, int64 tick) {
        Lobby@ l = lobby::get();
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        input_dir = math::clamp(input_dir, -1.0, 1.0);
        auto server_tick = l.public_data.get_int("tick_count");
        // tick cannot be in future
        if (server_tick < tick) {
            throw("invalid tick");
        }
        auto relative_tick = server_tick - tick;
        auto peer_tick = peer.private_data.get_int("tick_count");
        // eg. if peer is at tick 10, and input is sent for tick 12
        // we need to catch up 1 ticks
        auto catchup_count = tick - peer_tick;
        if (relative_tick > MAX_PREV_TICKS - 1 || peer_tick >= server_tick) {
            // expired tick
            return;
        }
        peer.private_data.set("tick_count", tick);
        if (relative_tick > 0) {
            // we are behind, so we need to catch up
            // eg. if the peer is at tick 10, and server is at tick 12
            for (int64 i = 0; i < catchup_count; i++) {
                _move_peer(peer, l, double(l.tick_rate) * 0.001);
            }
            // old movement, do it right away to catch up.
            _move_peer(peer, l, double(l.tick_rate) * 0.001, input_dir, input_jump);
        } else {
            peer.private_data.set("input_dir", input_dir);
            peer.private_data.set("input_jump", input_jump);
        }
    }
    void _init_peer() {
        Lobby@ l = lobby::get();
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        peer.public_data.set("pos_x", 62.0);
        peer.public_data.set("pos_y", 180.0);
        peer.private_data.set("vel_x", 0.0);
        peer.private_data.set("vel_y", 0.0);
        peer.private_data.set("input_dir", 0.0);
        peer.private_data.set("input_jump", false);
        peer.private_data.set("tick_count", 0);
    }
    void _on_create() {
        Lobby@ l = lobby::get();
        if (l.max_players != 1000) {
            throw("max players needs to be 1000");
        }
        l.public_data.set("tick_count", 0);
        l.public_data.set("tick_rate", l.tick_rate);
        main::_init_peer();
    }
    void _on_join() {
        main::_init_peer();
    }
    void _can_chat(string message) {}
    void _can_tags(dictionary tags) {}
    void _can_kick(string kicked_peer_id) {}
    void _can_ready(bool ready) {}
    void _can_seal(bool seal) {}
    void _on_left() {}
    void _move_peer(LobbyPeer@ peer, Lobby@ l, double delta, double input_dir = 0.0, bool input_jump = false) {
        auto pos_x = peer.public_data.get_double("pos_x");
        auto pos_y = peer.public_data.get_double("pos_y");
        auto vel_x = peer.private_data.get_double("vel_x");
        auto vel_y = peer.private_data.get_double("vel_y");
        auto server_tick = l.public_data.get_int("tick_count");
        Player@ player = Player(Vector2(pos_x, pos_y), Vector2(vel_x, vel_y));

        if (input_dir != 0.0 || input_jump) {
            player.move(input_dir, input_jump, delta);
        }
        peer.public_data.set("update_tick", server_tick);
        peer.public_data.set("pos_x", player.pos.x);
        peer.public_data.set("pos_y", player.pos.y);
        peer.public_data.set("vel_x", player.vel.x);
        peer.public_data.set("vel_y", player.vel.y);
    
        peer.private_data.set("input_dir", 0.0);
        peer.private_data.set("input_jump", false);
    }
    void _on_tick(int64 game_tickrate) {
        Lobby@ l = lobby::get();
        int64 tick = l.public_data.get_int("tick_count");
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        if (peer !is null) {
            _move_peer(peer, l, double(game_tickrate) * 0.001, peer.private_data.get_double("input_dir"), peer.private_data.get_bool("input_jump"));
        }
        l.public_data.set("tick_count", tick + 1);
    }
}
