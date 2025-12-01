#include "PluginEditor.h"

BraidsVSTEditor::BraidsVSTEditor(BraidsVSTProcessor& p)
    : AudioProcessorEditor(&p), processor_(p)
{
    setSize(400, 300);
}

BraidsVSTEditor::~BraidsVSTEditor() = default;

void BraidsVSTEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("BraidsVST", getLocalBounds(), juce::Justification::centred, 1);
}

void BraidsVSTEditor::resized()
{
}
