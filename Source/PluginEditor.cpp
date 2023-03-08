/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VSTSamplerAudioProcessorEditor::VSTSamplerAudioProcessorEditor(VSTSamplerAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(400, 300);

    importButton.onClick = [this] {importButtonClicked();};
    addAndMakeVisible(importButton);
    importButton.setButtonText("Import audio");

    formatManager.registerBasicFormats();

}

void VSTSamplerAudioProcessorEditor::importButtonClicked()
{
    juce::FileChooser chooser("Choose audio", juce::File::getSpecialLocation(juce::File::userDesktopDirectory), "*.wav", true, false, nullptr);
    if (chooser.browseForFileToOpen()) {
        juce::File audioFile;
        audioFile = chooser.getResult();
        juce::AudioFormatReader* reader = formatManager.createReaderFor(audioFile);
        std::unique_ptr < juce::AudioFormatReaderSource > tempSource(new juce::AudioFormatReaderSource(reader, true));  
    }
}

VSTSamplerAudioProcessorEditor::~VSTSamplerAudioProcessorEditor()
{
}

//==============================================================================
void VSTSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("VST SAMPLER BY ALIM!", getLocalBounds(), juce::Justification::centred, 1);
}

void VSTSamplerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto leftMargin = getWidth() * 0.02;
    auto topMargin = getHeight() * 0.04;
    auto buttonWidth = getWidth() * 0.25;
    auto buttonHeight = getHeight() * 0.1;


    importButton.setBounds(leftMargin, topMargin, buttonWidth, buttonHeight);
}
