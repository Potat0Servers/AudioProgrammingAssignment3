/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"





//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AudioProgrammingAssignment2AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout newlayout;

	// Arguments: parameter ID, parameter name, min value, max value, default value

    // ADSR initialization
	// use lambda function to avoid code repetition
    auto addADSR = [&newlayout](const juce::String& prefix, const juce::String& namePrefix) {
        newlayout.add(std::make_unique<juce::AudioParameterFloat>(prefix + "_ATTACK", namePrefix + " Attack", 0.001f, 2.0f, 0.3f));
        newlayout.add(std::make_unique<juce::AudioParameterFloat>(prefix + "_DECAY", namePrefix + " Decay", 0.001f, 2.0f, 0.1f));
        newlayout.add(std::make_unique<juce::AudioParameterFloat>(prefix + "_SUSTAIN", namePrefix + " Sustain", 0.001f, 2.0f, 0.6f));
        newlayout.add(std::make_unique<juce::AudioParameterFloat>(prefix + "_RELEASE", namePrefix + " Release", 0.001f, 2.0f, 0.01f));
        };

	// generate ADSR parameters for each channel using the above lambda function
    // pulse width initialization for pulse channels exclusively
    addADSR("CH1", "Pulse 1");
    newlayout.add(std::make_unique<juce::AudioParameterFloat>("CH1_PULSE_WIDTH", "Pulse 1 Width", 0.0f, 1.0f, 0.5f));

    addADSR("CH2", "Pulse 2");
    newlayout.add(std::make_unique<juce::AudioParameterFloat>("CH2_PULSE_WIDTH", "Pulse 2 Width", 0.0f, 1.0f, 0.5f));

	addADSR("CH3", "Triangle"); // though nintendo's triangle channel doesn't have a real ADSR, we still give it one for better control
    addADSR("CH4", "Noise");


    // parameters below apply to all channels

	// Bit depth and downsample factor initialization. Both only accept integer values.
    newlayout.add(std::make_unique<juce::AudioParameterInt>("BIT_DEPTH", "Bit Depth", 1, 16, 8));
    newlayout.add(std::make_unique<juce::AudioParameterInt>("DOWNSAMPLE_FACTOR", "Downsample Factor", 1, 50, 1));

	// Arpeggiator on/off switch initialization
    newlayout.add(std::make_unique<juce::AudioParameterBool>("ARP_ON", "Arp On/Off", false));

    return newlayout;
}

//==============================================================================
AudioProgrammingAssignment2AudioProcessor::AudioProgrammingAssignment2AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       // Construction of a juce::AudioProcessorValueTreeState class instance, apvts.
	   // Due to too many parameters, a seperate function, createParameterLayout(), is used to create the layout of the parameters.
       apvts(*this,
           nullptr,
           "PARAMETERS",
           createParameterLayout())
#endif
{
	// create and add voices and sounds to the synthesiser object, and assign channel numbers to each voice and sound
	// channel 1 & 2：pulse wave
    synth.addVoice(new PulseVoice(1));
    synth.addSound(new My_SynthSound(1));
    synth.addVoice(new PulseVoice(2));
    synth.addSound(new My_SynthSound(2));

	// channel 3：triangle wave
    synth.addVoice(new TriangleVoice(3));
    synth.addSound(new My_SynthSound(3));

	// channel 4：white noise
    synth.addVoice(new NoiseVoice(4));
    synth.addSound(new My_SynthSound(4));


	// use lambda function to link ADSR parameter pointers for each channel, to avoid code repetition
    auto linkParams = [this](ChannelParameters& params, const juce::String& prefix) {
        params.attack_ptr = apvts.getRawParameterValue(prefix + "_ATTACK");
        params.decay_ptr = apvts.getRawParameterValue(prefix + "_DECAY");
        params.sustain_ptr = apvts.getRawParameterValue(prefix + "_SUSTAIN");
        params.release_ptr = apvts.getRawParameterValue(prefix + "_RELEASE");
        };

    linkParams(ch1Params, "CH1");
    linkParams(ch2Params, "CH2");
    linkParams(ch3Params, "CH3");
    linkParams(ch4Params, "CH4");

	// link pulse width parameters for pulse channels
    ch1Params.pulseWidth_ptr = apvts.getRawParameterValue("CH1_PULSE_WIDTH");
    ch2Params.pulseWidth_ptr = apvts.getRawParameterValue("CH2_PULSE_WIDTH");

	// link overall parameters
    bitDepth_ptr = apvts.getRawParameterValue("BIT_DEPTH");
    downsampleFactor_ptr = apvts.getRawParameterValue("DOWNSAMPLE_FACTOR");
    arp_on_ptr = apvts.getRawParameterValue("ARP_ON");
}

AudioProgrammingAssignment2AudioProcessor::~AudioProgrammingAssignment2AudioProcessor()
{
}

//==============================================================================
const juce::String AudioProgrammingAssignment2AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioProgrammingAssignment2AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioProgrammingAssignment2AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioProgrammingAssignment2AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioProgrammingAssignment2AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioProgrammingAssignment2AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioProgrammingAssignment2AudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioProgrammingAssignment2AudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AudioProgrammingAssignment2AudioProcessor::getProgramName (int index)
{
    return {};
}

void AudioProgrammingAssignment2AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AudioProgrammingAssignment2AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // set the sample rate for our synth object  
    synth.setCurrentPlaybackSampleRate(sampleRate);  

	// set the sample rate for our arpeggiator object
    myArp.prepareToPlay(sampleRate);

	// resize the vector according to channel number, each channel has one scaler value. And initialize values to 0.0f
	heldSamples.resize(getTotalNumOutputChannels(), 0.0f);
	sampleCounters.resize(getTotalNumOutputChannels(), 0);
}

void AudioProgrammingAssignment2AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioProgrammingAssignment2AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AudioProgrammingAssignment2AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

	// kill extremly small values to improve performance
    juce::ScopedNoDenormals noDenormals;

	// clear the output buffer
    buffer.clear();



    // =========================================================================
	// update basic parameters
    // =========================================================================

	// get global parameter values that apply to all channels
    int bitDepth = bitDepth_ptr->load();
    int downsampleFactor = downsampleFactor_ptr->load();
    bool arpIsOn = arp_on_ptr->load() > 0.5f;

    // loop through all voices
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
		// if the voice is an instance of My_SynthVoice, then we can update its parameters
        if (auto* myVoice = dynamic_cast<My_SynthVoice*>(synth.getVoice(i)))
        {
            int channel = myVoice->getChannel();
            juce::ADSR::Parameters adsrData;
            float currentPulseWidth = 0.5f;

            // get parameter values according to channel number
            if (channel == 1) {
                adsrData.attack = ch1Params.attack_ptr->load();
                adsrData.decay = ch1Params.decay_ptr->load();
                adsrData.sustain = ch1Params.sustain_ptr->load();
                adsrData.release = ch1Params.release_ptr->load();
                currentPulseWidth = ch1Params.pulseWidth_ptr->load();
            }
            else if (channel == 2) {
                adsrData.attack = ch2Params.attack_ptr->load();
                adsrData.decay = ch2Params.decay_ptr->load();
                adsrData.sustain = ch2Params.sustain_ptr->load();
                adsrData.release = ch2Params.release_ptr->load();
                currentPulseWidth = ch2Params.pulseWidth_ptr->load();
            }
            else if (channel == 3) {
                adsrData.attack = ch3Params.attack_ptr->load();
                adsrData.decay = ch3Params.decay_ptr->load();
                adsrData.sustain = ch3Params.sustain_ptr->load();
                adsrData.release = ch3Params.release_ptr->load();
            }
            else if (channel == 4) {
                adsrData.attack = ch4Params.attack_ptr->load();
                adsrData.decay = ch4Params.decay_ptr->load();
                adsrData.sustain = ch4Params.sustain_ptr->load();
                adsrData.release = ch4Params.release_ptr->load();
            }

			// send new value to voice
            myVoice->updateADSR(adsrData);

			// only for pulse channels, update pulse width parameter
            if (auto* pulseVoice = dynamic_cast<PulseVoice*>(myVoice)) {
                pulseVoice->updatePulseWidth(currentPulseWidth);
            }
        }
    }



    // =========================================================================
    // Arpeggiator
    // =========================================================================

    // clear private midi buffer for arpeggiator to write into
    arpMidiBuffer.clear();

    // process with arpeggiator with on/off switch
    myArp.processBlock(midiMessages, arpMidiBuffer, buffer.getNumSamples(), arpIsOn);

    // clear the original midi buffer, just in case 
    midiMessages.clear();



    // =========================================================================
    // Render the next block
    // =========================================================================

	// Render the next block of audio from the synth
    // arguments: audio buffer, midi buffer, start sample (0), number of samples
    synth.renderNextBlock(buffer, arpMidiBuffer, 0, buffer.getNumSamples());



    // =========================================================================
    // Bitcrusher & Downsampler
    // =========================================================================

    // calculate quantization steps（for example, 8-bit -> 256 steps）
    float numberOfSteps = std::pow(2.0f, bitDepth);

	// loop through each channel and sample
    for (int channel = 0; channel < buffer.getNumChannels(); channel++) 
    {
        // get the pointer to the start of the current channel's data
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); sample++) 
        {
			// When the sample counter reaches the downsample factor
			// Must use >= instead of ==, because when we decrease the downsample factor, 
            // the counter may be larger than the new factor, which will cause the downsampler to fail.
            if (sampleCounters[channel] >= downsampleFactor) 
            {
                sampleCounters[channel] = 0;                        // reset counter 
                heldSamples[channel] = channelData[sample];         // update held sample value
            }
            else 
            {
                channelData[sample] = heldSamples[channel];         // set value of sample to held sample value
				sampleCounters[channel]++;                          // increase counter       
            }

            // wrap from -1.0~1.0 to 0.0~1.0
            float shiftedSample = (channelData[sample] + 1.0f) * 0.5f;

            // quantization based on given numberOfSteps
            float quantized = std::floor(shiftedSample * numberOfSteps) / numberOfSteps;

            // wrao back to -1.0~1.0 and send value to buffer
            channelData[sample] = (quantized * 2.0f) - 1.0f;
        }
    }
}

//==============================================================================
bool AudioProgrammingAssignment2AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioProgrammingAssignment2AudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void AudioProgrammingAssignment2AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AudioProgrammingAssignment2AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioProgrammingAssignment2AudioProcessor();
}
