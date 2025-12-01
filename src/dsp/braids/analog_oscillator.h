// Ported from Mutable Instruments Braids analog_oscillator.h
// Original: Copyright 2012 Emilie Gillet (MIT License)
// Modifications for BraidsVST: GPL v3

#pragma once

#include <cstdint>
#include "../stmlib/stmlib.h"

namespace braids {

enum AnalogOscillatorShape {
    OSC_SHAPE_SAW,
    OSC_SHAPE_VARIABLE_SAW,
    OSC_SHAPE_CSAW,
    OSC_SHAPE_SQUARE,
    OSC_SHAPE_TRIANGLE,
    OSC_SHAPE_SINE,
    OSC_SHAPE_TRIANGLE_FOLD,
    OSC_SHAPE_SINE_FOLD,
    OSC_SHAPE_BUZZ,
    OSC_SHAPE_LAST
};

class AnalogOscillator {
public:
    AnalogOscillator() = default;
    ~AnalogOscillator() = default;

    void Init();

    void set_shape(AnalogOscillatorShape shape) { shape_ = shape; }
    void set_pitch(int16_t pitch) { pitch_ = pitch; }
    void set_parameter(int16_t parameter) { parameter_ = parameter; }
    void set_aux_parameter(int16_t aux) { aux_parameter_ = aux; }

    void Render(const uint8_t* sync, int16_t* buffer, size_t size);

private:
    void RenderSaw(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderVariableSaw(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderCSaw(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderSquare(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderTriangle(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderSine(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderTriangleFold(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderSineFold(const uint8_t* sync, int16_t* buffer, size_t size);
    void RenderBuzz(const uint8_t* sync, int16_t* buffer, size_t size);

    uint32_t ComputePhaseIncrement(int16_t midi_pitch);

    AnalogOscillatorShape shape_ = OSC_SHAPE_SAW;
    int16_t pitch_ = 0;
    int16_t parameter_ = 0;      // Primary parameter (timbre)
    int16_t aux_parameter_ = 0;  // Secondary parameter (color)

    uint32_t phase_ = 0;
    int32_t next_sample_ = 0;
    bool high_ = false;

    DISALLOW_COPY_AND_ASSIGN(AnalogOscillator);
};

} // namespace braids
