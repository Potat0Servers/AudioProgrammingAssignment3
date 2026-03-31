#pragma once
#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include <cmath>

/**
* @brief High - precision constant for Pi.
*
* We define our own PI constant rather than relying on the non - standard M_PI macro
* from <cmath> via #define _USE_MATH_DEFINES.In a large framework like JUCE,
* <cmath> is often included deep within the framework's headers (e.g., JuceHeader.h) 
* long before this file is evaluated.Due to standard include guards, our local macro
* definition would be ignored, leading to "undeclared identifier" compilation errors.
* Defining a static constant ensures robust, cross - platform compilation.
*/
const float M_PI = 3.14159265358979323846f;






/**
 * @class Oscillator
 * @brief Basic Digital Audio Oscillator.
 *
 * @details Generates 4 types of waveforms: sine, square, triangle, and ramp.
 * Maintains internal phase and frequency state for per-sample processing.
 */
class Oscillator {
private:
    float frequency = 440.0f;                  // The current frequency in Hz
    float phase = 0.0f;                        // The current phase value, bounded between [0.0, 1.0]
    float sampleRate = 44100.0f;               // The host audio system sample rate in Hz
    float phaseDelta = frequency / sampleRate; // The amount to increment the phase per audio sample
    float pulseWidth = 0.5f;                   // The pulse width modulation value, bounded between [0.0, 1.0]

    /**
     * @brief Updates the internal phase of the oscillator.
     *
     * Adds the phaseDelta to the current phase and wraps it back
     * to the [0.0, 1.0] range to prevent overflow and maintain periodicity.
     */
    void updatePhase() {
        phase += phaseDelta;
        if (phase > 1.0f) phase -= 1.0f;
    }

public:

    /**
     * @brief Sets the target frequency and updates the internal phase delta.
     *
     * @param newfreq The target frequency in Hz.
     */
    void setFrequency(float newfreq) {
        frequency = newfreq;
        phaseDelta = frequency / sampleRate;
    }

    /**
     * @brief Sets the host system's audio sample rate and updates the internal phase delta.
     *
     * @param newSampleRate The sample rate in Hz (e.g., 44100.0f).
     */
    void setSampleRate(float newSampleRate) {
        sampleRate = newSampleRate;
        phaseDelta = frequency / sampleRate;
    }

    /**
     * @brief Sets the internal phase of the oscillator directly.
     *
     * @param newPhase The target phase, typically bounded between 0.0 and 1.0.
     */
    void setPhase(float newPhase) {
        phase = newPhase;
    }

    /**
     * @brief Sets the pulse width of the oscillator directly.
     *
     * @param newPulseWidth The target pulse width, bounded between 0.0 and 1.0.
     */
    void setPulseWidth(float newPulseWidth) {
		// Clamping the pulse width value to ensure it stays within the valid range of [0.0, 1.0]
        pulseWidth = std::fmax(0.0f, std::fmin(1.0f, newPulseWidth));
    }

    /**
     * Process a single sine wave audio sample
     *
     * @return sine wave sample bounded within [-1.0, 1.0]
     */
    float processSine() {
		float output = std::sin(phase * 2.0f * M_PI);    // Calculate sine wave output based on current phase
        updatePhase();
        return output;
    }
	
    /**
     * Process a single square wave audio sample
     *
     * @return square wave sample (-1.0 or 1.0)
     */
    float processSquare() {
		float output = 0.0f;    // Initialize output variable
        if (phase < 0.5f) {
			output = 1.0f;  // Square wave oscillates between 1 and -1
        }
        else {
			output = -1.0f; // Square wave oscillates between 1 and -1
        }
        updatePhase();
        return output;
    }

    /**
     * Process a single triangle wave audio sample
     *
     * @return triangle wave sample bounded within [-1.0, 1.0]
     */
    float processTriangle() {
		float output = 0.0f;    // Initialize output variable
        if (phase < 0.5f) {	
            output = 4.0f * phase - 1.0f;   // first half of the cycle: rising from -1.0 to 1.0
        }
        else {
            output = 3.0f - 4.0f * phase;   // second half of the cycle: falling from 1.0 to -1.0
        }
        updatePhase();
        return output;
    }

    /**
     * Process a single ramp wave audio sample
     *
     * @return ramp wave sample bounded within [-1.0, 1.0]
     */
    float processRamp() {
        float output = 0.0f;    // Initialize output variable

        // Map the phase (0.0 to 1.0) to the audio range (-1.0 to 1.0)
        output = 2.0f * phase - 1.0f;   // full cycle: rising steadily from -1.0 to 1.0

        updatePhase();
        return output;
    }

    /**
     * Process a single pulse wave audio sample with variable duty cycle
     *
     * @return pulse wave sample (-1.0 or 1.0)
     */
    float processPulse() {
        float output = 0.0f;
        // Use pulseWidth instead of fixed 0.5f as threshold
        if (phase < pulseWidth) {
            output = 1.0f;
        }
        else {
            output = -1.0f;
        }
        updatePhase();
        return output;
    }
};

#endif