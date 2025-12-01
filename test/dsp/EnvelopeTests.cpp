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
    env.Trigger(1000, 2000);  // Short attack, medium decay
    EXPECT_FALSE(env.done());
}

TEST(Envelope, AttackRises)
{
    braids::Envelope env;
    env.Init();
    env.Trigger(8000, 8000);  // Medium attack/decay

    uint16_t prev = 0;
    bool rising = false;
    for (int i = 0; i < 100; ++i) {
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
    env.Trigger(1000, 1000);  // Very short attack/decay

    // Run for many samples
    for (int i = 0; i < 100000; ++i) {
        env.Render();
        if (env.done()) break;
    }
    EXPECT_TRUE(env.done());
}

TEST(Envelope, ReachesFullAmplitude)
{
    braids::Envelope env;
    env.Init();
    env.Trigger(8000, 16000);  // Medium attack, longer decay

    uint16_t max_val = 0;
    for (int i = 0; i < 10000; ++i) {
        uint16_t val = env.Render();
        if (val > max_val) max_val = val;
        if (env.done()) break;
    }
    // Should reach at least 90% of full amplitude
    EXPECT_GT(max_val, 58000);
}
