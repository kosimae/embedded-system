#include <regs.h>
#include "./fpga_io.h"

unsigned char key_scan(unsigned char * key_addr, unsigned char * key_addr_rd)
{
  unsigned char key_value;
  int i;

  *key_addr=0x01;
  for(i=0;i<10;i++);
  key_value=*key_addr_rd;
  key_value&=0x0f;

  if(key_value>0){
    if(key_value==0x01)key_value=1;
    if(key_value==0x02)key_value=2;
    if(key_value==0x04)key_value=3;
    if(key_value==0x08)key_value=4;
    goto scan_end;
  }

  *key_addr=0x02;
  for(i=0;i<10;i++);
  key_value=*key_addr_rd;
  key_value&=0x0f;

  if(key_value>0){
    if(key_value==0x01)key_value=5;
    if(key_value==0x02)key_value=6;
    if(key_value==0x04)key_value=7;
    if(key_value==0x08)key_value=8;
    goto scan_end;
  }

  *key_addr=0x04;
  for(i=0;i<10;i++);
  key_value=*key_addr_rd;
  key_value&=0x0f;

  if(key_value>0){
    if(key_value==0x01)key_value=9;
    if(key_value==0x02)key_value=10;
    if(key_value==0x04)key_value=11;
    if(key_value==0x08)key_value=12;
    goto scan_end;
  }

  *key_addr=0x08;
  for(i=0;i<10;i++);
  key_value=*key_addr_rd;
  key_value&=0x0f;
  if(key_value>0){
    if(key_value==0x01)key_value=13;
    if(key_value==0x02)key_value=14;
    if(key_value==0x04)key_value=15;
    if(key_value==0x08)key_value=16;
    goto scan_end;
  }

scan_end:
  return key_value;
}

int key_test(void)
{
  int i,j;
  char ch;
  unsigned char * key_addr;
  unsigned char * key_addr_rd;
  unsigned short * buz_addr;
  unsigned char key_value;
  unsigned char old=0;

  key_addr = (unsigned char *)ADDR_OF_KEY_LD;
  key_addr_rd = (unsigned char *)ADDR_OF_KEY_RD;
  buz_addr = (unsigned short *)ADDR_OF_BUZ;

  while(1){
    key_value=key_scan(key_addr,key_addr_rd);
    if((key_value!=old)&&(key_value>0)) {
      *buz_addr|=0x01;
      printf("key pad value is %d\n",key_value);
      for(i=0;i<100;i++)
        for(j=0;j<65536;j++);
      *buz_addr&=0x0;
    }
    old=key_value;
    for(i=0;i<5;i++)
      for(j=0;j<65536;j++);

    if(key_value == 16)
      break;
  }
  return 0;
}

