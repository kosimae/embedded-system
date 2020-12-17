//[*]--------------------------------------------------------------------------------------------------[*]
/*
 * S5PV210 _MDSCPE_SENSOR_H_ Header file 
 */
//[*]--------------------------------------------------------------------------------------------------[*]

#ifndef	_MDSCPE_SENSOR_H_
#define	_MDSCPE_SENSOR_H_

#define SENSOR_MAGIC          'M'
#define SENSOR_CALIB_START    _IO(SENSOR_MAGIC,0)
#define SENSOR_CALIB_WRITE    _IO(SENSOR_MAGIC,1)
#define SENSOR_CALIB_READ     _IOR(SENSOR_MAGIC,2,struct mdscpe_cal_data)
#define SENSOR_DATA_READ      _IOR(SENSOR_MAGIC,3,short)

struct mdscpe_cal_data
{
  short xoff;
  short yoff;
  short xsc;
  short ysc;
  short k;
};

//struct mdscpe_cal_data caldata;

void SSI1_Start(void);
char SSI1_Sendcode(char SSI_Adr, char SSI_command, char SSI_RW);
int  SSI1_rdata(void);
extern  void  mdscpe_port_init  (void);
extern  void  mdscpe_reset      (void);
extern  void  mdscpe_calib_init (void);
extern  unsigned int mdscpe_send_command(char code);
extern  void  write_data_Code(char code);
extern  void  read_data_Code(char code);

#ifdef  DEBUG   // raoudi-20101014
#define COMPASS_DEBUG
#endif

#endif		/* _MDSCPE_SENSOR_H_ */
