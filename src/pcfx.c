/*
 * Gameblabla small PCFX lib Copyright 2024
 * Licensed under MIT license
 * See LICENSE file for more
 * 
 * The original timer code is from Optimus6128 and the license doesn't cover this part (initTimer and my_timer_irq)
 * He has assured me you can use it for any purpose though.
*/

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <eris/types.h>
#include <eris/std.h>
#include <eris/v810.h>
#include <eris/king.h>
#include <eris/tetsu.h>
#include <eris/romfont.h>
#include <eris/timer.h>
#include <eris/cd.h>
#include <eris/pad.h>
#include <eris/7up.h>
#include <eris/low/7up.h>
#include <eris/low/pad.h>
#include <eris/low/scsi.h>
#include <eris/low/soundbox.h>

#include "fastking.h"
#include "pcfx.h"

uint32_t mainfreq;

void Clear_BG0(int bpp);

int currentvid = -1;


// FOR PSG

unsigned char samplepsg_buf[SAMPLES_PSG_NUMBER][65535*2];
uint32_t samplepsg_play[SAMPLES_PSG_NUMBER];
uint32_t samplepsg_size[SAMPLES_PSG_NUMBER];
uint32_t samplepsg_loop[SAMPLES_PSG_NUMBER];
uint32_t samplepsg_ch[SAMPLES_PSG_NUMBER];
#define BG_KRAM_PAGE 1
#define ADPCM_KRAM_PAGE 0



void Set_Video(uint32_t bpp)
{
	int i;
	u16 microprog[16];
	
	if (currentvid == bpp) return;
	currentvid = bpp;
	
	eris_tetsu_set_priorities(1, 6, 5, 4, 3, 2, 0);
	eris_tetsu_set_7up_palette(0, 0);
	eris_tetsu_set_king_palette(0, 0, 0, 0);
	eris_tetsu_set_rainbow_palette(0);

	eris_king_set_bg_prio(KING_BGPRIO_0, KING_BGPRIO_HIDE, KING_BGPRIO_HIDE, KING_BGPRIO_HIDE, 0);
	
	eris_king_set_bg_mode(bpp, 0, 0, 0);
	
	eris_king_set_kram_pages(0, BG_KRAM_PAGE, 0, ADPCM_KRAM_PAGE);

	for(i = 0; i < 16; i++) {
		microprog[i] = KING_CODE_NOP;
	}
	
	eris_tetsu_set_palette(0, 0x0088);
	eris_tetsu_set_palette(1, 0xE088);
	eris_tetsu_set_palette(2, 0xE0F0);
	eris_tetsu_set_palette(3, 0x602C);
	eris_tetsu_set_palette(4, 0x0088);
	eris_tetsu_set_palette(5, 0xE088);
	eris_tetsu_set_palette(6, 0xE0F0);
	eris_tetsu_set_palette(7, 0x602C);
	for(i = 9; i < 16; i++) {
		eris_tetsu_set_palette(i, 0xfc88);
	}
	
	switch(bpp)
	{
		default:
		case KING_BGMODE_16M:
		case KING_BGMODE_64K:
			microprog[0] = KING_CODE_BG0_CG_0;
			microprog[1] = KING_CODE_BG0_CG_1;
			microprog[2] = KING_CODE_BG0_CG_2;
			microprog[3] = KING_CODE_BG0_CG_3;
			microprog[4] = KING_CODE_BG0_CG_4;
			microprog[5] = KING_CODE_BG0_CG_5;
			microprog[6] = KING_CODE_BG0_CG_6;
			microprog[7] = KING_CODE_BG0_CG_7;	
		break;
		
		case KING_BGMODE_4_PAL:
			microprog[0] = KING_CODE_BG0_CG_0;
		break;
		
		case KING_BGMODE_16_PAL:
			microprog[0] = KING_CODE_BG0_CG_0;
			microprog[1] = KING_CODE_BG0_CG_1;
		break;
		
		case KING_BGMODE_256_PAL:
			microprog[1] = KING_CODE_BG0_CG_1;
			microprog[2] = KING_CODE_BG0_CG_2;
			microprog[3] = KING_CODE_BG0_CG_3;
			microprog[4] = KING_CODE_BG0_CG_4;
			microprog[5] = KING_CODE_BG1_CG_0;
			microprog[6] = KING_CODE_ROTATE;
		break;
	}

				
	eris_king_disable_microprogram();
	eris_king_write_microprogram(microprog, 0, 16);
	eris_king_enable_microprogram();
	
	/*
	void eris_tetsu_set_video_mode(tetsu_lines lines, int ext_sync,
			tetsu_dotclock dotclock, tetsu_colordepth bg_depth,
			tetsu_colordepth spr_depth, int bg7up_show,
			int spr7up_show, int bg0_disp, int bg1_disp,
			int bg2_disp, int bg3_disp, int rainbow_disp);
	*/
	eris_tetsu_set_video_mode(TETSU_LINES_262, 0, TETSU_DOTCLOCK_5MHz, TETSU_COLORS_16,
				TETSU_COLORS_16, 0, 1, 1, 0, 0, 0, 0);
	eris_king_set_bat_cg_addr(KING_BG0, 0, 0);
	eris_king_set_bat_cg_addr(KING_BG0SUB, 0, 0);
	eris_king_set_scroll(KING_BG0, 0, 0);
	eris_king_set_bg_size(KING_BG0, KING_BGSIZE_256, KING_BGSIZE_256, KING_BGSIZE_256, KING_BGSIZE_256);



	Clear_BG0(bpp);
/*
	eris_low_sup_set_vram_write(VDC_CHIP_0, 0);
	for(i = 0; i < 64; i++) {
		eris_low_sup_vram_write(0, array[i]);
	}
	
	eris_sup_set(VDC_CHIP_0);
	eris_sup_spr_set(0);
	eris_sup_spr_create(0, 0, 0, 0);*/

}

void Init_Sprite(int chip)
{
	/*
	eris_low_sup_set_control(VDC_CHIP_0, 0, 1, 1);
	eris_low_sup_set_access_width(0, 0, SUP_LOW_MAP_32X32, 0, 0);
	eris_low_sup_set_scroll(0, 0, 0);
	eris_low_sup_set_video_mode(VDC_CHIP_0, 2, 2, 4, 0x1F, 0x11, 2, 239, 2);
	 */
	eris_low_sup_set_control(chip, 0, 0, 1);
	eris_low_sup_set_access_width(0, 0, SUP_LOW_MAP_32X32, 0, 0);
	eris_low_sup_set_scroll(0, 0, 0);
	eris_low_sup_set_video_mode(chip, 2, 2, 4, 0x1F, 0x11, 2, 239, 2);
}

void Update_Sprite(int chip, unsigned short array[], int size_sprite, int offset_spr)
{
	int i;
	eris_low_sup_set_vram_write(VDC_CHIP_0, offset_spr);
	for(i = 0; i < size_sprite; i++) {
		eris_low_sup_vram_write(0, array[i]);
	}
	
	eris_sup_set(chip);
	eris_sup_spr_set(0);
	eris_sup_spr_create(0, 0, 0, 0);	
}

void Move_Sprite(int chip, int x, int y)
{
	eris_sup_set(chip);
	eris_sup_spr_set(0);
	eris_sup_spr_xy(x, y);
}

void Upload_Palette(unsigned short pal[], int sizep)
{
	int i;
	for(i=0;i<sizep;i++)
	{
		eris_tetsu_set_palette(i, pal[i]);
	}
}


void Clear_BG0(int bpp)
{
	int i, space;
	
	space = (256*240)*2;

	eris_king_set_kram_write(0, 1);
	for(i = 0x0; i < space; i++) {
		eris_king_kram_write(0);
	}
	eris_king_set_kram_write(0, 1);
}


void Initialize_ADPCM(uint32_t freq)
{
	mainfreq = freq;
	eris_low_adpcm_set_control(freq, 1, 1, 0, 0);
	eris_low_adpcm_set_volume(0, 63, 63);
	eris_low_adpcm_set_volume(1, 63, 63);
	
/*
	When in ring buffer mode, the buffer contents are looped. Sequential mode does not loop
	Bits	Description
	0	Buffer Mode 	
	0 = Sequential
	1 = Ring
	1	End interrupt 	
	0 = Disabled
	1 = Allow
	2	Intermediate interrupt
	0 = Disabled
	1 = Allow
*/
	out16(0x600,0x51);
	out16(0x604,0);
	
	out16(0x600,0x52);
	out16(0x604,0);
}




// FOR ADPCM

void LoadADPCMCD(u32 lba, u32 addr, uint32_t size_sample)
{
	// temp
	unsigned char* temp_sample_space;
	temp_sample_space = malloc(size_sample);
	
	// Works but slower
	// There's eris_cd_read_kram(BINARY_LBA_TITLEI_VOX, start_adress, sspace);
	// but for whatever reason, it fails to work for PAGE1, so it only works for PAGE0... :/
	eris_cd_read(lba, temp_sample_space, size_sample);
	
	eris_king_set_kram_read(addr, 1);
	eris_king_set_kram_write(addr, 1);	

	king_kram_write_buffer(temp_sample_space, size_sample);
	
	free(temp_sample_space);
}

void Reset_ADPCM()
{
	// Channel 1
	out16(0x600,0x52);
	out16(0x604,0);
	out16(0x600,0x5C);
	out16(0x604,0);
	
	// Channel 0
	out16(0x600,0x51);
	out16(0x604,0);
	out16(0x600,0x58);
	out16(0x604,0);

	/*
		Now I'm telling KING to set the end address. This is 0x59 for channel 0 and 0x5D for channel 1.
	*/
	out16(0x600,0x5D);
	out32(0x604,0);
	
	out16(0x600,0x59);
	out32(0x604,0);

	out16(0x600,0x50);
    out16(0x604, 0);
}

void Play_ADPCM(uint32_t channel, uint32_t start_adress, uint32_t sizet, unsigned char loop, uint32_t freq)
{
	//eris_king_set_kram_pages(0, BG_KRAM_PAGE, 0, ADPCM_KRAM_PAGE);
	eris_king_set_kram_read(start_adress, 1);
	
	/*
		0x58 is the ADPCM channel 1 start address.
		This is going to tell KING where to get the ADPCM data from.
		ADPCM data is kept in normal KRAM. Using 0x5C instead of 0x58 does the same for channel 1.
	*/
	if (channel == 1)
	{
		out16(0x600,0x52);
		out16(0x604,loop);
		
		out16(0x600,0x5C);
	}
	else
	{
		out16(0x600,0x51);
		out16(0x604,loop);

		out16(0x600,0x58);
	}

	/*
		The KRAM address has to be divided by 256. I wrote this out longhand to demonstrate.
	*/
	out16(0x604,(start_adress/256));

	/*
		Now I'm telling KING to set the end address. This is 0x59 for channel 0 and 0x5D for channel 1.
	*/
	if (channel == 1)
	{
		out16(0x600,0x5D);
	}
	else
	{
		out16(0x600,0x59);
	}

	/*
		The end address is an absolute address, not a divided one.
		Size should be in unsigned short
		Now that you've told KING the range of the sample, it's time to actually play it.

		OldRover made a mistake in his example and used out16 but in fact, it's a word.
	*/
	out32(0x604,((start_adress)+(((sizet-2048)>>1))));

	//Intermediate address, divided by 64. Used only if the intermediate interrupt bit in the channel's control is enabled.
	/*if (channel == 1) out16(0x600,0x5E);
	else out16(0x600,0x5A);
	out16(0x606, ((start_adress)+ (sizet >> 16)));*/

	out16(0x600,0x50);

	int playCommand;
	
	playCommand |= 1; // Set bit 0 for channel 0
	playCommand |= 2; // Set bit 1 for channel 1


    // Set frequency bits
    switch (freq) {
        case ADPCM_RATE_16000: playCommand |= 4; break;
        case ADPCM_RATE_8000: playCommand |= 8; break;
        case ADPCM_RATE_4000: playCommand |= 16; break;
        // Add more cases here for other frequencies
    }

    out16(0x604, playCommand);
}

void Play_PSGSample(int ch, int sample_numb, int loop)
{
	eris_low_psg_set_main_volume(15, 15);
	eris_low_psg_set_channel(ch);
	eris_low_psg_set_volume(31, 1, 1);
	eris_low_psg_set_balance(15, 15);	
	eris_low_psg_set_freq(0);
	eris_low_psg_set_noise(0, 0);
	samplepsg_play[sample_numb] = 1;
	samplepsg_loop[sample_numb] = loop;
	samplepsg_ch[sample_numb] = ch;
}

void Load_PSGSample(u32 lba, int numb, uint32_t size_sample)
{
	samplepsg_size[numb] = size_sample;
	eris_cd_read(lba, samplepsg_buf[numb], size_sample);
}


void Stop_PSGSample(int ch, int sample_numb, int loop)
{
	eris_low_psg_set_channel(ch);
	samplepsg_play[sample_numb] = 0;
	samplepsg_loop[sample_numb] = 0;
}

volatile int __attribute__ ((zda)) zda_timer_count = 0;
volatile int __attribute__ ((zda)) frame_text = 0;

/* Declare this "noinline" to ensure that my_timer_irq() is not a leaf. */
__attribute__ ((noinline)) void increment_zda_timer_count (void)
{
	zda_timer_count++;
	frame_text++;
}


__attribute__ ((interrupt)) void samplepsg_timer_irq (void)
{
	const int i = 0;
	eris_timer_ack_irq();

#if 1
	#if SAMPLES_PSG_NUMBER > 1
	for(int i=0;i<SAMPLES_PSG_NUMBER;i++)
	#endif
	{
		if (samplepsg_play[i])
		{
			eris_low_psg_set_channel(samplepsg_ch[i]);
			eris_low_psg_waveform_data(samplepsg_buf[i][samplepsg_play[i]-1]);
			samplepsg_play[i]++;
			if (samplepsg_play[i] >= samplepsg_size[i]-2047)
			{
				samplepsg_play[i] = 0;
				if (samplepsg_loop[i])
				{
					samplepsg_play[i] = 1;
				}
			}
		}
	}
#else
	#if SAMPLES_PSG_NUMBER > 1
	for(int i = 0; i < SAMPLES_PSG_NUMBER; i++)
	#endif
	{
		if (samplepsg_play[i])
		{
			eris_low_psg_set_channel(samplepsg_ch[i]);
			
			// Calculate the byte index and bit offset for the current 5-bit value.
			int byteIndex = (samplepsg_play[i] * 5) / 8;
			int bitOffset = (samplepsg_play[i] * 5) % 8;
			
			// Extract the 5-bit value from the packed data.
			unsigned char value = (samplepsg_buf[i][byteIndex] >> bitOffset) & 0x1F; // Extract first part
			if (bitOffset > 3) { // If bits span across two bytes
				value |= (samplepsg_buf[i][byteIndex + 1] << (8 - bitOffset)) & 0x1F;
			}
			
			eris_low_psg_waveform_data(value);
			
			samplepsg_play[i]++;
			
			// Adjust the condition to wrap around correctly based on 5-bit value count.
			if (samplepsg_play[i] >= (samplepsg_size[i] * 8) / 5 - 2047)
			{
				samplepsg_play[i] = 0;
				if (samplepsg_loop[i])
				{
					samplepsg_play[i] = 1;
				}
			}
		}
	}
#endif
}

__attribute__ ((interrupt)) void my_timer_irq (void)
{
	eris_timer_ack_irq();
	increment_zda_timer_count();
}

// Function to initialize the timer with a custom IRQ handler and period
void initTimer(int psg, int period) {
    // Disable all interrupts before changing handlers.
    irq_set_mask(0x7F);

    // Set the custom IRQ handler
    if (psg)
    {
		irq_set_raw_handler(0x9, samplepsg_timer_irq);
	}
    else
    {
		irq_set_raw_handler(0x9, my_timer_irq);
	}

    // Enable Timer interrupt.
    irq_set_mask(0x3F);

    // Reset and start the Timer with the specified period.
    eris_timer_init();
    eris_timer_set_period(period);

    eris_timer_start(1);

    // Clear any necessary variables or flags.
    zda_timer_count = 0;

    // Allow all IRQs.
    irq_set_level(8);

    // Enable V810 CPU's interrupt handling.
    irq_enable();
}




int GetSeconds()
{
	if (zda_timer_count == 0) return 0;
	return zda_timer_count / 1000;
}

void Reset_ZDA()
{
	zda_timer_count = 0;
}



void printstr(u32* str, int x, int y, int tall);
void chartou32(char* str, u32* o);

void chartou32(char* str, u32* o)
{
	int i;
	int len = strlen8(str);
	for(i = 0; i < len; i++)
		o[i] = str[i];
	o[i] = 0;
}

static inline void printch(u32 sjis, u32 kram, int tall)
{
	u16 px;
	int x, y;
	u8* glyph = eris_romfont_get(sjis, tall ? ROMFONT_ANK_8x16 : ROMFONT_ANK_8x8);
	for(y = 0; y < (tall ? 16 : 8); y++) {
		eris_king_set_kram_write(kram + (y << 5), 1);
		px = 0;
		for(x = 0; x < 8; x++) {
			if((glyph[y] >> x) & 1) {
				px |= 1 << (x << 1);
			}
		}
		eris_king_kram_write(px);
	}
}

void printstr(u32* str, int x, int y, int tall)
{
	int i;
	u32 kram = x + (y << 5);
	int len = strlen32(str);
	for(i = 0; i < len; i++) {
		printch(str[i], kram + i, tall);
	}
}





/* CD-ROM */
/*
void cd_pausectrl(u8 resume)
{
	u8 scsicmd10[10];
	
	eris_low_scsi_reset();
	
	if (resume > 1) resume = 1; // single bit; top 7 bits reserved
	scsicmd10[0] = 0x47; // operation code PAUSE RESUME
	scsicmd10[1] = 0; // Logical unit number
	scsicmd10[2] = 0; // reserved
	scsicmd10[3] = 0; // reserved
	scsicmd10[4] = 0; // reserved
	scsicmd10[5] = 0; // reserved
	scsicmd10[6] = 0; // reserved
	scsicmd10[7] = 0; // reserved
	scsicmd10[8] = resume; // Resume bit
	scsicmd10[9] = 0; // control
	eris_low_scsi_command(scsicmd10,10);
}
*/
