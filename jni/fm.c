/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fm.h"
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#define LOG_TAG "FM_HW"

#define NUM_SEEK_PRESETS 20

#define WAIT_OVER 0
#define SEEK_WAITING 1
#define NO_WAIT 2
#define TUNE_WAITING 4
#define RDS_WAITING 5
#define SEEK_CANCEL 6

/*dev settings*/
/*band*/
#define BAND_87500_108000_kHz 1
#define BAND_76000_108000_kHz 2
#define BAND_76000_90000_kHz 3

/*channel spacing*/
#define CHAN_SPACING_200_kHz 20 /*US*/
#define CHAN_SPACING_100_kHz 10 /*Europe,Japan*/
#define CHAN_SPACING_50_kHz 5

/*DE-emphasis Time Constant*/
#define DE_TIME_CONSTANT_50 1 /*Europe,Japan,Australia*/
#define DE_TIME_CONSTANT_75 0 /*US*/


/*****************IOCTLS******************/
/*magic no*/
#define Si4709_IOC_MAGIC 0xFA
/*max seq no*/
#define Si4709_IOC_NR_MAX 40

/*commands*/
#define Si4709_IOC_POWERUP _IO(Si4709_IOC_MAGIC, 0)
#define Si4709_IOC_POWERDOWN _IO(Si4709_IOC_MAGIC, 1)
#define Si4709_IOC_BAND_SET _IOW(Si4709_IOC_MAGIC, 2, int)
#define Si4709_IOC_CHAN_SPACING_SET _IOW(Si4709_IOC_MAGIC, 3, int)
#define Si4709_IOC_CHAN_SELECT _IOW(Si4709_IOC_MAGIC, 4, uint32_t)
#define Si4709_IOC_CHAN_GET _IOR(Si4709_IOC_MAGIC, 5, uint32_t)
#define Si4709_IOC_SEEK_UP _IOR(Si4709_IOC_MAGIC, 6, uint32_t)
#define Si4709_IOC_SEEK_DOWN _IOR(Si4709_IOC_MAGIC, 7, uint32_t)
/*VNVS:28OCT'09---- Si4709_IOC_SEEK_AUTO is disabled as of now*/
//#define Si4709_IOC_SEEK_AUTO _IOR(Si4709_IOC_MAGIC, 8, u32)
#define Si4709_IOC_RSSI_SEEK_TH_SET _IOW(Si4709_IOC_MAGIC, 9, u8)
#define Si4709_IOC_SEEK_SNR_SET _IOW(Si4709_IOC_MAGIC, 10, u8)
#define Si4709_IOC_SEEK_CNT_SET _IOW(Si4709_IOC_MAGIC, 11, u8)
#define Si4709_IOC_CUR_RSSI_GET _IOR(Si4709_IOC_MAGIC, 12, rssi_snr_t)
#define Si4709_IOC_VOLEXT_ENB _IO(Si4709_IOC_MAGIC, 13)
#define Si4709_IOC_VOLEXT_DISB _IO(Si4709_IOC_MAGIC, 14)
#define Si4709_IOC_VOLUME_SET _IOW(Si4709_IOC_MAGIC, 15, uint8_t)
#define Si4709_IOC_VOLUME_GET _IOR(Si4709_IOC_MAGIC, 16, uint8_t)
#define Si4709_IOC_MUTE_ON _IO(Si4709_IOC_MAGIC, 17)
#define Si4709_IOC_MUTE_OFF _IO(Si4709_IOC_MAGIC, 18)
#define Si4709_IOC_MONO_SET _IO(Si4709_IOC_MAGIC, 19)
#define Si4709_IOC_STEREO_SET _IO(Si4709_IOC_MAGIC, 20)
#define Si4709_IOC_RSTATE_GET _IOR(Si4709_IOC_MAGIC, 21, dev_state_t)
#define Si4709_IOC_RDS_DATA_GET _IOR(Si4709_IOC_MAGIC, 22, radio_data_t)
#define Si4709_IOC_RDS_ENABLE _IO(Si4709_IOC_MAGIC, 23)
#define Si4709_IOC_RDS_DISABLE _IO(Si4709_IOC_MAGIC, 24)
#define Si4709_IOC_RDS_TIMEOUT_SET _IOW(Si4709_IOC_MAGIC, 25, u32)
#define Si4709_IOC_SEEK_CANCEL _IO(Si4709_IOC_MAGIC, 26)/*VNVS:START 13-OCT'09---- Added IOCTLs for reading the device-id,chip-id,power configuration, system configuration2 registers*/
#define Si4709_IOC_DEVICE_ID_GET _IOR(Si4709_IOC_MAGIC, 27,device_id)
#define Si4709_IOC_CHIP_ID_GET _IOR(Si4709_IOC_MAGIC, 28,chip_id)
#define Si4709_IOC_SYS_CONFIG2_GET _IOR(Si4709_IOC_MAGIC,29,sys_config2)
#define Si4709_IOC_POWER_CONFIG_GET _IOR(Si4709_IOC_MAGIC,30,power_config)
#define Si4709_IOC_AFCRL_GET _IOR(Si4709_IOC_MAGIC,31,u8) /*For reading AFCRL bit, to check for a valid channel*/
#define Si4709_IOC_DE_SET _IOW(Si4709_IOC_MAGIC,32,uint8_t) /*Setting DE-emphasis Time Constant. For DE=0,TC=50us(Europe,Japan,Australia) and DE=1,TC=75us(USA)*/
#define Si4709_IOC_SYS_CONFIG3_GET _IOR(Si4709_IOC_MAGIC, 33, sys_config3)
#define Si4709_IOC_STATUS_RSSI_GET _IOR(Si4709_IOC_MAGIC, 34, status_rssi)
#define Si4709_IOC_SYS_CONFIG2_SET _IOW(Si4709_IOC_MAGIC, 35, sys_config2)
#define Si4709_IOC_SYS_CONFIG3_SET _IOW(Si4709_IOC_MAGIC, 36, sys_config3)
#define Si4709_IOC_DSMUTE_ON _IO(Si4709_IOC_MAGIC, 37)
#define Si4709_IOC_DSMUTE_OFF _IO(Si4709_IOC_MAGIC, 38)
#define Si4709_IOC_RESET_RDS_DATA _IO(Si4709_IOC_MAGIC, 39)

/*****************************************/

#define THE_DEVICE "/dev/fmradio"

typedef enum {GROUP_0A=0,GROUP_0B,GROUP_1A,GROUP_1B,GROUP_2A,GROUP_2B,
                   GROUP_3A,GROUP_3B,GROUP_4A,GROUP_4B,GROUP_5A,GROUP_5B,
                   GROUP_6A,GROUP_6B,GROUP_7A,GROUP_7B,GROUP_8A,GROUP_8B,
                   GROUP_9A,GROUP_9B,GROUP_10A,GROUP_10B,GROUP_11A,GROUP_11B,
                   GROUP_12A,GROUP_12B,GROUP_13A,GROUP_13B,GROUP_14A,GROUP_14B,
                   GROUP_15A,GROUP_15B,GROUP_UNKNOWN} RDSGroupType;

typedef enum {TMC_GROUP=0, TMC_SINGLE, TMC_SYSTEM, TMC_TUNING} TMCtype;

char group_name [][4] = {"0A ", "0B ", "1A ", "1B ", "2A ", "2B ", "3A ", "3B ",
						 "4A ", "4B ", "5A ", "5B ", "6A ", "6B ", "7A ", "7B ",
						 "8A ", "8B ", "9A ", "9B ", "10A", "10B", "11A", "11B",
						 "12A", "12B", "13A", "13B", "14A", "14B", "15A", "15B",
						 "???"};

int cur_service;
int next_service;
char service_name[9];
char tmc_service_name[9];

static int radioEnabled = 0;
static int lastFreq = 0;

static int send_signal(int signal)
{
  int rt, fd;
  fd = open(THE_DEVICE, O_RDWR);
  if (fd < 0)
    return -1;

  rt = ioctl(fd, signal);

  close(fd);
  return rt;
}

static int send_signal_with_value(int signal, void* value)
{
  int rt, fd;
  fd = open(THE_DEVICE, O_RDWR);
  if (fd < 0)
    return -1;

  rt = ioctl(fd, signal, value);

  close(fd);
  return rt;
}

int fm_exists()
{
  int fd;
  fd = open(THE_DEVICE, O_RDWR);
  if (fd < 0)
    return 0;

  close(fd);
  return 1;
}

int rds_on()
{
  int ret = 0;
  ret = send_signal(Si4709_IOC_RDS_ENABLE);
  return ret;
}

int rds_off()
{
  int ret = 0;
  ret = send_signal(Si4709_IOC_RDS_DISABLE);
  return ret;
}

int rds_get(radio_data_t* rdat)
{
  int ret = 0;

  ret = send_signal_with_value(Si4709_IOC_RDS_DATA_GET, (void*)rdat);

  return ret;
}

uint8_t lower_byte(uint16_t word)
{
	return (word & 0xFF);
}

uint8_t upper_byte(uint16_t word)
{
	return ((word >> 8) & 0xFF);
}

void do_tmc(radio_data_t* rdat)
{

	char rds_usable[4]; //some redundancy here...
	rds_usable[0] = (rdat->blera < 2); //implied by getting here
	rds_usable[1] = (rdat->blerb < 2); //same
	rds_usable[2] = (rdat->blerc < 2);
	rds_usable[3] = (rdat->blerd < 2);

	TMCtype type = (lower_byte(rdat->rdsb) & 0x18) >> 3;
	int duration = lower_byte(rdat->rdsb) & 0x07;
	printf("Type: %X, duration: %d\n", type, duration);
	switch (type) {
		case TMC_GROUP:
			if (duration == 0)
				printf("Encrypted Service\n");
			else
				printf("Unencrypted Service\n");
			break;
		case TMC_SYSTEM:
			switch (duration) {
				case 4:
					if (rds_usable[2]) {
						tmc_service_name[0] = upper_byte(rdat->rdsc);
						tmc_service_name[1] = lower_byte(rdat->rdsc);
					}
					if (rds_usable[3]) {
						tmc_service_name[2] = upper_byte(rdat->rdsd);
						tmc_service_name[3] = lower_byte(rdat->rdsd);
					}
					break;
				case 5:
					if (rds_usable[2]) {
						tmc_service_name[4] = upper_byte(rdat->rdsc);
						tmc_service_name[5] = lower_byte(rdat->rdsc);
					}
					if (rds_usable[3]) {
						tmc_service_name[6] = upper_byte(rdat->rdsd);
						tmc_service_name[7] = lower_byte(rdat->rdsd);
					}
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	printf("provider = %s\n", tmc_service_name);
}

int rds_print()
{
  //printf("RDS Get...\n");
  radio_data_t rdat;
  service_name[8] = '\0'; //this can go elsewhere
  int ret = rds_get(&rdat);
  char C_bits;
  if (ret < 0) {
    //printf("RDS get failed (%d).\n", ret);
  } else {
/*
    uint16_t rdsa;
    uint16_t rdsb;
    uint16_t rdsc;
    uint16_t rdsd;
    uint8_t curr_rssi;
    uint32_t curr_channel;
    uint8_t blera;
    uint8_t blerb;
    uint8_t blerc;
    uint8_t blerd;
*/
	//printf("rdsa: %X\n", rdat.rdsa);
	//printf("rdsb: %X\n", rdat.rdsb);
	//printf("rdsc: %X\n", rdat.rdsc);
	//printf("rdsd: %X\n", rdat.rdsd);
	//printf("curr_rssi: %X\n", rdat.curr_rssi);
	//printf("curr_channel: %X\n", rdat.curr_channel);
	//printf("blera: %X\n", rdat.blera);
	//printf("blerb: %X\n", rdat.blerb);
	//printf("blerc: %X\n", rdat.blerc);
	//printf("blerd: %X\n", rdat.blerd);

	char rds_usable[4];
	//0 = no errors, 1 = some [see data sheet], 2 = more, 3 = uncorrectable
	//using < 3 still results in corrupted text, so only use 1.
	rds_usable[0] = (rdat.blera < 1);
	rds_usable[1] = (rdat.blerb < 1);
	rds_usable[2] = (rdat.blerc < 1);
	rds_usable[3] = (rdat.blerd < 1);

	if (!rds_usable[0] || !rds_usable[1])
		return -1;

	RDSGroupType group_type = upper_byte(rdat.rdsb) >> 3;
	
    printf("PID: %X\n", rdat.rdsa);
	if (next_service == rdat.rdsa) {
		if (cur_service != next_service)
			service_name[0] = '\0';
		cur_service = next_service;
	} else {
		next_service = rdat.rdsa;
	}
    printf("Group Type: %s\n", group_name[group_type]);
	printf("Service Name: %s\n", service_name);
	switch (group_type) {
		case GROUP_0A:
		case GROUP_0B:
			if (!rds_usable[2])
				return -1;
			C_bits = lower_byte(rdat.rdsb) & 0x3;

			if (rds_usable[3]) {
				service_name[2*C_bits] = upper_byte(rdat.rdsd);
				service_name[2*C_bits+1] = lower_byte(rdat.rdsd);
			}
			break;
		case GROUP_3A:
			break;
		case GROUP_8A:
			do_tmc(&rdat);
			printf("tmc group\n");
			break;
		default:
			break;
	}
  }
  return ret;
}

int fm_on()
{
  //LOGV("%s", __func__);

  if (1 == radioEnabled)
  {
    return 0;
  }

  int ret = 0;
  ret = send_signal(Si4709_IOC_POWERUP);

  if (ret < 0)
  {
    //LOGE("%s: IOCTL Si4709_IOC_POWERUP failed %d", __func__, ret);
    return -1;
  }

  radioEnabled = 1;

  if (lastFreq != 0)
  {
    fm_set_freq(lastFreq);
  }

  return 0;
}

int fm_off()
{
  //LOGV("%s", __func__);
  if (0 == radioEnabled)
  {
    return 0;
  }

  int ret = 0;
  ret = send_signal(Si4709_IOC_POWERDOWN);

  if (ret < 0)
  {
    //LOGE("%s: IOCTL Si4709_IOC_POWERDOWN failed %d", __func__, ret);
    return -1;
  }

  radioEnabled = 0;

  return 0;
}

int fm_set_freq(int freq)
{
  //LOGV("%s", __func__);

  // The minimal spacing is 50KHZ
  freq /= 10;

  int ret = 0;
  ret = send_signal_with_value(Si4709_IOC_CHAN_SELECT, &freq);

  if (ret < 0)
  {
    //LOGE("%s: IOCTL Si4709_IOC_CHAN_SELECT failed %d", __func__, ret);
    return -1;
  }

  lastFreq = freq;
  return 0;
}

int fm_get_freq()
{
  //LOGV("%s", __func__);
  int ret = 0;
  uint32_t freq = 0;

  ret = send_signal_with_value(Si4709_IOC_CHAN_GET, &freq);
  if (ret < 0)
  {
    //LOGE("%s: IOCTL Si4709_IOC_CHAN_GET failed %d", __func__, ret);
    return ret;
  }

  // set unit as KHZ
  freq *= 10;
  return freq;
}

int fm_seek_up()
{
  int ret = 0;
  uint32_t freq = 0;

  ret = send_signal_with_value(Si4709_IOC_SEEK_UP, &freq);
  if (ret < 0)
  {
    return ret;
  }

  return freq * 10;
}

int fm_seek_down()
{
  int ret = 0;
  uint32_t freq = 0;

  ret = send_signal_with_value(Si4709_IOC_SEEK_DOWN, &freq);
  if (ret < 0)
  {
    return ret;
  }

  return freq * 10;
}

int fm_cancel_seek()
{
  int ret = 0;
  ret = send_signal(Si4709_IOC_SEEK_CANCEL);

  if (ret < 0)
  {
    return -1;
  }

  return 0;
}

int fm_set_band(int band)
{
  int ret = 0;

  ret = send_signal_with_value(Si4709_IOC_BAND_SET, &band);
  if (ret < 0)
  {
    return -1;
  }

  return 0;
}

int fm_set_vol(int vol)
{
  int ret = 0;

  ret = send_signal_with_value(Si4709_IOC_VOLUME_SET, &vol);
  if (ret < 0)
  {
    return ret;
  }

  return 0;
}

int fm_get_vol()
{
  int ret;
  uint8_t vol = 0;

  ret = send_signal_with_value(Si4709_IOC_VOLUME_GET, &vol);
  if (ret < 0)
  {
    return ret;
  }

  return vol;
}

int fm_mute_on()
{
  return send_signal(Si4709_IOC_MUTE_ON);
}

int fm_mute_off()
{
  return send_signal(Si4709_IOC_MUTE_OFF);
}

int fm_get_power_config()
{
  int rt;

  int pc = 0;
  rt = send_signal_with_value(Si4709_IOC_POWER_CONFIG_GET, &pc);

  if (rt < 0)
  {
    return rt;
  }

  return pc;
}

// -1: failed
// 0: power off
// 1: power on
int fm_get_power_state()
{
  int pc = fm_get_power_config();
  if (pc < 0)
  {
    return pc;
  }

  if((pc & 256) > 0)
  {
    return 1;
  }

  return 0;
}

int fm_set_chan_spacing(int spacing)
{
  int ret = 0;
  ret = send_signal_with_value(Si4709_IOC_CHAN_SPACING_SET, &spacing);
  if (ret < 0)
  {
    return -1;
  }

  return 0;
}

void printCurrentFreq()
{
  int freq = fm_get_freq();
  printf("Current Freq: %d\n", freq);
}

int main()
{
  printf("FM Power On: %d\n", fm_get_power_state());

  printf("Turn on fm:\n");
  int ret = fm_on();
  if (ret < 0)
  {
    printf("Fail to turn on.");
    return -1;
  }

  //printf("Set Band\n");
  //fm_set_band(BAND_76000_108000_kHz);

  printCurrentFreq();

  int spacing = CHAN_SPACING_50_kHz;
  printf("Set channel spacing: %d\n", spacing);
  fm_set_chan_spacing(spacing);
  printCurrentFreq();

  int freq = 101700;
  fm_set_freq(freq);
  printCurrentFreq();

  printf("Turning RDS on...\n");
  ret = rds_on();
  if (ret < 0) {
    printf("RDS failed to turn on.");
  }
/*
  printf("Seeking down ...\n");
  ret = fm_seek_down();
  if (freq < 0)
  {
    printf("Fail to seek down.\n");
  }
  printCurrentFreq();
*/
  while (1) {
	  do {
		ret = rds_print();
	  } while (ret >= 0);
	  usleep(50000);
  }

  /*
  printf("Turning RDS off...\n");
  ret = rds_off();
  if (ret < 0) {
    printf("RDS failed to turn off.");
  }
*/
  

  return 0;
}

