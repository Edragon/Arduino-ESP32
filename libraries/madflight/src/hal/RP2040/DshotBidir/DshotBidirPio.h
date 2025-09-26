/*==========================================================================================
MIT License

Copyright (c) 2025 https://github.com/qqqlab

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
==========================================================================================*/

/* 
DSHOT PROTOCOL
--------------

Available modes: DSHOT150, DSHOT300, DSHOT600, DSHOT1200. The number is the bit rate in kHz.

Frame Structure

11 bit throttle: 2048 possible values. 0 is reserved for disarmed. 1-47 are reserved for special commands. Leaving 48 to 2047 (2000 steps) for the actual throttle value
1 bit telemetry request - if this is set, telemetry data is sent back via a separate channel
4 bit CRC: (Cyclic Redundancy) Check to validate data (throttle and telemetry request bit)

Resulting in a frame with the following structure:

SSSSSSSSSSSTCCCC (16 bits, MSB transmitted first)

Pause (output low) between frames: 21 bits recommended, 2 minimum

Bit Structure

A bit period (3.333 us for DSHOT300) is split in 8 sub periods. A zero-bit is encoded as 3H 5L, and a one-bit is encoded as 6H 2L.

BIDIRECTIONAL DSHOT PROTOCOL
----------------------------

Available modes: DSHOT300, DSHOT600, DSHOT1200. The number is the bit rate in kHz.

The outgoing Frames are the same as unidirectional dshot, but inverted. Normal line level is high, not low. 

After the frame is sent, a nominal 30 us highrt level gap follows. Measured actual gaps were 20 ~ 40 us. In this time the transmitter should switch the line from high to pull-up.

The ESC replies with a 21 bit reply, the reply bit rate is 25% higher (5/4) than the transmission bit rate. 

See https://brushlesswhoop.com/dshot-and-bidirectional-dshot/ for full details on decoding.
*/

#define DSHOT_DMA_BUF_SIZE (256 / 4) 
//NOTE: dma buffer could be smaller, which will give faster rates but with risk of missing replies...
//dma sampling period for 256 samples with 5.33x oversampling is: 256 / 5.33 * 4/5 Tbit = 39 Tbit
//total period is: 16 Tbit (TX) + 2 Tbit (delay) + 39 Tbit (RX) + 10 Tbit (extra) =  67 Tbit
//So, max rate for DSHOT300_BIDIR is 300/67 = 4 kHz

#pragma once

#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "dshot_bidir.pio.h"

class DshotBidirPio {
private:
    bool setup_done = false;
    uint8_t pin_count;
    uint8_t pin;
    int rate_khz;

    PIO pio;
    uint sm;
    uint offset;
    uint32_t write_ts = 0; //last write timestamp

    int dma_ch;
    uint32_t dma_buf[DSHOT_DMA_BUF_SIZE];

public:
    uint32_t interval_us = 0; //minimal interval between two writes in us (16 bits transmission + 21 bits pause)

    ~DshotBidirPio() {
        end();
    }

    bool begin(uint8_t pin, uint8_t pin_count, int rate_khz) {
        if(pin_count < 1 || pin_count > 8 || rate_khz < 0) return false;

        this->pin = pin;
        this->pin_count = pin_count;
        this->rate_khz = rate_khz;
        this->interval_us = (16 + 2 + 39 + 10) * 1000 / rate_khz; //16 (tx) + 2 (delay) + 39 (dma) + 10 (extra) -- FIXME: calculate dma delay based on DSHOT_DMA_BUF_SIZE and OSR

        for(int i=0;i<DSHOT_DMA_BUF_SIZE;i++) dma_buf[i] = 0xffffffff; //init to high level, i.e. "no bidir reply"

        for(int i = 0 ; i < pin_count; i++) {
            gpio_pull_up(pin + i);
        }

        // Get a free dma channel
        dma_ch = dma_claim_unused_channel(false);
        if(dma_ch < 0) return false;

        // This will find a free pio and state machine for our program and load it for us
        // We use pio_claim_free_sm_and_add_program_for_gpio_range (for_gpio_range variant)
        // so we will get a PIO instance suitable for addressing gpios >= 32 if needed and supported by the hardware
        bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&dshot_bidir_program, &pio, &sm, &offset, pin, pin_count, true);
        if(!success) {
          dma_channel_unclaim(dma_ch);
          return false;
        }

        //config dma
        dma_channel_config c = dma_channel_get_default_config(dma_ch);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
        channel_config_set_write_increment(&c, true);
        channel_config_set_read_increment(&c, false);
        channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));
        dma_channel_configure(
            dma_ch,        // Channel to be configured
            &c,            // The configuration we just created
            NULL,           // The initial write address - set when starting dma
            &(pio->rxf[sm]),           // The read address
            DSHOT_DMA_BUF_SIZE, // Number of transfers; in this case each is 1 byte.
            false           // do not start
        );

        setup_done = true;
        return true;
    }

    void end() {
        if(setup_done) {
            setup_done = false;

            dma_channel_unclaim(dma_ch);

            // This will free resources and unload our program
            pio_remove_program_and_unclaim_sm(&dshot_bidir_program, pio, sm, offset);
        }
    }

    //set throttle for all ESCs
    bool set_throttle(uint16_t *throttle) {
        uint16_t values[8];
        for(int i = 0; i < pin_count; i++) {
          values[i] = ( throttle[i] == 0 ? 0 : throttle[i] + 48);
        }
        return write(values);
    }

    //only write if last write was more than interval_us in the past
    bool write(uint16_t *values) {
        if(micros() - write_ts < interval_us) return false;
        if(!write_raw(values)) return false;
        write_ts = micros();
        return true;
    }

    //immediately write, don't check interval
    bool write_raw(uint16_t *values) {
        if(!setup_done) return false;

        //convert values into frames (i.e. shift bits, add T=0 no telemetry bit, add CRC)
        uint16_t frames[8];
        for(int ch = 0; ch < pin_count; ch++) {
            uint16_t f = values[ch];
            if(f > 2047) f = 2047; //clip to max throttle
            f <<= 5; //shift and add T=0 no telemetry bit
            //f |= (1<<4); //set telemetry bit
            uint16_t crc = (f >> 4) ^ (f >> 8) ^ (f >> 12);
            f |= (~crc & 0x0F); //add inverted CRC
            frames[ch] = f;
        }

        //convert frames into fifo stream
        uint32_t d[4] = {};
        for(int ch = 0; ch < pin_count; ch++) {
            for(int bit = 0; bit < 16; bit++) {
                if( frames[ch] & (1u << bit) ) {
                    uint byte = 15 - bit; //byte in fifo stream (MSB of frame is sent first)
                    uint32_t word = byte / 4; //32bit word in fifo stream
                    uint32_t shift = (byte % 4) * 8; //shift in word (LSB of fifo is sent first)
                    d[word] |= (1u << shift);
                }
            }
        }

        //send the inverted frames to the PIO (leaves sm in disabled state)
        dshot_bidir_program_init(pio, sm, offset, pin, pin_count, rate_khz * 1000);  //reload program (in disabled state)
        for(int i = 0; i < 4; i++) {
            pio_sm_put(pio, sm, ~d[i]);
        }

        // set dma buffer, and start dma
        dma_channel_set_write_addr(dma_ch, dma_buf, true);

        //Serial.printf("a_dma_busy=%d\n", dma_channel_is_busy(dma_ch));

        //enable PIO to send our frames in parallel
        pio_sm_set_enabled(pio, sm, true); 

        return true;
    }

    void get_erpm( int* erpm) {
      for(int i = 0; i < pin_count; i++) {
        erpm[i] = read_erpm(i);
      }
    }

    //read erpm value for channel, returns negative on error
    int read_erpm(uint8_t channel) {
        uint32_t tlm_val;
        int tlm_type = read_telem(channel, &tlm_val);
        if(tlm_type<0) return tlm_type; //no data
        if(tlm_type>0) return -100 - tlm_type; //data received, but not erpm
        return tlm_val;
    }

    /*
    read telemetry for channel
    return values:
    -3: dma busy
    -2: mapping failure
    -1: crc failure
    0: val is erpm_us
    1-8: val is telemetry
    2: 0x02 	Temperature in C
    3: 0x04 	Voltage: 0.25V per step
    4: 0x06 	Current in Amp
    5: 0x08 	Debug value 1
    6: 0x0A 	Debug value 2
    7: 0x0C 	Debug value 3
    8: 0x0E 	State/Event 
    */
    int read_telem(uint8_t channel, uint32_t *tlm_val) {
        if(dma_channel_is_busy(dma_ch)) return -3;

        uint8_t *byte_buf = (uint8_t *)dma_buf;
        uint8_t mask = 1<<channel;

        /*
        for(int i=0;i<DSHOT_DMA_BUF_SIZE*4;i++) {
            uint8_t bit = (byte_buf[i] & mask ? 0 : 1); //bit value
            Serial.print(bit ? '1' : '-');
        }
        Serial.println();
        */

        //over sampling rates * 100
        int osr100 = (dshot_bidir_T1 + dshot_bidir_T2 + dshot_bidir_T3) * 100 * 32 / 40 / dshot_bidir_TRX;
        int osr100off = osr100 * 6 / 10;  //should be 50% in theory, but 60% boosts shorter pulses a bit which appears to help decoding
        //Serial.printf("osr100=%d ", osr100);

        uint8_t bit_last = 0;
        int pulse_len = 0;
        int bit_cnt = -1;
        uint32_t reply = 0;

        for(int i=0;i<DSHOT_DMA_BUF_SIZE*4;i++) {
            pulse_len++;
            uint8_t bit = (byte_buf[i] & mask ? 0 : 1); //bit value
            if(bit != bit_last) {
                int bits = (pulse_len * 100 + osr100off) / osr100;
                if(bits == 0) bits = 1; //accept short pulses as single bit
                //Serial.printf("bit=%d len=%d bits=%d reply=%X bit_cnt=%d\n", bit_last, pulse_len, bits, reply, bit_cnt);
                if(bit_cnt < 0) {
                    bit_cnt = 0;
                }else{
                    while(bit_cnt<21 && bits > 0) {
                        reply = (reply << 1) | bit_last; //store bits
                        bit_cnt++;
                        bits--;
                    }
                }
                pulse_len = 0;
                bit_last = bit;
                //Serial.printf("reply=%X bit_cnt=%d\n", reply, bit_cnt);
            }               
            if(bit_cnt>=21) break;
        }

        if(bit_cnt<21) reply <<= 21 - bit_cnt; //append zeroes

        *tlm_val = 0;
        return decode_telem(reply, tlm_val);
    }

private:
    //map 5 bits to 4 bits. returns 0xff on failure
    static uint8_t revertMapping(uint16_t value) {
        switch(value) {
            case 0x19: return 0x00;
            case 0x1B: return 0x01;
            case 0x12: return 0x02;
            case 0x13: return 0x03;
            case 0x1D: return 0x04;
            case 0x15: return 0x05;
            case 0x16: return 0x06;
            case 0x17: return 0x07;
            case 0x1A: return 0x08;
            case 0x09: return 0x09;
            case 0x0A: return 0x0A;
            case 0x0B: return 0x0B;
            case 0x1E: return 0x0C;
            case 0x0D: return 0x0D;
            case 0x0E: return 0x0E;
            case 0x0F: return 0x0F;
        }

        return 0xFF;
    }

    //decode received 21 bits to 12 bit value. Returns -1 on crc failure, -2 on mapping failure
    static int decode_bidir(uint32_t value) {
        uint16_t newValue;
        uint16_t mapped = 0x00;
        uint8_t leftShift = 0;

        value ^= (value >> 1); //21 bit -> 20 bit

        for(int i = 0; i < 20; i += 5) {
            newValue = revertMapping(((value >> i) & 0x1F));
            if(newValue == 0xff) return -2;
            mapped |= newValue << leftShift;
            leftShift += 4;
        }

        int crc = (~((mapped >> 4) ^ (mapped >> 8) ^ (mapped >> 12) )) & 0x0F;

        if(crc != (mapped & 0x0F) ) {
            return -1;
        }
        return mapped >> 4;
    }

/*
decode telemetry - return values:
-2: mapping failure
-1:crc failure
0:val is erpm_us
1-8:val is telemetry
2: 0x02 	Temperature in C
3: 0x04 	Voltage: 0.25V per step
4: 0x06 	Current in Amp
5: 0x08 	Debug value 1
6: 0x0A 	Debug value 2
7: 0x0C 	Debug value 3
8: 0x0E 	State/Event 
*/
    static int decode_telem(uint32_t reply, uint32_t *val) {
        int r = decode_bidir(reply); //decode shifted gcr value to 12 bits
        if(r<0) return r; //invalid

        //decode telemetry value
        if(r & 0x100) {
            //erpm value
            if(r == 0xFFF) {
                *val = 0;
            }else{
                *val = (r & 0x1ff) << (r >> 9); //erpm period in us
            }
            return 0;
        }else{
            //telemetry value
            *val = r & 0xff;
            return ((r >> 9) & 0x7) + 1;
        }
      }
};