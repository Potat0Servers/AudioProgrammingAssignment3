

#pragma once
#include "Oscillator.h"

// ===========================
// ===========================
// SOUND
class My_SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote      (int) override      { return true; }
    //--------------------------------------------------------------------------
    bool appliesToChannel   (int) override      { return true; }
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

	//这是一个构造函数，在这里设置一些初始参数
    My_SynthVoice() {
		// set the parameters of the envelope
        //envParams.attack = 0.3f;
		//envParams.decay = 0.1f;
		//envParams.sustain = 0.8f;
		//envParams.release = 0.1f;
        // then passing this Parameters instance to actual ADSR object
		env.setParameters(envParams);
    }


    // 新增一个方法，专门用来接收外面传进来的新参数
    void updateADSR(const juce::ADSR::Parameters& newParams)
    {
        // 直接整体赋值给类成员 envParams
        envParams = newParams;
		// 然后将更新后的参数传递给 ADSR 对象
        env.setParameters(envParams);
    }

    // 在 My_SynthVoice 类里面新增这个方法
    void updatePulseWidth(float newPulseWidth)
    {
        // 直接传给底层的振荡器
        osc1.setPulseWidth(newPulseWidth);
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
        env.noteOff();

        ending = true;
        
    }
    
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
                // An example white noise generater as a placeholder - replace with your own code
                // float currentSample = random.nextFloat()*2 - 1.0;

				float currentSample = osc1.processPulse() * env.getNextSample();
                
                // for each channel, write the currentSample float to the output
                for (int chan = 0; chan<outputBuffer.getNumChannels(); chan++)
                {
                    // The output sample is scaled by 0.2 so that it is not too loud by default
                    outputBuffer.addSample (chan, sampleIndex, currentSample * 0.2);
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
     Can this voice play a sound. I wouldn't worry about this for the time being

     @param sound a juce::SynthesiserSound* base class pointer
     @return sound cast as a pointer to an instance of My_SynthSound
     */
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<My_SynthSound*> (sound) != nullptr;
    }


    //--------------------------------------------------------------------------
private:
    //--------------------------------------------------------------------------
    // Set up any necessary variables here
    /// Should the voice be playing?
    bool playing = false;

    bool ending = false;

    /// a random object for use in our test noise function
    juce::Random random;

	// add an instance of oscillator
	Oscillator osc1;

    // create an adsr instance
    juce::ADSR env;

	// create an instance of the ADSR parameters struct, which will hold the parameters for our envelope
    juce::ADSR::Parameters envParams;



};
