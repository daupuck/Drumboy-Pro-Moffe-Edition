#ifndef __SAMPLE_FORMAT_H
#define __SAMPLE_FORMAT_H

// WAV PCM -> float conversion shared by the sample loader (Controller::layerInst_setSampleLoaded)
// and the sample preview. No hardware dependencies, so it is unit tested on the host
// (02 Software/Tests/test_sample_format.cpp).
//
// The rules reproduce the original loader exactly:
//  - one output value per frame, taken from the first (left) channel;
//  - 88.2/96 kHz and 176.4/192 kHz are decimated by 2 and 4 by stepping over whole frames;
//  - rates below 44.1 kHz are upsampled by repeating each value (sample-and-hold);
//  - 8-bit is unsigned, 16/24-bit signed little-endian, 32-bit is read as IEEE float.

#include <stdint.h>
#include <string.h>

namespace sampleformat {

struct Plan {
    uint8_t bitsPerSample;  // 8, 16, 24 or 32
    uint8_t offsetSize;     // bytes consumed per output value (frame size x decimation)
    uint8_t writeCount;     // output values written per input value (upsampling)
};

inline Plan plan(uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample) {
    Plan p;
    const uint8_t coefSamplerate = (sampleRate >= 176400) ? 4 : ((sampleRate >= 88200) ? 2 : 1);
    const uint8_t coefChannel = (channels == 2) ? 2 : 1;
    const uint8_t coefBps = static_cast<uint8_t>(bitsPerSample / 8);
    p.bitsPerSample = static_cast<uint8_t>(bitsPerSample);
    p.offsetSize = static_cast<uint8_t>(coefSamplerate * coefChannel * coefBps);

    p.writeCount = 1;
    if ((sampleRate < 44100) && (sampleRate > 0)) {
        if (sampleRate == 8000)
            p.writeCount = 6;
        else if (sampleRate == 11025)
            p.writeCount = 4;
        else if (sampleRate == 16000)
            p.writeCount = 3;
        else if ((sampleRate == 22050) || (sampleRate == 24000))
            p.writeCount = 2;
        else
            p.writeCount = static_cast<uint8_t>(44100 / sampleRate);
    }
    return p;
}

inline float toFloat(const uint8_t* p, uint8_t bitsPerSample) {
    float val = 0.0f;
    if (bitsPerSample == 8) {
        val = static_cast<float>(static_cast<int>(p[0]) - 128) / 128.0f;
    } else if (bitsPerSample == 16) {
        const int16_t s16 = static_cast<int16_t>(p[0] | (p[1] << 8));
        val = static_cast<float>(s16) / 32768.0f;
    } else if (bitsPerSample == 24) {
        int32_t s24 = static_cast<int32_t>(p[0] | (p[1] << 8) | (p[2] << 16));
        if (s24 & 0x800000) s24 |= static_cast<int32_t>(0xFF000000);
        val = static_cast<float>(s24) / 8388607.0f;
    } else if (bitsPerSample == 32) {
        memcpy(&val, p, 4);
    }
    return val;
}

}  // namespace sampleformat

#endif
