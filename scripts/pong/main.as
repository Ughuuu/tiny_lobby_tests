namespace main {
    void move(int dir) {
        print(dir);
        Lobby@ l = lobby::get();
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        auto pad_y = peer.public_data.get_int("pad_y");
        switch(dir) {
            case -1:
                print("up");
                peer.public_data.set("pad_y", pad_y - 1);
                break;
            case 1:
                print("down");
                peer.public_data.set("pad_y", pad_y + 1);
                break;
            default:
                throw("Invalid direction: " + dir);
                break;
        }
    }
    void _can_create() {
        Lobby@ l = lobby::get();
        if (l.max_players != 2) {
            throw("Invalid max players: 2");
        }
    }
    void _on_create() {
        Lobby@ l = lobby::get();
        l.public_data.set("ball_x", int64(1));
        l.public_data.set("ball_y", int64(2));
        _init_player();
    }
    void _on_join() {
        _init_player();
    }
    void _init_player() {
        Lobby@ l = lobby::get();
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        peer.public_data.set("pad_x", 3);
        peer.public_data.set("pad_y", 4);
    }
    void _on_chat(string message) {}
    void _on_tags(dictionary tags) {}
    void _on_kick(string kicked_peer_id) {}
    void _on_ready(bool ready) {}
    void _on_seal(bool seal) {}
    void _on_left() {}
    void _on_tick(int64 delta) {
        Lobby@ l = lobby::get();
        auto ball_x = l.public_data.get_int("ball_x");
        l.public_data.set("ball_x", ball_x + 1);
    }
}
