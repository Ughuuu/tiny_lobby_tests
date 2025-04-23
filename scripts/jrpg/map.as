#include "vector2i.as"
namespace map {
    enum TILE_TYPE {
        TILE_EMPTY = 0,
        TILE_WALKABLE = 1,
        TILE_DAMAGE = 2,
        TILE_WATER = 3,
        TILE_MOUNTAIN = 4,
        TILE_HILLS =5,
        TILE_FOREST = 6,
        TILE_INTERACTABLE = 7,
        TILE_BLOCK = 8,
        TILE_PORTAL = 9,
        TILE_NPC = 10,
    }
    int64 _tile_speed(int64 id) {
        switch(id) {
            case map::TILE_TYPE::TILE_WALKABLE:
                return 200;
            case map::TILE_TYPE::TILE_DAMAGE:
                return 300;
            case map::TILE_TYPE::TILE_WATER:
                return 100;
            case map::TILE_TYPE::TILE_HILLS:
            case map::TILE_TYPE::TILE_FOREST:
                return 300;
            default:
                return 0;
        }
    }
    bool _is_interactable(int64 id) {
        switch(id) {
            case map::TILE_TYPE::TILE_INTERACTABLE:
            case map::TILE_TYPE::TILE_PORTAL:
            case map::TILE_TYPE::TILE_NPC:
                return true;
            default:
                return false;
        }
    }
    bool _is_walkable(int64 id) {
        switch(id) {
            case map::TILE_TYPE::TILE_WALKABLE:
            case map::TILE_TYPE::TILE_DAMAGE:
            case map::TILE_TYPE::TILE_HILLS:
            case map::TILE_TYPE::TILE_FOREST:
                return true;
            default:
                return false;
        }
    }
    void _read_map() {
        Lobby@ l = lobby::get();
        auto json = decodeFromFile("map.json");
        array<any> tiles;
        json.retrieve(tiles);
        
        for (uint64 i=0; i < tiles.length(); i++) {
            dictionary cell;
            tiles[i].retrieve(cell);
            auto cell_pos = Vector2i(int64(cell["x"]), int64(cell["y"]));
            int64 id = int64(cell["id"]);
            if (id == map::TILE_TYPE::TILE_EMPTY) {
                continue;
            }
            dictionary cell_data;
            cell_data["id"] = id;
            if (cell.exists("npc")) {
                auto npc = cast<array<any>>(cell["npc"]);
                cell_data["npc"] = npc;
            }
            l.private_data.set(cell_pos.ToString(), id);
        }
    }
}
