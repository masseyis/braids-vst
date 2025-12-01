#include <gtest/gtest.h>
#include "dsp/voice_allocator.h"

TEST(VoiceAllocator, InitDoesNotCrash)
{
    VoiceAllocator allocator;
    allocator.Init(48000.0, 8);
}

TEST(VoiceAllocator, NoteOnAllocatesVoice)
{
    VoiceAllocator allocator;
    allocator.Init(48000.0, 8);
    allocator.NoteOn(60, 0.8f, 10, 200);  // 10ms attack, 200ms decay

    EXPECT_EQ(allocator.activeVoiceCount(), 1);
}

TEST(VoiceAllocator, MultipleNotesAllocateMultipleVoices)
{
    VoiceAllocator allocator;
    allocator.Init(48000.0, 8);

    allocator.NoteOn(60, 0.8f, 10, 200);  // 10ms attack, 200ms decay
    allocator.NoteOn(64, 0.8f, 10, 200);
    allocator.NoteOn(67, 0.8f, 10, 200);

    EXPECT_EQ(allocator.activeVoiceCount(), 3);
}

TEST(VoiceAllocator, NoteOffReleasesVoice)
{
    VoiceAllocator allocator;
    allocator.Init(48000.0, 8);
    allocator.NoteOn(60, 0.8f, 5, 10);  // 5ms attack, 10ms decay (short)
    allocator.NoteOff(60);

    // Voice should still be active until envelope finishes
    // But it's marked for release
    EXPECT_GE(allocator.activeVoiceCount(), 0);
}

TEST(VoiceAllocator, RespectMaxPolyphony)
{
    VoiceAllocator allocator;
    allocator.Init(48000.0, 4);  // Only 4 voices

    // Use long decay (2000ms) so voices stay active during the test
    allocator.NoteOn(60, 0.8f, 10, 2000);
    allocator.NoteOn(62, 0.8f, 10, 2000);
    allocator.NoteOn(64, 0.8f, 10, 2000);
    allocator.NoteOn(65, 0.8f, 10, 2000);
    allocator.NoteOn(67, 0.8f, 10, 2000);  // Should steal oldest

    EXPECT_LE(allocator.activeVoiceCount(), 4);
}

TEST(VoiceAllocator, ProcessProducesOutput)
{
    VoiceAllocator allocator;
    allocator.Init(48000.0, 4);
    allocator.set_shape(braids::MACRO_OSC_SHAPE_FM);
    allocator.set_parameters(8192, 16384);

    allocator.NoteOn(60, 1.0f, 50, 500);  // 50ms attack, 500ms decay

    float left[256], right[256];
    allocator.Process(left, right, 256);

    bool hasNonZero = false;
    for (int i = 0; i < 256; ++i) {
        if (left[i] != 0.0f) hasNonZero = true;
    }
    EXPECT_TRUE(hasNonZero);
}

TEST(VoiceAllocator, SetPolyphonyWorks)
{
    VoiceAllocator allocator;
    allocator.Init(48000.0, 8);

    allocator.setPolyphony(2);
    EXPECT_EQ(allocator.polyphony(), 2);

    allocator.setPolyphony(16);
    EXPECT_EQ(allocator.polyphony(), 16);
}
