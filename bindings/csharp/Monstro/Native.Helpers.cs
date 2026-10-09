using System;
using System.Runtime.InteropServices;

namespace Monstro;

public static unsafe partial class Native
{
    /// <summary>Decode a borrowed NUL-terminated UTF-8 result before its native owner changes.</summary>
    public static string? Utf8(byte* value) => value == null ? null : Marshal.PtrToStringUTF8((IntPtr)value);

    public static readonly MonstroVec2 MONSTRO_VEC2_ZERO = new();
    public static readonly MonstroVec2 MONSTRO_VEC2_INVALID = new() { x = float.NegativeInfinity, y = float.NegativeInfinity };

    // These helpers are static inline in C; they are not native exports.
    public static bool monstro_rect_contains(MonstroRect r, float px, float py) =>
        px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;

    // Channels are clamped to 0..1 first; NaN becomes 0.
    public static uint monstro_color_to_u32(MonstroColor c) =>
        ColorChannel(c.a) << 24 | ColorChannel(c.b) << 16 | ColorChannel(c.g) << 8 | ColorChannel(c.r);

    private static uint ColorChannel(float v) => v > 0.0f ? (v < 1.0f ? (uint)(v * 255.0f + 0.5f) : 255u) : 0u;


    public static MonstroColor monstro_color_from_u32(uint c) => new()
    {
        r = (c & 255) / 255.0f, g = ((c >> 8) & 255) / 255.0f,
        b = ((c >> 16) & 255) / 255.0f, a = ((c >> 24) & 255) / 255.0f,
    };

    public static MonstroColor monstro_color_rgba(float r, float g, float b, float a) => new() { r = r, g = g, b = b, a = a };
    public static MonstroColor monstro_color_rgb(float r, float g, float b) => monstro_color_rgba(r, g, b, 1);
}
