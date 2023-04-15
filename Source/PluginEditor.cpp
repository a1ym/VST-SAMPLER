/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VSTSamplerAudioProcessorEditor::VSTSamplerAudioProcessorEditor(VSTSamplerAudioProcessor& p)
    : AudioProcessorEditor(&p), p(p), state(Stopped)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(400, 300);

    importButton.onClick = [this] {importButtonClicked();};
    addAndMakeVisible(&importButton);
    importButton.setButtonText("Import audio");

    playButton.onClick = [this] {playButtonClicked();};
    addAndMakeVisible(&playButton);
    playButton.setButtonText("Play");
    playButton.setEnabled(true);


    stopButton.onClick = [this] {stopButtonClicked();};
    addAndMakeVisible(&stopButton);
    stopButton.setButtonText("Stop");
    stopButton.setEnabled(false);


    chordIdButton.onClick = [this] {importButtonClicked();};
    addAndMakeVisible(&chordIdButton);
    chordIdButton.setButtonText("Chord ID");



    formatManager.registerBasicFormats();
    p.transport.addChangeListener(this);

}

void VSTSamplerAudioProcessorEditor::importButtonClicked()
{
    juce::FileChooser chooser("Choose audio", juce::File::getSpecialLocation(juce::File::userDesktopDirectory), "*.wav; *.mp3", true, false, nullptr);
    if (chooser.browseForFileToOpen()) {
        juce::File audioFile;
        audioFile = chooser.getResult();
        juce::AudioFormatReader* reader = formatManager.createReaderFor(audioFile);
        if (reader != nullptr) { //for when the new file is selected
            std::unique_ptr < juce::AudioFormatReaderSource > tempSource(new juce::AudioFormatReaderSource(reader, true));
            p.transport.setSource(tempSource.get());
            transportStateChanged(Stopped);
            playSource.reset(tempSource.release());
        }
    }
}


void VSTSamplerAudioProcessorEditor::playButtonClicked() 
{
    transportStateChanged(Starting);
}

void VSTSamplerAudioProcessorEditor::stopButtonClicked()
{
    transportStateChanged(Stopping);
}


void VSTSamplerAudioProcessorEditor::transportStateChanged(TransportState newState) 
{
    if (newState != state)
    {
        state = newState;

        switch (state) {
            case Stopped:
                playButton.setEnabled(true);
                p.transport.setPosition(0.0);
                break;

            case Playing:
                playButton.setEnabled(true);
                break;

            case Starting:
                stopButton.setEnabled(true);
                playButton.setEnabled(false);
                p.transport.start();
                break;

            case Stopping:
                playButton.setEnabled(true);
                stopButton.setEnabled(false);
                p.transport.stop();
                break;
        }


    }
}


//Listening for changes - if there is a change, the state is changed to stopped
void VSTSamplerAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster* source) {
    if (source == &p.transport) {
        if (p.transport.isPlaying()) {
            transportStateChanged(Playing);
        }
        else {
            transportStateChanged(Stopped);
        }
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
    playButton.setBounds(leftMargin, topMargin + 40, buttonWidth, buttonHeight);
    stopButton.setBounds(leftMargin, topMargin + 80, buttonWidth, buttonHeight);
    chordIdButton.setBounds(leftMargin, topMargin + 120, buttonWidth, buttonHeight);

}
