namespace main {
    void move(int dir) {
        Lobby@ l = lobby::get();
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        auto pad_y = peer.public_data.get_int("pad_y");
        switch(dir) {
            case -1:
                //print(get_ticks_ms());
                peer.public_data.set("timestamp", get_ticks_ms());
                peer.public_data.set("pad_y", pad_y - 1);
                break;
            case 1:
                peer.public_data.set("pad_y", pad_y + 1);
                break;
            default:
                throw("Invalid direction: " + dir);
                break;
        }
    }
    void _can_create_lobby() {
        Lobby@ l = lobby::get();
        if (l.max_players != 2) {
            throw("Invalid max players: 2");
        }
    }
    void _on_lobby_created() {
        Lobby@ l = lobby::get();
        l.public_data.set("ball_x", int64(1));
        l.public_data.set("ball_y", int64(2));
        _init_player();
    }
    void _on_peer_joined() {
        _init_player();
    }
    void _init_player() {
        Lobby@ l = lobby::get();
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        peer.public_data.set("pad_x", 3);
        peer.public_data.set("pad_y", 4);
    }
    void _can_peer_chat(string message) {}
    void _can_host_set_tags(dictionary tags) {}
    void _can_host_kick(string kicked_peer_id) {}
    void _can_peer_ready(bool ready) {}
    void _can_host_seal(bool seal) {}
    void _on_peer_leave() {}
    void _on_lobby_tick(int64 delta) {
        Lobby@ l = lobby::get();
        auto ball_x = l.public_data.get_int("ball_x");
        l.public_data.set("ball_x", ball_x + 1);
    }
}
