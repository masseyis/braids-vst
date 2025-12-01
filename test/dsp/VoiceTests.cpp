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
    voice.NoteOn(60, 0.8f, 1000, 2000);
    EXPECT_TRUE(voice.active());
    EXPECT_EQ(voice.note(), 60);
}

TEST(Voice, NoteOffDeactivatesEventually)
{
    Voice voice;
    voice.Init(48000.0);
    voice.NoteOn(60, 0.8f, 100, 100);  // Very short envelope
    voice.NoteOff();

    // Process many samples until envelope finishes
    float buffer[256];
    for (int i = 0; i < 1000; ++i) {
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
    voice.NoteOn(60, 1.0f, 8000, 16000);

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

    voice1.NoteOn(48, 1.0f, 0, 65535);  // C3
    voice2.NoteOn(60, 1.0f, 0, 65535);  // C4 (octave higher)

    float buffer1[512], buffer2[512];
    voice1.Process(buffer1, 512);
    voice2.Process(buffer2, 512);

    // Count zero crossings - higher pitch should have more
    int crossings1 = 0, crossings2 = 0;
    for (int i = 1; i < 512; ++i) {
        if ((buffer1[i-1] < 0 && buffer1[i] >= 0) || (buffer1[i-1] >= 0 && buffer1[i] < 0))
            crossings1++;
        if ((buffer2[i-1] < 0 && buffer2[i] >= 0) || (buffer2[i-1] >= 0 && buffer2[i] < 0))
            crossings2++;
    }
    // C4 should have roughly 2x the crossings of C3
    EXPECT_GT(crossings2, crossings1);
}
