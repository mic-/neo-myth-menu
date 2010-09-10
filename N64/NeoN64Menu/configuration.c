#include "configuration.h"
#include <ff.h>
#include <malloc.h>
extern void c2wstrcpy(void *dst, void *src);


int32_t config_test_sdc_speed()
{
	int32_t cnt,r;
	uint32_t hash;
	WCHAR wname[32];
	FIL lSDFile;
	UINT ts;

	c2wstrcpy(wname,".testsdspeed");

	if(f_open(&lSDFile, wname, FA_CREATE_ALWAYS | FA_WRITE | FA_READ) == FR_OK)
	{
		cnt = hash = ts = 0;
		r = 1;

		//write a few longs and compare them
		do
		{
			f_write(&lSDFile,(void*)&hash,4,&ts);
			f_lseek(&lSDFile,(lSDFile.fptr >= 4) ? (lSDFile.fptr-4) : 0);
			f_read(&lSDFile,(void*)&r,4,&ts);

			r = (r == hash);
			hash += (cnt + ( (lSDFile.fptr+1) >> 1 ));//new "hash"
		}while(( (++cnt) < ( 2048 >> 2 )) && (r) && (ts == 4));

		r = (r) && (lSDFile.fsize == (2048 >> 2)) && (ts == 4);

		f_close(&lSDFile);
		f_unlink(wname);

		return (r);
	}

	return 0;
}

void config_create_initial_settings(const char* filename)
{
	config_shutdown();
	config_init();

	config_push("firstRun","1");
	config_push("forcePAL","0");
	config_push("lastGameLoaded","NULL");
	config_push("lastGameLoadedSaveType","NULL");//EEP16K,EEP4K,SRAM32K,FLASHRAM
	config_push("useBoxarts","0");
	config_push("showLogo","0");
	config_push("menuBootcode","6102");
	config_push("fastSDMode",(config_test_sdc_speed() != 0) ? "1" : "0");

	config_save(filename);
}

void config_save(const char* filename)
{
	return;//disabled currently (untested)
	uint32_t pred;
	unsigned char* mem;
	FIL file;
	WCHAR out[512];

	pred = config_predictOutputBufferSize();

	if(!pred)
		return;

	mem = (unsigned char*)malloc(pred);

	if(!mem)
		return;

	//mute,always
	config_push("firstRun","0");

	if(!config_saveToBuffer(mem))
	{
		c2wstrcpy((void*)out,(void*)filename);

		if(f_open(&file,out,FA_CREATE_ALWAYS|FA_WRITE) == FR_OK)
		{
			f_write(&file,(void*)mem,(UINT)pred,(UINT*)&out[0]);
			f_close(&file);
		}
	}

	free(mem);
}

void config_load(const char* filename)
{
}

