namespace player {
    enum PLAYER_DIR {
        DIR_LEFT = 0,
        DIR_UP = 1,
        DIR_RIGHT = 2,
        DIR_DOWN = 3,
    }
    enum PLAYER_BUFF {
        BUFF_NONE = 0,
        BUFF_SPEED = 1,
        BUFF_WATER_WALKING = 2
    }
    void _init_peer() {
        Lobby@ l = lobby::get();
        print("init peer");
        print(l.id);
        print(l.calling_peer_id);
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        peer.public_data.set("pos_x", int64(11));
        peer.public_data.set("pos_y", int64(9));
        peer.public_data.set("move_start", int64(0));
        peer.public_data.set("move_time", int64(0));
        peer.public_data.set("is_interactable", false);
        peer.public_data.set("is_moving", false);
        peer.public_data.set("dir", 3);
    }
}
