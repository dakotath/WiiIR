// NEC_and_NECext.c - (C)2025 Dakota Thorpe.
// Implementation of the NEC and NECext protocol.

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

// Includes
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <gccore.h>
#include "WiiIR/IR.hpp"

// Repeating signal (a.k.a. Key Held Down).
void IR_RepeatNEC() {
    // Burst 9mS AGC.
    IR_Transmit(IR_NEC_CAR_FREQ, IR_NEC_BGN_SPACE, 0.33f);
    usleep(IR_NEC_LOGICAL_1);
    
    // Quick burst.
    IR_Transmit(IR_NEC_CAR_FREQ, IR_NEC_BURST, 0.33f);
    usleep(IR_NEC_FULL_FRMT-IR_NEC_BGN_SPACE-IR_NEC_BURST); // Subtract used time from frame time.
}

// Send a byte via NEC encoding
void IR_SendByteNEC(u8 byte, bool inverse) {
    // Iterate over each bit of the byte (from MSB to LSB)
    for (int bit = 0; bit <= 7; bit++) {
        // Extract each bit (starting from the MSB)
        bool bitValue = (byte >> bit) & 0x01;

        // If inversion is enabled, flip the bit
        if (inverse) {
            bitValue = !bitValue;  // Flip 0 to 1 and 1 to 0
        }

        // Send the burst (typically IR_NEC_BURST µs for each bit)
        IR_Transmit(IR_NEC_CAR_FREQ, IR_NEC_BURST, 0.33f);

        // Logically send the bit
        if (bitValue) {
            // Logical 1: Wait for the time defined for Logical 1
            usleep(IR_NEC_LOGICAL_1-IR_NEC_BURST);
        } else {
            // Logical 0: Wait for the time defined for Logical 0
            usleep(IR_NEC_LOGICAL_0-IR_NEC_BURST);
        }
    }
}

// IR Command (NECext)
void IR_SendNECext(u8 adrl, u8 adrm, u8 datal, u8 datam, bool invert_dm) {
    // Prepare system to serve an IR request
    u32 restoreLevel = IRQ_Disable();

    // 9mS Burst (Signal Data Start).
    IR_Transmit(IR_NEC_CAR_FREQ, IR_NEC_BGN_SPACE, 0.5f);
    usleep(IR_NEC_END_SPACE);

    // Send Random adr (0x1001)
    IR_SendByteNEC(adrl, false); // LSB
    IR_SendByteNEC(adrm, false); // MSB

    // Random Command (0x69)
    IR_SendByteNEC(datal, false); // LSB-H
    IR_SendByteNEC(datam, invert_dm); // MSB-H

    // End of transmission.
    IR_Transmit(IR_NEC_CAR_FREQ, IR_NEC_BURST, 0.5f);

    // We're done
    IRQ_Restore(restoreLevel);
}

// IR Command (NEC, Standard)
void IR_SendNEC(u8 adr, u8 data) {
    // Prepare system to serve an IR request.
    u32 restoreLevel = IRQ_Disable();

    // 9mS Burst (Signal Data Start).
    IR_Transmit(IR_NEC_CAR_FREQ, IR_NEC_BGN_SPACE, 0.5f);
    usleep(IR_NEC_END_SPACE);

    // Send Adr
    IR_SendByteNEC(adr, false);
    IR_SendByteNEC(adr, true);

    // Send Command
    IR_SendByteNEC(data, false);
    IR_SendByteNEC(data, true);

    // End of transmission.
    IR_Transmit(IR_NEC_CAR_FREQ, IR_NEC_BURST, 0.5f);

    // We're done
    IRQ_Restore(restoreLevel);
}