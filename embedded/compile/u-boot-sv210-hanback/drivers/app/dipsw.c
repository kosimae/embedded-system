#include <regs.h>
#include "./fpga_io.h"

int dipsw_test(void)
{
#if CONFIG_EMPOS3SV210
  int i,j;
  char ch;
  char *SW_name1;
  char *SW_name2;
  unsigned char *dip_ctl, *dip_data;
  unsigned char key;
  unsigned char old_key=0;
  unsigned char key1;
  unsigned char old_key1=0;

  SW_name1="S27";
  SW_name2="S26";

  dip_ctl = (unsigned char *)ADDR_OF_DIP_CTL;
  dip_data = (unsigned char *)ADDR_OF_DIP_DATA;

  while(1) {
    *dip_ctl = 0x00;
    key=*dip_data;
    if(key!=old_key) printf("dip SW(%s) value is %x\n",SW_name1,key);
    old_key=key;

    for(i=0;i<30;i++)
      for(j=0;j<65536;j++);

    *dip_ctl = 0x01;
    key1=*dip_data;

    if(key1!=old_key1)printf("dip SW(%s) value is %x\n",SW_name2,key1);
    old_key1=key1;

    for(i=0;i<30;i++)
      for(j=0;j<65536;j++);

    if(key == 0xff && key1 == 0xff) break;
  }
#else
  int i,j;
  char ch;
  char *SW_name1;
  char *SW_name2;
  unsigned short *dip_data;
  unsigned char key;
  unsigned char old_key=0;
  unsigned char key1;
  unsigned char old_key1=0;

  SW_name1="S2";
  SW_name2="S3";

  dip_data = (unsigned short *)ADDR_OF_DIP_DATA;

  while(1) {
    key=*dip_data >> 8;

    if(key!=old_key) printf("dip SW(%s) value is %x\n",SW_name1,key);
    old_key=key;

    for(i=0;i<30;i++)
      for(j=0;j<65536;j++);

    key1=*dip_data;

    if(key1!=old_key1)printf("dip SW(%s) value is %x\n",SW_name2,key1);
    old_key1=key1;

    for(i=0;i<30;i++)
      for(j=0;j<65536;j++);

    if(key == 0xff && key1 == 0xff) break;
  }
#endif
  return 0;
}

