"""Generate the branded Inno Setup wizard artwork from the GameHQ icon."""

from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[2]
ICON_PATH = ROOT / "assets" / "icons" / "gamehq.ico"
OUTPUT_DIR = ROOT / "assets" / "installer"


def vertical_gradient(size: tuple[int, int], top: str, bottom: str) -> Image.Image:
    width, height = size
    start = tuple(bytes.fromhex(top.removeprefix("#")))
    end = tuple(bytes.fromhex(bottom.removeprefix("#")))
    image = Image.new("RGB", size)
    pixels = image.load()
    for y in range(height):
        amount = y / max(height - 1, 1)
        color = tuple(round(a + (b - a) * amount) for a, b in zip(start, end))
        for x in range(width):
            pixels[x, y] = color
    return image


def load_icon(size: int) -> Image.Image:
    with Image.open(ICON_PATH) as icon:
        source = icon.ico.getimage((256, 256)).convert("RGBA")
    return source.resize((size, size), Image.Resampling.LANCZOS)


def make_large() -> Image.Image:
    image = vertical_gradient((164, 314), "#17244f", "#080d1c")
    draw = ImageDraw.Draw(image, "RGBA")
    draw.ellipse((-70, -10, 230, 290), fill=(52, 104, 255, 30))
    draw.line((163, 0, 163, 314), fill=(102, 132, 255, 70), width=1)
    icon = load_icon(104)
    image.paste(icon, (30, 88), icon)
    return image


def make_small() -> Image.Image:
    image = Image.new("RGBA", (55, 55))
    image.alpha_composite(load_icon(51), (2, 2))
    return image


def main() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    make_large().save(OUTPUT_DIR / "wizard-large.png", optimize=True)
    make_small().save(OUTPUT_DIR / "wizard-small.png", optimize=True)
    print("Generated installer wizard artwork.")


if __name__ == "__main__":
    main()
