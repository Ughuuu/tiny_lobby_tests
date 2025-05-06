# AngelScript Lobby System Documentation

## Global Print Functions
| Function | Description |
|----------|-------------|
| `void print(int64)` | Prints 64-bit integer |
| `void print(double)` | Prints double |
| `void print(bool)` | Prints boolean |
| `void print(const string &in)` | Prints string |

## Object Types
| Type | Description |
|------|-------------|
| `Lobby` | Reference-counted lobby object |
| `LobbyData` | Reference-counted lobby data object |
| `LobbyPeer` | Reference-counted peer object |

## Lobby Methods
### Getters
| Property | Returns |
|----------|---------|
| `string calling_peer_id const property` | Calling peer ID |
| `string id const property` | Lobby ID |
| `int64 tick_rate const property` | Tick rate |
| `string name const property` | Lobby name |
| `string host const property` | Host ID |
| `int64 max_players const property` | Max players |
| `int64 create_time const property` | Creation time |
| `bool sealed const property` | Sealed state |
| `string game_id const property` | Game ID |
| `int64 peers_count const property` | Peer count |
| `LobbyData@ public_data const property` | Public data |
| `LobbyData@ private_data const property` | Private data |
| `LobbyData@ tags const property` | Tags |
| `dictionary@ peers const property` | Peer dictionary |

### LobbyPeer
| Property | Returns |
|----------|---------|
| `LobbyData@ user_data const property` | User data |
| `LobbyData@ public_data const property` | Public data |
| `LobbyData@ private_data const property` | Private data |
| `string id const property` | Peer ID |
| `int order_id const property` | Order ID |
| `bool ready const property` | Ready state |
| `bool disconnected const property` | Disconnected state |

## LobbyData Methods
### Set Methods
| Method | Description |
|--------|-------------|
| `void set(string key, any@ value)` | Sets generic value |
| `void set(string key, int64 value)` | Sets 64-bit integer |
| `void set(string key, bool value)` | Sets boolean |
| `void set(string key, double value)` | Sets double |
| `void set(string key, string value)` | Sets string |
| `void set(string key, array<any> &value)` | Sets array |
| `void set(string key, dictionary &value)` | Sets dictionary |

### Get Methods
| Method | Returns |
|--------|---------|
| `any@ get(string key)` | Generic value |
| `int64 get_int(string key)` | 64-bit integer |
| `bool get_bool(string key)` | Boolean |
| `double get_double(string key)` | Double |
| `array<any>@ get_array(string key)` | Array |
| `dictionary@ get_dictionary(string key)` | Dictionary |
| `string get_string(string key)` | String |

## Lobby Namespace Functions
| Function | Description | Convention |
|----------|-------------|------------|
| `void start_timer(string timer_id, double duration, array<any>@ args = array<any>())` | Starts timer | `asCALL_GENERIC` |
| `void stop_timer(string timer_id)` | Stops timer | `asCALL_GENERIC` |
| `void notify(string peer_id, any@ message)` | Sends notification | `asCALL_GENERIC` |
| `void broadcast_chat(string message)` | Broadcasts chat | `asCALL_GENERIC` |
| `int64 get_ticks_ms()` | Gets current time (ms) | `asCALL_GENERIC` |
| `Lobby@ get()` | Gets current lobby | `asCALL_GENERIC` |
