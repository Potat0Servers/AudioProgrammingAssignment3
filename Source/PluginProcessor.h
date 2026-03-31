/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "MySynthesiser.h"
#include "Arpeggiator.h"

//==============================================================================
/**
*/
class AudioProgrammingAssignment2AudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioProgrammingAssignment2AudioProcessor();
    ~AudioProgrammingAssignment2AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;


private:

    juce::Synthesiser synth;        // the synthesiser object that will play our sounds
    int voiceCount = 4;             // the number of voices that we want our synthesiser to have

	Arpeggiator myArp;              // the arpeggiator object that will process our MIDI messages

	// ceate a function that will return a layout of our parameters, which will be used in the constructor of the AudioProcessorValueTreeState class
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

	// create an instance of the AudioProcessorValueTreeState class, which will hold all our parameters and their current values
    juce::AudioProcessorValueTreeState apvts;

	// pointers to the parameters, which will be used in the processBlock() function to get the current values of the parameters
    std::atomic <float>* attack_ptr;
    std::atomic <float>* decay_ptr;
    std::atomic <float>* sustain_ptr;
    std::atomic <float>* release_ptr;
    std::atomic <float>* bitDepth_ptr;
    std::atomic <float>* pulseWidth_ptr;
    std::atomic <float>* downsampleFactor_ptr;

	// create a MidiBuffer to hold the MIDI messages that will be processed by the arpeggiator
    juce::MidiBuffer arpMidiBuffer;

	// Downsampler variables. Vector for multiple channels.
    std::vector<float> heldSamples;
    std::vector<int> sampleCounters;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioProgrammingAssignment2AudioProcessor)
};
