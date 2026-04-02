
#pragma once
#include <JuceHeader.h>

/**
 * A simple white noise generator class.
 *
 * This class utilizes a pseudo-random number generator to produce
 * a continuous stream of random values, representing white noise
 * across the full audio dynamic range.
 */
class WhiteNoise {
private:
    juce::Random random;        // A random number generator

public:
    /**
     * Generates the next sample of white noise.
     *
     * @return A random float value scaled to the range of [-1.0, 1.0)
     */
    float process() {
		// we scale the output of nextFloat() from [0.0, 1.0) to [-1.0, 1.0) to get a full range of white noise
        return random.nextFloat() * 2.0f - 1.0f;
    }
};
