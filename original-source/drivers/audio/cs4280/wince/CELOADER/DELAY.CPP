//****************************************************************************
//
// DELAY.CPP - Execution delay routines
//
// Copyright (c) 1997-1998 Crystal Semiconductor Corp.
//
// Functions: delay
//
//****************************************************************************
#include "delay.h"
extern "C"
{
    #include <conio.h>
}

//****************************************************************************
//
// delay will stall execution for the specified number of micro-seconds.
//
//****************************************************************************
void
delay(unsigned long ulNumMicroSecs)
{
    unsigned long reference_count, current_count, absolute_count = 0;
    unsigned long last_count,  delay_count;
    
    int lo_byte, hi_byte, rollover = 0;
        
    delay_count = (ulNumMicroSecs*1000)/(838);
    
    //
    // Get reference tick count from 8253 timer 0
    //
    lo_byte = inp(0x40);
    hi_byte = inp(0x40);
    reference_count = (hi_byte << 8) | lo_byte;

    //
    // Just in case the counter rolls before the first compare
    //
    last_count = reference_count;
       
    while(absolute_count < delay_count)
    {
        lo_byte = inp(0x40);
        hi_byte = inp(0x40);
        current_count = (hi_byte << 8) | lo_byte;
                    
        //
        // if current_count > last_count the counter rolled over 
        //
        if(current_count > last_count)
            rollover = rollover + 1;
                  
        absolute_count = (reference_count + ((rollover*65535) - current_count));
        last_count = current_count;
    }
}

