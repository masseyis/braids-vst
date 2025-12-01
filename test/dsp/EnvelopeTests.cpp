#include <gtest/gtest.h>
#include "dsp/braids/envelope.h"

TEST(Envelope, InitDoesNotCrash)
{
    braids::Envelope env;
    env.Init();
}

TEST(Envelope, StartsAtZero)
{
    braids::Envelope env;
    env.Init();
    // Without trigger, should be at zero (dead)
    EXPECT_TRUE(env.done());
}

TEST(Envelope, TriggerStartsAttack)
{
    braids::Envelope env;
    env.Init();
    env.Trigger(10, 20);  // 10ms attack, 20ms decay
    EXPECT_FALSE(env.done());
}

TEST(Envelope, AttackRises)
{
    braids::Envelope env;
    env.Init();
    env.Trigger(50, 50);  // 50ms attack/decay

    uint16_t prev = 0;
    bool rising = false;
    for (int i = 0; i < 1000; ++i) {
        uint16_t val = env.Render();
        if (val > prev) {
            rising = true;
            break;
        }
        prev = val;
    }
    EXPECT_TRUE(rising);
}

TEST(Envelope, EventuallyFinishes)
{
    braids::Envelope env;
    env.Init();
    env.Trigger(10, 10);  // 10ms attack + 10ms decay = 20ms total

    // At 96kHz, 20ms = 1920 samples. Run for plenty more.
    for (int i = 0; i < 10000; ++i) {
        env.Render();
        if (env.done()) break;
    }
    EXPECT_TRUE(env.done());
}

TEST(Envelope, ReachesFullAmplitude)
{
    braids::Envelope env;
    env.Init();
    env.Trigger(50, 100);  // 50ms attack, 100ms decay

    uint16_t max_val = 0;
    // At 96kHz, 150ms = 14400 samples. Run for plenty more.
    for (int i = 0; i < 20000; ++i) {
        uint16_t val = env.Render();
        if (val > max_val) max_val = val;
        if (env.done()) break;
    }
    // Should reach at least 90% of full amplitude
    EXPECT_GT(max_val, 58000);
}
