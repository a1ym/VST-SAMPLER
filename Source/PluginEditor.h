/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class VSTSamplerAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    VSTSamplerAudioProcessorEditor (VSTSamplerAudioProcessor&);
    ~VSTSamplerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    VSTSamplerAudioProcessor& audioProcessor;

    juce::TextButton importButton;

    void importButtonClicked();
    juce::AudioFormatManager formatManager;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VSTSamplerAudioProcessorEditor)
};
