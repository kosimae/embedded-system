#include "./fpga_io.h"

volatile unsigned short *lcd_addr;
void setcommand(unsigned short command);
void writebyte(char ch);
void initialize_textlcd();
void write_string(char *str,int length);
int function_set(int rows, int nfonts);
int display_control(int display_enable, int cursor_enable, int nblink);
int cusrsor_shit(int set_screen, int set_rightshit);
int entry_mode_set(int increase, int nshift);
int return_home();
int clear_display();
int set_ddram_address(int pos);

void setcommand(unsigned short command)
{
  command &= 0x00FF;

  *lcd_addr = command | 0x0000;
  udelay(5000);
  *lcd_addr = command | 0x0100;
  udelay(5000);
  *lcd_addr = command | 0x0000;
  udelay(5000);
}

void writebyte(char ch)
{
  unsigned short data;
  data = ch & 0x00FF;

  *lcd_addr = data & 0x400;
  //udelay(25000);
  udelay(10000);
  *lcd_addr = data|0x500;
  //udelay(25000);
  udelay(10000);
  *lcd_addr = data|0x400;
  //udelay(10000);
  udelay(5000);
}

int function_set(int rows, int nfonts){
  unsigned short command = 0x30;

  if(rows == 2) command |= 0x08;
  else if(rows == 1) command &= 0xf7;
  else return -1;

  command = nfonts ? (command | 0x04) : command;
  setcommand(command);
  return 1;
}

int display_control(int display_enable, int cursor_enable, int nblink){
  unsigned short command = 0x08;
  command = display_enable ? (command | 0x04) : command;
  command = cursor_enable ? (command | 0x02) : command;
  command = nblink ? (command | 0x01) : command;
  setcommand(command);
  return 1;
}

int cusrsor_shit(int set_screen, int set_rightshit){
  unsigned short command = 0x10;
  command = set_screen ? (command | 0x08) : command;
  command = set_rightshit ? (command | 0x04) : command;
  setcommand(command);
  return 1;
}

int entry_mode_set(int increase, int nshift){
  unsigned short command = 0x04;
  command = increase ? (command | 0x2) : command;
  command = nshift ? ( command | 0x1) : command;
  setcommand(command);
  return 1;
}

int return_home(){
  unsigned short command = 0x02;
  setcommand(command);
  return 1;
}

int clear_display(){
  unsigned short command = 0x01;
  setcommand(command);
  return 1;
}

int set_ddram_address(int pos){
  unsigned short command = 0x80;
  command += pos;
  setcommand(command);
  return 1;
}

void initialize_textlcd(){
  function_set(2,0); //Function Set:8bit,display 2lines,5x7 mod
  display_control(1,0,0); // Display on, Cursor off
  clear_display(); // Display clear
  entry_mode_set(1,0); // Entry Mode Set : shift right cursor
  return_home(); // go home
}

int BootMessage(void)
{
  int i;
  char *line1;
  char *line2;

  lcd_addr = (unsigned short *)ADDR_OF_LCD;
  line1 = "HBE-EMPOS3-SV210";
  line2 = "HANBACK Co.,Ltd. ";

  initialize_textlcd();
  for(i=0;i<16;i++) writebyte(line1[i]);
  set_ddram_address(0x40);
  for(i=0;i<16;i++) writebyte(line2[i]);

  return 0;
}

int lcd_test(void)
{
  int i;
  char *line1;
  char *line2;

  lcd_addr = (unsigned short *)ADDR_OF_LCD;
  line1 = "Welcome to       ";
  line2 = "Embedded World   ";

  initialize_textlcd();
  for(i=0;i<16;i++) writebyte(line1[i]);
  set_ddram_address(0x40);
  for(i=0;i<16;i++) writebyte(line2[i]);

  return 0;
}

