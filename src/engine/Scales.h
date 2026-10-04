#pragma once

#include <array>

namespace arp
{

struct Scale
{
    const char* name;
    int size;
    std::array<int, 12> intervals;
};

inline const std::array<Scale, 14>& scales()
{
    static const std::array<Scale, 14> list {{
        { "Major",            7, { 0, 2, 4, 5, 7, 9, 11 } },
        { "Minor",            7, { 0, 2, 3, 5, 7, 8, 10 } },
        { "Dorian",           7, { 0, 2, 3, 5, 7, 9, 10 } },
        { "Phrygian",         7, { 0, 1, 3, 5, 7, 8, 10 } },
        { "Lydian",           7, { 0, 2, 4, 6, 7, 9, 11 } },
        { "Mixolydian",       7, { 0, 2, 4, 5, 7, 9, 10 } },
        { "Locrian",          7, { 0, 1, 3, 5, 6, 8, 10 } },
        { "Harmonic Minor",   7, { 0, 2, 3, 5, 7, 8, 11 } },
        { "Melodic Minor",    7, { 0, 2, 3, 5, 7, 9, 11 } },
        { "Major Pentatonic", 5, { 0, 2, 4, 7, 9 } },
        { "Minor Pentatonic", 5, { 0, 3, 5, 7, 10 } },
        { "Blues",            6, { 0, 3, 5, 6, 7, 10 } },
        { "Whole Tone",       6, { 0, 2, 4, 6, 8, 10 } },
        { "Chromatic",       12, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } },
    }};
    return list;
}

inline int floorDiv (int a, int b)
{
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

/** Moves a note by a number of scale degrees. Notes outside the scale keep
    their chromatic distance to the scale degree just below them. */
inline int transposeInKey (int note, int degrees, int key, int scaleIndex)
{
    const auto& scale = scales()[(size_t) scaleIndex];
    const int rel = note - key;
    int octave = floorDiv (rel, 12);
    const int pc = rel - octave * 12;

    int degree = 0;
    for (int i = 0; i < scale.size; ++i)
        if (scale.intervals[(size_t) i] <= pc)
            degree = i;

    const int remainder = pc - scale.intervals[(size_t) degree];
    int target = degree + degrees;
    octave += floorDiv (target, scale.size);
    target -= floorDiv (target, scale.size) * scale.size;

    return key + octave * 12 + scale.intervals[(size_t) target] + remainder;
}

} // namespace arp
