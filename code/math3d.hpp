#ifndef MATH3D_HPP
#define MATH3D_HPP

#ifdef __cplusplus
#include <cmath>
#define SIN(x) std::sin(x)
#define COS(x) std::cos(x)
#define SQRT(x) std::sqrt(x)
#define TAN(x) tanf(x)
#else
#include <math.h>
#define SIN(x) sin(x)
#define COS(x) cos(x)
#define SQRT(x) sqrt(x)
#define TAN(x) tanf(x)
#endif

#include "types.hpp"

const r32 PI = 3.14159265358979323846f;
const r32 PI_HALF = PI / 2.0f;
const r32 RAD = PI / 180.0f;
const r32 DEG = 180.0f / PI;

struct v2 {
    union {
        r32 Values[2] = {0};
        struct {
            r32 X; r32 Y;
        };
    };
    v2() {
      X = 0.0f; Y = 0.0f;
    }

    v2(const v2 &v) {
        X = v.X; Y = v.Y;
    }

    v2(r32 x) {
        X = x; Y = x;
    }

    v2(const r32 *x) {
        X = x[0]; Y = x[1];
    }

    v2(r32 *x) {
        X = x[0]; Y = x[1];
    }

    v2(r32 x, r32 y) {
        X = x; Y = y;
    }

    inline r32 Magnitude() const {
        return SQRT(X*X + Y*Y);
    }
    
    inline v2& Normalize() {
        r32 L = this->Magnitude();
        X /= L; Y /= L;
        return *this;
    }

    inline v2 GetNormalized() const {
        r32 L = Magnitude();
        return v2(X / L, Y / L);
    }

    inline r32& operator[] (i32 i) {
        return Values[i];
    }

    inline const r32& operator[] (i32 i) const {
        return Values[i];
    }

    inline v2& operator= (const v2 &a) {
        X = a.X; Y = a.Y;
        return *this;
    }

    inline v2& operator+= (const v2 &a) {
        X += a.X; Y += a.Y;
        return *this;
    }

    inline v2& operator-= (const v2 &a) {
        X -= a.X; Y -= a.Y;
        return *this;
    }

    inline v2& operator/= (r32 s) {
        X /= s; Y /= s;
        return *this;
    }

    inline v2& operator*= (r32 s) {
        X *= s; Y *= s;
        return *this;
    }

    inline v2 operator+ (const v2 &a) const {
        return v2(X + a.X, Y + a.Y);
    }

    inline v2 operator- () const {
        return v2(-X, -Y);
    }

    inline v2 operator- (const v2 &a) const {
        return *this + (-a);
    }

    inline v2 operator/ (r32 s) const {
        return v2(X / s, Y / s);
    }

    inline v2 operator* (r32 s) const {
        return v2(s * X, s * Y);
    }

    // NOTE(Jovan): Dot product
    inline r32 operator* (const v2 &a) const {
        return X * a.X + Y * a.Y;
    }
};

struct v3 {
    union {
        r32 Values[3] = {0};
        struct {
            r32 X;
            r32 Y;
            r32 Z;
        };
    };

    v3() {
      X = 0.0f; Y = 0.0f; Z = 0.0f;
    }

    v3(const v3 &v) {
        X = v.X; Y = v.Y; Z = v.Z;
    }

    v3(r32 x) {
        X = x; Y = x; Z = x;
    }

    v3(const r32 *x) {
        X = x[0]; Y = x[1]; Z = x[2];
    }

    v3(r32 *x) {
        X = x[0]; Y = x[1]; Z = x[2];
    }

    v3(r32 x, r32 y, r32 z) {
        X = x; Y = y; Z = z;
    }

    inline r32 Magnitude() const {
        return SQRT(X*X + Y*Y + Z*Z);
    }
    
    inline v3& Normalize() {
        r32 L = this->Magnitude();
        X /= L; Y /= L; Z /= L;
        return *this;
    }

    inline v3 GetNormalized() const {
        r32 L = Magnitude();
        return v3(X / L, Y / L, Z / L);
    }

    inline r32& operator[] (i32 i) {
        return Values[i];
    }

    inline const r32& operator[] (i32 i) const {
        return Values[i];
    }

    inline v3& operator= (const v3 &a) {
        X = a.X; Y = a.Y; Z = a.Z;
        return *this;
    }

    inline v3& operator+= (const v3 &a) {
        X += a.X; Y += a.Y; Z += a.Z;
        return *this;
    }

    inline v3& operator-= (const v3 &a) {
        X -= a.X; Y -= a.Y; Z -= a.Z;
        return *this;
    }

    inline v3& operator/= (r32 s) {
        X /= s; Y /= s; Z /= s;
        return *this;
    }

    inline v3& operator*= (r32 s) {
        X *= s; Y *= s; Z *= s;
        return *this;
    }

    inline v3 operator+ (const v3 &a) const {
        return v3(X + a.X, Y + a.Y, Z + a.Z);
    }

    inline v3 operator- () const {
        return v3(-X, -Y, -Z);
    }

    inline v3 operator- (const v3 &a) const {
        return *this + (-a);
    }

    inline v3 operator/ (r32 s) const {
        return v3(X / s, Y / s, Z / s);
    }

    inline v3 operator* (r32 s) const {
        return v3(s * X, s * Y, s * Z);
    }

    // NOTE(Jovan): Dot product
    inline r32 operator* (const v3 &a) const {
        return X * a.X + Y * a.Y + Z * a.Z;
    }

    // NOTE(Jovan): Cross product
    inline v3 operator^ (const v3 &a) const {
        return v3(Y * a.Z - Z * a.Y, Z * a.X - X * a.Z, X * a.Y - Y * a.X);
    }
};

inline v3 operator* (r32 s, const v3 &a) {
    return a * s;
}

struct v4 {
    union {
        r32 Values[4] = {0};
        struct {
            r32 X; r32 Y; r32 Z; r32 W;
        };
    };
    v4() {
        X = 0.0f; Y = 0.0f; Z = 0.0f;
    }

    v4(const v4 &v) {
        X = v.X; Y = v.Y; Z = v.Z; W = v.W;
    }

    v4(r32 x) {
        X = x; Y = x; Z = x; W = x;
    }

    v4(r32 *x) {
        X = x[0]; Y = x[1]; Z = x[2]; W = x[3];
    }

    v4(const r32 *x) {
        X = x[0]; Y = x[1]; Z = x[2]; W = x[3];
    }

    v4(r32 x, r32 y, r32 z, r32 w) {
        X = x; Y = y; Z = z; W = w;
    }

    inline r32 Magnitude() {
        return SQRT(X*X + Y*Y + Z*Z + W*W);
    }
    
    inline v4& Normalize() {
        r32 L = this->Magnitude();
        X /= L; Y /= L; Z /= L; W /= L;
        return *this;
    }

    inline v4 GetNormalized() {
        r32 L = this->Magnitude();
        return v4(X / L, Y / L, Z / L, W / L);
    }

    inline r32& operator[] (i32 i) {
        return Values[i];
    }

    inline const r32& operator[] (i32 i) const {
        return Values[i];
    }
    
    inline v4& operator= (const v4 &a) {
        X = a.X; Y = a.Y; Z = a.Z; W = a.W;
        return *this;
    }

    inline v4& operator+= (const v4 &a) {
        X += a.X; Y += a.Y; Z += a.Z; W += a.W;
        return *this;
    }

    inline v4& operator-= (const v4 &a) {
        X -= a.X; Y -= a.Y; Z -= a.Z; W -= a.W;
        return *this;
    }

    inline v4& operator/= (r32 s) {
        X /= s; Y /= s; Z /= s; W /= s;
        return *this;
    }

    inline v4& operator*= (r32 s) {
        X *= s; Y *= s; Z *= s; W *= s;
        return *this;
    }

    inline v4 operator+ (const v4 &a) const {
        return v4(X + a.X, Y + a.Y, Z + a.Z, W + a.W);
    }

    inline v4 operator- () const {
        return v4(-X, -Y, -Z, -W);
    }

    inline v4 operator- (const v4 &a) const {
        return *this + (-a);
    }

    inline v4 operator* (r32 s) const {
        return v4(s * X, s * Y, s * Z, s * W);
    }
};

inline v4 operator* (r32 s, const v4 &a) {
    return a * s;
}

struct quat {
    r32 R;
    v3 V;

    quat() {
       R = 0.0f; V = v3(0.0f);
    }

    quat(const quat &q) {
        R = q.R; V = q.V;
    }

    quat(r32 x) {
        R = x; V = v3(x);
    }

    quat(r32 r, v3 v) {
        R = r; V = v;
    }

    quat(r32 r, r32 i, r32 j, r32 k) {
        R = r; V[0] = i; V[1] = j; V[2] = k;
    }

    quat(const r32 *x) {
        R = x[0]; V = v3(&x[1]);
    }

    // NOTE(Jovan): Angle gets halved
    quat(const v3 &axis, r32 angle) {
        r32 Rad = (angle / 2.0f) * RAD;
        r32 Sin = SIN(Rad);
        r32 Cos = COS(Rad);
        R = Cos; V = Sin * (axis.GetNormalized());
    }

    inline quat& Normalize() {
        r32 L = Magnitude();
        
        return *this;
    }

    inline quat GetNormalized() {
        r32 L = Magnitude();
        return quat(R / L, V / L);
    }

    inline r32 Magnitude() const {
        return SQRT(R * R + V * V);
    }
    
    inline r32& operator[] (i32 i) {
        return i == 0 ? R : V[i - 1];
    }

    inline const r32& operator[] (i32 i) const {
        return i == 0 ? R : V[i - 1];
    }

    inline quat& operator= (const quat &q) {
        R = q.R; V = q.V;
        return *this;
    }

    inline quat& operator+= (const quat &q) {
        R += q.R; V += q.V;
        return *this;
    }

    inline quat& operator-= (const quat &q) {
        R -= q.R; V -= q.V;
        return *this;
    }

    inline quat& operator/= (r32 s) {
        R /= s; V /= s;
        return *this;
    }

    inline quat operator+ (const quat &q) {
        return quat(R + q.R, V + q.V);
    }

    inline quat operator- () {
        return quat(-R, -V);
    }

    inline quat operator- (const quat &q) {
        return quat(R - q.R, V - q.V);
    }

    inline quat operator* (r32 s) const {
        return quat(s * R, s * V);
    }

    inline quat operator/ (r32 s) const {
        return quat(R / s, V / s);
    }

    // NOTE(Jovan): Dot product
    inline r32 operator* (const quat &q) const {
        return R * q.R + V * q.V;
    }

    // NOTE(Jovan): Hamilton product
    inline quat operator^ (const quat &q) const {
        return quat(R * q.R    - V[0] * q.V[0] - V[1] * q.V[1] - V[2] * q.V[2],
                    R * q.V[0] + V[0] * q.R    + V[1] * q.V[2] - V[2] * q.V[1],
                    R * q.V[1] - V[0] * q.V[2] + V[1] * q.R    + V[2] * q.V[0],
                    R * q.V[2] + V[0] * q.V[1] - V[1] * q.V[0] + V[2] * q.R);
    }

    // NOTE(Jovan): Conjugate
    inline quat operator!() {
        return quat(R, -V);
    }

    // NOTE(Jovan): Inverse
    inline quat operator~() {
        return (!(*this)) / ((*this) * (*this));
    }

};

inline quat operator* (r32 s, const quat &q) {
    return q * s;
}

// NOTE(Jovan): Row major
struct m44 {
    union {
        v4 Values[4];
        struct {
            r32 X0, Y0, Z0, W0;
            r32 X1, Y1, Z1, W1;
            r32 X2, Y2, Z2, W2;
            r32 X3, Y3, Z3, W3;
        };
    };

    m44() {
        for(u8 i = 0; i < 4; ++i) {
            for(u8 j = 0; j < 4; ++j) {
                Values[i][j] = 0.0f;
            }
        }
    }

    m44(const m44 &m) {
            X0 = m.X0; X1 = m.X1; X2 = m.X2; X3 = m.X3;
            Y0 = m.Y0; Y1 = m.Y1; Y2 = m.Y2; Y3 = m.Y3;
            Z0 = m.Z0; Z1 = m.Z1; Z2 = m.Z2; Z3 = m.Z3;
            W0 = m.W0; W1 = m.W1; W2 = m.W2; W3 = m.W3;
    }

    m44(r32 x) {
        for(u8 i = 0; i < 4; ++i) {
            for(u8 j = 0; j < 4; ++j) {
                Values[i][j] = 0.0f;
            }
        }
        X0 = x; Y1 = x; Z2 = x; W3 = x;
    }

    m44(const r32 *x) {
        for(u8 i = 0; i < 4; ++i) {
            for(u8 j = 0; j < 4; ++j) {
                Values[i][j] = x[i * 4 + j];
            }
        }
    }

    m44(r32 *x) {
        for(u8 i = 0; i < 4; ++i) {
            for(u8 j = 0; j < 4; ++j) {
                Values[i][j] = x[i * 4 + j];
            }
        }
    }

    m44(const v4 &x, const v4 &y, const v4 &z, const v4&w) {
        Values[0] = x; Values[1] = y; Values[2] = z; Values[3] = w;
    }

    m44(r32 x0, r32 y0, r32 z0, r32 w0,
        r32 x1, r32 y1, r32 z1, r32 w1,
        r32 x2, r32 y2, r32 z2, r32 w2,
        r32 x3, r32 y3, r32 z3, r32 w3) {
            X0 = x0; X1 = x1; X2 = x2; X3 = x3;
            Y0 = y0; Y1 = y1; Y2 = y2; Y3 = y3;
            Z0 = z0; Z1 = z1; Z2 = z2; Z3 = z3;
            W0 = w0; W1 = w1; W2 = w2; W3 = w3;
    }

    m44(const quat &q) {
        X0 = 1.0f - 2.0f * (q.V.Y *  q.V.Y + q.V.Z * q.V.Z); Y0 = 2.0f * (q.V.X * q.V.Y + q.R * q.V.Z);          Z0 = 2.0f * (q.V.X * q.V.Z - q.R * q.V.Y);          W0 = 0.0f;
        X1 = 2.0f * (q.V.X * q.V.Y - q.R * q.V.Z);           Y1 = 1.0f - 2.0f * (q.V.X * q.V.X + q.V.Z * q.V.Z); Z1 = 2.0f * (q.V.Y * q.V.Z + q.R * q.V.X);          W1 = 0.0f;
        X2 = 2.0f * (q.V.X * q.V.Z + q.R * q.V.Y);           Y2 = 2.0f * (q.V.Y * q.V.Z - q.R * q.V.X);          Z2 = 1.0f - 2.0f * (q.V.X * q.V.X + q.V.Y * q.V.Y); W2 = 0.0f;
        X3 = 0.0f;                                           Y3 = 0.0f;                                          Z3 = 0.0f;                                          W3 = 1.0f;
    }

    void LoadIdentity() {
        X0 = 1.0f; Y0 = 0.0f; Z0 = 0.0f; W0 = 0.0f;
        X1 = 0.0f; Y1 = 1.0f; Z1 = 0.0f; W1 = 0.0f;
        X2 = 0.0f; Y2 = 0.0f; Z2 = 1.0f; W2 = 0.0f;
        X3 = 0.0f; Y3 = 0.0f; Z3 = 0.0f; W3 = 1.0f;
    }

    r32 Determinant() const {
        // NOTE(Jovan): Determinants of 2x2
        r32 Z1W0 = Z1 * W0 - Z0 * W1;
        r32 Z2W0 = Z2 * W0 - Z0 * W2;
        r32 Z2W1 = Z2 * W1 - Z1 * W2;
        r32 Z3W0 = Z3 * W0 - Z0 * W3;
        r32 Z3W1 = Z3 * W1 - Z1 * W3;
        r32 Z3W2 = Z3 * W2 - Z2 * W3;

        r32 Min00 = Y2 * Z1W0 - Y1 * Z2W0 + Y0 * Z2W1;
        r32 Min01 = Y3 * Z1W0 - Y1 * Z3W0 + Y0 * Z3W1;
        r32 Min02 = Y3 * Z2W0 - Y2 * Z3W0 + Y0 * Z3W2;
        r32 Min03 = Y3 * Z2W1 - Y2 * Z3W1 + Y1 * Z3W2;

        return X3 * Min00 - X2 * Min01 + X1 * Min02 - X0 * Min03;
    }

    inline m44& Translate(const v3 &v) {
        (*this)[3] = v.X * (*this)[0] + v.Y * (*this)[1] + v.Z * (*this)[2] + (*this)[3];
        return *this;
    }

    inline m44& Scale(const v3 &v) {
        (*this)[0] = v.X * (*this)[0];
        (*this)[1] = v.Y * (*this)[1];
        (*this)[2] = v.Z * (*this)[2];
        return *this;
    }

    // TODO(Jovan): Optimize && check why reverse mul is required
    inline m44& Rotate(const quat &q) {
        *this = m44(q) * (*this);
        return *this;
    }

    inline v4& operator[] (i32 i) {
        return Values[i];
    }

    inline const v4& operator[] (i32 i) const {
        return Values[i];
    }

    inline m44& operator= (const m44 &a) {
        X0 = a.X0; Y0 = a.Y0; Z0 = a.Z0; W0 = a.W0;
        X1 = a.X1; Y1 = a.Y1; Z1 = a.Z1; W1 = a.W1;
        X2 = a.X2; Y2 = a.Y2; Z2 = a.Z2; W2 = a.W2;
        X3 = a.X3; Y3 = a.Y3; Z3 = a.Z3; W3 = a.W3;
        return *this;
    }

    inline m44& operator*= (const m44 &a) {
        *this = m44(a.X0 * X0 + a.X1 * Y0 + a.X2 * Z0 + a.X3 * W0,
            a.Y0 * X0 + a.Y1 * Y0 + a.Y2 * Z0 + a.Y3 * W0,
            a.Z0 * X0 + a.Z1 * Y0 + a.Z2 * Z0 + a.Z3 * W0,
            a.W0 * X0 + a.W1 * Y0 + a.W2 * Z0 + a.W3 * W0,
            a.X0 * X1 + a.X1 * Y1 + a.X2 * Z1 + a.X3 * W1,
            a.Y0 * X1 + a.Y1 * Y1 + a.Y2 * Z1 + a.Y3 * W1,
            a.Z0 * X1 + a.Z1 * Y1 + a.Z2 * Z1 + a.Z3 * W1,
            a.W0 * X1 + a.W1 * Y1 + a.W2 * Z1 + a.W3 * W1,
            a.X0 * X2 + a.X1 * Y2 + a.X2 * Z2 + a.X3 * W2,
            a.Y0 * X2 + a.Y1 * Y2 + a.Y2 * Z2 + a.Y3 * W2,
            a.Z0 * X2 + a.Z1 * Y2 + a.Z2 * Z2 + a.Z3 * W2,
            a.W0 * X2 + a.W1 * Y2 + a.W2 * Z2 + a.W3 * W2,
            a.X0 * X3 + a.X1 * Y3 + a.X2 * Z3 + a.X3 * W3,
            a.Y0 * X3 + a.Y1 * Y3 + a.Y2 * Z3 + a.Y3 * W3,
            a.Z0 * X3 + a.Z1 * Y3 + a.Z2 * Z3 + a.Z3 * W3,
            a.W0 * X3 + a.W1 * Y3 + a.W2 * Z3 + a.W3 * W3);
        return *this;
    }

    
    inline m44 operator* (const m44 &a) const {
        m44 T(*this);
        T *= a;
        return T;
    }

    inline m44 operator+ (const m44 &a) const {
        return m44(Values[0] + a[0], Values[1] + a[1], Values[2] + a[2], Values[3] + a[3]);
    }

    inline m44 operator- () const {
        return m44(-Values[0], -Values[1], -Values[2], -Values[3]);
    }

    inline m44 operator- (const m44 &a) const {
        return m44(Values[0] + (-a[0]), Values[1] + (-a[1]), Values[2] + (-a[2]), Values[3] + (-a[3]));
    }

    inline m44 operator* (r32 s) const {
        return m44(s * Values[0], s * Values[1], s * Values[2], s * Values[3]);
    }

    // NOTE(Jovan): Transpose
    inline m44 operator! () {
        return m44(X0, X1, X2, X3,
                   Y0, Y1, Y2, Y3,
                   Z0, Z1, Z2, Z3,
                   W0, W1, W2, W3);
    }

    // NOTE(Jovan): Inverse
    m44 operator~ () {
        // NOTE(Jovan): Determinants of 2x2
        r32 Y1Z0 = Y1 * Z0 - Y0 * Z1;
        r32 Y2Z0 = Y2 * Z0 - Y0 * Z2;
        r32 Y2Z1 = Y2 * Z1 - Y1 * Z2;
        r32 Y1W0 = Y1 * W0 - Y0 * W1;
        r32 Y2W0 = Y2 * W0 - Y0 * W2;
        r32 Y2W1 = Y2 * W1 - Y1 * W2;
        r32 Y3Z0 = Y3 * Z0 - Y0 * Z3;
        r32 Y3Z1 = Y3 * Z1 - Y1 * Z3;
        r32 Y3Z2 = Y3 * Z2 - Y2 * Z3;
        r32 Y3W0 = Y3 * W0 - Y0 * W3;
        r32 Y3W1 = Y3 * W1 - Y1 * W3;
        r32 Y3W2 = Y3 * W2 - Y2 * W3;
        r32 Z1W0 = Z1 * W0 - Z0 * W1;
        r32 Z2W0 = Z2 * W0 - Z0 * W2;
        r32 Z2W1 = Z2 * W1 - Z1 * W2;
        r32 Z3W0 = Z3 * W0 - Z0 * W3;
        r32 Z3W1 = Z3 * W1 - Z1 * W3;
        r32 Z3W2 = Z3 * W2 - Z2 * W3;

        // NOTE(Jovan): Matrix minors
        r32 Min00 = Y2 * Z1W0 - Y1 * Z2W0 + Y0 * Z2W1;
        r32 Min01 = Y3 * Z1W0 - Y1 * Z3W0 + Y0 * Z3W1;
        r32 Min02 = Y3 * Z2W0 - Y2 * Z3W0 + Y0 * Z3W2;
        r32 Min03 = Y3 * Z2W1 - Y2 * Z3W1 + Y1 * Z3W2;
        r32 Min10 = X2 * Z1W0 - X1 * Z2W0 + X0 * Z2W1;
        r32 Min11 = X3 * Z1W0 - X1 * Z3W0 + X0 * Z3W1;
        r32 Min12 = X3 * Z2W0 - X2 * Z3W0 + X0 * Z3W2;
        r32 Min13 = X3 * Z2W1 - X2 * Z3W1 + X1 * Z3W2;
        r32 Min20 = X2 * Y1W0 - X1 * Y2W0 + X0 * Y2W1;
        r32 Min21 = X3 * Y1W0 - X1 * Y3W0 + X0 * Y3W1;
        r32 Min22 = X3 * Y2W0 - X2 * Y3W0 + X0 * Y3W2;
        r32 Min23 = X3 * Y2W1 - X2 * Y3W1 + X1 * Y3W2;
        r32 Min30 = X2 * Y1Z0 - X1 * Y2Z0 + X0 * Y2Z1;
        r32 Min31 = X3 * Y1Z0 - X1 * Y3Z0 + X0 * Y3Z1;
        r32 Min32 = X3 * Y2Z0 - X2 * Y3Z0 + X0 * Y3Z2;
        r32 Min33 = X3 * Y2Z1 - X2 * Y3Z1 + X1 * Y3Z2;
        

        // NOTE(Jovan): Determinant of 4x4
        r32 IDet = 1.0f / (X3 * Min00 - X2 * Min01 + X1 * Min02 - X0 * Min03);

        m44 TransposedCofactor = m44(-Min03, Min02, -Min01, Min00,
                        Min13, -Min12, Min11, -Min10,
                        -Min23, Min22, -Min21, Min20,
                        Min33, -Min32, Min31, -Min30);
        // NOTE(Jovan): Finally
        return TransposedCofactor * IDet;
    }
};

inline m44 operator* (r32 s, const m44 &a) {
    return a * s;
}

inline v4 operator* (const v4 &v, const m44 &m) {
    return v4(v[0] * m[0] + v[1] * m[1] + v[2] * m[2] + v[3] * m[3]);
} 

inline m44 perspective(r32 angleFOVY, r32 aspectRatio, r32 near, r32 far) {
    r32 F = 1.0f / TAN((angleFOVY * RAD) / 2.0f);
    return m44(F / aspectRatio, 0.0f, 0.0f,                              0.0f,
              0.0f,                F, 0.0f,                              0.0f, 
              0.0f,             0.0f, (far + near) / (near - far),      -1.0f,
              0.0f,             0.0f, 2.0f * far * near / (near - far),  0.0f);
}

m44 lookAt(const v3 &eye, const v3 &center, const v3 &up) {
    v3 F = (center - eye).GetNormalized();
    v3 S = (F ^ up).GetNormalized();
    v3 U = S ^ F;
    return m44(S.X,          U.X,       -F.X,     0.0f,
               S.Y,          U.Y,       -F.Y,     0.0f,
               S.Z,          U.Z,       -F.Z,     0.0f,
               -(S * eye),   -(U * eye), F * eye, 1.0f);
}

#endif