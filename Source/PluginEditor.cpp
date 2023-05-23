/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <vector>
#include <cassert>
#include <fstream>

//==============================================================================
VSTSamplerAudioProcessorEditor::VSTSamplerAudioProcessorEditor(VSTSamplerAudioProcessor& p)
    : AudioProcessorEditor(&p), p(p), state(Stopped), sampleRate(p.getSampleRate())
{

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    setSize(400, 250);

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
    //g.drawFittedText ("Chord Recognition", getLocalBounds(), juce::Justification::centred, 1);
}

void VSTSamplerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto leftMargin = 8;
    auto topMargin = 10;
    auto buttonWidth = 80;
    auto buttonHeight = 30;

    importButton.setBounds(leftMargin, topMargin + 120, buttonWidth, buttonHeight);
    playButton.setBounds(leftMargin + 100, topMargin+120, buttonWidth, buttonHeight);
    stopButton.setBounds(leftMargin + 200, topMargin+120, buttonWidth, buttonHeight);
    
    
    chordIdButton.setBounds(leftMargin + 300, topMargin + 120, buttonWidth, buttonHeight);
    chordLabel.setBounds(leftMargin + buttonWidth + 20, topMargin + 10, getWidth() * 0.5, buttonHeight);

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
        static constexpr auto fftOrder = 12;            
        static constexpr auto frameSize = 1 << fftOrder;

        // FFT object
        juce::dsp::FFT fft(fftOrder);

        const int hopSize = frameSize / 2;

        // Buffer to store the audio data
        juce::AudioBuffer<float> audioBuffer(2, frameSize);

        // Buffer to store the time-domain data
        std::vector<float> timeDomainData(frameSize*2);
        std::fill(timeDomainData.begin(), timeDomainData.end(), 0.0f);

        // Buffer to store the magnitudes of the frequency-domain data
        std::vector<float> magnitudes(frameSize / 2);

        // Buffer to store the windowing function
        std::vector<float> window(frameSize);

        // Hamming window 
        std::vector<float> hammingWindow(frameSize);
        for (int i = 0; i < frameSize; ++i) {
            hammingWindow[i] = 0.54 - (0.46 * cos((2 * juce::MathConstants<float>::pi * i) / frameSize - 1));
        }

        // Blackman-Harris window 
        std::vector<float> blackmanHarrisWindow(frameSize);
        for (int i = 0; i < frameSize; ++i) {
            blackmanHarrisWindow[i] = 0.35875 -
                0.48829 * cos((2 * juce::MathConstants<float>::pi * i) / (frameSize - 1)) +
                0.14128 * cos((4 * juce::MathConstants<float>::pi * i) / (frameSize - 1)) -
                0.01168 * cos((6 * juce::MathConstants<float>::pi * i) / (frameSize - 1));
        }

        // Hann window 
        std::vector<float> hannWindow(frameSize);
        for (int i = 0; i < frameSize; ++i) {
            hannWindow[i] = 0.5f * (1 - cos((2 * juce::MathConstants<float>::pi * i) / (frameSize - 1)));
        }

        auto currentPosition = playSource->getNextReadPosition();

        sampleRate = reader->sampleRate;

        float stepSizeInSeconds = 0.5f;
        int stepSize = static_cast<int>(sampleRate * stepSizeInSeconds);

        // Iterate through the audio in chunks of stepSize
        for (int position = 0; position + frameSize <= playSource->getTotalLength(); position += stepSize)

        {
            // Set the read position for playSource
            playSource->setNextReadPosition(position);

            double timestamp = static_cast<double>(position) / sampleRate;
            DBG("Timestamp: " << timestamp);

            // Read the audio-data from source into the buffer
            playSource->getNextAudioBlock(juce::AudioSourceChannelInfo(audioBuffer));
            
            // Apply window
            std::vector<float> channelData(frameSize, 0.0f);

            for (int channel = 0; channel < 2; ++channel)
            {
                for (int i = 0; i < frameSize; ++i)
                {
                    channelData[i] = audioBuffer.getWritePointer(channel)[i] * hannWindow[i];
                }

                for (int i = 0; i < frameSize; ++i)
                {
                    timeDomainData[i] += channelData[i];
                }
            }


            // Perform FFT on audio-data
            fft.performFrequencyOnlyForwardTransform(timeDomainData.data());

            // Extract magnitudes
            
            for (int i = 0; i < frameSize / 2; ++i)
            {
                magnitudes[i] = timeDomainData[i];
            }

            // Saving magnitudes for visual analisys
            if (timestamp == 1.0) {
                
                std::ofstream file("magnitudes.csv");
                for (const auto& magnitude : magnitudes)
                {
                    file << magnitude << ",";
                }
                file.close();
            }

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




float dotProduct(const std::vector<float>& a, const std::vector<float>& b) {
    float result = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        result += a[i] * b[i];
    }
    return result;
}


juce::String VSTSamplerAudioProcessorEditor::detectChord(const std::vector<float>& magnitudes) {
    // Note names and chord templates
    const std::vector<juce::String> noteNames = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const std::vector<std::pair<std::vector<int>, juce::String>> chordTemplates = {
        {{0, 4, 7}, "Major"},
        {{0, 3, 7}, "Minor"},
        {{0, 4, 7, 11}, "Major 7th"}
        // More can be added
    };

    // PCP
    std::vector<float> pitchClassProfile(12, 0.0f);
    const std::vector<float> noteFrequencies = { 16.35, 17.32, 18.35, 19.45, 20.60, 21.83, 23.12, 24.50, 25.96, 27.50, 29.14, 30.87 };
    std::vector<float> logFrequencies(noteFrequencies.size());
    std::transform(noteFrequencies.begin(), noteFrequencies.end(), logFrequencies.begin(), [](float freq) { return std::log2(freq); });

    for (int i = 1; i < magnitudes.size() - 1; ++i) {
        float mag = magnitudes[i];
        if (mag > magnitudes[i - 1] && mag > magnitudes[i + 1])  // Peak detected
        {
            // Convert peak index to frequency
            float freq = static_cast<float>(i) * sampleRate / (2 * magnitudes.size());
            float logFreq = std::log2(freq);

            for (int octave = 0; octave < 8; ++octave) {
                for (int nf = 0; nf < 12; ++nf) {
                    float lowerBoundLog = (logFrequencies[nf] + logFrequencies[(nf - 1 + 12) % 12]) / 2 + octave;
                    float upperBoundLog = (logFrequencies[nf] + logFrequencies[(nf + 1) % 12]) / 2 + octave;
                    if (logFreq > lowerBoundLog && logFreq <= upperBoundLog) {
                        pitchClassProfile[nf] += mag;
                    }
                }
            }
        }
    }

/*
    // Normalize PCP
    float maxValue = *std::max_element(pitchClassProfile.begin(), pitchClassProfile.end());
    for (int i = 0; i < 12; ++i)
    {
        pitchClassProfile[i] /= maxValue;
    }
*/
 
    juce::String bestChord;
    float bestScore = 0.0f;

    // Compare PCP and chord templates and find the best match
    for (int rootNote = 0; rootNote < 12; ++rootNote)
    {
        for (const auto& templateChord : chordTemplates)
        {
            float score = 0.0f;
            for (int note : templateChord.first)
            {
                score += pitchClassProfile[(rootNote + note) % 12];
            }

            if (score > bestScore)
            {
                bestScore = score;
                bestChord = noteNames[rootNote] + " " + templateChord.second;
            }
        }
    }

    return bestChord;
}