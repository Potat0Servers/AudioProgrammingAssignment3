

#pragma once
#include "Oscillator.h"
#include "WhiteNoise.h"

// ===========================
// ===========================
// SOUND
class My_SynthSound : public juce::SynthesiserSound
{
public:

    int myChannel; // Sound's channel number

    // the constructor. Must get channel number as an argument
    My_SynthSound(int channel) : myChannel(channel) {}

    bool appliesToNote      (int) override      { return true; }
    //--------------------------------------------------------------------------
	// the appliesToChannel method decides which sound to trigger for a given MIDI message. 
    // In this case, we check if the MIDI channel of the input message matches the channel that this Sound is assigned to.
    bool appliesToChannel   (int midiChannel) override      { return midiChannel == myChannel; }
};




// =================================
// =================================
// Synthesiser Voice - your synth code goes in here

/*!
 @class My_SynthVoice
 @abstract struct defining the DSP associated with a specific voice.
 @discussion multiple My_SynthVoice objects will be created by the Synthesiser so that it can be played polyphicially
 
 @namespace none
 @updated 2019-06-18
 */
class My_SynthVoice : public juce::SynthesiserVoice
{
public:

    /**
     * Constructor for the My_SynthVoice class.
     * Initializes the voice with a specific channel index and sets up
     * default ADSR envelope parameters.
     *
     * @param channel The channel number assigned to this specific voice instance.
     */
    My_SynthVoice(int channel) : myChannel(channel) 
    {
		// initial ADSR parameter value. 
		// However for the whole plugin, the default values is determined 
        // in the createParameterLayout() function in PluginProcessor.cpp, 
        // which will overwrite these default values when the plugin is loaded.
        envParams.attack = 0.1f;
        envParams.decay = 0.1f;
        envParams.sustain = 0.8f;
        envParams.release = 0.1f;

		// pass the parameters to the ADSR object
		env.setParameters(envParams); 
    }


    /**
     * Updates the ADSR envelope parameters with a new configuration.
     *
     * @param newParams A reference to a juce::ADSR::Parameters object
     * containing the updated attack, decay, sustain, and release values.
     */
    void updateADSR(const juce::ADSR::Parameters& newParams)
    {
        env.setParameters(newParams);
    }

    /**
     * Updates the pulse width of the first oscillator.
     *
     * @param newPulseWidth The new pulse width value, expecting value ranging from 0.0 to 1.0.
     */
    void updatePulseWidth(float newPulseWidth)
    {
        osc1.setPulseWidth(newPulseWidth);
    }

    /**
    * Retrieves the channel index assigned to this voice.
    *
    * @return The integer ID representing the specific channel this voice is bound to.
    */
    int getChannel()
    {
        return myChannel;
    }



    //--------------------------------------------------------------------------
    /**
     What should be done when a note starts

     @param midiNoteNumber
     @param velocity
     @param SynthesiserSound unused variable
     @param / unused variable
     */
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        playing = true;
        
		// set the sample rate of the oscillator and the envelope
        osc1.setSampleRate(getSampleRate());
        env.setSampleRate(getSampleRate());

		// midi pitch -> frequency conversion, and set the frequency of the oscillator
        float freq = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
		osc1.setFrequency(freq);

		// set note on
        env.noteOn();
    }


    //--------------------------------------------------------------------------
    /// Called when a MIDI noteOff message is received
    /**
     What should be done when a note stops

     @param / unused variable
     @param allowTailOff bool to decie if the should be any volume decay
     */
    void stopNote(float /*velocity*/, bool allowTailOff) override
    {
        // set note off
        env.noteOff();

		// ending flag is used to tell the renderNextBlock() function that the note is in its release stage
        ending = true;
    }
    




	// Create a virtual function for rendering the oscillator, 
    // which will be implemented differently in different types of voices (pulse, triangle, noise)
    virtual float renderOscillator() = 0;

    //--------------------------------------------------------------------------
    /**
     The Main DSP Block: Put your DSP code in here
     
     If the sound that the voice is playing finishes during the course of this rendered block, it must call clearCurrentNote(), to tell the synthesiser that it has finished

     @param outputBuffer pointer to output
     @param startSample position of first sample in buffer
     @param numSamples number of smaples in output buffer
     */
    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        if (playing) // check to see if this voice should be playing
        {
            // iterate through the necessary number of samples (from startSample up to startSample + numSamples)
            for (int sampleIndex = startSample;   sampleIndex < (startSample+numSamples);   sampleIndex++)
            {

				// render the current sample from the oscillator, and multiply it by the current envelope value
				float currentSample = renderOscillator() * env.getNextSample();
                
                // for each channel, write the currentSample float to the output
                for (int channel = 0; channel < outputBuffer.getNumChannels(); channel++)
                {
                    // The output sample is scaled by 0.2 so that it is not too loud by default
                    outputBuffer.addSample (channel, sampleIndex, currentSample * 0.2);
                }

				// if the envelope has finished its release stage, stop playing and clear the note
                if (env.getNextSample() < 0.00001 && ending) {
                    clearCurrentNote();
                    playing = false;
                }
            }
        }
    }


    //--------------------------------------------------------------------------
    void pitchWheelMoved(int) override {}


    //--------------------------------------------------------------------------
    void controllerMoved(int, int) override {}


    //--------------------------------------------------------------------------
    /**
     * Determines whether this voice is capable of playing the given sound.
     * This method performs a type check and verifies if the sound's channel assignment
     * matches the channel ID of this specific voice instance.
     *
     * @param sound A pointer to the SynthesiserSound object being evaluated.
     * @return True if the sound's channel matches the voice's channel; false otherwise.
     */
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        // Type Check
        if (auto* mySound = dynamic_cast<My_SynthSound*>(sound))
			// Channel Check: Verify if the sound's channel matches this voice's channel
            return mySound->myChannel == myChannel;

        return false;
    }




    //--------------------------------------------------------------------------
protected:
    //--------------------------------------------------------------------------
    // Set up any necessary variables here
    /// Should the voice be playing?
    bool playing = false;

	// A flag to indicate if the note is in its release stage
    bool ending = false;

	// add an instance of oscillator
	Oscillator osc1;

    // create an adsr instance
    juce::ADSR env;

	// create an instance of the ADSR parameters struct, which will hold the parameters for our envelope
    juce::ADSR::Parameters envParams;

    // channel number
    int myChannel; 

};



/**
 * A specialized synthesizer voice for generating pulse waves.
 * This class inherits from My_SynthVoice and is specifically intended for
 * use on channels 1 and 2 to provide a square or pulse wave output.
 */
class PulseVoice : public My_SynthVoice
{
public:
	// constructor that initializes the base My_SynthVoice with the given channel number
    PulseVoice(int channel) : My_SynthVoice(channel) {}

	// provide the own implementation of the renderOscillator function to generate pulse wave
    float renderOscillator() override 
    {
        return osc1.processPulse();
    }
};



/**
 * A specialized synthesizer voice for generating triangle waves.
 * This class inherits from My_SynthVoice and is specifically intended for
 * use on channel 3 to provide triangle wave output.
 */
class TriangleVoice : public My_SynthVoice
{
public:
    // constructor that initializes the base My_SynthVoice with the given channel number
    TriangleVoice(int channel) : My_SynthVoice(channel) {}

    // provide the own implementation of the renderOscillator function to generate triangle wave
    float renderOscillator() override 
    {
        return osc1.processTriangle();
    }
};



/**
 * A specialized synthesizer voice for generating white noise.
 * This class inherits from My_SynthVoice and is specifically intended for
 * use on channel 4 to provide white noise output.
 */
class NoiseVoice : public My_SynthVoice
{
private:
    // initialize an instance of the WhiteNoise class
    WhiteNoise noiseGen; 
public:
    // constructor that initializes the base My_SynthVoice with the given channel number
    NoiseVoice(int channel) : My_SynthVoice(channel) {}

    // provide the own implementation of the renderOscillator function to generate white noise
    float renderOscillator() override 
    {
        return noiseGen.process();
    }
};