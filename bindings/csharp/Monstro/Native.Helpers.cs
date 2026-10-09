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

    public static uint monstro_color_to_u32(MonstroColor c) =>
        (uint)(byte)(c.a * 255.0f + 0.5f) << 24 | (uint)(byte)(c.b * 255.0f + 0.5f) << 16 |
        (uint)(byte)(c.g * 255.0f + 0.5f) << 8 | (byte)(c.r * 255.0f + 0.5f);

    public static MonstroColor monstro_color_from_u32(uint c) => new()
    {
        r = (c & 255) / 255.0f, g = ((c >> 8) & 255) / 255.0f,
        b = ((c >> 16) & 255) / 255.0f, a = ((c >> 24) & 255) / 255.0f,
    };

    public static MonstroColor monstro_color_rgba(float r, float g, float b, float a) => new() { r = r, g = g, b = b, a = a };
    public static MonstroColor monstro_color_rgb(float r, float g, float b) => monstro_color_rgba(r, g, b, 1);
}
