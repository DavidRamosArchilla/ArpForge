#pragma once

#include "ArpTypes.h"
#include <vector>

namespace arp
{

/** Sentinel step value meaning "play every held note at once". */
constexpr int chordStep = -1;

/** Builds the step order for a style as indices into the held notes sorted
    by pitch (0 = lowest). Play Order and the random styles return 0..n-1;
    the engine reorders or shuffles those itself. */
std::vector<int> buildPattern (Style style, int numNotes);

} // namespace arp
