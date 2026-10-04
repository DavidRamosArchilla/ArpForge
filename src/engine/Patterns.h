#pragma once

#include "ArpTypes.h"
#include <string_view>
#include <vector>

namespace arp
{

/** One step of a pattern: what to play, how long and how hard. */
struct Step
{
    enum class Kind : uint8_t
    {
        Note,    // one held note, by index from the lowest (wraps up into octaves)
        Chord,   // every held note
        Bass,    // the lowest held note
        Top,     // the highest held note
        Upper,   // every held note except the lowest
        Rest
    };

    Kind kind = Kind::Note;
    int index = 0;
    int octave = 0;
    int length = 1;          // in steps; ties make it longer
    float velocity = 1.0f;   // multiplies the played velocity

    bool operator== (const Step&) const = default;
};

/** Velocity multipliers used by written patterns. */
constexpr float accentVelocity = 1.0f;
constexpr float normalVelocity = 0.85f;
constexpr float ghostVelocity  = 0.55f;

/** Parses a written pattern, one token per step, separated by spaces:
      0-9   held note by index from the lowest (wraps into higher octaves)
      C     whole chord      B  bass note      T  top note      U  chord without the bass
      .     rest             -  tie (makes the previous note one step longer)
    Suffixes: '  octave up   ,  octave down   !  accent   ?  ghost note
    Example: "0! . 1 2 C - . T'" */
std::vector<Step> parsePattern (std::string_view text);

/** Builds the steps for a style. Order-based styles (Up, Down...) depend on
    the number of held notes; written patterns don't. Play Order and the
    random styles return the notes in pitch order and the engine reorders
    or shuffles them itself. */
std::vector<Step> buildPattern (Style style, int numNotes);

/** True for the styles that are written rhythms rather than note orders. */
bool isWrittenPattern (Style style);

} // namespace arp
