/*
 * Gameblabla ADPCM example Copyright 2024
 * Licensed under MIT license
 * See LICENSE file for more
*/

#include <eris/v810.h>
#include <eris/king.h>
#include <eris/tetsu.h>
#include <eris/romfont.h>
#include <eris/timer.h>
#include <eris/cd.h>
#include <eris/pad.h>
#include <eris/low/pad.h>
#include <eris/low/scsi.h>
#include <eris/low/soundbox.h>
#include <eris/7up.h>
#include <eris/low/7up.h>
#include <string.h>
#include <math.h>

#include "main.h"
#include "pcfx.h"
#include "fastking.h"

#include "LEFT.H"
#include "RIGHT.H"
#include "SOUNDT.H"

#include "LEFT16.H"
#include "RIGHT16.H"
#include "SOUNDT16.H"


static u32 padtype, paddata;
static int curr_sec;
static u32 pad_time = 0;

#define KRAM_PAGE0 0x00000000
#define KRAM_PAGE1 0x80000000

#define ADPCM_OFFSET 0x00000000 | KRAM_PAGE1
#define CHANNEL_0_ADPCM 0x51
#define CHANNEL_1_ADPCM 0x52

#define CDDA_SILENT 0
#define CDDA_NORMAL 0x03
#define CDDA_LOOP 0x04


static u8 framebuffer[139264];

int nframe = 0;

static int phase_text = 0;


const unsigned short game_sprite[64] =
{
    0x4000, 0x7000, 0x7c00, 0x7f80, 0x7fe0, 0x7ff8, 0x7ffe, 0x7fff,
    0x7ffe, 0x7ff0, 0x7fc0, 0x7e00, 0x7800, 0x6000, 0x4000, 0x0000,
    0x4000, 0x7000, 0x7c00, 0x7f80, 0x7fe0, 0x7ff8, 0x7ffe, 0x7fff,
    0x7ffe, 0x7ff0, 0x7fc0, 0x7e00, 0x7800, 0x6000, 0x4000, 0x0000,
    0x4000, 0x7000, 0x7c00, 0x7f80, 0x7fe0, 0x7ff8, 0x7ffe, 0x7fff,
    0x7ffe, 0x7ff0, 0x7fc0, 0x7e00, 0x7800, 0x6000, 0x4000, 0x0000,
    0x4000, 0x7000, 0x7c00, 0x7f80, 0x7fe0, 0x7ff8, 0x7ffe, 0x7fff,
    0x7ffe, 0x7ff0, 0x7fc0, 0x7e00, 0x7800, 0x6000, 0x4000, 0x0000
};

static void print_text(const char* text, int y_increment) {
    const char* line_start = text;
    int line_length;
    uint32_t str[256]; // Assuming a maximum line length
    int line_number = 0;

    for (const char* it = text; ; ++it) {
        // Check for newline character or end of string
        if (*it == '\n' || *it == '\0') {
            line_length = it - line_start;
            if (line_length > 0) {
                // Copy the line into a temporary buffer
                char line_buffer[line_length + 1];
                strncpy(line_buffer, line_start, line_length);
				//strcpy(line_buffer, line_start);
                line_buffer[line_length] = '\0';

                // Convert and print the line
                chartou32(line_buffer, str);
                printstr(str, 1, y_increment + (line_number*16), 1);
            }

            if (*it == '\0') {
                break; // End of the main string
            }

            line_start = it + 1;
            ++line_number;
        }
    }
}




static int game_status = 0;
/*
void cd_endtrk2(u8 end, u8 mode)
{	
	u8 scsicmd10[10];
	memset(scsicmd10, 0, sizeof(scsicmd10));
	scsicmd10[0] = 0xD9;
	scsicmd10[1] = mode;
	scsicmd10[2] = end;
	scsicmd10[9] = 0x80;
	eris_low_scsi_command(scsicmd10,10);
	
	while(eris_low_scsi_status() == SCSI_LOW_STATUS_IN_PROGRESS || eris_low_scsi_status() == SCSI_LOW_STATUS_BUSY);
}*/

#define WAIT_CD 0x800

static void cd_start_track(u8 start)
{	
	/*
	 * 
	 * To play a CD-DA track, you need to use both 0xD8 and 0xD9.
	 * 0xD8 is for the starting track and 0xD9 is for the ending track as well and controlling whenever
	 * or not the track should loop (after it's done playing).
	*/
	int r10;
	u8 scsicmd10[10];
	memset(scsicmd10, 0, sizeof(scsicmd10));
	
	scsicmd10[0] = 0xD8;
	scsicmd10[1] = 0x00;
	scsicmd10[2] = start;
	scsicmd10[9] = 0x80; // 0x80, 0x40 LBA, 0x00 MSB, Other : Illegal
	eris_low_scsi_command(scsicmd10,10);

	/* Same here. Without this, it will freeze the whole application. */
	r10 = WAIT_CD; 
    while (r10 != 0) {
        r10--;
		__asm__ (
        "nop\n"
        "nop\n"
        "nop\n"
        "nop"
        :
        :
        :
		);
    }
	eris_low_scsi_status();
}

static void cd_end_track(u8 end, u8 loop)
{	
	/*
	 * 
	 * To play a CD-DA track, you need to use both 0xD8 and 0xD9.
	 * 0xD8 is for the starting track and 0xD9 is for the ending track as well and controlling whenever
	 * or not the track should loop (after it's done playing).
	*/
	int r10;
	u8 scsicmd10[10];
	
	memset(scsicmd10, 0, sizeof(scsicmd10));
	scsicmd10[0] = 0xD9;
	scsicmd10[1] = loop; // 0 : Silent, 4: Loop, Other: Normal
	scsicmd10[2] = end;
	scsicmd10[9] = 0x80; // 0x80, 0x40 LBA, 0x00 MSB, Other : Illegal

	eris_low_scsi_command(scsicmd10,10);
	
	/* Same here. Without this, it will freeze the whole application. */
	r10 = WAIT_CD; 
    while (r10 != 0) {
        r10--;
		__asm__ (
        "nop\n"
        "nop\n"
        "nop\n"
        "nop"
        :
        :
        :
		);
    }
	eris_low_scsi_status();

}

/*
void bufDisplay32()
{
	eris_king_set_kram_write((SCREEN_WIDTH_IN_BYTES/2) * ((236-ANIM_HEIGHT)/2), 1);	//4 to 235

	king_kram_write_buffer_bytes(framebuffer, ANIM_SIZE/2);

	eris_king_set_kram_write(0, 1);
}

static int getFps()
{
	static int fps = 0;
	static int prev_sec = 0;
	static int prev_nframe = 0;

	const int curr_sec = zda_timer_count / 1000;
	if (curr_sec != prev_sec) {
		fps = nframe - prev_nframe;
		prev_sec = curr_sec;
		prev_nframe = nframe;
	}
	return fps;
}*/
/*
int getTicks()
{
	return zda_timer_count;
}*/

/*
char* itoa(int val)
{
	static char buf[32] = {0};
	int base = 10;
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = "0123456789abcdef"[val % base];
	return &buf[i+1];
}*/

int main()
{
	game_status = 0;
	
	Reset_ADPCM();
	
	Initialize_ADPCM(ADPCM_RATE_32000);
	
	eris_sup_init(0, 1);
	eris_king_init();
	eris_tetsu_init();
	eris_pad_init(0);

	//eris_low_cdda_set_volume(63,63);

	
	Set_Video(KING_BGMODE_4_PAL);

	eris_king_set_kram_write(0, 1);
	print_text("SOUND TEST\nPRESS I for LEFT\nPRESS II for RIGHT\nPRESS RUN FOR MONO STEREO\n", 16);
	print_text("PRESS III for LEFT/16000hz\nPRESS IV for RIGHT/16000hz\nSELECT MONO STEREO 16khz\n", 16*5);
	
	eris_king_set_kram_write(0, 1);

	
	for(;;) 
	{
		pad_time++;
		curr_sec = GetSeconds();
		//padtype = eris_pad_type(0);
		paddata = eris_pad_read(0);
		
		if(paddata & (1 << 0))
		{
			eris_king_set_kram_write(ADPCM_OFFSET, 1);	
			king_kram_write_buffer(LEFT, sizeof(LEFT));
			eris_low_adpcm_set_volume(1, 63, 0);
			Play_ADPCM(1, ADPCM_OFFSET, sizeof(LEFT), 0, ADPCM_RATE_32000);
		}
		else if(paddata & (1 << 1))
		{
			eris_king_set_kram_write(ADPCM_OFFSET, 1);	
			king_kram_write_buffer(RIGHT, sizeof(RIGHT));
			eris_low_adpcm_set_volume(1, 0, 63);
			Play_ADPCM(1, ADPCM_OFFSET, sizeof(RIGHT), 0, ADPCM_RATE_32000);
		}
		
		else if(paddata & (1 << 7))
		{
			eris_king_set_kram_write(ADPCM_OFFSET, 1);	
			king_kram_write_buffer(SOUNDT, sizeof(SOUNDT));
			eris_low_adpcm_set_volume(1, 63, 63);
			Play_ADPCM(1, ADPCM_OFFSET, sizeof(SOUNDT), 0, ADPCM_RATE_32000);
		}
		
		else if(paddata & (1 << 2))
		{
			eris_king_set_kram_write(ADPCM_OFFSET, 1);	
			king_kram_write_buffer(LEFT16, sizeof(LEFT16));
			eris_low_adpcm_set_volume(1, 63, 0);
			Play_ADPCM(1, ADPCM_OFFSET, sizeof(LEFT16), 0, ADPCM_RATE_16000);
		}
		
		else if(paddata & (1 << 3))
		{
			eris_king_set_kram_write(ADPCM_OFFSET, 1);	
			king_kram_write_buffer(RIGHT16, sizeof(RIGHT16));
			eris_low_adpcm_set_volume(1, 0, 63);
			Play_ADPCM(1, ADPCM_OFFSET, sizeof(RIGHT16), 0, ADPCM_RATE_16000);
		}
		
		else if(paddata & (1 << 6))
		{
			eris_low_adpcm_set_volume(1, 63, 63);
			eris_king_set_kram_write(ADPCM_OFFSET, 1);	
			king_kram_write_buffer(SOUNDT16, sizeof(SOUNDT16));
			Play_ADPCM(1, ADPCM_OFFSET, sizeof(SOUNDT16), 0, ADPCM_RATE_16000);
		}
		
		++nframe;
	}

	return 0;
}
