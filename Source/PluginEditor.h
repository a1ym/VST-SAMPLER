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
    VSTSamplerAudioProcessorEditor (VSTSamplerAudioProcessor& p);
    ~VSTSamplerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;



private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.

    enum TransportState
    {
        Stopped,
        Playing,
        Starting,
        Stopping
    };

    TransportState state;

    VSTSamplerAudioProcessor& p;

    juce::TextButton importButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton chordIdButton;
    
    void importButtonClicked();
    
    void playButtonClicked();
    void stopButtonClicked();
    void transportStateChanged(TransportState newState);

    juce::AudioFormatManager formatManager;
   

    std::unique_ptr<juce::AudioFormatReaderSource> playSource;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VSTSamplerAudioProcessorEditor)
};
