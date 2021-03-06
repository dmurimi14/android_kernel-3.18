#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/slab.h>
//#include <linux/xlog.h>


/*===FEATURE SWITH===*/
 // #define FPTPDAFSUPPORT   //for pdaf switch
 // #define FANPENGTAO   //for debug log
/*===FEATURE SWITH===*/

/****************************Modify Following Strings for Debug****************************/
#define PFX "S5K3L8PDAF"
#define LOG_1 LOG_INF("S5K3L8,MIPI 4LANE\n")
//#define LOG_DBG(format, args...) xlog_printk(ANDROID_LOG_DEBUG ,PFX, "[%S] " format, __FUNCTION__, ##args)
#define LOG_INF(format, args...)    pr_debug(PFX "[%s] " format, __FUNCTION__, ##args)
#define LOGE(format, args...) xlog_printk(ANDROID_LOG_ERROR,PFX,"[%s]"format, __FUNCTION__, ##args)
/****************************   Modify end    *******************************************/

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern void kdSetI2CSpeed(u16 i2cSpeed);
//extern int iBurstWriteReg_multi(u8 *pData, u32 bytes, u16 i2cId, u16 transfer_length);
extern int iMultiReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId, u8 number);


#define USHORT             unsigned short
#define BYTE               unsigned char
#define Sleep(ms)          mdelay(ms)

/**************  CONFIG BY SENSOR >>> ************/
#define EEPROM_WRITE_ID   0xa0
#define I2C_SPEED        100  
#define MAX_OFFSET		    0xFFFF
#define DATA_SIZE         1404
#define START_ADDR        0X0763
BYTE S5K3L8B_eeprom_data[DATA_SIZE]= {0};

/**************  CONFIG BY SENSOR <<< ************/

static kal_uint16 read_cmos_sensor_byte(kal_uint16 addr)
{
    kal_uint16 get_byte=0;
    char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };

    kdSetI2CSpeed(I2C_SPEED); // Add this func to set i2c speed by each sensor
    iReadRegI2C(pu_send_cmd , 2, (u8*)&get_byte,1,EEPROM_WRITE_ID);
    return get_byte;
}
#if 0
static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

    kdSetI2CSpeed(I2C_SPEED); // Add this func to set i2c speed by each sensor
    iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, EEPROM_WRITE_ID);
    return get_byte;
}

static void write_cmos_sensor_byte(kal_uint32 addr, kal_uint32 para)
{
    char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

    kdSetI2CSpeed(I2C_SPEED); // Add this func to set i2c speed by each sensor
    iWriteRegI2C(pu_send_cmd, 3, EEPROM_WRITE_ID);
}

static void write_cmos_sensor(kal_uint16 addr, kal_uint16 para)
{
    char pusendcmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};

    kdSetI2CSpeed(I2C_SPEED); // Add this func to set i2c speed by each sensor
    iWriteRegI2C(pusendcmd , 4, EEPROM_WRITE_ID);
}
#endif
static bool _read_eeprom(kal_uint16 addr, kal_uint32 size ){
  //continue read reg by byte:
  int i=0;
  for(; i<size; i++){
    S5K3L8B_eeprom_data[i] = read_cmos_sensor_byte(addr+i);
    LOG_INF("add = 0x%x,\tvalue = 0x%x",i, S5K3L8B_eeprom_data[i]);
  }
  return true;
}

bool S5K3L8B_read_eeprom( kal_uint16 addr, BYTE* data, kal_uint32 size){
	addr = START_ADDR;
	size = DATA_SIZE;
	
	LOG_INF("Read EEPROM, addr = 0x%x, size = 0d%d\n", addr, size);

	if(!_read_eeprom(addr, size)){
	LOG_INF("error:read_eeprom fail!\n");
	return false;
	}

	memcpy(data, S5K3L8B_eeprom_data, size);
	return true;
}


#ifdef CONFIG_FN_FF_FUNCTION

#define FDAF_OTP_LEN 22
#define FF_OTP_ADDR_START 0x0CE9


/** _get_otp_checksum:

  Calculates a checksum for a buffer:= (SUM(buf) % 256).

  @param buf
      A buffer pointer.

  @param len
      The length of the buffer.
 */
static kal_uint8 _get_otp_checksum(const kal_uint8 *buf, kal_int32 len)
{
    kal_int32 sum = 0;

    while (len > 0) {
        sum += buf[0];
        buf++;
        len--;
    }

    return (kal_uint8)(sum % 255);  //return (kal_uint8)(sum % 256);
}

void temperatureRead(kal_int32* params)
{
#if 0
	kal_int32 val;

    // Reading is triggered by a 0 to 1 transition.
    write_cmos_sensor(REG_TEMP_CTL, 0x01);

    val = read_cmos_sensor(REG_TEMP_OUT);
    LOG_INF("temperatureRead {sendCommand}  val = (0x%x)\n", val);

    if( params )
    {
    	if(val < 192)
    	{
    		*params = val - 64;
    	} else
    	{
    		*params = 192 - val;
    	}
        
    }
#else
    *params = 50; //debug only
#endif
}

void ReadOtpAF(kal_uint8* buff)
{
	kal_int32 i = 0;
	kal_uint8  checkSum = 0;
	
	if(!buff)
	{
	    LOG_INF("FN_FF ReadOtpAF failed: buff is null\n");
	    return ;
    }

    LOG_INF("FN_FF ReadOtpAF begin\n");

	//
    // Reading Group data.

	// Read OTP into a buffer.
	//S5K3L8B_read_eeprom(FF_OTP_ADDR_START, buff, FDAF_OTP_LEN);
	for(i = 0; i < FDAF_OTP_LEN; i++)
	{
		buff[i] = read_cmos_sensor_byte(FF_OTP_ADDR_START + i);
		LOG_INF("FN_FF ReadOtpAF --- buff[%d] = %d\n", i, buff[i]); 
	}

    checkSum = _get_otp_checksum(buff, FDAF_OTP_LEN-1);
    if( buff[FDAF_OTP_LEN-1] != checkSum )
	{
	    LOG_INF("FN_FF ReadOtpAF  _get_otp_checksum failed! (buff[FDAF_OTP_LEN-1] = %d) != (checksum = %d)\n", buff[FDAF_OTP_LEN-1], checkSum); 
	}

    LOG_INF("FN_FF ReadOtpAF end\n");

    return;
}
#endif



