// parse_pronto.c - (C)2025 Dakota Thorpe.
// Pronto code parser for WiiIR.

/*
 * Copyright 2025 Dakota Thorpe and Larsen Vallecillo
 *
 * This file is part of the Wii IR project.
 *
 * Permission is hereby granted to view the source code of this project for informational purposes only.
 *
 * The following rights are explicitly prohibited for all individuals and entities except Dakota Thorpe 
 * and Larsen Vallecillo:
 * - Copying
 * - Modifying
 * - Distributing
 * - Sharing
 * - Using
 *
 * No part of this project may be reproduced, distributed, or transmitted in any form or by any means, 
 * including but not limited to copying, modification, or incorporation into other projects, without 
 * prior written permission from Dakota Thorpe and Larsen Vallecillo.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "WiiIR/IR.hpp"

/*
    Pronto CCF Signal Format:
    A Pronto IR signal consists of a sequence of 4-digit hexadecimal numbers.

    Header Format:
        (u16) pronto[0] - Signal Type:
            0x0000: Modulated raw IR signal.
            0x0100: Non-modulated raw IR signal.
            0x5000: RC5 protocol signal.
            0x6000: RC6 protocol signal.
            0x900A: NEC1 protocol signal.

        (u16) pronto[1] - Carrier Frequency Code:
            The frequency in Hertz can be calculated as:
                frequency = 1000000 / (carrier_code * 0.241246)

        (u16) pronto[2] - Number of pairs in the starting sequence (half the durations).
        (u16) pronto[3] - Number of pairs in the repeating sequence (half the durations).

    Sequence Format:
        The signal data follows the header, consisting of start and repeat sequences.
        Each sequence is a series of time durations, alternating between on-periods
        and off-periods. The durations are expressed as multiples of the carrier period time:
            period_time = 1 / frequency
        For non-modulated signals, a frequency must still be specified to compute durations.

        Example:
            0000 006C 0022 0002 015B 00AD 0016 0041 ...
            - Signal Type: 0000 (modulated raw IR signal)
            - Frequency Code: 006C -> 38381 Hz
            - Starting Pairs: 0022 (34 pairs)
            - Repeating Pairs: 0002 (2 pairs)
            - Start Sequence: Durations follow (e.g., 015B, 00AD...)
*/

// Function to calculate frequency from the carrier code.
float _pronto_calculate_frequency(uint16_t carrier_code) {
    return 1000.0f / (carrier_code * PRONTO_FREQCALC_FLOAT_VAL);
}

// Function to send pronto codes.
void IR_SendPronto(const uint16_t *pronto, size_t length) {
    // Pronto header is 4 bytes.
    if (length < 4) {
        fprintf(stderr, "Invalid Pronto signal: too short.\n");
        return;
    }

    // Extract the values of the pronto header.
    uint16_t signal_type = pronto[0];
    uint16_t carrier_code = pronto[1];
    uint16_t start_pairs = pronto[2];
    uint16_t repeat_pairs = pronto[3];

    // Determine carrier frequency (in KHz)
    float frequency = _pronto_calculate_frequency(carrier_code);

    printf("Signal Type: 0x%04X\n", signal_type);
    printf("Carrier Frequency: %.2f KHz\n", frequency);
    printf("Starting Pairs: %u\n", start_pairs);
    printf("Repeating Pairs: %u\n", repeat_pairs);
    printf("Attemting to transmit, Pronto codes aren't yet fully tested.\n");
    printf("Don't expect results, even if you have given the proper pronto.\n");

    // Send any start pairs.
    if (start_pairs > 0) {
        for (size_t i = 4; i < 4 + 2 * start_pairs; i += 2) {
            int us_on_time = (int)((pronto[i]/frequency)*1000);
            int us_off_time = (int)((pronto[i + 1]/frequency)*1000);
            IR_Transmit(frequency, us_on_time, 0.33f);
            usleep(us_off_time);
        }
    }

    // Send any repeat pairs.
    if (repeat_pairs > 0) {
        for (size_t i = 4 + 2 * start_pairs; i < 4 + 2 * (start_pairs + repeat_pairs); i += 2) {
            int us_on_time = (int)((pronto[i]/frequency)*1000);
            int us_off_time = (int)((pronto[i + 1]/frequency)*1000);
            IR_Transmit(frequency, us_on_time, 0.33f);
            usleep(us_off_time);
        }
    }
}