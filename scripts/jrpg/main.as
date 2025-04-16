// AngelScript
namespace main {
    // dir 0 for left
    // dir 1 for up
    // dir 2 for right
    // dir 3 for down
    void move(double dir_double) {
        Lobby@ l = lobby::get();
        int dir = int(dir_double);
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        peer.private_data.set("dir", dir);
    }
    void _init_peer() {
        Lobby@ l = lobby::get();
        auto peer = cast<LobbyPeer@>(l.peers[l.calling_peer_id]);
        peer.public_data.set("pos_x", 0);
        peer.public_data.set("pos_y", 0);
        peer.private_data.set("dir", -1);
    }
    void _on_create() {
        Lobby@ l = lobby::get();
        if (l.max_players != 1000) {
            throw("max players needs to be 1000");
        }
        main::_init_peer();
        file f;
        if( f.open("map.csv", "r") >= 0 )
        {
            string str = f.readString(f.getSize());
            print(str);
            f.close();
        } else {
            throw("cannot read file");
        }
    }
    void _on_join() {
        main::_init_peer();
    }
    void _on_chat(string message) {}
    void _on_tags(dictionary tags) {}
    void _on_kick(string kicked_peer_id) {}
    void _on_ready(bool ready) {}
    void _on_seal(bool seal) {}
    void _on_left() {}
    void _move_peer(LobbyPeer@ peer) {
        auto dir = peer.private_data.get_int("dir");
        if (dir == -1) {
            // No movement key pressed
            return;
        }

        auto pos_x = peer.public_data.get_int("pos_x");
        auto pos_y = peer.public_data.get_int("pos_y");

        switch(dir) {
            case 0:
                pos_x -= 1;
            break;
            case 1:
                pos_y -= 1;
            break;
            case 2:
                pos_x += 1;
            break;
            case 3:
                pos_y += 1;
            break;
            default:
            break;
        }
        
        peer.public_data.set("pos_x", pos_x);
        peer.public_data.set("pos_x", pos_x);
        peer.private_data.set("dir", -1);
    }
    void _on_tick(int delta) {
        Lobby@ l = lobby::get();
        auto keys = l.peers.getKeys();
        for (uint i=0; i < keys.length(); i++) {
            auto peer = cast<LobbyPeer@>(l.peers[keys[i]]);
            main::_move_peer(peer);
        }
    }
}
