/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <vector>
#include <cassert>

//==============================================================================
VSTSamplerAudioProcessorEditor::VSTSamplerAudioProcessorEditor(VSTSamplerAudioProcessor& p)
    : AudioProcessorEditor(&p), p(p), state(Stopped), sampleRate(p.getSampleRate())
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


    chordIdButton.onClick = [this] {chordDetectionButtonClicked(); };
    addAndMakeVisible(&chordIdButton);
    chordIdButton.setButtonText("Chord ID");



    formatManager.registerBasicFormats();
    p.transport.addChangeListener(this);


    chordLabel.setFont(20.0f);
    chordLabel.setJustificationType(juce::Justification::centred);
    chordLabel.setText("No Chord Detected", juce::dontSendNotification);
    addAndMakeVisible(chordLabel);

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
    chordLabel.setBounds(leftMargin + buttonWidth + 20, topMargin, getWidth() * 0.5, buttonHeight);

}


void VSTSamplerAudioProcessorEditor::importButtonClicked()
{
    juce::FileChooser chooser("Choose audio", juce::File::getSpecialLocation(juce::File::userDesktopDirectory), "*.wav; *.mp3", true, false, nullptr);
    if (chooser.browseForFileToOpen()) {
        juce::File audioFile;
        audioFile = chooser.getResult();
        std::shared_ptr<juce::AudioFormatReader> tempReader(formatManager.createReaderFor(audioFile));
        if (tempReader != nullptr) { //For when the new file is selected
            std::unique_ptr < juce::AudioFormatReaderSource > tempSource(new juce::AudioFormatReaderSource(tempReader.get(), false));
            p.transport.setSource(tempSource.get());
            transportStateChanged(Stopped);
            playSource.reset(tempSource.release());
            reader = tempReader; // Assign tempReader to the member variable reader
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

void VSTSamplerAudioProcessorEditor::chordDetectionButtonClicked() {
    // Check whether the audio is loaded
    if (playSource != nullptr && reader)
    {
        // FFT Parameters
        static constexpr auto fftOrder = 13;            
        static constexpr auto frameSize = 1 << fftOrder;
        const int hopSize = frameSize / 2;
        DBG("frameSize: " << frameSize);

        // FFT object
        juce::dsp::FFT fft(fftOrder);

        // Buffer to store the audio data
        juce::AudioBuffer<float> audioBuffer(2, frameSize);

        // Buffer to store the time-domain data
        //std::array<float, frameSize * 2> timeDomainData;
        std::vector<float> timeDomainData(frameSize*2);

        std::fill(timeDomainData.begin(), timeDomainData.end(), 0.0f);


        // Buffer to store the magnitudes of the frequency-domain data
        std::vector<float> magnitudes(frameSize / 2);

        // Buffer to store the windowing function
        std::vector<float> window(frameSize);


        // Windowing function
        for (int i = 0; i < frameSize; ++i)
        {
            window[i] = 0.5f * (1.0f - cos(2.0f * juce::float_Pi * i / (frameSize - 1)));
        }

 
        auto currentPosition = playSource->getNextReadPosition();


        sampleRate = reader->sampleRate;

        // Iterate through the audio in chunks of hopSize
        float stepSizeInSeconds = 0.1f;
        int stepSize = static_cast<int>(sampleRate * stepSizeInSeconds);

        //int position = 0;
        for (int position = 0; position + frameSize <= playSource->getTotalLength(); position += stepSize)
        {
            // Set the read position for playSource
            playSource->setNextReadPosition(position);

            double timestamp = static_cast<double>(position) / sampleRate;
            DBG("Timestamp: " << timestamp);

            //DBG("Position: " << position);
            //DBG("Next read position: " << playSource->getNextReadPosition());
            //DBG("Total length: " << playSource->getTotalLength());

            // Read the audio-data from source into the buffer
            playSource->getNextAudioBlock(juce::AudioSourceChannelInfo(audioBuffer));
            
            // Apply windowing function and copy data to the time-domain buffer
            std::vector<float> channelData(frameSize, 0.0f);

            for (int channel = 0; channel < 2; ++channel)
            {
                for (int i = 0; i < frameSize; ++i)
                {
                    channelData[i] = audioBuffer.getWritePointer(channel)[i] * window[i];
                }

                for (int i = 0; i < frameSize; ++i)
                {
                    timeDomainData[i] += channelData[i];
                }
            }
            //DBG("timeDomainData size: " << timeDomainData.size());

            // Perform FFT on audio-data
            fft.performFrequencyOnlyForwardTransform(timeDomainData.data());



            // Copy magnitudes to its vector
            for (int i = 0; i < frameSize / 2; ++i)
            {
                magnitudes[i] = timeDomainData[i];
            }


            //DBG("Magnitudes size: " << magnitudes.size());

            // Analyze frequency data to detect chord
            juce::String chord = detectChord(magnitudes);
            DBG("Detected Chord: " << chord);
            // Update chord label text

            if (chord.isNotEmpty()) {
                chordLabel.setText(chord, juce::dontSendNotification);
                DBG("Updated Chord Label: " << chord);
            }
            else
            {
                DBG("No chord detected");
            }

            if (position + frameSize + hopSize > playSource->getTotalLength())
            {
                //break;
            }
        }

        // Restore to original playSource position
        playSource->setNextReadPosition(currentPosition);
        DBG("FINISH");


    }
}






juce::String VSTSamplerAudioProcessorEditor::detectChord(const std::vector<float>& magnitudes) {
    // Note frequencies and chord templates
    const std::vector<juce::String> noteNames = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const std::vector<std::pair<std::vector<int>, juce::String>> chordTemplates = {
    {{0, 4, 7}, "Major"},
    {{0, 3, 7}, "Minor"},
    {{0, 4, 7, 11}, "Major 7th"}
    // Add more if needed
    };

    // Vector for PCP
    std::vector<float> pitchClassProfile(12, 0.0f);

    // Calculate PCP
    for (int i = 1; i < magnitudes.size() - 1; ++i)
    {
        float mag = magnitudes[i];
        if (mag > magnitudes[i - 1] && mag > magnitudes[i + 1])  // Peak detected
        {
            // Convert peak index to frequency
            float freq = static_cast<float>(i) * sampleRate / (2 * magnitudes.size());

 
            int midiNote = round(69 + 12 * log2(freq / 440.0f));
            int noteIndex = midiNote % 12;
            pitchClassProfile[noteIndex] += mag;
        }
    }

    // Normalize PCP
    float maxValue = *std::max_element(pitchClassProfile.begin(), pitchClassProfile.end());
    for (int i = 0; i < 12; ++i)
    {
        pitchClassProfile[i] /= maxValue;
    }

    

    juce::String bestChord;
    int bestScore = 0;




    // Compare PCP and chord templates and find best results
    for (int rootNote = 0; rootNote < 12; ++rootNote)
    {
        for (const auto& templateChord : chordTemplates)
        {
            int score = 0;
            for (int note : templateChord.first)
            {
                score += pitchClassProfile[(rootNote + note) % 12];
            }

            if (score > bestScore)
            {
                bestScore = score;
                bestChord = noteNames[rootNote] + " " + templateChord.second;
                //DBG("Detected Chord: " << bestChord); 
            }
        }
    }


    return bestChord;
}

