// controller.c - (C)2025 Dakota Thorpe.
// Handles basic level Infrared commanding.

// TODO: Write seperate function to completely halt any CPU work, and do this.
// Frame timing can really matter.

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
#include <math.h>
#include "WiiIR/IR.hpp"

// Dawg, if I gotta explain this to you, you sinceirly need to reconsider why
// you might be here.
void _IR_SET_GPIO(u32 gpio, u32 value)
{
    u32 val = (*_gpio_out_reg & ~gpio);
    if (value)
        val |= gpio;
    *_gpio_out_reg = val;
}

// Transmit a specified carrier signal with a duty cycle (0.0 - 1.0) for the specified time.
void IR_Transmit(float carrier_frequency, int duration_us, float duty_cycle)
{
    if (duty_cycle < 0.0f || duty_cycle > 1.0f) {
        printf("Error: Duty cycle must be between 0.0 and 1.0.\n");
        return;
    }

    // Calculate the period of one cycle in microseconds.
    float period_us = (1000.0f / carrier_frequency); // Period in microseconds (1 second / frequency).
    int cycles = duration_us / period_us;               // Total number of cycles to generate.

    // Calculate the on and off times based on the duty cycle.
    float on_time = floor(period_us * duty_cycle);              // On time based on duty cycle.
    float off_time = floor(period_us - on_time);                // Off time is the remaining period.

    // Loop through each cycle
    for (int i = 0; i < cycles; i++)
    {
        _IR_SET_GPIO(0x100, 255);  // IR LED ON
        usleep((int)on_time);      // Wait for the calculated on-time.

        _IR_SET_GPIO(0x100, 0);    // IR LED OFF
        usleep((int)off_time);     // Wait for the calculated off-time.
    }
}