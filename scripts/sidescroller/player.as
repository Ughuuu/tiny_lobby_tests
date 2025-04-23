#include "vector2.as"

const Vector2 PLAYER_SIZE = Vector2(20, 20);
const double GRAVITY = 980;
const double JUMP_SPEED = -1000.0;
const double MOVE_SPEED = 500.0;
const double MAX_FALL_SPEED = 10000.0;

class Player {
    Vector2 pos;
    Vector2 vel;

    Player(Vector2 _pos, Vector2 _vel) {
        pos = _pos;
        vel = _vel;
    }

    bool is_grounded() {
        return pos.y >= 179.0;
    }

    void move(double input_dir, bool input_jump, double delta) {
        // Apply gravity
        vel.y += GRAVITY * delta;
        if (vel.y > MAX_FALL_SPEED) {
            vel.y = MAX_FALL_SPEED;
        }
        // Jumping
        if (is_grounded() && input_jump) {
            vel.y = JUMP_SPEED;
        }

        if (input_dir == 0.0) {
            // move towards 0 with MOVE_SPEED
            if (vel.x > 0.0) {
                vel.x -= MOVE_SPEED;
                if (vel.x < 0.0) {
                    vel.x = 0.0;
                }
            } else if (vel.x < 0.0) {
                vel.x += MOVE_SPEED;
                if (vel.x > 0.0) {
                    vel.x = 0.0;
                }
            }
        } else {
            vel.x = input_dir * MOVE_SPEED;
        }


        // Apply velocity to position
        pos += vel * delta;

        // Ground collision
        if (pos.y > 180.0) {
            pos.y = 180.0;
            vel.y = 0.0;
        }
    }
}
