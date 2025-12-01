#include "PluginEditor.h"
#include <cmath>

// Row configuration: label, type, min, max, smallStep, largeStep, suffix
const BraidsVSTEditor::RowConfig BraidsVSTEditor::kRowConfigs[kNumRows] = {
    {"PRESET", RowType::Preset,    0,   0,    1,   1,  ""},   // min/max set dynamically
    {"SHAPE",  RowType::Shape,     0,   9,    1,   1,  ""},
    {"TIMBRE", RowType::Timbre,    0,   127,  1,   13, ""},
    {"COLOR",  RowType::Color,     0,   127,  1,   13, ""},
    {"CUTOFF", RowType::Cutoff,    0,   127,  1,   13, ""},
    {"RESO",   RowType::Resonance, 0,   127,  1,   13, ""},
    {"ATTACK", RowType::Attack,    0,   500,  5,   50, "ms"},
    {"DECAY",  RowType::Decay,     10,  2000, 20,  200, "ms"},
    {"VOICES", RowType::Voices,    1,   16,   1,   2,  ""},
    {"LFO1",   RowType::Lfo1,      0,   0,    1,   1,  ""},   // Multi-field row
    {"LFO2",   RowType::Lfo2,      0,   0,    1,   1,  ""},   // Multi-field row
    {"ENV1",   RowType::Env1,      0,   0,    1,   1,  ""},   // Multi-field row
    {"ENV2",   RowType::Env2,      0,   0,    1,   1,  ""},   // Multi-field row
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
    const juce::Colour kModOverlayColor(0x60ffffff);  // Semi-transparent white for modulation
    const juce::Colour kFieldSelectedColor(0xff8ad0d0);  // Highlight for selected field

    // Layout
    constexpr int kWindowWidth = 320;
    constexpr int kWindowHeight = 382;  // 13 rows now
    constexpr int kTitleHeight = 32;
    constexpr int kRowHeight = 26;
    constexpr int kRowMargin = 2;
    constexpr int kLabelWidth = 50;  // Narrower for mod rows
    constexpr int kValueWidth = 100;  // Wider for preset names
    constexpr int kPadding = 8;

    // Mod row field widths
    constexpr int kModFieldGap = 4;
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
        case RowType::Cutoff:
            return static_cast<int>(processor_.getCutoffParam()->get() * 127.0f + 0.5f);
        case RowType::Resonance:
            return static_cast<int>(processor_.getResonanceParam()->get() * 127.0f + 0.5f);
        case RowType::Attack:
            return static_cast<int>(processor_.getAttackParam()->get() * 500.0f + 0.5f);
        case RowType::Decay:
            return static_cast<int>(processor_.getDecayParam()->get() * 1990.0f + 10.0f + 0.5f);
        case RowType::Voices:
            return processor_.getPolyphonyParam()->get();
        // Mod rows have multiple fields, handled by getModFieldValue instead
        case RowType::Lfo1:
        case RowType::Lfo2:
        case RowType::Env1:
        case RowType::Env2:
            return 0;
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
        case RowType::Cutoff:
            value = juce::jlimit(0, 127, value);
            *processor_.getCutoffParam() = value / 127.0f;
            processor_.getPresetManager().markModified();
            break;
        case RowType::Resonance:
            value = juce::jlimit(0, 127, value);
            *processor_.getResonanceParam() = value / 127.0f;
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
        // Mod rows have multiple fields, handled by setModFieldValue instead
        case RowType::Lfo1:
        case RowType::Lfo2:
        case RowType::Env1:
        case RowType::Env2:
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

    if (cfg.type == RowType::Timbre || cfg.type == RowType::Color ||
        cfg.type == RowType::Cutoff || cfg.type == RowType::Resonance) {
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

        if (isModRow(i)) {
            // Paint modulation row with multiple fields
            paintModRow(g, i, rowRect, selected);
        } else {
            // Value bar
            float norm = normalizedValue(i);
            int barWidth = static_cast<int>(norm * rowRect.getWidth());
            g.setColour(selected ? kBarSelectedColor : kBarColor);
            g.fillRect(rowRect.getX(), rowRect.getY(), barWidth, rowRect.getHeight());

            // Paint modulation overlay for modulatable rows
            const auto& cfg = kRowConfigs[i];
            if (cfg.type == RowType::Timbre || cfg.type == RowType::Color ||
                cfg.type == RowType::Cutoff || cfg.type == RowType::Resonance) {
                paintModulationOverlay(g, i, rowRect);
            }

            // Label (dynamic for Timbre/Color based on shape)
            g.setColour(selected ? kTextSelectedColor : kTextColor);
            juce::String label = getDynamicLabel(i);
            g.drawText(label, rowRect.getX() + 4, rowRect.getY(),
                       kLabelWidth, rowRect.getHeight(), juce::Justification::centredLeft);

            // Value
            juce::String valueStr = formatValue(i);
            g.drawText(valueStr, rowRect.getRight() - kValueWidth - 4, rowRect.getY(),
                       kValueWidth, rowRect.getHeight(), juce::Justification::centredRight);
        }

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

    // Tab/Shift+Tab for field navigation within mod rows
    if (key.getKeyCode() == juce::KeyPress::tabKey) {
        if (isModRow(selectedRow_)) {
            int numFields = getNumFieldsForRow(selectedRow_);
            if (shift) {
                selectedField_ = (selectedField_ - 1 + numFields) % numFields;
            } else {
                selectedField_ = (selectedField_ + 1) % numFields;
            }
            repaint();
            return true;
        }
    }

    if (key.getKeyCode() == juce::KeyPress::upKey) {
        selectedRow_ = (selectedRow_ - 1 + kNumRows) % kNumRows;
        selectedField_ = 0;  // Reset field selection when changing rows
        repaint();
        return true;
    }
    if (key.getKeyCode() == juce::KeyPress::downKey) {
        selectedRow_ = (selectedRow_ + 1) % kNumRows;
        selectedField_ = 0;  // Reset field selection when changing rows
        repaint();
        return true;
    }
    if (key.getKeyCode() == juce::KeyPress::leftKey) {
        if (isModRow(selectedRow_)) {
            // Adjust the current field
            int delta = shift ? -10 : -1;
            int current = getModFieldValue(selectedRow_, selectedField_);
            setModFieldValue(selectedRow_, selectedField_, current + delta);
        } else {
            const auto& cfg = kRowConfigs[selectedRow_];
            int step = shift ? cfg.largeStep : cfg.smallStep;
            adjustValue(selectedRow_, -step);
        }
        return true;
    }
    if (key.getKeyCode() == juce::KeyPress::rightKey) {
        if (isModRow(selectedRow_)) {
            // Adjust the current field
            int delta = shift ? 10 : 1;
            int current = getModFieldValue(selectedRow_, selectedField_);
            setModFieldValue(selectedRow_, selectedField_, current + delta);
        } else {
            const auto& cfg = kRowConfigs[selectedRow_];
            int step = shift ? cfg.largeStep : cfg.smallStep;
            adjustValue(selectedRow_, step);
        }
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

// ============================================================================
// Modulation Row Helpers
// ============================================================================

bool BraidsVSTEditor::isModRow(int row) const
{
    const auto& cfg = kRowConfigs[row];
    return cfg.type == RowType::Lfo1 || cfg.type == RowType::Lfo2 ||
           cfg.type == RowType::Env1 || cfg.type == RowType::Env2;
}

int BraidsVSTEditor::getNumFieldsForRow(int row) const
{
    const auto& cfg = kRowConfigs[row];
    // LFO: Rate, Shape, Dest, Amount (4 fields)
    // ENV: Attack, Decay, Dest, Amount (4 fields)
    if (cfg.type == RowType::Lfo1 || cfg.type == RowType::Lfo2 ||
        cfg.type == RowType::Env1 || cfg.type == RowType::Env2) {
        return 4;
    }
    return 1;  // Single field for non-mod rows
}

int BraidsVSTEditor::getModFieldValue(int row, int field) const
{
    const auto& cfg = kRowConfigs[row];

    if (cfg.type == RowType::Lfo1) {
        switch (field) {
            case 0: return processor_.getLfo1RateParam()->getIndex();
            case 1: return processor_.getLfo1ShapeParam()->getIndex();
            case 2: return processor_.getLfo1DestParam()->getIndex();
            case 3: return processor_.getLfo1AmountParam()->get();
        }
    } else if (cfg.type == RowType::Lfo2) {
        switch (field) {
            case 0: return processor_.getLfo2RateParam()->getIndex();
            case 1: return processor_.getLfo2ShapeParam()->getIndex();
            case 2: return processor_.getLfo2DestParam()->getIndex();
            case 3: return processor_.getLfo2AmountParam()->get();
        }
    } else if (cfg.type == RowType::Env1) {
        switch (field) {
            case 0: return static_cast<int>(processor_.getEnv1AttackParam()->get() * 500.0f + 0.5f);
            case 1: return static_cast<int>(processor_.getEnv1DecayParam()->get() * 1990.0f + 10.0f + 0.5f);
            case 2: return processor_.getEnv1DestParam()->getIndex();
            case 3: return processor_.getEnv1AmountParam()->get();
        }
    } else if (cfg.type == RowType::Env2) {
        switch (field) {
            case 0: return static_cast<int>(processor_.getEnv2AttackParam()->get() * 500.0f + 0.5f);
            case 1: return static_cast<int>(processor_.getEnv2DecayParam()->get() * 1990.0f + 10.0f + 0.5f);
            case 2: return processor_.getEnv2DestParam()->getIndex();
            case 3: return processor_.getEnv2AmountParam()->get();
        }
    }
    return 0;
}

void BraidsVSTEditor::setModFieldValue(int row, int field, int value)
{
    const auto& cfg = kRowConfigs[row];

    if (cfg.type == RowType::Lfo1) {
        switch (field) {
            case 0:  // Rate
                value = juce::jlimit(0, static_cast<int>(braids::LfoRateDivision::NumDivisions) - 1, value);
                *processor_.getLfo1RateParam() = value;
                break;
            case 1:  // Shape
                value = juce::jlimit(0, static_cast<int>(braids::LfoShape::NumShapes) - 1, value);
                *processor_.getLfo1ShapeParam() = value;
                break;
            case 2:  // Dest
                value = juce::jlimit(0, static_cast<int>(braids::ModDestination::NumDestinations) - 1, value);
                *processor_.getLfo1DestParam() = value;
                break;
            case 3:  // Amount
                value = juce::jlimit(-64, 63, value);
                *processor_.getLfo1AmountParam() = value;
                break;
        }
    } else if (cfg.type == RowType::Lfo2) {
        switch (field) {
            case 0:
                value = juce::jlimit(0, static_cast<int>(braids::LfoRateDivision::NumDivisions) - 1, value);
                *processor_.getLfo2RateParam() = value;
                break;
            case 1:
                value = juce::jlimit(0, static_cast<int>(braids::LfoShape::NumShapes) - 1, value);
                *processor_.getLfo2ShapeParam() = value;
                break;
            case 2:
                value = juce::jlimit(0, static_cast<int>(braids::ModDestination::NumDestinations) - 1, value);
                *processor_.getLfo2DestParam() = value;
                break;
            case 3:
                value = juce::jlimit(-64, 63, value);
                *processor_.getLfo2AmountParam() = value;
                break;
        }
    } else if (cfg.type == RowType::Env1) {
        switch (field) {
            case 0:  // Attack
                value = juce::jlimit(0, 500, value);
                *processor_.getEnv1AttackParam() = value / 500.0f;
                break;
            case 1:  // Decay
                value = juce::jlimit(10, 2000, value);
                *processor_.getEnv1DecayParam() = (value - 10.0f) / 1990.0f;
                break;
            case 2:  // Dest
                value = juce::jlimit(0, static_cast<int>(braids::ModDestination::NumDestinations) - 1, value);
                *processor_.getEnv1DestParam() = value;
                break;
            case 3:  // Amount
                value = juce::jlimit(-64, 63, value);
                *processor_.getEnv1AmountParam() = value;
                break;
        }
    } else if (cfg.type == RowType::Env2) {
        switch (field) {
            case 0:
                value = juce::jlimit(0, 500, value);
                *processor_.getEnv2AttackParam() = value / 500.0f;
                break;
            case 1:
                value = juce::jlimit(10, 2000, value);
                *processor_.getEnv2DecayParam() = (value - 10.0f) / 1990.0f;
                break;
            case 2:
                value = juce::jlimit(0, static_cast<int>(braids::ModDestination::NumDestinations) - 1, value);
                *processor_.getEnv2DestParam() = value;
                break;
            case 3:
                value = juce::jlimit(-64, 63, value);
                *processor_.getEnv2AmountParam() = value;
                break;
        }
    }

    processor_.getPresetManager().markModified();
    repaint();
}

juce::String BraidsVSTEditor::getModDestinationName(int destIdx) const
{
    auto dest = static_cast<braids::ModDestination>(destIdx);
    int shapeIndex = processor_.getShapeParam()->getIndex();

    // Use dynamic names for Timbre and Color based on current shape
    if (dest == braids::ModDestination::Timbre && shapeIndex >= 0 && shapeIndex < 10) {
        return timbreLabels_[shapeIndex];
    }
    if (dest == braids::ModDestination::Color && shapeIndex >= 0 && shapeIndex < 10) {
        return colorLabels_[shapeIndex];
    }

    // Use static names for other destinations
    return braids::ModulationMatrix::GetDestinationName(dest);
}

juce::String BraidsVSTEditor::formatModFieldValue(int row, int field) const
{
    const auto& cfg = kRowConfigs[row];

    if (cfg.type == RowType::Lfo1 || cfg.type == RowType::Lfo2) {
        switch (field) {
            case 0: {  // Rate
                int idx = getModFieldValue(row, field);
                return braids::Lfo::GetRateName(static_cast<braids::LfoRateDivision>(idx));
            }
            case 1: {  // Shape
                int idx = getModFieldValue(row, field);
                return braids::Lfo::GetShapeName(static_cast<braids::LfoShape>(idx));
            }
            case 2: {  // Dest
                int idx = getModFieldValue(row, field);
                return getModDestinationName(idx);
            }
            case 3: {  // Amount
                int amt = getModFieldValue(row, field);
                return (amt >= 0 ? "+" : "") + juce::String(amt);
            }
        }
    } else if (cfg.type == RowType::Env1 || cfg.type == RowType::Env2) {
        switch (field) {
            case 0: {  // Attack
                int ms = getModFieldValue(row, field);
                return juce::String(ms) + "ms";
            }
            case 1: {  // Decay
                int ms = getModFieldValue(row, field);
                return juce::String(ms) + "ms";
            }
            case 2: {  // Dest
                int idx = getModFieldValue(row, field);
                return getModDestinationName(idx);
            }
            case 3: {  // Amount
                int amt = getModFieldValue(row, field);
                return (amt >= 0 ? "+" : "") + juce::String(amt);
            }
        }
    }
    return "???";
}

void BraidsVSTEditor::paintModRow(juce::Graphics& g, int row, const juce::Rectangle<int>& rowRect, bool selected)
{
    const auto& cfg = kRowConfigs[row];
    int numFields = getNumFieldsForRow(row);

    // Draw row label
    g.setColour(selected ? kTextSelectedColor : kTextColor);
    g.drawText(cfg.label, rowRect.getX() + 4, rowRect.getY(),
               kLabelWidth - 8, rowRect.getHeight(), juce::Justification::centredLeft);

    // Calculate field positions - fields are to the right of the label
    int fieldX = rowRect.getX() + kLabelWidth;
    int availableWidth = rowRect.getWidth() - kLabelWidth - 4;
    int fieldWidth = (availableWidth - (numFields - 1) * kModFieldGap) / numFields;

    for (int f = 0; f < numFields; ++f) {
        juce::Rectangle<int> fieldRect(fieldX, rowRect.getY() + 2,
                                        fieldWidth, rowRect.getHeight() - 4);

        // Draw field background - highlight if this is the selected field
        bool isSelectedField = selected && (f == selectedField_);
        g.setColour(isSelectedField ? kFieldSelectedColor.withAlpha(0.3f) : kRowBgColor.brighter(0.1f));
        g.fillRoundedRectangle(fieldRect.toFloat(), 2.0f);

        // Draw field border if selected
        if (isSelectedField) {
            g.setColour(kFieldSelectedColor);
            g.drawRoundedRectangle(fieldRect.toFloat(), 2.0f, 1.0f);
        }

        // Draw field value
        g.setColour(isSelectedField ? kTextSelectedColor : kTextColor);
        juce::String valueStr = formatModFieldValue(row, f);
        g.drawText(valueStr, fieldRect, juce::Justification::centred);

        fieldX += fieldWidth + kModFieldGap;
    }
}

void BraidsVSTEditor::paintModulationOverlay(juce::Graphics& g, int row, const juce::Rectangle<int>& rowRect)
{
    const auto& cfg = kRowConfigs[row];

    float baseValue = 0.0f;
    float modulatedValue = 0.0f;

    if (cfg.type == RowType::Timbre) {
        baseValue = processor_.getTimbreParam()->get();
        modulatedValue = processor_.getModulatedTimbre();
    } else if (cfg.type == RowType::Color) {
        baseValue = processor_.getColorParam()->get();
        modulatedValue = processor_.getModulatedColor();
    } else if (cfg.type == RowType::Cutoff) {
        baseValue = processor_.getCutoffParam()->get();
        modulatedValue = processor_.getModulatedCutoff();
    } else if (cfg.type == RowType::Resonance) {
        baseValue = processor_.getResonanceParam()->get();
        modulatedValue = processor_.getModulatedResonance();
    }

    // Only draw if there's a visible difference
    float diff = modulatedValue - baseValue;
    if (std::abs(diff) < 0.001f) return;

    // Draw the modulation range as a semi-transparent overlay
    int baseX = rowRect.getX() + static_cast<int>(baseValue * rowRect.getWidth());
    int modX = rowRect.getX() + static_cast<int>(modulatedValue * rowRect.getWidth());

    int leftX = std::min(baseX, modX);
    int rightX = std::max(baseX, modX);

    g.setColour(kModOverlayColor);
    g.fillRect(leftX, rowRect.getY(), rightX - leftX, rowRect.getHeight());

    // Draw a vertical line at the modulated position
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.drawVerticalLine(modX, static_cast<float>(rowRect.getY()),
                       static_cast<float>(rowRect.getBottom()));
}
