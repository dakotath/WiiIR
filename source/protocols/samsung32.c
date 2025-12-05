// samsung32.c - (C)2025 Dakota Thorpe
// Samsung32 Protocol.

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

// Send 1 bit using Samsung32 pulse-distance encoding
static inline void IR_SendBit_Samsung32(bool bit)
{
    // Emit burst
    IR_Transmit(IR_SAMSUNG32_CAR_FREQ, IR_SAMSUNG32_BURST, 0.33f);

    // Space depends on the bit
    if (bit)
        usleep(IR_SAMSUNG32_LOGICAL_1);
    else
        usleep(IR_SAMSUNG32_LOGICAL_0);
}

// Send a byte LSB-first
static void IR_SendByte_Samsung32(uint8_t b)
{
    for (int i = 0; i < 8; i++)
        IR_SendBit_Samsung32((b >> i) & 1);
}

// Main Samsung32 function
void IR_SendSamsung32(uint8_t address, uint8_t command)
{
    #ifdef NINTENDOWII
    uint32_t irq = IRQ_Disable();
    #endif

    // Header
    IR_Transmit(IR_SAMSUNG32_CAR_FREQ, IR_SAMSUNG32_BGN_SPACE, 0.33f);

    // Send 32-bit payload: address, ~address, command, ~command
    IR_SendByte_Samsung32(address);
    IR_SendByte_Samsung32(~address);
    IR_SendByte_Samsung32(command);
    IR_SendByte_Samsung32(~command);

    // Stop bit
    IR_Transmit(IR_SAMSUNG32_CAR_FREQ, IR_SAMSUNG32_STOP, 0.33f);

    #ifdef NINTENDOWII
    IRQ_Restore(irq);
    #endif
}