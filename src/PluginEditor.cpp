#include "PluginEditor.h"

BraidsVSTEditor::BraidsVSTEditor(BraidsVSTProcessor& p)
    : AudioProcessorEditor(&p), processor_(p)
{
    setSize(400, 350);

    // Shape combo box
    shapeLabel_.setText("Shape", juce::dontSendNotification);
    shapeLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(shapeLabel_);

    shapeCombo_.addItemList({
        "Saw", "Morph", "Saw/Square", "Sine/Triangle", "Buzz",
        "Square+Sub", "Saw+Sub", "Square Sync", "Saw Sync", "FM"
    }, 1);
    shapeCombo_.setSelectedItemIndex(processor_.getShapeParam()->getIndex(), juce::dontSendNotification);
    shapeCombo_.onChange = [this] {
        *processor_.getShapeParam() = shapeCombo_.getSelectedItemIndex();
    };
    addAndMakeVisible(shapeCombo_);

    // Timbre slider
    timbreLabel_.setText("Timbre", juce::dontSendNotification);
    timbreLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(timbreLabel_);

    timbreSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    timbreSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    timbreSlider_.setRange(0.0, 1.0);
    timbreSlider_.setValue(processor_.getTimbreParam()->get(), juce::dontSendNotification);
    timbreSlider_.onValueChange = [this] {
        *processor_.getTimbreParam() = static_cast<float>(timbreSlider_.getValue());
    };
    addAndMakeVisible(timbreSlider_);

    // Color slider
    colorLabel_.setText("Color", juce::dontSendNotification);
    colorLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(colorLabel_);

    colorSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    colorSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    colorSlider_.setRange(0.0, 1.0);
    colorSlider_.setValue(processor_.getColorParam()->get(), juce::dontSendNotification);
    colorSlider_.onValueChange = [this] {
        *processor_.getColorParam() = static_cast<float>(colorSlider_.getValue());
    };
    addAndMakeVisible(colorSlider_);

    // Attack slider
    attackLabel_.setText("Attack", juce::dontSendNotification);
    attackLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(attackLabel_);

    attackSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    attackSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    attackSlider_.setRange(0.0, 1.0);
    attackSlider_.setValue(processor_.getAttackParam()->get(), juce::dontSendNotification);
    attackSlider_.onValueChange = [this] {
        *processor_.getAttackParam() = static_cast<float>(attackSlider_.getValue());
    };
    addAndMakeVisible(attackSlider_);

    // Decay slider
    decayLabel_.setText("Decay", juce::dontSendNotification);
    decayLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(decayLabel_);

    decaySlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    decaySlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    decaySlider_.setRange(0.0, 1.0);
    decaySlider_.setValue(processor_.getDecayParam()->get(), juce::dontSendNotification);
    decaySlider_.onValueChange = [this] {
        *processor_.getDecayParam() = static_cast<float>(decaySlider_.getValue());
    };
    addAndMakeVisible(decaySlider_);
}

BraidsVSTEditor::~BraidsVSTEditor() = default;

void BraidsVSTEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2d2d2d));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawText("BraidsVST", 0, 10, getWidth(), 30, juce::Justification::centred);
}

void BraidsVSTEditor::resized()
{
    const int margin = 20;
    const int knobSize = 80;
    const int labelHeight = 20;
    const int comboHeight = 24;

    // Shape selector at top
    shapeLabel_.setBounds(margin, 50, 60, labelHeight);
    shapeCombo_.setBounds(margin + 70, 50, 200, comboHeight);

    // Knobs in a row
    const int knobY = 100;
    const int knobSpacing = 90;
    int x = margin;

    timbreLabel_.setBounds(x, knobY, knobSize, labelHeight);
    timbreSlider_.setBounds(x, knobY + labelHeight, knobSize, knobSize);
    x += knobSpacing;

    colorLabel_.setBounds(x, knobY, knobSize, labelHeight);
    colorSlider_.setBounds(x, knobY + labelHeight, knobSize, knobSize);
    x += knobSpacing;

    attackLabel_.setBounds(x, knobY, knobSize, labelHeight);
    attackSlider_.setBounds(x, knobY + labelHeight, knobSize, knobSize);
    x += knobSpacing;

    decayLabel_.setBounds(x, knobY, knobSize, labelHeight);
    decaySlider_.setBounds(x, knobY + labelHeight, knobSize, knobSize);
}
