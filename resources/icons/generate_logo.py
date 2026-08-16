"""Regenerate Marknote PNG/ICO assets from the designed logo source.

Prefer a high-resolution source PNG with the blue rounded tile and white "m".
Falls back to drawing a geometric m if the source file is missing.
"""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

ROOT = Path(__file__).resolve().parent
SOURCE_CANDIDATES = [
    ROOT / "marknote-logo-source.png",
    Path(r"C:\Users\16032\.cursor\projects\c-code-ai-md\assets\marknote-logo-source.png"),
]


def rounded_mask(size: int, radius: int) -> Image.Image:
    mask = Image.new("L", (size, size), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, size - 1, size - 1), radius=radius, fill=255)
    return mask


def from_source(path: Path) -> Image.Image:
    src = Image.open(path).convert("RGBA")
    pixels = src.load()
    width, height = src.size
    for y in range(height):
        for x in range(width):
            r, g, b, _ = pixels[x, y]
            if r < 18 and g < 18 and b < 18:
                pixels[x, y] = (0, 0, 0, 0)

    mask = rounded_mask(width, int(round(width * 30 / 128)))
    out = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    out.paste(src, (0, 0), mask)
    r, g, b, a = out.split()
    out = Image.merge("RGBA", (r, g, b, ImageChops.multiply(a, mask)))
    return out


def export(master: Image.Image) -> None:
    sizes = [16, 24, 32, 48, 64, 128, 256, 512]
    images = {}
    for size in sizes:
        image = master.resize((size, size), Image.Resampling.LANCZOS)
        images[size] = image
        image.save(ROOT / f"marknote-{size}.png")
    images[256].save(ROOT / "marknote.png")
    ico_sizes = [16, 24, 32, 48, 64, 128, 256]
    ico_images = [images[size] for size in ico_sizes]
    ico_images[-1].save(
        ROOT / "marknote.ico",
        format="ICO",
        sizes=[(size, size) for size in ico_sizes],
        append_images=ico_images[:-1],
    )


def main() -> None:
    source = next((path for path in SOURCE_CANDIDATES if path.exists()), None)
    if source is None:
        raise SystemExit("No logo source PNG found")
    export(from_source(source))
    print(f"exported icons from {source}")


if __name__ == "__main__":
    main()
