#include "Patterns.h"
#include <algorithm>

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
} // namespace

std::vector<int> buildPattern (Style style, int n)
{
    if (n <= 0)
        return {};

    if (n == 1)
        return style == Style::ChordTrigger ? std::vector<int> { chordStep } : std::vector<int> { 0 };

    const int top = n - 1;

    switch (style)
    {
        case Style::Up:          return up (n);
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

        case Style::ChordTrigger: return { chordStep };

        case Style::PlayOrder:
        case Style::Random:
        case Style::RandomOther:
        case Style::RandomOnce:
            break;
    }

    return up (n);
}

} // namespace arp
