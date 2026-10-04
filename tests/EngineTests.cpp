// Engine tests. No dependencies: build with tests/Makefile and run ./engine_tests.

#include "../src/engine/ArpEngine.h"
#include "../src/engine/Patterns.h"
#include "../src/engine/Scales.h"

#include <algorithm>
#include <cstdio>
#include <functional>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace
{
int failures = 0;
int checks = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++checks;                                                                \
        if (! (cond)) { ++failures; std::printf ("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
    } while (0)

template <typename T>
std::string str (const std::vector<T>& v)
{
    std::string s = "{";
    for (size_t i = 0; i < v.size(); ++i)
        s += (i ? "," : "") + std::to_string (v[i]);
    return s + "}";
}

#define CHECK_EQ_VEC(a, b)                                                       \
    do {                                                                         \
        ++checks;                                                                \
        auto va = (a); auto vb = (b);                                            \
        if (va != vb) { ++failures; std::printf ("  FAIL %s:%d  %s = %s, expected %s\n", __FILE__, __LINE__, #a, str (va).c_str(), str (vb).c_str()); } \
    } while (0)

constexpr double sr = 48000.0;
constexpr long beat = 24000;      // samples per beat at 120 bpm
constexpr long sixteenth = beat / 4;

struct Out
{
    long time;
    bool on;
    int note, velocity;
};

struct Harness
{
    arp::Engine engine;
    arp::Params params;
    arp::Transport transport;
    int blockSize = 512;
    long now = 0;
    bool playing = true;
    double bpm = 120.0;
    double ppqBase = 0.0;    // host position = ppqBase + now / samplesPerBeat

    // Host imperfections, like FL Studio's: variable block sizes, a jittery
    // song position, and blocks that repeat the previous block's position.
    bool randomBlocks = false;
    double ppqJitter = 0.0;   // +/- beats
    double staleChance = 0.0;
    std::mt19937 hostRng { 1234 };
    double lastReported = 0.0;
    std::multimap<long, arp::NoteEvent> pending;
    std::vector<Out> out;

    Harness()
    {
        engine.prepare (sr);
        engine.setParams (params);
    }

    void apply() { engine.setParams (params); }

    void on (long t, int note, int vel = 100)  { pending.insert ({ t, { 0, true, 1, note, vel } }); }
    void off (long t, int note)                { pending.insert ({ t, { 0, false, 1, note, 0 } }); }

    void run (long until)
    {
        std::vector<arp::NoteEvent> in, generated;

        while (now < until)
        {
            const int size = randomBlocks ? std::uniform_int_distribution<int> (1, 1024) (hostRng) : blockSize;
            const int len = (int) std::min<long> (size, until - now);
            in.clear();
            generated.clear();

            for (auto it = pending.begin(); it != pending.end() && it->first < now + len;)
            {
                auto e = it->second;
                e.sampleOffset = (int) (it->first - now);
                in.push_back (e);
                it = pending.erase (it);
            }

            transport.bpm = bpm;
            transport.playing = playing;
            transport.hasPosition = true;
            double reported = ppqBase + (double) now * bpm / 60.0 / sr;
            if (ppqJitter > 0.0)
                reported += std::uniform_real_distribution<double> (-ppqJitter, ppqJitter) (hostRng);
            if (staleChance > 0.0 && std::uniform_real_distribution<double> (0.0, 1.0) (hostRng) < staleChance)
                reported = lastReported;
            lastReported = reported;
            transport.ppqPosition = reported;

            engine.process (transport, len, in, generated);

            for (const auto& e : generated)
                out.push_back ({ now + e.sampleOffset, e.isNoteOn, e.note, e.velocity });

            now += len;
        }
    }

    std::vector<int> onNotes() const
    {
        std::vector<int> v;
        for (const auto& o : out)
            if (o.on)
                v.push_back (o.note);
        return v;
    }

    std::vector<long> onTimes() const
    {
        std::vector<long> v;
        for (const auto& o : out)
            if (o.on)
                v.push_back (o.time);
        return v;
    }

    std::vector<long> offTimes() const
    {
        std::vector<long> v;
        for (const auto& o : out)
            if (! o.on)
                v.push_back (o.time);
        return v;
    }

    bool balanced() const
    {
        std::map<int, int> count;
        for (const auto& o : out)
        {
            count[o.note] += o.on ? 1 : -1;
            if (count[o.note] < 0 || count[o.note] > 1)
                return false;
        }
        for (const auto& [note, c] : count)
            if (c != 0)
                return false;
        return true;
    }

    void chord (long t, std::vector<int> notes, int vel = 100)
    {
        for (int n : notes)
            on (t, n, vel);
    }

    void release (long t, std::vector<int> notes)
    {
        for (int n : notes)
            off (t, n);
    }
};

// Pattern steps as note indices (-1 = whole chord) for compact checks.
std::vector<int> order (arp::Style style, int n)
{
    std::vector<int> v;
    for (const auto& s : arp::buildPattern (style, n))
        v.push_back (s.kind == arp::Step::Kind::Chord ? -1 : s.index);
    return v;
}

std::vector<int> first (std::vector<int> v, size_t n)
{
    v.resize (std::min (v.size(), n));
    return v;
}

using Test = std::pair<const char*, std::function<void()>>;

const std::vector<Test> tests = {

    { "patterns for four notes", []
    {
        using S = arp::Style;
        CHECK_EQ_VEC (order (S::Up, 4),            (std::vector<int> { 0, 1, 2, 3 }));
        CHECK_EQ_VEC (order (S::Down, 4),          (std::vector<int> { 3, 2, 1, 0 }));
        CHECK_EQ_VEC (order (S::UpDown, 4),        (std::vector<int> { 0, 1, 2, 3, 2, 1 }));
        CHECK_EQ_VEC (order (S::DownUp, 4),        (std::vector<int> { 3, 2, 1, 0, 1, 2 }));
        CHECK_EQ_VEC (order (S::UpAndDown, 4),     (std::vector<int> { 0, 1, 2, 3, 3, 2, 1, 0 }));
        CHECK_EQ_VEC (order (S::DownAndUp, 4),     (std::vector<int> { 3, 2, 1, 0, 0, 1, 2, 3 }));
        CHECK_EQ_VEC (order (S::Converge, 4),      (std::vector<int> { 0, 3, 1, 2 }));
        CHECK_EQ_VEC (order (S::Diverge, 4),       (std::vector<int> { 2, 1, 3, 0 }));
        CHECK_EQ_VEC (order (S::ConAndDiverge, 4), (std::vector<int> { 0, 3, 1, 2, 1, 3 }));
        CHECK_EQ_VEC (order (S::PinkyUp, 4),       (std::vector<int> { 0, 3, 1, 3, 2, 3 }));
        CHECK_EQ_VEC (order (S::PinkyUpDown, 4),   (std::vector<int> { 0, 3, 1, 3, 2, 3, 1, 3 }));
        CHECK_EQ_VEC (order (S::ThumbUp, 4),       (std::vector<int> { 0, 1, 0, 2, 0, 3 }));
        CHECK_EQ_VEC (order (S::ThumbUpDown, 4),   (std::vector<int> { 0, 1, 0, 2, 0, 3, 0, 2 }));
        CHECK_EQ_VEC (order (S::ChordTrigger, 4),  (std::vector<int> { -1 }));
    } },

    { "patterns with one and two notes", []
    {
        for (int s = 0; s < arp::numStyles; ++s)
        {
            const auto style = (arp::Style) s;
            CHECK (! arp::buildPattern (style, 1).empty());
            CHECK (! arp::buildPattern (style, 2).empty());
            if (! arp::isWrittenPattern (style))
            {
                CHECK (arp::buildPattern (style, 1).size() == 1);
                for (int v : order (style, 2))
                    CHECK (v == -1 || (v >= 0 && v < 2));
            }
        }
        CHECK (arp::buildPattern (arp::Style::Up, 0).empty());
    } },

    { "pattern notation: notes, octaves, ties, rests, accents", []
    {
        using K = arp::Step::Kind;
        const auto s = arp::parsePattern ("0! 2' - . C - - B, U? T . - 1");
        CHECK (s.size() == 13);
        CHECK ((s[0] == arp::Step { K::Note, 0, 0, 1, arp::accentVelocity }));
        CHECK ((s[1] == arp::Step { K::Note, 2, 1, 2, arp::normalVelocity }));
        CHECK (s[2].kind == K::Rest && s[3].kind == K::Rest);
        CHECK (s[4].kind == K::Chord && s[4].length == 3);
        CHECK ((s[7] == arp::Step { K::Bass, 0, -1, 1, arp::normalVelocity }));
        CHECK (s[8].kind == K::Upper && s[8].velocity == arp::ghostVelocity);
        CHECK (s[9].kind == K::Top);
        CHECK (s[11].kind == K::Rest);   // a tie after a rest stays a rest
        CHECK (s[12].kind == K::Note && s[12].index == 1);
    } },

    { "every written pattern parses to whole beats", []
    {
        for (int i = (int) arp::Style::Gallop; i < arp::numStyles; ++i)
        {
            const auto steps = arp::buildPattern ((arp::Style) i, 3);
            CHECK (steps.size() >= 4 && steps.size() % 4 == 0);
            CHECK (steps.front().velocity == arp::accentVelocity || steps.front().kind == arp::Step::Kind::Rest);
        }
    } },

    { "up at 1/16 lands exactly on the grid", []
    {
        Harness h;
        h.chord (0, { 60, 64, 67 });
        h.run (4 * sixteenth + 10);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 60, 64, 67, 60, 64 }));
        CHECK_EQ_VEC (h.onTimes(), (std::vector<long> { 0, 6000, 12000, 18000, 24000 }));
        CHECK_EQ_VEC (first ({ (int) h.offTimes()[0], (int) h.offTimes()[1] }, 2), (std::vector<int> { 3000, 9000 }));
    } },

    { "timing does not depend on block size", []
    {
        std::vector<long> reference;
        for (int block : { 512, 333, 64, 1, 4096 })
        {
            Harness h;
            h.blockSize = block;
            h.chord (100, { 60, 64, 67 });
            h.release (50000, { 60, 64, 67 });
            h.run (60000);
            auto times = h.onTimes();
            for (auto t : h.offTimes())
                times.push_back (t);
            if (reference.empty())
                reference = times;
            CHECK_EQ_VEC (times, reference);
            CHECK (h.balanced());
        }
    } },

    { "releasing the keys stops the arp and closes every note", []
    {
        Harness h;
        h.chord (0, { 60, 64, 67 });
        h.release (13000, { 60, 64, 67 });
        h.run (beat * 4);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 60, 64, 67 }));
        CHECK (h.balanced());
    } },

    { "chord change at the same instant keeps the pattern going", []
    {
        Harness h;
        h.chord (0, { 60, 64, 67 });
        h.release (2 * sixteenth, { 60, 64, 67 });
        h.chord (2 * sixteenth, { 62, 65, 69 });
        h.run (4 * sixteenth - 1);
        // Retrigger off: position 2 continues on the new chord.
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 60, 64, 69, 62 }));
    } },

    { "retrigger note restarts the pattern on new notes", []
    {
        Harness h;
        h.params.retrigger = arp::Retrigger::Note;
        h.apply();
        h.chord (0, { 60, 64, 67 });
        h.release (2 * sixteenth, { 60, 64, 67 });
        h.chord (2 * sixteenth, { 62, 65, 69 });
        h.run (4 * sixteenth - 1);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 60, 64, 62, 65 }));
    } },

    { "retrigger beat restarts the pattern every beat", []
    {
        Harness h;
        h.params.retrigger = arp::Retrigger::Beat;
        h.params.retriggerBeats = 1.0;
        h.apply();
        h.chord (0, { 60, 64, 67 });
        h.run (2 * beat - 1);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 60, 64, 67, 60, 60, 64, 67, 60 }));
    } },

    { "hold keeps playing after release and a new chord replaces it", []
    {
        Harness h;
        h.params.hold = true;
        h.apply();
        h.chord (0, { 60, 64 });
        h.release (100, { 60, 64 });
        h.run (4 * sixteenth - 1);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 60, 64, 60, 64 }));

        h.on (4 * sixteenth, 70);
        h.off (4 * sixteenth + 100, 70);
        h.run (6 * sixteenth - 1);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 60, 64, 60, 64, 70, 70 }));

        h.params.hold = false;
        h.apply();
        h.run (beat * 3);
        CHECK (h.onNotes().size() == 6);
        CHECK (h.balanced());
    } },

    { "hold adds notes pressed while other keys are down", []
    {
        Harness h;
        h.params.hold = true;
        h.apply();
        h.on (0, 60);
        h.on (100, 67);
        h.off (200, 60);
        h.off (200, 67);
        h.run (4 * sixteenth - 1);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 60, 60, 67, 60 }));
    } },

    { "repeats stop the arp after n cycles", []
    {
        Harness h;
        h.params.repeats = 2;
        h.apply();
        h.chord (0, { 60, 64, 67 });
        h.run (beat * 3);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 60, 64, 67, 60, 64, 67 }));
        CHECK (h.balanced());
    } },

    { "transpose shift adds octaves per cycle", []
    {
        Harness h;
        h.params.transposeDistance = 12;
        h.params.transposeSteps = 1;
        h.apply();
        h.chord (0, { 60, 64, 67 });
        h.run (7 * sixteenth - 1);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 60, 64, 67, 72, 76, 79, 60 }));
    } },

    { "transpose key moves by scale degrees", []
    {
        Harness h;
        h.params.transposeMode = arp::TransposeMode::Key;
        h.params.transposeKey = 0;
        h.params.transposeScale = 0;   // major
        h.params.transposeDistance = 1;
        h.params.transposeSteps = 2;
        h.apply();
        h.chord (0, { 60, 64, 67 });
        h.run (9 * sixteenth - 1);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 60, 64, 67, 62, 65, 69, 64, 67, 71 }));
        CHECK (arp::transposeInKey (71, 1, 0, 0) == 72);
        CHECK (arp::transposeInKey (60, -1, 0, 0) == 59);
        CHECK (arp::transposeInKey (61, 1, 0, 0) == 63);   // C# keeps its +1 over the degree below
    } },

    { "offset rotates the pattern", []
    {
        Harness h;
        h.params.offset = 1;
        h.apply();
        h.chord (0, { 60, 64, 67 });
        h.run (4 * sixteenth - 1);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 64, 67, 60, 64 }));
    } },

    { "play order follows the order keys were pressed", []
    {
        Harness h;
        h.params.style = arp::Style::PlayOrder;
        h.apply();
        h.on (0, 67);
        h.on (0, 60);
        h.on (0, 64);
        h.run (4 * sixteenth - 1);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 67, 60, 64, 67 }));
    } },

    { "chord trigger plays every note at once", []
    {
        Harness h;
        h.params.style = arp::Style::ChordTrigger;
        h.apply();
        h.chord (0, { 60, 64, 67 });
        h.run (2 * sixteenth - 1);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 60, 64, 67, 60, 64, 67 }));
        CHECK_EQ_VEC (h.onTimes(), (std::vector<long> { 0, 0, 0, 6000, 6000, 6000 }));
    } },

    { "random other plays every note once per cycle", []
    {
        Harness h;
        h.params.style = arp::Style::RandomOther;
        h.apply();
        h.chord (0, { 60, 62, 64, 65, 67 });
        h.run (20 * sixteenth - 1);
        const auto notes = h.onNotes();
        CHECK (notes.size() == 20);
        for (size_t c = 0; c + 5 <= notes.size(); c += 5)
            CHECK ((std::set<int> (notes.begin() + (long) c, notes.begin() + (long) c + 5).size() == 5));
    } },

    { "random once repeats one order", []
    {
        Harness h;
        h.params.style = arp::Style::RandomOnce;
        h.apply();
        h.chord (0, { 60, 62, 64, 65, 67, 69 });
        h.run (12 * sixteenth - 1);
        const auto notes = h.onNotes();
        CHECK (notes.size() == 12);
        CHECK (std::equal (notes.begin(), notes.begin() + 6, notes.begin() + 6));
    } },

    { "random only plays held notes", []
    {
        Harness h;
        h.params.style = arp::Style::Random;
        h.apply();
        h.chord (0, { 60, 64, 67 });
        h.run (16 * sixteenth - 1);
        for (int n : h.onNotes())
            CHECK (n == 60 || n == 64 || n == 67);
    } },

    { "swing 16 delays the off-beat sixteenths", []
    {
        Harness h;
        h.params.groove = arp::Groove::Swing16;
        h.params.swing = 0.75;
        h.apply();
        h.chord (0, { 60, 64 });
        h.run (4 * sixteenth - 1);
        CHECK_EQ_VEC (h.onTimes(), (std::vector<long> { 0, 9000, 12000, 21000 }));
    } },

    { "stopped transport starts the arp on the first note", []
    {
        Harness h;
        h.playing = false;
        h.chord (1234, { 60, 64 });
        h.run (1234 + 2 * sixteenth + 10);
        CHECK_EQ_VEC (h.onTimes(), (std::vector<long> { 1234, 1234 + 6000, 1234 + 12000 }));
    } },

    { "playing transport: off-grid note starts at once then locks to grid", []
    {
        Harness h;
        h.chord (1000, { 60, 64 });
        h.run (2 * sixteenth + 10);
        CHECK_EQ_VEC (h.onTimes(), (std::vector<long> { 1000, 6000, 12000 }));
    } },

    { "a note just before a grid line waits for it", []
    {
        Harness h;
        h.chord (6000 - 200, { 60, 64 });
        h.run (2 * sixteenth + 10);
        CHECK_EQ_VEC (h.onTimes(), (std::vector<long> { 6000, 12000 }));
    } },

    { "loop jump realigns to the song grid", []
    {
        Harness h;
        h.params.hold = true;
        h.apply();
        h.chord (0, { 60 });
        h.off (10, 60);
        h.run (beat);
        // Song jumps back to ppq 0.1 (loop) while samples keep going.
        h.ppqBase = 0.1 - (double) beat / beat;
        h.run (beat + 2 * sixteenth);
        const auto t = h.onTimes();
        // After the jump, steps land where host ppq is a multiple of 0.25.
        CHECK (t.size() >= 6);
        CHECK (t[4] == beat + (long) (0.15 * beat));
        CHECK (t[5] == beat + (long) (0.40 * beat));
        CHECK (h.offTimes().size() >= 5);
    } },

    { "free rate uses milliseconds", []
    {
        Harness h;
        h.params.sync = false;
        h.params.freeRateMs = 100.0;
        h.apply();
        h.chord (500, { 60, 64 });
        h.run (500 + 3 * 4800 - 1);
        CHECK_EQ_VEC (h.onTimes(), (std::vector<long> { 500, 5300, 10100 }));
    } },

    { "gate above 100% ends a repeated pitch before replaying it", []
    {
        Harness h;
        h.params.gate = 1.8;
        h.apply();
        h.chord (0, { 60 });
        h.release (3 * sixteenth - 1, { 60 });
        h.run (beat * 2);
        CHECK (h.onNotes().size() == 3);
        CHECK (h.balanced());
    } },

    { "velocity decays towards the target", []
    {
        Harness h;
        h.params.velocityOn = true;
        h.params.velocityTarget = 20;
        h.params.velocityDecayMs = 500.0;
        h.apply();
        h.chord (0, { 60, 64 }, 120);
        h.run (beat * 2);
        std::vector<int> v;
        for (const auto& o : h.out)
            if (o.on)
                v.push_back (o.velocity);
        CHECK (v.front() == 120);
        CHECK (std::is_sorted (v.rbegin(), v.rend()));
        CHECK (v.back() == 20);
    } },

    { "velocity passes through when off", []
    {
        Harness h;
        h.on (0, 60, 33);
        h.on (0, 64, 99);
        h.run (2 * sixteenth - 1);
        CHECK ((h.out[0].velocity == 33 && h.out[2].velocity == 99) || h.out.size() < 3);
    } },

    { "rate change mid-play stays on the new grid", []
    {
        Harness h;
        h.chord (0, { 60, 64 });
        h.run (beat);
        h.params.syncRateBeats = 0.5;
        h.apply();
        h.run (2 * beat - 1);
        const auto t = h.onTimes();
        CHECK_EQ_VEC (std::vector<long> (t.begin() + 4, t.end()), (std::vector<long> { 24000, 36000 }));
    } },

    { "chord rhythm plays stabs on its steps only", []
    {
        Harness h;
        h.params.style = arp::Style::OffbeatStabs;
        h.apply();
        h.chord (0, { 60, 64, 67 });
        h.run (beat * 4 - 1);
        auto t = h.onTimes();
        t.erase (std::unique (t.begin(), t.end()), t.end());
        CHECK_EQ_VEC (t, (std::vector<long> { 2 * sixteenth, 6 * sixteenth, 10 * sixteenth, 14 * sixteenth }));
        CHECK (h.onNotes().size() == 12);
        CHECK (h.balanced());
    } },

    { "ties make notes longer", []
    {
        Harness h;
        h.params.style = arp::Style::Ballad;   // "B! - - - U - U - ..."
        h.params.gate = 1.0;
        h.apply();
        h.chord (0, { 48, 60, 64 });
        h.run (beat * 2);
        CHECK (h.out[0].on && h.out[0].note == 48 && h.out[0].time == 0);
        // The bass lasts four steps, the first upper chord two.
        long bassOff = -1;
        for (const auto& o : h.out)
            if (! o.on && o.note == 48 && bassOff < 0)
                bassOff = o.time;
        CHECK (bassOff == 4 * sixteenth);
        CHECK_EQ_VEC (first (h.onNotes(), 5), (std::vector<int> { 48, 60, 64, 60, 64 }));
    } },

    { "accents and ghost notes scale the velocity", []
    {
        Harness h;
        h.params.style = arp::Style::PulseAccents;   // "C! . C? . C ..."
        h.apply();
        h.on (0, 60, 100);
        h.run (5 * sixteenth - 1);
        std::vector<int> v;
        for (const auto& o : h.out)
            if (o.on)
                v.push_back (o.velocity);
        CHECK_EQ_VEC (v, (std::vector<int> { 100, 55, 85 }));
    } },

    { "pattern indices wrap into higher octaves", []
    {
        Harness h;
        h.params.style = arp::Style::OctaveBounce;   // "0! 0' 1 1' 2 2' 1 1'"
        h.apply();
        h.chord (0, { 60, 67 });
        h.run (8 * sixteenth - 1);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 60, 72, 67, 79, 72, 84, 67, 79 }));
    } },

    { "bass and upper split the chord", []
    {
        Harness h;
        h.params.style = arp::Style::PianoComp;   // "B! - U . U ..."
        h.apply();
        h.chord (0, { 48, 60, 64, 67 });
        h.run (3 * sixteenth - 1);
        CHECK_EQ_VEC (h.onNotes(), (std::vector<int> { 48, 60, 64, 67 }));
        CHECK_EQ_VEC (h.onTimes(), (std::vector<long> { 0, 2 * sixteenth, 2 * sixteenth, 2 * sixteenth }));
    } },

    { "steady steps with a jittery host position", []
    {
        for (double jitter : { 0.002, 0.01, 0.03 })
        {
            Harness h;
            h.randomBlocks = true;
            h.ppqJitter = jitter;
            h.staleChance = 0.2;
            h.chord (0, { 60, 64, 67 });
            h.run (beat * 16);

            const auto t = h.onTimes();
            CHECK (t.size() == 64);
            // The clock settles on the host position within the first beat
            // (within 5 ms), then every step is exact to 1 ms.
            int irregular = 0;
            for (size_t i = 1; i < t.size(); ++i)
            {
                const long error = std::abs ((t[i] - t[i - 1]) - sixteenth);
                if (error > (i <= 4 ? 240 : 48))
                {
                    ++irregular;
                    std::printf ("    jitter %.3f: step %zu interval %ld\n", jitter, i, t[i] - t[i - 1]);
                }
            }
            CHECK (irregular == 0);

            // Every note keeps the same length.
            std::vector<long> lengths;
            std::map<int, long> started;
            for (const auto& o : h.out)
            {
                if (o.on)
                    started[o.note] = o.time;
                else
                    lengths.push_back (o.time - started[o.note]);
            }
            const auto [lo, hi] = std::minmax_element (lengths.begin(), lengths.end());
            CHECK (*hi - *lo <= 96);
        }
    } },

    { "a real jump in the song position still realigns", []
    {
        Harness h;
        h.randomBlocks = true;
        h.ppqJitter = 0.01;
        h.params.hold = true;
        h.apply();
        h.chord (0, { 60 });
        h.off (10, 60);
        h.run (beat * 2);
        h.ppqBase = 0.1 - 2.0;   // loop back to ppq 0.1
        const long jumpAt = h.now;
        h.run (beat * 3);
        std::vector<long> after;
        for (auto t : h.onTimes())
            if (t >= jumpAt)
                after.push_back (t - jumpAt);
        CHECK (after.size() >= 3);
        // Grid lines after ppq 0.1 are at 0.25, 0.5 ... => 0.15, 0.40 beats later.
        CHECK (after.size() >= 2 && std::abs (after[0] - (long) (0.15 * beat)) < 300);
        CHECK (after.size() >= 2 && std::abs (after[1] - (long) (0.40 * beat)) < 300);
    } },

    { "release all closes sounding notes", []
    {
        arp::Engine e;
        e.prepare (sr);
        arp::Params p;
        p.gate = 2.0;
        e.setParams (p);
        std::vector<arp::NoteEvent> in { { 0, true, 1, 60, 100 } }, out;
        arp::Transport t;
        e.process (t, 256, in, out);
        CHECK (out.size() == 1);
        out.clear();
        e.releaseAll (5, out);
        CHECK (out.size() == 1 && ! out[0].isNoteOn && out[0].sampleOffset == 5);
        in.clear();
        out.clear();
        e.process (t, 48000, in, out);
        CHECK (out.empty());
    } },
};
} // namespace

int main()
{
    for (const auto& [name, fn] : tests)
    {
        const int before = failures;
        fn();
        std::printf ("%s %s\n", failures == before ? "ok  " : "FAIL", name);
    }

    std::printf ("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
