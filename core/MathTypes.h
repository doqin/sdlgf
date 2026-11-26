#pragma once
// Math and colour types used throughout the framework.
//
// Replaces the D3DX9 math library the code base grew up on. The conventions are the ones the
// existing game code was written against, so values are interchangeable with the old types:
//   * row vectors: a point is transformed as v * M, and M1 * M2 applies M1 first
//   * Mat4 is row-major, 16 floats, same layout as D3DXMATRIX (so it can be copied straight
//     into one while the legacy renderer still exists)
//   * Color is 0xAARRGGBB; ColorF is the same colour as four floats in [0, 1]
// The tests compare every function against the real D3DX implementation while that is still
// available (tests/math_test.cpp).
#include <cmath>
#include <cstdint>

namespace sdlgf {
	inline constexpr float Pi = 3.14159265358979323846f;
	constexpr float ToRadians(float degrees) { return degrees * (Pi / 180.0f); }
	constexpr float ToDegrees(float radians) { return radians * (180.0f / Pi); }

	/// 0xAARRGGBB
	using Color = std::uint32_t;
}

/// Builds a Color from 0-255 channels (arguments are masked to 8 bits).
#define COLOR_ARGB(a, r, g, b) ((::sdlgf::Color)((((a) & 0xff) << 24) | (((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff)))
#define COLOR_XRGB(r, g, b) COLOR_ARGB(0xff, r, g, b)

namespace sdlgf {

	struct Vec2 {
		float x = 0.0f, y = 0.0f;

		constexpr Vec2() = default;
		constexpr Vec2(float x, float y) : x(x), y(y) {}

		constexpr Vec2 operator+() const { return *this; }
		constexpr Vec2 operator-() const { return { -x, -y }; }
		constexpr Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
		constexpr Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
		constexpr Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
		constexpr Vec2& operator/=(float s) { const float inv = 1.0f / s; x *= inv; y *= inv; return *this; }
		friend constexpr Vec2 operator+(const Vec2& a, const Vec2& b) { return { a.x + b.x, a.y + b.y }; }
		friend constexpr Vec2 operator-(const Vec2& a, const Vec2& b) { return { a.x - b.x, a.y - b.y }; }
		friend constexpr Vec2 operator*(const Vec2& v, float s) { return { v.x * s, v.y * s }; }
		friend constexpr Vec2 operator*(float s, const Vec2& v) { return { s * v.x, s * v.y }; }
		friend constexpr Vec2 operator/(const Vec2& v, float s) { const float inv = 1.0f / s; return { v.x * inv, v.y * inv }; }
		friend constexpr bool operator==(const Vec2& a, const Vec2& b) { return a.x == b.x && a.y == b.y; }
		friend constexpr bool operator!=(const Vec2& a, const Vec2& b) { return !(a == b); }
	};

	struct Vec3 {
		float x = 0.0f, y = 0.0f, z = 0.0f;

		constexpr Vec3() = default;
		constexpr Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

		constexpr Vec3 operator+() const { return *this; }
		constexpr Vec3 operator-() const { return { -x, -y, -z }; }
		constexpr Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
		constexpr Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
		constexpr Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
		constexpr Vec3& operator/=(float s) { const float inv = 1.0f / s; x *= inv; y *= inv; z *= inv; return *this; }
		friend constexpr Vec3 operator+(const Vec3& a, const Vec3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
		friend constexpr Vec3 operator-(const Vec3& a, const Vec3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
		friend constexpr Vec3 operator*(const Vec3& v, float s) { return { v.x * s, v.y * s, v.z * s }; }
		friend constexpr Vec3 operator*(float s, const Vec3& v) { return { s * v.x, s * v.y, s * v.z }; }
		friend constexpr Vec3 operator/(const Vec3& v, float s) { const float inv = 1.0f / s; return { v.x * inv, v.y * inv, v.z * inv }; }
		friend constexpr bool operator==(const Vec3& a, const Vec3& b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
		friend constexpr bool operator!=(const Vec3& a, const Vec3& b) { return !(a == b); }
	};

	struct Vec4 {
		float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;

		constexpr Vec4() = default;
		constexpr Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

		constexpr Vec4 operator+() const { return *this; }
		constexpr Vec4 operator-() const { return { -x, -y, -z, -w }; }
		constexpr Vec4& operator+=(const Vec4& o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
		constexpr Vec4& operator-=(const Vec4& o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
		constexpr Vec4& operator*=(float s) { x *= s; y *= s; z *= s; w *= s; return *this; }
		constexpr Vec4& operator/=(float s) { const float inv = 1.0f / s; x *= inv; y *= inv; z *= inv; w *= inv; return *this; }
		friend constexpr Vec4 operator+(const Vec4& a, const Vec4& b) { return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
		friend constexpr Vec4 operator-(const Vec4& a, const Vec4& b) { return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }
		friend constexpr Vec4 operator*(const Vec4& v, float s) { return { v.x * s, v.y * s, v.z * s, v.w * s }; }
		friend constexpr Vec4 operator*(float s, const Vec4& v) { return { s * v.x, s * v.y, s * v.z, s * v.w }; }
		friend constexpr Vec4 operator/(const Vec4& v, float s) { const float inv = 1.0f / s; return { v.x * inv, v.y * inv, v.z * inv, v.w * inv }; }
		friend constexpr bool operator==(const Vec4& a, const Vec4& b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }
		friend constexpr bool operator!=(const Vec4& a, const Vec4& b) { return !(a == b); }
	};

	constexpr float Dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
	constexpr float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
	constexpr float LengthSq(const Vec2& v) { return v.x * v.x + v.y * v.y; }
	constexpr float LengthSq(const Vec3& v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
	inline float Length(const Vec2& v) { return std::sqrt(LengthSq(v)); }
	inline float Length(const Vec3& v) { return std::sqrt(LengthSq(v)); }

	/// Unit vector in the same direction; the zero vector stays zero.
	inline Vec2 Normalize(const Vec2& v)
	{
		const float len = Length(v);
		return len != 0.0f ? Vec2(v.x / len, v.y / len) : Vec2(0.0f, 0.0f);
	}
	inline Vec3 Normalize(const Vec3& v)
	{
		const float len = Length(v);
		return len != 0.0f ? Vec3(v.x / len, v.y / len, v.z / len) : Vec3(0.0f, 0.0f, 0.0f);
	}

	/// 4x4 row-major matrix for row vectors (v' = v * M). Layout matches D3DXMATRIX.
	struct Mat4 {
		union {
			struct {
				float _11, _12, _13, _14;
				float _21, _22, _23, _24;
				float _31, _32, _33, _34;
				float _41, _42, _43, _44;
			};
			float m[4][4];
		};

		/// Identity.
		constexpr Mat4() : _11(1), _12(0), _13(0), _14(0), _21(0), _22(1), _23(0), _24(0),
			_31(0), _32(0), _33(1), _34(0), _41(0), _42(0), _43(0), _44(1) {}
		constexpr Mat4(float m11, float m12, float m13, float m14,
			float m21, float m22, float m23, float m24,
			float m31, float m32, float m33, float m34,
			float m41, float m42, float m43, float m44)
			: _11(m11), _12(m12), _13(m13), _14(m14), _21(m21), _22(m22), _23(m23), _24(m24),
			_31(m31), _32(m32), _33(m33), _34(m34), _41(m41), _42(m42), _43(m43), _44(m44) {}

		float& operator()(int row, int col) { return m[row][col]; }
		float operator()(int row, int col) const { return m[row][col]; }

		static constexpr Mat4 Identity() { return Mat4(); }
		static constexpr Mat4 Translation(float x, float y, float z)
		{
			Mat4 r; r._41 = x; r._42 = y; r._43 = z; return r;
		}
		static constexpr Mat4 Scaling(float x, float y, float z)
		{
			Mat4 r; r._11 = x; r._22 = y; r._33 = z; return r;
		}
		/// Rotation about Z by `radians`, in the same direction as D3DXMatrixRotationZ.
		static Mat4 RotationZ(float radians)
		{
			const float s = std::sin(radians), c = std::cos(radians);
			Mat4 r; r._11 = c; r._12 = s; r._21 = -s; r._22 = c; return r;
		}
		/// Orthographic projection, left-handed, with the given clip planes (D3DXMatrixOrthoOffCenterLH).
		static constexpr Mat4 OrthoOffCenterLH(float left, float right, float bottom, float top, float zNear, float zFar)
		{
			Mat4 r;
			r._11 = 2.0f / (right - left);
			r._22 = 2.0f / (top - bottom);
			r._33 = 1.0f / (zFar - zNear);
			r._41 = (left + right) / (left - right);
			r._42 = (top + bottom) / (bottom - top);
			r._43 = zNear / (zNear - zFar);
			return r;
		}
		/// Writes the inverse of `in` to `*out` and returns true. If `in` is singular returns false
		/// and leaves `*out` untouched (as D3DXMatrixInverse did).
		static bool Inverse(Mat4* out, const Mat4& in);
	};

	/// Matrix product: applying `a * b` to a row vector applies `a` first, then `b`.
	inline Mat4 operator*(const Mat4& a, const Mat4& b)
	{
		Mat4 r;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
			}
		}
		return r;
	}
	inline Mat4& operator*=(Mat4& a, const Mat4& b) { a = a * b; return a; }

	/// v * M for a 4D row vector.
	constexpr Vec4 Transform(const Vec4& v, const Mat4& m)
	{
		return {
			v.x * m._11 + v.y * m._21 + v.z * m._31 + v.w * m._41,
			v.x * m._12 + v.y * m._22 + v.z * m._32 + v.w * m._42,
			v.x * m._13 + v.y * m._23 + v.z * m._33 + v.w * m._43,
			v.x * m._14 + v.y * m._24 + v.z * m._34 + v.w * m._44,
		};
	}

	/// Transforms the point (v, 1) and divides by the resulting w (D3DXVec3TransformCoord).
	/// A resulting w of zero gives the zero vector.
	inline Vec3 TransformCoord(const Vec3& v, const Mat4& m)
	{
		const Vec4 r = Transform(Vec4(v.x, v.y, v.z, 1.0f), m);
		return r.w != 0.0f ? Vec3(r.x / r.w, r.y / r.w, r.z / r.w) : Vec3(0.0f, 0.0f, 0.0f);
	}

	inline bool Mat4::Inverse(Mat4* out, const Mat4& in)
	{
		// Cofactor expansion in double precision: the determinant of a nearly singular matrix
		// is where float cancellation hurts.
		const auto a = [&](int r, int c) { return static_cast<double>(in.m[r][c]); };
		double inv[16];
		const double m00 = a(0, 0), m01 = a(0, 1), m02 = a(0, 2), m03 = a(0, 3);
		const double m10 = a(1, 0), m11 = a(1, 1), m12 = a(1, 2), m13 = a(1, 3);
		const double m20 = a(2, 0), m21 = a(2, 1), m22 = a(2, 2), m23 = a(2, 3);
		const double m30 = a(3, 0), m31 = a(3, 1), m32 = a(3, 2), m33 = a(3, 3);

		inv[0] = m11 * (m22 * m33 - m23 * m32) - m12 * (m21 * m33 - m23 * m31) + m13 * (m21 * m32 - m22 * m31);
		inv[1] = -(m01 * (m22 * m33 - m23 * m32) - m02 * (m21 * m33 - m23 * m31) + m03 * (m21 * m32 - m22 * m31));
		inv[2] = m01 * (m12 * m33 - m13 * m32) - m02 * (m11 * m33 - m13 * m31) + m03 * (m11 * m32 - m12 * m31);
		inv[3] = -(m01 * (m12 * m23 - m13 * m22) - m02 * (m11 * m23 - m13 * m21) + m03 * (m11 * m22 - m12 * m21));

		inv[4] = -(m10 * (m22 * m33 - m23 * m32) - m12 * (m20 * m33 - m23 * m30) + m13 * (m20 * m32 - m22 * m30));
		inv[5] = m00 * (m22 * m33 - m23 * m32) - m02 * (m20 * m33 - m23 * m30) + m03 * (m20 * m32 - m22 * m30);
		inv[6] = -(m00 * (m12 * m33 - m13 * m32) - m02 * (m10 * m33 - m13 * m30) + m03 * (m10 * m32 - m12 * m30));
		inv[7] = m00 * (m12 * m23 - m13 * m22) - m02 * (m10 * m23 - m13 * m20) + m03 * (m10 * m22 - m12 * m20);

		inv[8] = m10 * (m21 * m33 - m23 * m31) - m11 * (m20 * m33 - m23 * m30) + m13 * (m20 * m31 - m21 * m30);
		inv[9] = -(m00 * (m21 * m33 - m23 * m31) - m01 * (m20 * m33 - m23 * m30) + m03 * (m20 * m31 - m21 * m30));
		inv[10] = m00 * (m11 * m33 - m13 * m31) - m01 * (m10 * m33 - m13 * m30) + m03 * (m10 * m31 - m11 * m30);
		inv[11] = -(m00 * (m11 * m23 - m13 * m21) - m01 * (m10 * m23 - m13 * m20) + m03 * (m10 * m21 - m11 * m20));

		inv[12] = -(m10 * (m21 * m32 - m22 * m31) - m11 * (m20 * m32 - m22 * m30) + m12 * (m20 * m31 - m21 * m30));
		inv[13] = m00 * (m21 * m32 - m22 * m31) - m01 * (m20 * m32 - m22 * m30) + m02 * (m20 * m31 - m21 * m30);
		inv[14] = -(m00 * (m11 * m32 - m12 * m31) - m01 * (m10 * m32 - m12 * m30) + m02 * (m10 * m31 - m11 * m30));
		inv[15] = m00 * (m11 * m22 - m12 * m21) - m01 * (m10 * m22 - m12 * m20) + m02 * (m10 * m21 - m11 * m20);

		const double det = m00 * inv[0] + m01 * inv[4] + m02 * inv[8] + m03 * inv[12];
		if (det == 0.0) return false;
		const double invDet = 1.0 / det;
		for (int r = 0; r < 4; ++r) {
			for (int c = 0; c < 4; ++c) {
				out->m[r][c] = static_cast<float>(inv[r * 4 + c] * invDet);
			}
		}
		return true;
	}

	/// RGBA colour as floats in [0, 1].
	struct ColorF {
		float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;

		constexpr ColorF() = default;
		constexpr ColorF(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
		/// From 0xAARRGGBB.
		constexpr ColorF(Color argb)
			: r(static_cast<float>((argb >> 16) & 0xff) * (1.0f / 255.0f)),
			g(static_cast<float>((argb >> 8) & 0xff) * (1.0f / 255.0f)),
			b(static_cast<float>(argb & 0xff) * (1.0f / 255.0f)),
			a(static_cast<float>((argb >> 24) & 0xff) * (1.0f / 255.0f)) {}

		/// To 0xAARRGGBB: each channel clamped to [0, 1] and rounded to the nearest 8-bit value.
		constexpr operator Color() const
		{
			return (to8(a) << 24) | (to8(r) << 16) | (to8(g) << 8) | to8(b);
		}

		friend constexpr ColorF operator+(const ColorF& x, const ColorF& y) { return { x.r + y.r, x.g + y.g, x.b + y.b, x.a + y.a }; }
		friend constexpr ColorF operator-(const ColorF& x, const ColorF& y) { return { x.r - y.r, x.g - y.g, x.b - y.b, x.a - y.a }; }
		friend constexpr ColorF operator*(const ColorF& c, float s) { return { c.r * s, c.g * s, c.b * s, c.a * s }; }
		friend constexpr ColorF operator*(float s, const ColorF& c) { return { s * c.r, s * c.g, s * c.b, s * c.a }; }
		friend constexpr bool operator==(const ColorF& x, const ColorF& y) { return x.r == y.r && x.g == y.g && x.b == y.b && x.a == y.a; }
		friend constexpr bool operator!=(const ColorF& x, const ColorF& y) { return !(x == y); }

	private:
		static constexpr Color to8(float v)
		{
			return v >= 1.0f ? 0xffu : v <= 0.0f ? 0x00u : static_cast<Color>(v * 255.0f + 0.5f);
		}
	};
}
