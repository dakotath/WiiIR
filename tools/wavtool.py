#!/usr/bin/env python3
import sys
import numpy as np
from scipy.io.wavfile import write

SAMPLE_RATE = 192000  # 192 kHz to get microsecond resolution

def generate_wave(pulse_durations_us):
    samples = []

    for i, duration_us in enumerate(pulse_durations_us):
        n_samples = max(1, int(duration_us * SAMPLE_RATE / 1_000_000))
        # Even index = ON, Odd index = OFF
        level = 32767 if i % 2 == 0 else -32768
        samples.extend([level] * n_samples)

    return np.array(samples, dtype=np.int16)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} output.wav pulse1 pulse2 ...")
        sys.exit(1)

    output_file = sys.argv[1]

    # Convert all arguments after output file into integers
    pulse_values = list(map(int, sys.argv[2:]))

    wav_data = generate_wave(pulse_values)
    write(output_file, SAMPLE_RATE, wav_data)
    print(f"WAV written: {output_file} ({len(wav_data)} samples at {SAMPLE_RATE} Hz)")
