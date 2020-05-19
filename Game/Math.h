#pragma once

#include <DirectXMath.h>

// Transformation helpers

namespace math
{

enum Space
{
    Model,
    World,
    View,
    Projection,
};

template<Space S = Model>
struct Vector
{
    Vector() = default;

    explicit Vector(DirectX::FXMVECTOR vec) :
        vec(vec)
    {
    }

    Vector(float f) :
        vec(DirectX::XMVectorReplicate(f))
    {
    }

    Vector(float x, float y, float z = 0.0f, float w = 0.0f) :
        vec(DirectX::XMVectorSet(x, y, z, w))
    {
    }

    Vector& operator+=(const Vector& o)
    {
        vec += o.vec;
        return *this;
    }

    Vector& operator-=(const Vector& o)
    {
        vec -= o.vec;
        return *this;
    }

    // TODO: how to determine which components to normalize?
    Vector<S> normalized() const
    {
        return Vector<S>(DirectX::XMVector3Normalize(vec));
    }

    DirectX::XMVECTOR vec;
};

using ModelVector = Vector<Model>;
using WorldVector = Vector<World>;
using ViewVector = Vector<View>;

template<Space From, Space To = From>
struct Matrix
{
    Matrix() = default;

    explicit Matrix(DirectX::FXMMATRIX mat) :
        mat(mat)
    {
    }

    DirectX::XMMATRIX transposed() const
    {
        return DirectX::XMMatrixTranspose(mat);
    }

    DirectX::XMMATRIX mat;
};

template<Space From, Space To>
inline Vector<To> operator*(const Vector<From>& v, const Matrix<From, To>& m)
{
    return Vector<To>{ DirectX::XMVector4Transform(v.vec, m.mat) };
}

template<Space From, Space To1, Space To2>
inline Matrix<From, To2> operator*(const Matrix<From, To1>& a, const Matrix<To1, To2>& b)
{
    return Matrix<From, To2>{ DirectX::XMMatrixMultiply(a.mat, b.mat) };
}

template<Space S>
inline Vector<S> operator+(const Vector<S>& a, const Vector<S>& b)
{
    return Vector<S>{ a.vec + b.vec };
}

template<Space S>
inline Vector<S> operator-(const Vector<S>& a, const Vector<S>& b)
{
    return Vector<S>{ DirectX::XMVectorSubtract(a.vec, b.vec) };
}

template<Space S>
inline Vector<S> operator*(const Vector<S>& a, float f)
{
    return Vector<S>{ DirectX::XMVectorMultiply(a.vec, DirectX::XMVectorReplicate(f)) };
}

template<Space S>
inline Vector<S> operator*(float f, const Vector<S>& a)
{
    return Vector<S>{ a.vec * DirectX::XMVectorReplicate(f) };
}

template<Space S>
inline Vector<S> operator/(const Vector<S>& a, float f)
{
    return Vector<S>{ a.vec / f };
}

template<Space S>
inline Vector<S> operator/(float f, const Vector<S>& a)
{
    return Vector<S>{ DirectX::XMVectorReplicate(f) / a.vec };
}

// Angles

struct Radians
{
    Radians(const struct Degrees&);

    float angle;
};

struct Degrees
{
    Degrees(const Radians&);

    float angle;
};

inline Radians::Radians(const Degrees& degrees) :
    angle(DirectX::XMConvertToRadians(degrees.angle))
{
}

inline Degrees::Degrees(const Radians& radians) :
    angle(DirectX::XMConvertToDegrees(radians.angle))
{
}

}
