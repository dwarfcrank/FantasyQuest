#pragma once
#ifndef im3d_config_h
#define im3d_config_h

// User-defined assertion handler (default is cassert assert()).
//#define IM3D_ASSERT(e) assert(e)

// User-defined malloc/free. Define both or neither (default is cstdlib malloc()/free()).
//#define IM3D_MALLOC(size) malloc(size)
//#define IM3D_FREE(ptr) free(ptr) 

// User-defined API declaration (e.g. __declspec(dllexport)).
//#define IM3D_API

// Use a thread-local context pointer.
//#define IM3D_THREAD_LOCAL_CONTEXT_PTR 1

// Use row-major internal matrix layout. 
//#define IM3D_MATRIX_ROW_MAJOR 1

// Force vertex data alignment (default is 4 bytes).
//#define IM3D_VERTEX_ALIGNMENT 4

// Enable internal culling for primitives (everything drawn between Begin*()/End()). The application must set a culling frustum via AppData.
#define IM3D_CULL_PRIMITIVES 0

// Enable internal culling for gizmos. The application must set a culling frustum via AppData.
#define IM3D_CULL_GIZMOS 0

#include "../Game/Math.h"
#include <DirectXMath.h>

// Conversion to/from application math types.
#define IM3D_VEC2_APP \
	Vec2(const DirectX::XMFLOAT2& _v)          { x = _v.x; y = _v.y;     } \
	operator DirectX::XMFLOAT2() const         { return DirectX::XMFLOAT2(x, y); }

#define IM3D_VEC3_APP \
	Vec3(const DirectX::XMFLOAT3& _v)          { x = _v.x; y = _v.y; z = _v.z; } \
	operator DirectX::XMFLOAT3() const         { return DirectX::XMFLOAT3(x, y, z);    } \
	Vec3(const math::Vector<math::World>& _v) {										\
		DirectX::XMFLOAT3A _v2; DirectX::XMStoreFloat3A(&_v2, _v.vec);	\
        x = _v2.x; y = _v2.y; z = _v2.z; }								\
	operator math::Vector<math::World>() const             { return math::Vector<math::World>(x, y, z); }

#define IM3D_VEC4_APP \
	Vec4(const DirectX::XMFLOAT4& _v)          { x = _v.x; y = _v.y; z = _v.z; w = _v.w; } \
	operator DirectX::XMFLOAT4() const         { return DirectX::XMFLOAT4(x, y, z, w);           } \
	Vec4(const math::Vector<math::World>& _v) {										\
		DirectX::XMFLOAT4A _v2; DirectX::XMStoreFloat4A(&_v2, _v.vec);	\
        x = _v2.x; y = _v2.y; z = _v2.z; w = _v2.w; }					\
	operator math::Vector<math::World>() const             { return math::Vector<math::World>(x, y, z, w); }

//#define IM3D_MAT3_APP \
//	Mat3(const glm::mat3& _m)          { for (int i = 0; i < 9; ++i) m[i] = *(&(_m[0][0]) + i); } \
//	operator glm::mat3() const         { glm::mat3 ret; for (int i = 0; i < 9; ++i) *(&(ret[0][0]) + i) = m[i]; return ret; }

#define IM3D_MAT4_APP \
	Mat4(const DirectX::XMMATRIX& _m)  { DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)m, _m); } \
	operator DirectX::XMMATRIX() const { return DirectX::XMLoadFloat4x4((const DirectX::XMFLOAT4X4*)m); }

	
#endif // im3d_config_h
