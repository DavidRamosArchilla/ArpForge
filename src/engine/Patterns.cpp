#include "Patterns.h"
#include <algorithm>
#include <array>

namespace arp
{
namespace
{
    std::vector<int> up (int n)
    {
        std::vector<int> s;
        for (int i = 0; i < n; ++i)
            s.push_back (i);
        return s;
    }

    std::vector<int> down (int n)
    {
        auto s = up (n);
        std::reverse (s.begin(), s.end());
        return s;
    }

    std::vector<int> converge (int n)
    {
        std::vector<int> s;
        for (int lo = 0, hi = n - 1; lo <= hi; ++lo, --hi)
        {
            s.push_back (lo);
            if (lo != hi)
                s.push_back (hi);
        }
        return s;
    }

    void append (std::vector<int>& a, const std::vector<int>& b, size_t from, size_t to)
    {
        for (size_t i = from; i < to && i < b.size(); ++i)
            a.push_back (b[i]);
    }

    /** Note order for the classic styles, as indices from the lowest note. */
    std::vector<int> noteOrder (Style style, int n)
    {
        if (n == 1)
            return { 0 };

        const int top = n - 1;

        switch (style)
        {
            case Style::Down:        return down (n);

            case Style::UpDown:
            {
                auto s = up (n);
                for (int i = n - 2; i >= 1; --i)
                    s.push_back (i);
                return s;
            }

            case Style::DownUp:
            {
                auto s = down (n);
                for (int i = 1; i <= n - 2; ++i)
                    s.push_back (i);
                return s;
            }

            case Style::UpAndDown:
            {
                auto s = up (n);
                const auto d = down (n);
                append (s, d, 0, d.size());
                return s;
            }

            case Style::DownAndUp:
            {
                auto s = down (n);
                const auto u = up (n);
                append (s, u, 0, u.size());
                return s;
            }

            case Style::Converge:    return converge (n);

            case Style::Diverge:
            {
                auto s = converge (n);
                std::reverse (s.begin(), s.end());
                return s;
            }

            case Style::ConAndDiverge:
            {
                auto s = converge (n);
                auto d = s;
                std::reverse (d.begin(), d.end());
                // Skip the shared middle note and the note the cycle restarts on.
                append (s, d, 1, d.size() - 1);
                return s;
            }

            case Style::PinkyUp:
            case Style::PinkyUpDown:
            {
                std::vector<int> s;
                for (int i = 0; i <= n - 2; ++i)
                    s.insert (s.end(), { i, top });
                if (style == Style::PinkyUpDown)
                    for (int i = n - 3; i >= 1; --i)
                        s.insert (s.end(), { i, top });
                return s;
            }

            case Style::ThumbUp:
            case Style::ThumbUpDown:
            {
                std::vector<int> s;
                for (int i = 1; i <= top; ++i)
                    s.insert (s.end(), { 0, i });
                if (style == Style::ThumbUpDown)
                    for (int i = n - 2; i >= 2; --i)
                        s.insert (s.end(), { 0, i });
                return s;
            }

            default:
                return up (n);
        }
    }

    /** The written patterns, in Style order starting at Style::Gallop. */
    constexpr std::array<std::string_view, (size_t) numStyles - (size_t) Style::Gallop> writtenPatterns {
        // Rhythmic
        "0! . 1 2 0 . 1 2 0! . 1 2 0 . 2 1",                 // Gallop
        "0! 1 2 0'! 2 1 2'! 1 0! 1 2 0'! 2 1 0'! 2",         // Tresillo (3+3+2 accents)
        "0! 0' 1 1' 2 2' 1 1'",                              // Octave Bounce
        "0! T 1 T",                                          // Alberti
        "0! 0 . 1 1 . 2 2 . 0'! 0' . 2 . 1 .",               // Stutter
        "0! . 2 . . 1 . 2' . . 0'! . 2 . 1 .",               // Syncopated
        "0! . . 1 . . 2 . . 0' . . 2 . 1 .",                 // Dotted 8ths
        "0! 1 0 2 0 0' 0 2",                                 // Pedal
        "2'! 1' 0' 1'! 0' 2 0'! 2 1 2! 1 0 1! 0 2, 0",       // Cascade (3 against 4)
        "0! 2' 1 1' 2 0'' 1 2'",                             // Ping Pong

        // Chord rhythms
        ". . C! . . . C . . . C! . . . C .",                 // Offbeat Stabs
        "C! - . C - . C - C! - . C - . C .",                 // Tresillo Chords
        "C! - - - - - C - . . . . C? . . .",                 // Charleston
        "C! . . C . . C! . . . C . C! . . .",                // Clave 3-2
        ". . . C! . . C . . . . C! . . C .",                 // Dembow
        "C! . C? . C . C? . C! . C? . C . C! .",             // Pulse Accents
        "C! C . C C . C C . C C . C! . C C",                 // Trance Gate
        "B! - U . U - B . B - U . U . U .",                  // Piano Comp
        "B! - - - U - U - B - - - U - U -",                  // Ballad
        ". . . . C! . C? . . . . . C! . C? .",               // Skank
        "C! . C C C . C C C! . C C C . C C",                 // Gallop Chords
        "C! C C C . . C - C! C . . C . C .",                 // Stutter Chords

        // Chasse, transcribed from Sytrus' "Chasse" preset: three 16ths and a
        // rest per beat, moving between bass, middle and top with some dyads.
        // B/M/T keep every hit on the same note when only one note is held.
        "B! B? M . M M+T B . M+T M+T! T! . T B? M+T . "
        "M+T M+T? B . T M+T? T . B B! B . B+T B+T B! .",       // Chasse
        "B! B? M . M M+T B . M+T M+T! T! . T' M' B'! .",      // Chasse Lift
        ". B! M T? . M+T B! M . T M! B . M+T! T B?",          // Chasse Offbeat
        "B+M! B+M? M+T . M+T B+T M+T! . B+M! M+T? B+T . M+T! B+M B+T' .", // Chasse Dyads
    };
} // namespace

std::vector<Step> parsePattern (std::string_view text)
{
    std::vector<Step> steps;
    int lastNote = -1;   // step a tie extends; a rest breaks the chain
    size_t i = 0;

    while (i < text.size())
    {
        while (i < text.size() && text[i] == ' ')
            ++i;

        const size_t start = i;
        while (i < text.size() && text[i] != ' ')
            ++i;

        if (start == i)
            break;

        const auto token = text.substr (start, i - start);
        Step step;
        step.velocity = normalVelocity;

        if (token[0] == '.' || token[0] == '-')
        {
            if (token[0] == '-' && lastNote >= 0)
                ++steps[(size_t) lastNote].length;
            else
                lastNote = -1;

            steps.push_back (step);   // a rest, or the held part of a tied note
            continue;
        }

        // One or more notes joined with '+', each with its own octave marks.
        size_t p = 0;
        while (p < token.size())
        {
            Pick pick;
            bool known = true;

            switch (token[p])
            {
                case 'C': pick.kind = Pick::Kind::Chord;  break;
                case 'B': pick.kind = Pick::Kind::Bass;   break;
                case 'M': pick.kind = Pick::Kind::Middle; break;
                case 'T': pick.kind = Pick::Kind::Top;    break;
                case 'U': pick.kind = Pick::Kind::Upper;  break;
                default:
                    known = token[p] >= '0' && token[p] <= '9';
                    pick.index = token[p] - '0';
                    break;
            }

            for (++p; p < token.size() && token[p] != '+'; ++p)
            {
                const char c = token[p];
                if (c == '\'')      ++pick.octave;
                else if (c == ',')  --pick.octave;
                else if (c == '!')  step.velocity = accentVelocity;
                else if (c == '?')  step.velocity = ghostVelocity;
            }

            if (known && step.numPicks < Step::maxPicks)
                step.picks[(size_t) step.numPicks++] = pick;

            ++p;   // skip '+'
        }

        if (step.isRest())
            continue;   // nothing recognisable: ignore the token

        lastNote = (int) steps.size();
        steps.push_back (step);
    }

    return steps;
}

bool isWrittenPattern (Style style)
{
    return (int) style >= (int) Style::Gallop;
}

std::vector<Step> buildPattern (Style style, int n)
{
    if (n <= 0)
        return {};

    if (isWrittenPattern (style))
    {
        static const auto parsed = []
        {
            std::array<std::vector<Step>, writtenPatterns.size()> all;
            for (size_t i = 0; i < all.size(); ++i)
                all[i] = parsePattern (writtenPatterns[i]);
            return all;
        }();

        return parsed[(size_t) style - (size_t) Style::Gallop];
    }

    if (style == Style::ChordTrigger)
        return { Step::of ({ Pick::Kind::Chord }) };

    std::vector<Step> steps;
    for (int index : noteOrder (style, n))
        steps.push_back (Step::of ({ Pick::Kind::Note, index }));
    return steps;
}

} // namespace arp
