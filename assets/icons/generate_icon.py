"""Assemble assets/icons/gamehq.ico from the per-size PNGs rendered by icongen.cpp.

Usage: python assets/icons/generate_icon.py <png-dir> <out.ico>
See icongen.cpp for the full regeneration recipe (SVG -> PNGs -> ICO).
"""
import sys
from pathlib import Path

from PIL import Image

SIZES = [16, 20, 24, 32, 40, 48, 64, 128, 256]


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__)
        return 1
    png_dir = Path(sys.argv[1])
    out_ico = Path(sys.argv[2])

    frames = []
    for s in SIZES:
        path = png_dir / f"icon_{s}.png"
        if not path.exists():
            print(f"missing {path}")
            return 1
        img = Image.open(path).convert("RGBA")
        if img.size != (s, s):
            print(f"{path} is {img.size}, expected {(s, s)}")
            return 1
        frames.append(img)

    # Pillow writes one ICO directory with each appended frame as its own entry,
    # so every size is a true per-size render instead of a downscale.
    base, extra = frames[-1], frames[:-1]
    base.save(
        out_ico,
        format="ICO",
        sizes=[(s, s) for s in SIZES],
        append_images=extra,
    )
    print(f"wrote {out_ico} ({out_ico.stat().st_size} bytes, {len(frames)} sizes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
