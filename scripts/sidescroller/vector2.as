class Vector2 {
    double x;
    double y;

    Vector2() {}
    Vector2(double _x, double _y) { x = _x; y = _y; }

    Vector2 opAdd(const Vector2 &in other) const {
        return Vector2(x + other.x, y + other.y);
    }

    Vector2 opSub(const Vector2 &in other) const {
        return Vector2(x - other.x, y - other.y);
    }

    Vector2 opMul(double scalar) const {
        return Vector2(x * scalar, y * scalar);
    }

    Vector2 opDiv(double scalar) const {
        return Vector2(x / scalar, y / scalar);
    }

    Vector2& opAddAssign(const Vector2 &in other) {
        x += other.x;
        y += other.y;
        return this;
    }

    Vector2& opSubAssign(const Vector2 &in other) {
        x -= other.x;
        y -= other.y;
        return this;
    }

    Vector2& opMulAssign(double scalar) {
        x *= scalar;
        y *= scalar;
        return this;
    }

    Vector2& opDivAssign(double scalar) {
        x /= scalar;
        y /= scalar;
        return this;
    }

    double dot(const Vector2 &in other) const {
        return x * other.x + y * other.y;
    }

    double length() const {
        return sqrt(x * x + y * y);
    }

    Vector2 normalized() const {
        double len = length();
        return len > 0 ? this / len : Vector2(0, 0);
    }

    Vector2 opNeg() const {
        return Vector2(-x, -y);
    }

    bool opEquals(const Vector2 &in other) const {
        return x == other.x && y == other.y;
    }

    string ToString() const {
        return "(" + x + ", " + y + ")";
    }
}
