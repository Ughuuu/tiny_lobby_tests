# Server/Client authoritative movement

**Movement** here refers to a character moving in the world both on the server and on the client (in game). **Authoritative** means that the movement will be done on the server side, and only replicated on the client side. There are some challenges associated with this in order to obtain perfect movement:
- **server rate limiting**: the server needs to rate limit and disconnect clients who send too many messages
- **server updates based on client inputs**: the server needs to update the position of the player with inputs from the client. The server notes what inputs it receives and calculates the next position the player will be in. If the calculations are easy enough, they can be replicated in a deterministic way on the client.
- **server movement with buffered inputs**: the server will buffer movement inputs and accept older inputs.
- **client prediction**: when the client presses the move key, the player needs to move on the client before the player moves on the server.
- **client animation**: the client needs to be able to animate the player moving. the challenges here are making sure the timestamp is synchronized between server and client, including lag/delays.

## Server Rate Limiting

The client may send more messages than needed:
    - we should have a **rate limiter** on the server (eg. drop the connection of the client if he sends more messages than needed). This will ensure the client only sends one message when he needs to.

```json
rate_limit = 3 messages/s
- 0s
|
| <- client sends message 1
| <- client sends message 2
| <- client sends message 3
| <- client sends message 4
| <- server disconnects client
|
- 1s
v
```

## Server Updates based on Client Inputs

For this, the server needs to update the player position based on the client input. The challenges here are:

- the client may not know how to animate the player:
    - we need to make sure the server updates the position every **interval** in a way that is known by the client. This will be later used by the client to animate movement.
Why is this an issue? If the client receives 3 positions and he does not know how much time he needs to animate them, the animation will look choppy.
```json
pos1.........pos2.........pos3
t0...........t1...........t2
```
In the example above, the client needs to know exactly how much time will pass between pos1 and pos2 update, so he can do a lerp between the 2 positions. If the server takes longer than needed to send pos2 update, the client will need to stop the player, resulting in a movement that stops instead of a fluid one.

To solve this, the server needs to send the movement updates in a reliable way such that the peer can animate based on known times. In our case, we will use a **tick_rate** which ensures the client will always receive updates every 10ms.
```json
server_tick_rate = 10ms
- t0, 0ms
|
| <- client sends input 1 for t0
| <- server sends position 1 based on previous inputs 0 for t0
| 
- t1, 10ms
|
| <- client sends input 2 for t1
| <- server sends position 2 based on previous inputs 1 for t1
|
- t2
v
```
Here it's important that the client immediately sends the input after the tick happened. This way the client will not miss his window.

## Server Updates with Buffered Inputs

The client has delay and sends inputs 1 or 2 events later.

```jsons
- t0 server processed t0
|
- t1 server processed t1
|
- t2 client sends input for t0
|
v
```

```json
- t0
|
| <- server processes t0
|
- t1
| <- client1 sends input for t0
| <- server process t0 for client1
|
- t2
```

# Client Prediction

The challenge here is that even though the client pressed the movement key, the server will only notify the update when the nearest server_tick occurs. Even if tick system is not used, there will still be delay before the server updates the client.

```json
server..............received_input1.................
client...input1......................received_pos1..
.........t0.........t1...............t2.............
         <----------delay------------------------>
```

In order to overcome this, the client needs to move the player to a future/predicted position, until he receives the actual position from the server.

# Client Animation

The client needs to animate the position of the player based on positions. For this there will be in play both the previous known position and the future/predicted position. The client also needs to know how much time the animation will take between these 2 positions.

```json
- t0, server_pos0
- t0, client_predicted_pos1
|
- t1, server_pos1
|
|
v
```

In the example above, the client receives a position from the server, and predicts the future position, and animates based on that until t1 until it gets a new position.

There will be some challenges with this also, as even though the server works on a tick rate, it's possible it doesn't send the position exactly at the correct interval:

```json
- t1, 0ms
|
- server_event t1', 2ms
|
- t2, 10ms
|
- server_event t2', 13ms
|
- t3, 20ms
|
- server_event t3', 21ms
|
v
```

The client will need to fix any issues that might occur in case the predicted_position and the new_position from server don't match.
