# Golden images

Reference output of the **Direct3D 9 / D3DX renderer**, captured before it was removed. The SDL
renderer is tested against these files: same scene in, same pixels out.

| File | What it contains |
|---|---|
| `text_<font><size>.png` | 15 rows of text per font/size actually used by the game: Latin, Vietnamese, diacritics, punctuation, `&`, multi-line, word wrap (`SetMaxWidth`), colour, alpha, bold, outlines, camera zoom 2x/1.5x, fractional position. Non-antialiased, so only a handful of colours. |
| `text_metrics.txt` | `GetWidth()`/`GetHeight()` that `ID3DXFont` (`DT_CALCRECT`) reported for every row. |
| `sprites.png` | `StaticSprite` (integer/fractional position, scale 1/2/2.5/3, flip, rotation, alpha, tint, stretch, source rect), `AnimatedSprite` frames, `NineSliceSprite` at four sizes/scales, zoomed offset camera. |
| `primitives.png` | Lines (1 px, thick), rectangles, ellipses, triangles, textured polygons (incl. UVs outside [0,1]), scissor test; left half opaque, right half with alpha blending; instant and camera variants. |
| `map_*.png` | `Map` tile layers (`labinside.tmx` at two cameras, `tutorial.tmx`). |

All images are opaque RGB, on a `0x303030` background.

## Findings captured here

* **`&` is not a mnemonic prefix.** `AB` = 12 px, `A&B` = 18 px, `A&&B` = 24 px at 13 px: each `&`
  is a normal glyph, so text layout needs no prefix handling.
* Text is rendered without antialiasing (GDI `NONANTIALIASED_QUALITY`).
* Line height equals the font's cell height (13/26/39 px for the 135OpenPixel sizes).

## Regenerating

Only possible from a commit that still has the D3D9 renderer:

    cmake -S . -B build -DSDLGF_BUILD_GOLDEN_GEN=ON
    cmake --build build --config Release --target sdlgf_golden_gen
    build/bin/Release/sdlgf_golden_gen.exe tests/golden

The generator is `tools/golden/golden_gen.cpp`. It is removed together with the D3D9 renderer;
after that these files are fixtures, not regenerable output.
