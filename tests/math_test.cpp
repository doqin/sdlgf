// Tests for MathTypes.h: known values everywhere, and on Windows a randomized comparison of every
// function against the real D3DX9 implementation the types replace.
#include "MathTypes.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <random>

#if defined(SDLGF_HAVE_D3DX)
#	define NOMINMAX
#	include <windows.h>
#	include <d3dx9math.h>
#endif

static int g_failures = 0;
#define CHECK(cond) \
	do { if (!(cond)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++g_failures; } } while (0)

using namespace sdlgf;

static bool Near(float a, float b, float tol = 1e-5f)
{
	return std::fabs(a - b) <= tol * (1.0f + std::fabs(a) + std::fabs(b));
}

static bool MatNear(const Mat4& a, const Mat4& b, float tol = 1e-5f)
{
	for (int r = 0; r < 4; ++r)
		for (int c = 0; c < 4; ++c)
			if (!Near(a.m[r][c], b.m[r][c], tol)) return false;
	return true;
}

static void TestLayout()
{
	static_assert(sizeof(Mat4) == 16 * sizeof(float), "Mat4 is 16 floats");
	static_assert(sizeof(Vec2) == 8 && sizeof(Vec3) == 12 && sizeof(Vec4) == 16 && sizeof(ColorF) == 16, "vector sizes");
	Mat4 m;
	m._23 = 7.0f;
	CHECK(m.m[1][2] == 7.0f);       // _23 is row 2, column 3
	CHECK(m(1, 2) == 7.0f);
	m._41 = 5.0f;
	CHECK(&m._41 == &m.m[3][0]);     // translation lives in the last row (row-vector convention)
}

static void TestColor()
{
	CHECK(COLOR_ARGB(0x12, 0x34, 0x56, 0x78) == 0x12345678u);
	CHECK(COLOR_XRGB(0xAA, 0xBB, 0xCC) == 0xFFAABBCCu);
	CHECK(COLOR_ARGB(256 + 1, 256 + 2, 3, 4) == 0x01020304u);   // channels are masked to 8 bits

	// every 8-bit value survives Color -> ColorF -> Color
	for (unsigned v = 0; v < 256; ++v) {
		const Color c = (v << 24) | ((255 - v) << 16) | (v << 8) | ((v * 7) & 0xff);
		CHECK(static_cast<Color>(ColorF(c)) == c);
	}
	const ColorF f(0x80402010u);
	CHECK(Near(f.a, 128.0f / 255.0f) && Near(f.r, 64.0f / 255.0f) && Near(f.g, 32.0f / 255.0f) && Near(f.b, 16.0f / 255.0f));

	// clamping and rounding
	CHECK(static_cast<Color>(ColorF(2.0f, -1.0f, 0.5f, 1.0f)) == 0xFFFF0080u);
	CHECK(static_cast<Color>(ColorF(0.0f, 0.0f, 0.0f, 0.0f)) == 0u);
	CHECK(static_cast<Color>(ColorF(1.0f, 1.0f, 1.0f, 1.0f)) == 0xFFFFFFFFu);

	const Color viaImplicit = ColorF(0.2f, 0.2f, 0.2f, 1.0f);   // how the game passes ColorF where a Color is expected
	CHECK(viaImplicit == 0xFF333333u);
}

static void TestVectors()
{
	const Vec2 a(3.0f, 4.0f);
	CHECK(LengthSq(a) == 25.0f && Length(a) == 5.0f);
	CHECK(Normalize(a) == Vec2(0.6f, 0.8f));
	CHECK(Normalize(Vec2(0.0f, 0.0f)) == Vec2(0.0f, 0.0f));
	CHECK(Dot(Vec2(1, 2), Vec2(3, 4)) == 11.0f);
	CHECK(a + Vec2(1, 1) == Vec2(4.0f, 5.0f));
	CHECK(a - Vec2(1, 1) == Vec2(2.0f, 3.0f));
	CHECK(a * 2.0f == Vec2(6.0f, 8.0f) && 2.0f * a == Vec2(6.0f, 8.0f));
	CHECK(a / 2.0f == Vec2(1.5f, 2.0f));
	CHECK(-a == Vec2(-3.0f, -4.0f));
	Vec2 b = a; b += Vec2(1, 1); b *= 2.0f;
	CHECK(b == Vec2(8.0f, 10.0f));
	CHECK(Vec2() == Vec2(0.0f, 0.0f));   // zero-initialised, unlike D3DXVECTOR2
}

static void TestMatrices()
{
	CHECK(MatNear(Mat4(), Mat4::Identity(), 0));      // default-constructed is the identity
	CHECK(Mat4()._11 == 1.0f && Mat4()._22 == 1.0f && Mat4()._12 == 0.0f);

	// translate THEN scale: row-vector convention, left factor applies first
	const Mat4 ts = Mat4::Translation(10, 20, 0) * Mat4::Scaling(2, 2, 1);
	const Vec4 p = Transform(Vec4(1, 2, 0, 1), ts);
	CHECK(p.x == 22.0f && p.y == 44.0f && p.w == 1.0f);
	// the other order scales first
	const Vec4 q = Transform(Vec4(1, 2, 0, 1), Mat4::Scaling(2, 2, 1) * Mat4::Translation(10, 20, 0));
	CHECK(q.x == 12.0f && q.y == 24.0f);

	// +90 degrees takes +X to +Y in this convention (same as D3DXMatrixRotationZ)
	const Vec4 r = Transform(Vec4(1, 0, 0, 1), Mat4::RotationZ(Pi / 2.0f));
	CHECK(Near(r.x, 0.0f) && Near(r.y, 1.0f));

	// a typical camera chain and its inverse
	const Mat4 cam = Mat4::Translation(-120, -80, 0) * Mat4::Scaling(2, 2, 1) * Mat4::RotationZ(0.3f) * Mat4::Translation(480, 360, 0);
	Mat4 inv;
	CHECK(Mat4::Inverse(&inv, cam));
	CHECK(MatNear(cam * inv, Mat4::Identity(), 1e-4f));
	const Vec3 world = TransformCoord(TransformCoord(Vec3(33, 44, 0), cam), inv);
	CHECK(Near(world.x, 33.0f, 1e-4f) && Near(world.y, 44.0f, 1e-4f));

	// singular: reports failure and leaves the output alone
	Mat4 untouched = Mat4::Translation(1, 2, 3);
	CHECK(!Mat4::Inverse(&untouched, Mat4::Scaling(0, 1, 1)));
	CHECK(untouched._41 == 1.0f && untouched._42 == 2.0f && untouched._43 == 3.0f);

	// orthographic projection used by the tile map: (0,0) -> top-left, (w,h) -> bottom-right
	const Mat4 ortho = Mat4::OrthoOffCenterLH(0, 960, 720, 0, 0, 1);
	const Vec4 tl = Transform(Vec4(0, 0, 0, 1), ortho);
	const Vec4 br = Transform(Vec4(960, 720, 0, 1), ortho);
	CHECK(Near(tl.x, -1.0f) && Near(tl.y, 1.0f) && Near(br.x, 1.0f) && Near(br.y, -1.0f));

	CHECK(ToRadians(180.0f) == Pi);
	CHECK(Near(ToDegrees(Pi / 2.0f), 90.0f));
}

#if defined(SDLGF_HAVE_D3DX)
// ---- cross-check against the real D3DX9 --------------------------------------------------------

static Mat4 FromD3DX(const D3DXMATRIX& d) { Mat4 m; std::memcpy(&m, &d, sizeof(m)); return m; }
static D3DXMATRIX ToD3DX(const Mat4& m) { D3DXMATRIX d; std::memcpy(&d, &m, sizeof(d)); return d; }

static void TestAgainstD3DX()
{
	static_assert(sizeof(Mat4) == sizeof(D3DXMATRIX), "layout");
	static_assert(offsetof(Mat4, _41) == offsetof(D3DXMATRIX, _41) && offsetof(Mat4, _34) == offsetof(D3DXMATRIX, _34), "layout");

	std::mt19937 rng(12345);
	std::uniform_real_distribution<float> val(-200.0f, 200.0f), modest(-3.0f, 3.0f), angle(-7.0f, 7.0f);

	int compared = 0;
	for (int i = 0; i < 2000; ++i) {
		const float x = val(rng), y = val(rng), z = modest(rng), a = angle(rng);
		D3DXMATRIX dt, ds, dr, di;
		D3DXMatrixTranslation(&dt, x, y, z);
		D3DXMatrixScaling(&ds, modest(rng), modest(rng), modest(rng));
		D3DXMatrixRotationZ(&dr, a);
		D3DXMatrixIdentity(&di);
		CHECK(MatNear(Mat4::Translation(x, y, z), FromD3DX(dt), 0));
		CHECK(MatNear(Mat4::RotationZ(a), FromD3DX(dr), 1e-6f));
		CHECK(MatNear(Mat4::Identity(), FromD3DX(di), 0));
		const float sx = modest(rng), sy = modest(rng), sz = modest(rng);
		D3DXMatrixScaling(&ds, sx, sy, sz);
		CHECK(MatNear(Mat4::Scaling(sx, sy, sz), FromD3DX(ds), 0));

		// product of three matrices
		D3DXMATRIX dprod = dt * ds * dr;
		const Mat4 prod = Mat4::Translation(x, y, z) * Mat4::Scaling(sx, sy, sz) * Mat4::RotationZ(a);
		CHECK(MatNear(prod, FromD3DX(dprod), 1e-4f));

		// 4D transform and divide-by-w transform
		const Vec4 v(val(rng), val(rng), val(rng), modest(rng));
		D3DXVECTOR4 dv(v.x, v.y, v.z, v.w), dout;
		D3DXVec4Transform(&dout, &dv, &dprod);
		const Vec4 mine = Transform(v, prod);
		CHECK(Near(mine.x, dout.x, 1e-4f) && Near(mine.y, dout.y, 1e-4f) && Near(mine.z, dout.z, 1e-4f) && Near(mine.w, dout.w, 1e-4f));

		D3DXVECTOR3 d3(v.x, v.y, v.z), d3out;
		D3DXVec3TransformCoord(&d3out, &d3, &dprod);
		const Vec3 mine3 = TransformCoord(Vec3(v.x, v.y, v.z), prod);
		CHECK(Near(mine3.x, d3out.x, 1e-4f) && Near(mine3.y, d3out.y, 1e-4f) && Near(mine3.z, d3out.z, 1e-4f));

		// inverse of a well-conditioned affine matrix
		D3DXMATRIX dinv;
		const bool dOk = D3DXMatrixInverse(&dinv, nullptr, &dprod) != nullptr;
		Mat4 minv;
		const bool mOk = Mat4::Inverse(&minv, prod);
		CHECK(dOk == mOk);
		if (dOk && mOk) CHECK(MatNear(minv, FromD3DX(dinv), 1e-3f));

		// vector helpers
		const D3DXVECTOR2 d2(val(rng), val(rng)); D3DXVECTOR2 n2;
		D3DXVec2Normalize(&n2, &d2);
		const Vec2 m2 = Normalize(Vec2(d2.x, d2.y));
		CHECK(Near(m2.x, n2.x, 1e-5f) && Near(m2.y, n2.y, 1e-5f));
		CHECK(Near(LengthSq(Vec2(d2.x, d2.y)), D3DXVec2LengthSq(&d2), 1e-5f));
		++compared;
	}
	CHECK(compared == 2000);

	// singular and special matrices behave the same
	D3DXMATRIX dsing, dres = ToD3DX(Mat4::Translation(1, 2, 3));
	D3DXMatrixScaling(&dsing, 0, 1, 1);
	Mat4 mres;
	CHECK(D3DXMatrixInverse(&dres, nullptr, &dsing) == nullptr);   // D3DX reports a singular matrix...
	CHECK(!Mat4::Inverse(&mres, Mat4::Scaling(0, 1, 1)));         // ...and so do we

	D3DXMATRIX dortho;
	D3DXMatrixOrthoOffCenterLH(&dortho, 0, 960, 720, 0, 0, 1);
	CHECK(MatNear(Mat4::OrthoOffCenterLH(0, 960, 720, 0, 0, 1), FromD3DX(dortho), 1e-6f));
	D3DXMatrixOrthoOffCenterLH(&dortho, -10, 250.5f, -3, 99, 0.1f, 50);
	CHECK(MatNear(Mat4::OrthoOffCenterLH(-10, 250.5f, -3, 99, 0.1f, 50), FromD3DX(dortho), 1e-5f));

	// colours: the float <-> 8-bit conversions must be bit-identical (rounding and clamping included)
	std::uniform_int_distribution<unsigned> bits(0, 0xFFFFFFFFu);
	std::uniform_real_distribution<float> chan(-0.5f, 1.5f);
	for (int i = 0; i < 20000; ++i) {
		const unsigned argb = bits(rng);
		const D3DXCOLOR dc(argb);
		const ColorF mc(static_cast<Color>(argb));
		CHECK(dc.r == mc.r && dc.g == mc.g && dc.b == mc.b && dc.a == mc.a);

		const float r = chan(rng), g = chan(rng), b = chan(rng), al = chan(rng);
		const D3DXCOLOR dfc(r, g, b, al);
		const DWORD dword = dfc;
		const Color mine = ColorF(r, g, b, al);
		if (dword != mine) std::fprintf(stderr, "color mismatch for (%f,%f,%f,%f): D3DX %08X, ours %08X\n", r, g, b, al, (unsigned)dword, (unsigned)mine);
		CHECK(dword == mine);

		const int ia = (int)(bits(rng) % 600) - 100, ir = (int)(bits(rng) % 600) - 100, ig = (int)(bits(rng) % 600) - 100, ib = (int)(bits(rng) % 600) - 100;
		CHECK(static_cast<DWORD>(D3DCOLOR_ARGB(ia, ir, ig, ib)) == COLOR_ARGB(ia, ir, ig, ib));
	}

	CHECK(D3DXToRadian(37.5f) == ToRadians(37.5f));
	CHECK(D3DX_PI == Pi);
}
#endif

int main()
{
	TestLayout();
	TestColor();
	TestVectors();
	TestMatrices();
#if defined(SDLGF_HAVE_D3DX)
	TestAgainstD3DX();
	std::puts("(compared against the real D3DX9)");
#endif
	if (g_failures == 0) std::puts("math_test: all tests passed");
	else std::fprintf(stderr, "math_test: %d check(s) failed\n", g_failures);
	return g_failures == 0 ? 0 : 1;
}
