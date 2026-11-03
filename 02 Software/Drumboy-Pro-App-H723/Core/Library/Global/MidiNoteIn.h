#ifndef __MIDI_NOTE_IN_H
#define __MIDI_NOTE_IN_H

// v2.08 MIDI IN notes: a byte-by-byte parser for Note On / Note Off on one channel, with
// running status (most keyboards send it) and Note On velocity 0 = Note Off, feeding a small
// queue so fast chords and quick releases are not lost between two main-loop passes.
// Pure C++ with no hardware access, so it is host-tested (02 Software/Tests/test_midi_in.cpp).
//
// The UART ISR calls Parser::feed() for every byte that is not real-time (0xF8..0xFF are handled
// before, and never break a running message); the main loop calls Queue::pop().

#include <stdint.h>

namespace midiin {

struct Event {
    uint8_t on = 0;  // 1 = note on, 0 = note off
    uint8_t key = 0;
    uint8_t velocity = 0;
};

// Single producer (ISR) / single consumer (main loop) ring: the ISR only writes `head`, the
// main loop only writes `tail` (one byte each, so no lock is needed on a Cortex-M).
struct Queue {
    static constexpr uint8_t kSize = 32;  // power of two
    Event events[kSize];
    volatile uint8_t head = 0;
    volatile uint8_t tail = 0;

    bool push(const Event& e) {
        const uint8_t next = static_cast<uint8_t>((head + 1) & (kSize - 1));
        if (next == tail) return false;  // full: drop (a note off is retried by the 60 s cap)
        events[head] = e;
        head = next;
        return true;
    }
    bool pop(Event& e) {
        if (tail == head) return false;
        e = events[tail];
        tail = static_cast<uint8_t>((tail + 1) & (kSize - 1));
        return true;
    }
    void clear() { tail = head; }
};

struct Parser {
    uint8_t status = 0;  // running status: 0x80 / 0x90 of our channel, or 0 = ignore data
    uint8_t key = 0;
    uint8_t stage = 0;  // 0 = waiting for the key, 1 = waiting for the velocity

    // Returns true and fills `out` when a message is complete.
    bool feed(uint8_t byte, uint8_t channel, Event& out) {
        if (byte >= 0xF8) return false;  // real-time: transparent
        if (byte & 0x80) {
            // A status byte: ours starts a message, any other one cancels running status.
            const uint8_t type = static_cast<uint8_t>(byte & 0xF0);
            status = (((type == 0x80) || (type == 0x90)) && ((byte & 0x0F) == (channel & 0x0F))) ? type : 0;
            stage = 0;
            return false;
        }
        if (status == 0) return false;  // data of a message we do not use
        if (stage == 0) {
            key = byte;
            stage = 1;
            return false;
        }
        stage = 0;  // the next data byte starts a new message (running status)
        out.key = key;
        out.velocity = byte;
        out.on = ((status == 0x90) && (byte > 0)) ? 1 : 0;
        return true;
    }
};

}  // namespace midiin

#endif
