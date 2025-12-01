#include "PluginEditor.h"

// Row configuration: label, type, min, max, smallStep, largeStep, suffix
const BraidsVSTEditor::RowConfig BraidsVSTEditor::kRowConfigs[kNumRows] = {
    {"PRESET", RowType::Preset, 0,   0,    1,   1,  ""},   // min/max set dynamically
    {"SHAPE",  RowType::Shape,  0,   9,    1,   1,  ""},
    {"TIMBRE", RowType::Timbre, 0,   127,  1,   13, ""},
    {"COLOR",  RowType::Color,  0,   127,  1,   13, ""},
    {"ATTACK", RowType::Attack, 0,   500,  5,   50, "ms"},
    {"DECAY",  RowType::Decay,  10,  2000, 20,  200, "ms"},
    {"VOICES", RowType::Voices, 1,   16,   1,   2,  ""},
};

namespace {
    // Colors
    const juce::Colour kBgColor(0xff1a1a1a);
    const juce::Colour kRowBgColor(0xff0d0d0d);
    const juce::Colour kBarColor(0xff4a9090);
    const juce::Colour kBarSelectedColor(0xff6abfbf);
    const juce::Colour kTextColor(0xffa0a0a0);
    const juce::Colour kTextSelectedColor(0xffffffff);
    const juce::Colour kTitleColor(0xffffffff);

    // Layout
    constexpr int kWindowWidth = 320;
    constexpr int kWindowHeight = 226;  // One more row
    constexpr int kTitleHeight = 32;
    constexpr int kRowHeight = 26;
    constexpr int kRowMargin = 2;
    constexpr int kLabelWidth = 70;
    constexpr int kValueWidth = 100;  // Wider for preset names
    constexpr int kPadding = 8;
}

BraidsVSTEditor::BraidsVSTEditor(BraidsVSTProcessor& p)
    : AudioProcessorEditor(&p), processor_(p)
{
    setSize(kWindowWidth, kWindowHeight);
    setWantsKeyboardFocus(true);
    startTimerHz(30);  // Refresh UI at 30fps for modified indicator
}

BraidsVSTEditor::~BraidsVSTEditor()
{
    stopTimer();
}

void BraidsVSTEditor::timerCallback()
{
    repaint();  // Refresh to show modified state and preset changes
}

int BraidsVSTEditor::getDisplayValue(int row) const
{
    const auto& cfg = kRowConfigs[row];
    switch (cfg.type) {
        case RowType::Preset:
            return processor_.getPresetManager().getCurrentPresetIndex();
        case RowType::Shape:
            return processor_.getShapeParam()->getIndex();
        case RowType::Timbre:
            return static_cast<int>(processor_.getTimbreParam()->get() * 127.0f + 0.5f);
        case RowType::Color:
            return static_cast<int>(processor_.getColorParam()->get() * 127.0f + 0.5f);
        case RowType::Attack:
            return static_cast<int>(processor_.getAttackParam()->get() * 500.0f + 0.5f);
        case RowType::Decay:
            return static_cast<int>(processor_.getDecayParam()->get() * 1990.0f + 10.0f + 0.5f);
        case RowType::Voices:
            return processor_.getPolyphonyParam()->get();
    }
    return 0;
}

void BraidsVSTEditor::setDisplayValue(int row, int value)
{
    const auto& cfg = kRowConfigs[row];

    switch (cfg.type) {
        case RowType::Preset: {
            int numPresets = processor_.getPresetManager().getNumPresets();
            if (numPresets > 0) {
                value = ((value % numPresets) + numPresets) % numPresets;
                processor_.getPresetManager().loadPreset(value);
            }
            break;
        }
        case RowType::Shape:
            value = juce::jlimit(0, 9, value);
            *processor_.getShapeParam() = value;
            processor_.getPresetManager().markModified();
            break;
        case RowType::Timbre:
            value = juce::jlimit(0, 127, value);
            *processor_.getTimbreParam() = value / 127.0f;
            processor_.getPresetManager().markModified();
            break;
        case RowType::Color:
            value = juce::jlimit(0, 127, value);
            *processor_.getColorParam() = value / 127.0f;
            processor_.getPresetManager().markModified();
            break;
        case RowType::Attack:
            value = juce::jlimit(0, 500, value);
            *processor_.getAttackParam() = value / 500.0f;
            processor_.getPresetManager().markModified();
            break;
        case RowType::Decay:
            value = juce::jlimit(10, 2000, value);
            *processor_.getDecayParam() = (value - 10.0f) / 1990.0f;
            processor_.getPresetManager().markModified();
            break;
        case RowType::Voices:
            value = juce::jlimit(1, 16, value);
            *processor_.getPolyphonyParam() = value;
            processor_.getPresetManager().markModified();
            break;
    }
    repaint();
}

juce::String BraidsVSTEditor::formatValue(int row) const
{
    const auto& cfg = kRowConfigs[row];

    if (cfg.type == RowType::Preset) {
        return processor_.getPresetManager().getCurrentPresetName();
    }
    if (cfg.type == RowType::Shape) {
        int value = getDisplayValue(row);
        return shapeNames_[value];
    }

    int value = getDisplayValue(row);
    juce::String str;

    if (cfg.type == RowType::Timbre || cfg.type == RowType::Color) {
        str = juce::String(value).paddedLeft('0', 3);
    } else if (cfg.type == RowType::Voices) {
        str = juce::String(value).paddedLeft('0', 2);
    } else {
        str = juce::String(value) + cfg.suffix;
    }
    return str;
}

float BraidsVSTEditor::normalizedValue(int row) const
{
    const auto& cfg = kRowConfigs[row];

    if (cfg.type == RowType::Preset) {
        int numPresets = processor_.getPresetManager().getNumPresets();
        if (numPresets <= 1) return 0.0f;
        int current = processor_.getPresetManager().getCurrentPresetIndex();
        return static_cast<float>(current) / static_cast<float>(numPresets - 1);
    }

    int value = getDisplayValue(row);
    return static_cast<float>(value - cfg.minVal) / static_cast<float>(cfg.maxVal - cfg.minVal);
}

void BraidsVSTEditor::adjustValue(int row, int delta)
{
    int current = getDisplayValue(row);
    setDisplayValue(row, current + delta);
}

int BraidsVSTEditor::rowAtY(int y) const
{
    int rowY = kTitleHeight;
    for (int i = 0; i < kNumRows; ++i) {
        if (y >= rowY && y < rowY + kRowHeight) {
            return i;
        }
        rowY += kRowHeight;
    }
    return -1;
}

void BraidsVSTEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBgColor);

    // Title
    g.setColour(kTitleColor);
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 16.0f, juce::Font::bold));
    g.drawText("BRAIDS VST", 0, 4, getWidth(), kTitleHeight - 4, juce::Justification::centred);

    // Rows
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain));

    int y = kTitleHeight;
    for (int i = 0; i < kNumRows; ++i) {
        bool selected = (i == selectedRow_);

        // Row background
        juce::Rectangle<int> rowRect(kPadding, y + kRowMargin,
                                      getWidth() - 2 * kPadding, kRowHeight - 2 * kRowMargin);
        g.setColour(kRowBgColor);
        g.fillRect(rowRect);

        // Value bar
        float norm = normalizedValue(i);
        int barWidth = static_cast<int>(norm * rowRect.getWidth());
        g.setColour(selected ? kBarSelectedColor : kBarColor);
        g.fillRect(rowRect.getX(), rowRect.getY(), barWidth, rowRect.getHeight());

        // Label (dynamic for Timbre/Color based on shape)
        g.setColour(selected ? kTextSelectedColor : kTextColor);
        juce::String label = getDynamicLabel(i);
        g.drawText(label, rowRect.getX() + 4, rowRect.getY(),
                   kLabelWidth, rowRect.getHeight(), juce::Justification::centredLeft);

        // Value
        juce::String valueStr = formatValue(i);
        g.drawText(valueStr, rowRect.getRight() - kValueWidth - 4, rowRect.getY(),
                   kValueWidth, rowRect.getHeight(), juce::Justification::centredRight);

        y += kRowHeight;
    }
}

void BraidsVSTEditor::resized()
{
    // Fixed size, nothing to do
}

bool BraidsVSTEditor::keyPressed(const juce::KeyPress& key)
{
    bool shift = key.getModifiers().isShiftDown();

    // Shift+S to save preset
    if (shift && key.getKeyCode() == 'S') {
        processor_.getPresetManager().saveCurrentAsNewPreset();
        repaint();
        return true;
    }

    if (key.getKeyCode() == juce::KeyPress::upKey) {
        selectedRow_ = (selectedRow_ - 1 + kNumRows) % kNumRows;
        repaint();
        return true;
    }
    if (key.getKeyCode() == juce::KeyPress::downKey) {
        selectedRow_ = (selectedRow_ + 1) % kNumRows;
        repaint();
        return true;
    }
    if (key.getKeyCode() == juce::KeyPress::leftKey) {
        const auto& cfg = kRowConfigs[selectedRow_];
        int step = shift ? cfg.largeStep : cfg.smallStep;
        adjustValue(selectedRow_, -step);
        return true;
    }
    if (key.getKeyCode() == juce::KeyPress::rightKey) {
        const auto& cfg = kRowConfigs[selectedRow_];
        int step = shift ? cfg.largeStep : cfg.smallStep;
        adjustValue(selectedRow_, step);
        return true;
    }

    return false;
}

void BraidsVSTEditor::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();

    int row = rowAtY(event.y);
    if (row >= 0) {
        selectedRow_ = row;
        dragStartValue_ = getDisplayValue(row);
        dragStartX_ = event.x;
        repaint();
    }
}

void BraidsVSTEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (selectedRow_ >= 0) {
        const auto& cfg = kRowConfigs[selectedRow_];
        int deltaX = event.x - dragStartX_;

        if (cfg.type == RowType::Preset) {
            // For presets, use a wider drag threshold
            int numPresets = processor_.getPresetManager().getNumPresets();
            if (numPresets > 0) {
                float sensitivity = static_cast<float>(numPresets) / static_cast<float>(getWidth());
                int deltaValue = static_cast<int>(deltaX * sensitivity);
                setDisplayValue(selectedRow_, dragStartValue_ + deltaValue);
            }
        } else {
            // Map drag distance to value change: full width = full range
            float sensitivity = static_cast<float>(cfg.maxVal - cfg.minVal) / static_cast<float>(getWidth());
            int deltaValue = static_cast<int>(deltaX * sensitivity);
            setDisplayValue(selectedRow_, dragStartValue_ + deltaValue);
        }
    }
}

void BraidsVSTEditor::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    int row = rowAtY(event.y);
    if (row >= 0) {
        selectedRow_ = row;
        const auto& cfg = kRowConfigs[row];
        int delta = wheel.deltaY > 0 ? cfg.smallStep : -cfg.smallStep;
        adjustValue(row, delta);
    }
}

juce::String BraidsVSTEditor::getDynamicLabel(int row) const
{
    const auto& cfg = kRowConfigs[row];

    // For Timbre and Color, return shape-specific labels
    if (cfg.type == RowType::Timbre || cfg.type == RowType::Color) {
        int shapeIndex = processor_.getShapeParam()->getIndex();
        if (shapeIndex >= 0 && shapeIndex < 10) {
            if (cfg.type == RowType::Timbre) {
                return timbreLabels_[shapeIndex];
            } else {
                return colorLabels_[shapeIndex];
            }
        }
    }

    // For all other rows, use the static label
    return cfg.label;
}
