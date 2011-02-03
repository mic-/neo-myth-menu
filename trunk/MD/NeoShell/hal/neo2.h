extern int set_sr(int sr);
extern void init_hardware(void);
extern unsigned short int get_pad(int pad);
extern void clear_screen(void);
extern void put_str(const char *str, int fcolor);
extern void set_usb(void);
extern short int neo_check_card(void);
extern void neo_run_game(int fstart, int fsize, int bbank, int bsize, int run);
extern void neo_run_psram(int pstart, int psize, int bbank, int bsize, int run);
extern void neo_run_myth_psram(int psize, int bbank, int bsize, int run);
extern void neo_copy_game(unsigned char *dest, int fstart, int len);
extern void neo_copyto_psram(unsigned char *src, int pstart, int len);
extern void neo_copyto_myth_psram(unsigned char *src, int pstart, int len);
extern void neo_copyfrom_myth_psram(unsigned char *dst, int pstart, int len);
extern void neo_copyto_sram(unsigned char *src, int sstart, int len);
extern void neo_copyfrom_sram(unsigned char *dst, int sstart, int len);
extern void neo_get_rtc(unsigned char *rtc);
extern void neo2_enable_sd(void);
extern void neo2_disable_sd(void);
extern DSTATUS MMC_disk_initialize(void);

extern void ints_on();
extern void ints_off();
extern void PlayVGM(void);