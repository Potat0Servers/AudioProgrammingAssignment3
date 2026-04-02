#pragma once
#include <JuceHeader.h>
#include <vector>
#include <algorithm>

/**
 * A high-speed arpeggiator class designed for 8-bit chiptune style effects.
 *
 * This class implements a robust MIDI processing logic using a private buffer
 * architecture to isolate input/output events. 
 */
class Arpeggiator
{
public:

    /**
     * Prepares the arpeggiator by initializing variables.
     *
     * @param newSampleRate The sample rate at which the audio processor is running.
     */
    void prepareToPlay(double newSampleRate)
    {
        sampleRate = newSampleRate;     
		setSpeed(15.0f);                // set default arpeggio speed to 15 Hz
		heldNotes.clear();              // clear any held notes
    }

    /**
     * Sets the playback speed of the arpeggiator.
     *
     * This method calculates the number of audio samples each note should last
     * based on the provided frequency and the current system sample rate.
     *
     * @param speedInHz The desired speed in Hertz (notes per second).
     */
    void setSpeed(float speedInHz)
    {
        if (speedInHz > 0.0f && sampleRate > 0.0) {
            // calculate how many samples each note should last 
            noteDuration = static_cast<int>(sampleRate / speedInHz);
        }
    }


     /**
      * Core DSP logic for the arpeggiator's MIDI processing.
      * This method manages the timing and generation of MIDI notes based on the
      * current held keys, or bypasses the effect if the arpeggiator is disabled.
      *
      * @param inputMidi  Read-only buffer containing incoming MIDI data from the host.
      * @param outputMidi Private MIDI buffer to be filled with generated arpeggio notes in MIDI.
      * @param numSamples The total number of samples in the current audio block.
      * @param isOn       Boolean flag indicating whether the arpeggiator effect is active.
      */
    void processBlock(const juce::MidiBuffer& inputMidi, juce::MidiBuffer& outputMidi, int numSamples, bool isOn)
    {
        // Bypass Mode
        if (!isOn)
        {
            // if arpoff is triggered during an arpeggio process，send Note Off and reset state variables
            if (currentPlayingNote != -1)
            {
				// send note off to the correct channel, and reset the state variables
                outputMidi.addEvent(juce::MidiMessage::noteOff(currentPlayingChannel, currentPlayingNote), 0);
                currentPlayingNote = -1;
                currentPlayingChannel = -1;
            }

			// send through all incoming MIDI events without modification, and clear the held notes state
            outputMidi.addEvents(inputMidi, 0, numSamples, 0);
            heldNotes.clear();

            // exit early
            return;
        }


        // ====================================================================
		// Processing of Input MIDI 
        // ====================================================================
        for (const auto meta : inputMidi)
        {
			// get midi message
            auto msg = meta.getMessage();

            // if it's a note on message, add the note to the heldNotes vector
            if (msg.isNoteOn())
            {
                // create a new ArpNote struct
                ArpNote newNote{ msg.getNoteNumber(), msg.getChannel() };

                // we only add the note if not found in the heldNotes vector already
                if (std::find(heldNotes.begin(), heldNotes.end(), newNote) == heldNotes.end())
                {
                    heldNotes.push_back(newNote);                   // then add it to the heldNotes vector
                    std::sort(heldNotes.begin(), heldNotes.end());  // sort the vector so that the arpeggio always goes from low to high
                }
            }
            // if it's a note off message, remove the corresponding note from the heldNotes vector
            else if (msg.isNoteOff())
            {
				// create an ArpNote struct to find the corresponding note in the heldNotes vector
                ArpNote oldNote{ msg.getNoteNumber(), msg.getChannel() };

				// remove the corresponding note from the heldNotes vector
                heldNotes.erase(std::remove(heldNotes.begin(), heldNotes.end(), oldNote), heldNotes.end());
            }
        }


        // ====================================================================
		// Some extra code for more robust Note Off and State Reset...
        // ====================================================================
        if (heldNotes.empty())
        {
            // if all notes have been released
            if (currentPlayingNote != -1)
            {
                // send note off to the correct channel, and reset the state variables
                outputMidi.addEvent(juce::MidiMessage::noteOff(currentPlayingChannel, currentPlayingNote), 0);
                currentPlayingNote = -1;
                currentPlayingChannel = -1;
            }

			// reset timing state variables, and exit early
            timeInSamples = 0;
            currentNoteIndex = 0;
            return;
        }


        // ====================================================================
        // Arpeggio Logic & Output
        // ====================================================================
        // loop through each sample in the block
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // if it's the first note，or it's time to switch to the next note
            if (currentPlayingNote == -1 || timeInSamples >= noteDuration)
            {
				// if its time to switch to the next note, then turn off the currently playing note
                if (currentPlayingNote != -1)
                {
                    outputMidi.addEvent(juce::MidiMessage::noteOff(currentPlayingChannel, currentPlayingNote), sample);
                }

                // if this is the first note, we set the index to 0
                if (currentPlayingNote == -1)
                {
                    currentNoteIndex = 0;
                }
                // if its time to switch to the next note
                else
                {
                    // move to the next note index
                    currentNoteIndex++;

					// if the index goes beyond the size of heldNotes vector, wrap it around to 0
                    if (currentNoteIndex >= heldNotes.size()) 
                    {
                        currentNoteIndex = 0;
                    }
                }

                // take the next note to play from the heldNotes vector, and send
                ArpNote nextNote = heldNotes[currentNoteIndex];
                outputMidi.addEvent(juce::MidiMessage::noteOn(nextNote.channel, nextNote.note, 1.0f), sample);

				// update the state variables and reset the timer
                currentPlayingNote = nextNote.note;
                currentPlayingChannel = nextNote.channel;
                timeInSamples = 0;
            }
            // ordinary case
            else
            {
				// increase the timer
                timeInSamples++;
            }
        }
    }

private:
    struct ArpNote          // define a struct that combines MIDI note number and channel
    {
        int note;
        int channel;

        // to make std::find work, only when both note and channel are the same, we consider it as the same note
        bool operator==(const ArpNote& other) const { return note == other.note && channel == other.channel; }

        // to make std::sort work, we only care about the note number
        bool operator<(const ArpNote& other) const { return note < other.note; }
    };

	// note info variables
    std::vector <ArpNote> heldNotes;    // a vector that stores all currently held MIDI notes
    int currentPlayingNote = -1;        // this variable stores the midi note number of the currently playing note. -1 means no note is currently playing
	int currentPlayingChannel = -1;     // this variable stores the midi channel of the currently playing note. -1 means no channel is currently playing
    size_t currentNoteIndex = 0;        // this variable is the index of the currently playing note in the heldNotes vector.
                                        // Using size_t because we use heldNotes.size() to compare with it

	// time and count variables
    double sampleRate = 44100.0;    
    int timeInSamples = 0;          // this variable counts how many samples have passed since the current note started playing
	int noteDuration = 0;           // this variable counts how many samples each note should last, calculated from the speed parameter and sample rate
};