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
| Property | Returns | Has Setter |
|----------|---------|---------|
| `string calling_peer_id const property` | Calling peer ID | |
| `string id const property` | Lobby ID | |
| `int64 tick_rate const property` | Tick rate | |
| `string name const property` | Lobby name | true |
| `string host const property` | Host ID | true |
| `int64 max_players const property` | Max players | true |
| `int64 create_time const property` | Creation time | |
| `bool sealed const property` | Sealed state | true |
| `string game_id const property` | Game ID | |
| `int64 peers_count const property` | Peer count | |
| `LobbyData@ public_data const property` | Public data | true |
| `LobbyData@ private_data const property` | Private data | true |
| `LobbyData@ tags const property` | Tags | true |
| `dictionary@ peers const property` | Peer dictionary | |

### LobbyPeer
| Property | Returns |
|----------|---------|
| `LobbyData@ user_data const property` | User data |
| `LobbyData@ public_data const property` | Public data |
| `LobbyData@ private_data const property` | Private data |
| `string platform const property` | Platform |
| `string platform_id const property` | Platform ID |
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

## Callbacks

| Function | Description | Convention |
|----------|-------------|------------|
| `_can_create` | If this returns error cannot create lobby.
| `_on_create` | After the lobby is created, sets data.
| `_can_join` | If this returns error you cannot join the lobby.
| `_on_join` | After the peer joined, set data.
| `_can_chat(string message)` | If this returns error the chat message is dropped.
| `_can_tags(dictionry tags)` | If this returns error the tags are not updated.
| `_can_kick(string peer_id)` | If this returns error the kick fails.
| `_can_ready(bool ready)` | If this returns error the ready set fails.
| `_can_seal(bool seal)` | If this returns error the seal fails.
| `_on_left` | Called after a peer leaves.
| `_can_resize(int max_players)` | Called before the lobby is resized.
| `_can_title(string title)` | If this returns error the lobby title cannot be updated.
