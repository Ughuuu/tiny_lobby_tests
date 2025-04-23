class Vector2i {
    int64 x;
    int64 y;

    Vector2i() {}
    Vector2i(int64 _x, int64 _y) { x = _x; y = _y; }

    Vector2i opAdd(const Vector2i &in other) const {
        return Vector2i(x + other.x, y + other.y);
    }

    Vector2i opSub(const Vector2i &in other) const {
        return Vector2i(x - other.x, y - other.y);
    }

    Vector2i opMul(int64 scalar) const {
        return Vector2i(x * scalar, y * scalar);
    }

    Vector2i opDiv(int64 scalar) const {
        return Vector2i(x / scalar, y / scalar);
    }

    Vector2i& opAddAssign(const Vector2i &in other) {
        x += other.x;
        y += other.y;
        return this;
    }

    Vector2i& opSubAssign(const Vector2i &in other) {
        x -= other.x;
        y -= other.y;
        return this;
    }

    Vector2i& opMulAssign(int64 scalar) {
        x *= scalar;
        y *= scalar;
        return this;
    }

    Vector2i& opDivAssign(int64 scalar) {
        x /= scalar;
        y /= scalar;
        return this;
    }

    int64 dot(const Vector2i &in other) const {
        return x * other.x + y * other.y;
    }

    double length() const {
        return sqrt(x * x + y * y);
    }

    Vector2i opNeg() const {
        return Vector2i(-x, -y);
    }

    bool opEquals(const Vector2i &in other) const {
        return x == other.x && y == other.y;
    }

    string ToString() const {
        return "(" + x + ", " + y + ")";
    }
}
