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
    newlayout.add(std::make_unique<juce::AudioParameterFloat>("ATTACK", "Attack", 0.001f, 2.0f, 0.3f));
    newlayout.add(std::make_unique<juce::AudioParameterFloat>("DECAY", "Decay", 0.001f, 2.0f, 0.1f));
    newlayout.add(std::make_unique<juce::AudioParameterFloat>("SUSTAIN", "Sustain", 0.001f, 2.0f, 0.6f));
    newlayout.add(std::make_unique<juce::AudioParameterFloat>("RELEASE", "Release", 0.001f, 2.0f, 0.01f));

    // Some Chiptune parameters

    // Pulse Width initialization. Ranging from 0 to 1.
    newlayout.add(std::make_unique<juce::AudioParameterFloat>("PULSE_WIDTH", "Pulse Width", 0.0f, 1.0f, 0.5f));

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
    // adds voiceCount voices to the synth
    for (int i = 0; i < voiceCount; i++) {
        synth.addVoice(new My_SynthVoice()); 
    }

	// adds a sound to the synth
    synth.addSound(new My_SynthSound());

    attack_ptr = apvts.getRawParameterValue("ATTACK");
    decay_ptr = apvts.getRawParameterValue("DECAY");
    sustain_ptr = apvts.getRawParameterValue("SUSTAIN");
    release_ptr = apvts.getRawParameterValue("RELEASE");
    bitDepth_ptr = apvts.getRawParameterValue("BIT_DEPTH");
	pulseWidth_ptr = apvts.getRawParameterValue("PULSE_WIDTH");
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

	// Create a struct to hold the ADSR parameters, which will be used to pass new values to the voices.
    juce::ADSR::Parameters adsrData;

	// Get value of parameters from APVTS.
	// using pointers to improve performance.
    adsrData.attack = attack_ptr->load();
    adsrData.decay = decay_ptr->load();
    adsrData.sustain = sustain_ptr->load();
    adsrData.release = release_ptr->load();
    int bitDepth = bitDepth_ptr->load();
    float pulseWidth = pulseWidth_ptr->load();
    int downsampleFactor = downsampleFactor_ptr->load();
    bool arpIsOn = arp_on_ptr->load() > 0.5f;

	// loop through all the voices in the synth
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        // get current voice, and try to cast to My_SynthVoice type
        if (auto* myVoice = dynamic_cast<My_SynthVoice*>(synth.getVoice(i)))
        {
            // if successful, use the method and update ADSR and pulse width
            myVoice->updateADSR(adsrData);
            myVoice->updatePulseWidth(pulseWidth);
        }
    }



    // =========================================================================
    // Arpeggiator
    // =========================================================================

    // 1. 清空私有缓冲
    arpMidiBuffer.clear();

    // 传入开关状态
    myArp.processBlock(midiMessages, arpMidiBuffer, buffer.getNumSamples(), arpIsOn);

    // 3. 拦截销毁宿主的输入，彻底避免 VST3 协议冲突！
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
    // 1. 从 APVTS 中复制出一份完整的状态（XML 格式）
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    // 2. 将这个 XML 转换成二进制数据存入 destData，交给宿主保管
    copyXmlToBinary(*xml, destData);
}

void AudioProgrammingAssignment2AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // 1. 从二进制数据中尝试还原 XML
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    // 2. 检查 XML 是否有效且名字匹配（我们在构造函数里起的 "PARAMETERS"）
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            // 3. 将 XML 数据推回到 APVTS 中，这会自动触发界面和 DSP 参数的更新
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
