// Engine tests. No dependencies: build with tests/Makefile and run ./engine_tests.

#include "../src/engine/ArpEngine.h"
#include "../src/engine/Patterns.h"
#include "../src/engine/Scales.h"

#include <algorithm>
#include <cstdio>
#include <functional>
#include <map>
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
            const int len = (int) std::min<long> (blockSize, until - now);
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
            transport.ppqPosition = ppqBase + (double) now * bpm / 60.0 / sr;

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
        CHECK_EQ_VEC (arp::buildPattern (S::Up, 4),            (std::vector<int> { 0, 1, 2, 3 }));
        CHECK_EQ_VEC (arp::buildPattern (S::Down, 4),          (std::vector<int> { 3, 2, 1, 0 }));
        CHECK_EQ_VEC (arp::buildPattern (S::UpDown, 4),        (std::vector<int> { 0, 1, 2, 3, 2, 1 }));
        CHECK_EQ_VEC (arp::buildPattern (S::DownUp, 4),        (std::vector<int> { 3, 2, 1, 0, 1, 2 }));
        CHECK_EQ_VEC (arp::buildPattern (S::UpAndDown, 4),     (std::vector<int> { 0, 1, 2, 3, 3, 2, 1, 0 }));
        CHECK_EQ_VEC (arp::buildPattern (S::DownAndUp, 4),     (std::vector<int> { 3, 2, 1, 0, 0, 1, 2, 3 }));
        CHECK_EQ_VEC (arp::buildPattern (S::Converge, 4),      (std::vector<int> { 0, 3, 1, 2 }));
        CHECK_EQ_VEC (arp::buildPattern (S::Diverge, 4),       (std::vector<int> { 2, 1, 3, 0 }));
        CHECK_EQ_VEC (arp::buildPattern (S::ConAndDiverge, 4), (std::vector<int> { 0, 3, 1, 2, 1, 3 }));
        CHECK_EQ_VEC (arp::buildPattern (S::PinkyUp, 4),       (std::vector<int> { 0, 3, 1, 3, 2, 3 }));
        CHECK_EQ_VEC (arp::buildPattern (S::PinkyUpDown, 4),   (std::vector<int> { 0, 3, 1, 3, 2, 3, 1, 3 }));
        CHECK_EQ_VEC (arp::buildPattern (S::ThumbUp, 4),       (std::vector<int> { 0, 1, 0, 2, 0, 3 }));
        CHECK_EQ_VEC (arp::buildPattern (S::ThumbUpDown, 4),   (std::vector<int> { 0, 1, 0, 2, 0, 3, 0, 2 }));
        CHECK_EQ_VEC (arp::buildPattern (S::ChordTrigger, 4),  (std::vector<int> { arp::chordStep }));
    } },

    { "patterns with one and two notes", []
    {
        for (int s = 0; s < arp::numStyles; ++s)
        {
            const auto style = (arp::Style) s;
            const auto one = arp::buildPattern (style, 1);
            CHECK (one.size() == 1);
            const auto two = arp::buildPattern (style, 2);
            CHECK (! two.empty());
            for (int v : two)
                CHECK (v == arp::chordStep || (v >= 0 && v < 2));
        }
        CHECK (arp::buildPattern (arp::Style::Up, 0).empty());
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
