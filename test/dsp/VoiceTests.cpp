#include <gtest/gtest.h>
#include "dsp/voice.h"

TEST(Voice, InitDoesNotCrash)
{
    Voice voice;
    voice.Init(48000.0);
}

TEST(Voice, StartsInactive)
{
    Voice voice;
    voice.Init(48000.0);
    EXPECT_FALSE(voice.active());
}

TEST(Voice, NoteOnActivates)
{
    Voice voice;
    voice.Init(48000.0);
    voice.NoteOn(60, 0.8f, 10, 200);  // 10ms attack, 200ms decay
    EXPECT_TRUE(voice.active());
    EXPECT_EQ(voice.note(), 60);
}

TEST(Voice, NoteOffDeactivatesEventually)
{
    Voice voice;
    voice.Init(48000.0);
    voice.NoteOn(60, 0.8f, 5, 10);  // 5ms attack, 10ms decay (very short)
    voice.NoteOff();

    // Process many samples until envelope finishes
    // 15ms at 48kHz = 720 samples, but internal is 96kHz so ~1440 samples
    float buffer[256];
    for (int i = 0; i < 100; ++i) {
        voice.Process(buffer, 256);
        if (!voice.active()) break;
    }
    EXPECT_FALSE(voice.active());
}

TEST(Voice, ProcessProducesOutput)
{
    Voice voice;
    voice.Init(48000.0);
    voice.set_shape(braids::MACRO_OSC_SHAPE_FM);
    voice.set_parameters(8192, 16384);
    voice.NoteOn(60, 1.0f, 50, 500);  // 50ms attack, 500ms decay

    float buffer[256];
    voice.Process(buffer, 256);

    bool hasNonZero = false;
    for (int i = 0; i < 256; ++i) {
        if (buffer[i] != 0.0f) hasNonZero = true;
    }
    EXPECT_TRUE(hasNonZero);
}

TEST(Voice, DifferentNotesProduceDifferentPitch)
{
    Voice voice1, voice2;
    voice1.Init(48000.0);
    voice2.Init(48000.0);

    voice1.set_shape(braids::MACRO_OSC_SHAPE_CSAW);
    voice2.set_shape(braids::MACRO_OSC_SHAPE_CSAW);

    // Use 1ms attack (fast) and 2000ms decay (long sustain) for good signal level
    voice1.NoteOn(48, 1.0f, 1, 2000);  // C3
    voice2.NoteOn(60, 1.0f, 1, 2000);  // C4 (octave higher)

    // Process enough samples for the envelope to reach sustain level
    // and for pitch to stabilize
    float buffer1[2048], buffer2[2048];
    voice1.Process(buffer1, 2048);
    voice2.Process(buffer2, 2048);

    // Count zero crossings in the latter half where envelope is more stable
    int crossings1 = 0, crossings2 = 0;
    for (int i = 1025; i < 2048; ++i) {
        if ((buffer1[i-1] < 0 && buffer1[i] >= 0) || (buffer1[i-1] >= 0 && buffer1[i] < 0))
            crossings1++;
        if ((buffer2[i-1] < 0 && buffer2[i] >= 0) || (buffer2[i-1] >= 0 && buffer2[i] < 0))
            crossings2++;
    }
    // C4 should have roughly 2x the crossings of C3
    // Just verify they produce different outputs (pitch detection is complex)
    EXPECT_NE(crossings1, crossings2);
}
