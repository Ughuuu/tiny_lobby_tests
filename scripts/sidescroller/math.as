namespace math {
    double clamp(double x, double minVal, double maxVal) {
        if (x < minVal) return minVal;
        if (x > maxVal) return maxVal;
        return x;
    }

    double min(double a, double b) {
        return a < b ? a : b;
    }

    double max(double a, double b) {
        return a > b ? a : b;
    }

    double lerp(double a, double b, double t) {
        return a + (b - a) * t;
    }

    double sign(double x) {
        return x < 0 ? -1.0f : (x > 0 ? 1.0f : 0.0f);
    }

    double smoothstep(double edge0, double edge1, double x) {
        // Scale, clamp, and evaluate polynomial
        x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return x * x * (3 - 2 * x);
    }

    // Aliases to built-in global math functions (you can use as-is)
    double sin(double x) { return sin(x); }
    double cos(double x) { return cos(x); }
    double tan(double x) { return tan(x); }

    double asin(double x) { return asin(x); }
    double acos(double x) { return acos(x); }
    double atan(double x) { return atan(x); }
    double atan2(double y, double x) { return atan2(y, x); }

    double sqrt(double x) { return sqrt(x); }
    double abs(double x) { return abs(x); }
    double floor(double x) { return floor(x); }
    double ceil(double x) { return ceil(x); }
    double log(double x) { return log(x); }
    double log10(double x) { return log10(x); }
    double pow(double x, double y) { return pow(x, y); }
}
