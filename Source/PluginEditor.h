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
class VSTSamplerAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                        public juce::ChangeListener
{
public:
    VSTSamplerAudioProcessorEditor (VSTSamplerAudioProcessor& p);
    ~VSTSamplerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    double getSampleRate() const
    {
        return getSampleRate();
    }


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

    double sampleRate;

    juce::TextButton importButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton chordIdButton;
    juce::Label chordLabel;
    
    void importButtonClicked();
    
    void playButtonClicked();
    void stopButtonClicked();
    void transportStateChanged(TransportState newState);
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void chordDetectionButtonClicked();
    juce::String detectChord(const std::vector<float>& frequencyData);


    juce::AudioFormatManager formatManager;
    

    std::shared_ptr<juce::AudioFormatReader> reader;



    std::unique_ptr<juce::AudioFormatReaderSource> playSource;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VSTSamplerAudioProcessorEditor)
};
