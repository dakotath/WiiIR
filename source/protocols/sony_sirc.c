// sony_sirc.c - (C)2025 Dakota Thorpe
// Sony SIRC12,15,20 Protocols.

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

// Includes.
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "WiiIR/IR.hpp"

// Send bytes
/*
    @param byte 8-bit Byte to transmit
    @param length Length of the byte (because SIRC is el'strango).
*/
void IR_SendBitSIRC(bool bitValue) {
    if (bitValue) {
        // Logical 1: 1.2ms burst + 0.6ms space
        IR_Transmit(IR_SIRC_CAR_FREQ, IR_SIRC_LOGICAL_1, 0.33f); // 1.2ms burst
        usleep(IR_SIRC_BURST); // 0.6ms space
    } else {
        // Logical 0: 0.6ms burst + 0.6ms space
        IR_Transmit(IR_SIRC_CAR_FREQ, IR_SIRC_BURST, 0.33f); // 0.6ms burst
        usleep(IR_SIRC_BURST); // 0.6ms space
    }
}

void IR_SendByteSIRC(u8 byte, u8 length) {
    for (int bit = 0; bit < length; bit++) {
        bool bitValue = (byte >> bit) & 0x01;
        IR_SendBitSIRC(bitValue);
    }
}

// Send a command.
/*
    DEV Notes:
        LSB First.
        SIRC12 - 7bit command, 5bit address.
        SIRC15 - 7bit command, 8bit address.
        SIRC20 - 7bit command, 5bit address, 8bit extra.

        Personally, I would like to shame on anybody if they don't like goto labels.
        These are more efficent then other functions, and when CPU cycles matter,
        this can easily break IR signals because of the carrier.

        @param mode SIRC Transmission mode (i.e. SIRC12,15,20).
        @param address Address to send command to. (5 or 8 bits, depending on mode).
        @param data SIRC data. 2 bytes. split into data[extend], data[command]
*/
void IR_SendSIRC(IRMode_SIRC mode, u8 address, u16 data) {
    // Prepare system to serve an IR request
    u32 restoreLevel = IRQ_Disable();

    // Extract the real data.
    u8 command, extended;
    extended = (data >> 8) & 0xFF; // First byte of data (0xFFNN)
    command  = data & 0xFF;        // Last byte of data (0xNNFF)

    // Jump to transmission mode address.
    switch(mode) {
        case IR_SIRC_MODE_12:
            goto TX_MODE12;
        case IR_SIRC_MODE_15:
            goto TX_MODE15;
        case IR_SIRC_MODE_20:
            goto TX_MODE20;
        default:
            goto TX_FAIL;
    }

    // ------------------------
    // Transmit SIRC12
    TX_MODE12:
    IR_Transmit(IR_SIRC_CAR_FREQ, IR_SIRC_SPACE, 0.25f); // Start Signal
    usleep(IR_SIRC_BURST);

    // 7-Bit Command + 5-Bit Address
    IR_SendByteSIRC(command, 7);
    IR_SendByteSIRC(address, 5);

    // Wait 45ms
    usleep(45*1000);
    goto TX_DONE;

    // ------------------------
    // Transmit SIRC15
    TX_MODE15:
    IR_Transmit(IR_SIRC_CAR_FREQ, IR_SIRC_SPACE, 0.25f); // Start Signal
    usleep(IR_SIRC_BURST);

    // 7-Bit Command + 8-Bit Address
    IR_SendByteSIRC(command, 7);
    IR_SendByteSIRC(address, 8);

    // Wait 45ms
    usleep(45*1000);
    goto TX_DONE;

    // ------------------------
    // Transmit SIRC20
    TX_MODE20:
    IR_Transmit(IR_SIRC_CAR_FREQ, IR_SIRC_SPACE, 0.25f); // Start Signal
    usleep(IR_SIRC_BURST);

    // 7-Bit Command + 5-Bit Address + 8-Bit Extended
    IR_SendByteSIRC(command, 7);
    IR_SendByteSIRC(address, 5);
    IR_SendByteSIRC(extended, 8);

    // Wait 45ms
    usleep(45*1000);
    goto TX_DONE;

    // ------------------------
    // Failure
    TX_FAIL:
    printf("Failed to send SIRC byte.\n");

    // ------------------------
    // Done
    TX_DONE:
    IRQ_Restore(restoreLevel);
    return;
}