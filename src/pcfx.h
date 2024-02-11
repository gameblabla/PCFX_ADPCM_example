#ifndef PCFX_H
#define PCFX_H

extern void Set_Video(uint32_t bpp);

extern void Initialize_ADPCM(uint32_t freq);

extern void Play_ADPCM(uint32_t channel, uint32_t start_adress, uint32_t sizet, unsigned char loop, uint32_t freq );

extern void chartou32(char* str, u32* o);

extern void printstr(u32* str, int x, int y, int tall);

extern void Clear_BG0(int bpp);

extern void Upload_Palette(unsigned short pal[], int sizep);

extern void LoadADPCMCD(u32 lba, u32 addr, uint32_t size_sample);

extern void Load_PSGSample(u32 lba, int numb, uint32_t size_sample);

extern void Play_PSGSample(int ch, int sample_numb, int loop);

extern void initTimer(int psg, int period);

extern __attribute__ ((interrupt)) void my_timer_irq (void);

extern __attribute__ ((interrupt)) void samplepsg_timer_irq (void);

extern void Init_Sprite(int chip);

extern void Update_Sprite(int chip, unsigned short array[], int size_sprite, int offset_spr);

extern void Reset_ADPCM();

extern volatile int __attribute__ ((zda)) frame_text ;

extern void Move_Sprite(int chip, int x, int y);

#define VDC_CHIP_0 0
#define VDC_CHIP_1 1

#define SAMPLES_PSG_NUMBER 1

extern int currentvid;

extern void cd_pausectrl(u8 resume);
extern void initTimer();
extern void Timer_Sample();
extern int GetSeconds();
extern void Reset_ZDA();

#endif
