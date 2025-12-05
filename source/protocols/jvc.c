// jvc.c - (C)2025 Dakota Thorpe
// JVC Protocol.

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

// Send 1 bit using JVC pulse-distance encoding
static inline void IR_SendBit_JVC(bool bit)
{
    // Emit burst
    IR_Transmit(IR_JVC_CAR_FREQ, IR_JVC_BURST, 0.33f);

    // Space depends on the bit
    if (bit)
        usleep(IR_JVC_LOGICAL_1 - IR_JVC_BURST);
    else
        usleep(IR_JVC_LOGICAL_0 - IR_JVC_BURST);
}

// Send a byte LSB-first
static void IR_SendByte_JVC(uint8_t b)
{
    for (int i = 0; i < 8; i++)
        IR_SendBit_JVC((b >> i) & 1);
}

// Main JVC function
void IR_SendJVC(uint8_t address, uint8_t command)
{
    #ifdef NINTENDOWII
    uint32_t irq = IRQ_Disable();
    #endif

    // Header
    IR_Transmit(IR_JVC_CAR_FREQ, IR_JVC_BGN_SPACE, 0.33f);
    usleep(IR_JVC_BGN_BREAK);

    // Send 32-bit payload: address, ~address, command, ~command
    IR_SendByte_JVC(address);
    IR_SendByte_JVC(command);
    
    // Stop bit
    //IR_Transmit(IR_JVC_CAR_FREQ, IR_JVC_STOP, 0.33f);

    #ifdef NINTENDOWII
    IRQ_Restore(irq);
    #endif
}