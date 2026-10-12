# Example fonts

The examples, the tests and the C# binding's checks load these fonts. The
examples' build stages the two the examples load, with the licence, in
`bin/data/fonts/`.

| File | What it is | Source |
| --- | --- | --- |
| `DejaVuSans.ttf` | DejaVu Sans 2.37 | [DejaVu fonts 2.37 release](https://github.com/dejavu-fonts/dejavu-fonts/releases/tag/version_2_37), `dejavu-fonts-ttf-2.37.tar.bz2`, unchanged |
| `DejaVuSansMono.ttf` | DejaVu Sans Mono 2.37 | the same release, unchanged |
| `DejaVuSansMono-bitmap.png` | A bitmap font for `monstro_font_load_bitmap`: printable ASCII from space in a 16 x 6 grid of 32-pixel cells, white with coverage in alpha | rendered from `DejaVuSansMono.ttf` by [`tools/make_bitmap_font.c`](../../tools/make_bitmap_font.c), whose header gives the command |

## Licence

DejaVu fonts are (c) Bitstream (Bitstream Vera licence); DejaVu's changes are
in the public domain; glyphs imported from the Arev fonts are (c) Tavmjong Bah.
The licence, from the release, is in [`LICENSE-DejaVu.txt`](LICENSE-DejaVu.txt)
and covers all three files, the bitmap font being a rendering of DejaVu Sans
Mono. It permits redistribution, including in commercial software, as long as
the notice goes with the fonts; a modified font must be renamed.
