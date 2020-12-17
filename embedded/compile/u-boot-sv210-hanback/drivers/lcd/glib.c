#include "system.h"
#include "library.h"
#include "glib.h"
#include "glib_font.h"
//#include "util.h"

GLIB oGlib;


void GLIB_Init(u32 uFbAddr, u32 uHSize, u32 uVSize, IMG_FMT eBpp)
{
    GLIB_InitInstance(uFbAddr, uHSize, uVSize, eBpp);
}

void GLIB_ClearFrame(COLOR color) 
{
  if (oGlib.m_uPalFlag == 0)
    GLIB_ClearFrame2(oGlib.m_uColorValue[color]); 
  else
    GLIB_ClearPalFrame(color);
}

void GLIB_PutLine(int x1, int y1, int x2, int y2, COLOR color) 
{
  if (oGlib.m_uPalFlag == 0)
    GLIB_PutLine2(x1, y1, x2, y2, oGlib.m_uColorValue[color]);
  else
    GLIB_PutPalLine(x1, y1, x2, y2, color);
}

void GLIB_PutRectangle(int x1, int y1, int dx, int dy, COLOR color) 
{
  if (oGlib.m_uPalFlag == 0)
    GLIB_PutRectangle2(x1, y1, dx, dy, oGlib.m_uColorValue[color]); 
  else
    GLIB_PutPalRectangle(x1, y1, dx, dy, color); 
}

void GLIB_FillRectangle(int x1, int y1, int dx, int dy, COLOR color)
{
  if (oGlib.m_uPalFlag == 0)
    GLIB_FillRectangle2(x1, y1, dx, dy, oGlib.m_uColorValue[color]);
  else
    GLIB_FillPalRectangle(x1, y1, dx, dy, color);   
}

void GLIB_PutPixel2(u32 x, u32 y, u32 cvalue)
{
  if (oGlib.m_uBpp == 8)
  {
    *((u8 *)(oGlib.m_pFrameBuf)+oGlib.m_uHsz*(y)+(x)) = (u8)cvalue;
  }
  else if (oGlib.m_uBpp == 16)
  {
    *((u16 *)(oGlib.m_pFrameBuf)+oGlib.m_uHsz*(y)+(x)) = (u16)cvalue;

  }
  else if (oGlib.m_uBpp == 18 || oGlib.m_uBpp == 24)
  {
    *((u32 *)(oGlib.m_pFrameBuf)+oGlib.m_uHsz*(y)+(x)) = cvalue;
  }
}

void GLIB_PutPalData(int nPosX, int nPosY, u32 uData)
{
  if (oGlib.m_uPalDataBpp == 8)
  {
    *((u8 *)(oGlib.m_pFrameBuf)+oGlib.m_uHsz*(nPosY)+(nPosX)) = (u8)uData;    
  }
  else if(oGlib.m_uPalDataBpp != 8)
  {
    Assert(0);
  }
}

void GLIB_PutRectangle2(int x1, int y1, int dx, int dy, u32 cvalue)
{
  u32 x2 = x1+dx-1;
  u32 y2 = y1+dy-1;

  GLIB_PutLine2(x1,y1,x2,y1,cvalue);
  GLIB_PutLine2(x2,y1,x2,y2,cvalue);
  GLIB_PutLine2(x1,y2,x2,y2,cvalue);
  GLIB_PutLine2(x1,y1,x1,y2,cvalue);
}

void GLIB_PutPalRectangle(int nPosX1, int nPosY1, int nHSz, int nVSz, u32 uData)
{
  u32 uPosX2 = nPosX1+nHSz-1;
  u32 uPosY2 = nPosY1+nVSz-1;

  GLIB_PutPalLine(nPosX1, nPosY1, uPosX2, nPosY1, uData);
  GLIB_PutPalLine(uPosX2, nPosY1, uPosX2, uPosY2, uData);
  GLIB_PutPalLine(nPosX1, uPosY2, uPosX2, uPosY2, uData);
  GLIB_PutPalLine(nPosX1, nPosY1, nPosX1, uPosY2, uData);
}

void GLIB_FillRectangle2(int x1, int y1, int dx, int dy, u32 cvalue)
{
  int i;
  u32 x2 = x1+dx-1;
  u32 y2 = y1+dy-1;

  for(i=y1;i<=y2;i++)
    GLIB_PutLine2(x1,i,x2,i,cvalue);
}

void GLIB_FillPalRectangle(int nPosX1, int nPosY1, int nHSz, int nVSz, u32 uData)
{
  int i;
  u32 uPosX2 = nPosX1+nHSz-1;
  u32 uPosY2 = nPosY1+nVSz-1;

  for(i=nPosY1;i<=uPosY2;i++)
    GLIB_PutPalLine(nPosX1,i,uPosX2,i,uData);
}

void GLIB_PutLine2(int x1, int y1, int x2, int y2, u32 cvalue)
{

  int dx,dy,e;
  dx=x2-x1;
  dy=y2-y1;

  if(dx>=0)
  {
    if(dy >= 0) // dy>=0
    {
      if(dx>=dy) // 1/8 octant
      {
        e=dy-dx/2;
        while(x1<=x2)
        {
          GLIB_PutPixel2(x1,y1,cvalue);
          if(e>0){y1+=1;e-=dx;}
          x1+=1;
          e+=dy;
        }
      }
      else    // 2/8 octant
      {
        e=dx-dy/2;
        while(y1<=y2)
        {
          GLIB_PutPixel2(x1,y1,cvalue);
          if(e>0){x1+=1;e-=dy;}
          y1+=1;
          e+=dx;
        }
      }
    }
    else       // dy<0
    {
      dy=-dy;   // dy=abs(dy)

      if(dx>=dy) // 8/8 octant
      {
        e=dy-dx/2;
        while(x1<=x2)
        {
          GLIB_PutPixel2(x1,y1,cvalue);
          if(e>0){y1-=1;e-=dx;}
          x1+=1;
          e+=dy;
        }
      }
      else    // 7/8 octant
      {
        e=dx-dy/2;
        while(y1>=y2)
        {
          GLIB_PutPixel2(x1,y1,cvalue);
          if(e>0){x1+=1;e-=dy;}
          y1-=1;
          e+=dx;
        }
      }
    }
  }
  else //dx<0
  {
    dx=-dx;   //dx=abs(dx)
    if(dy >= 0) // dy>=0
    {
      if(dx>=dy) // 4/8 octant
      {
        e=dy-dx/2;
        while(x1>=x2)
        {
          GLIB_PutPixel2(x1,y1,cvalue);
          if(e>0){y1+=1;e-=dx;}
          x1-=1;
          e+=dy;
        }
      }
      else    // 3/8 octant
      {
        e=dx-dy/2;
        while(y1<=y2)
        {
          GLIB_PutPixel2(x1,y1,cvalue);
          if(e>0){x1-=1;e-=dy;}
          y1+=1;
          e+=dx;
        }
      }
    }
    else       // dy<0
    {
      dy=-dy;   // dy=abs(dy)

      if(dx>=dy) // 5/8 octant
      {
        e=dy-dx/2;
        while(x1>=x2)
        {
          GLIB_PutPixel2(x1,y1,cvalue);
          if(e>0){y1-=1;e-=dx;}
          x1-=1;
          e+=dy;
        }
      }
      else    // 6/8 octant
      {
        e=dx-dy/2;
        while(y1>=y2)
        {
          GLIB_PutPixel2(x1,y1,cvalue);
          if(e>0){x1-=1;e-=dy;}
          y1-=1;
          e+=dx;
        }
      }
    }
  }
}

void GLIB_PutPalLine(int nPosX1, int nPosY1, int nPosX2, int nPosY2, u32 uData)
{

  int dx,dy,e;
  dx=nPosX2-nPosX1;
  dy=nPosY2-nPosY1;

  if(dx>=0)
  {
    if(dy >= 0) // dy>=0
    {
      if(dx>=dy) // 1/8 octant
      {
        e=dy-dx/2;
        while(nPosX1<=nPosX2)
        {
          GLIB_PutPalData(nPosX1,nPosY1,uData);
          if(e>0){nPosY1+=1;e-=dx;}
          nPosX1+=1;
          e+=dy;
        }
      }
      else    // 2/8 octant
      {
        e=dx-dy/2;
        while(nPosY1<=nPosY2)
        {
          GLIB_PutPalData(nPosX1,nPosY1,uData);
          if(e>0){nPosX1+=1;e-=dy;}
          nPosY1+=1;
          e+=dx;
        }
      }
    }
    else       // dy<0
    {
      dy=-dy;   // dy=abs(dy)

      if(dx>=dy) // 8/8 octant
      {
        e=dy-dx/2;
        while(nPosX1<=nPosX2)
        {
          GLIB_PutPalData(nPosX1,nPosY1,uData);
          if(e>0){nPosY1-=1;e-=dx;}
          nPosX1+=1;
          e+=dy;
        }
      }
      else    // 7/8 octant
      {
        e=dx-dy/2;
        while(nPosY1>=nPosY2)
        {
          GLIB_PutPalData(nPosX1,nPosY1,uData);
          if(e>0){nPosX1+=1;e-=dy;}
          nPosY1-=1;
          e+=dx;
        }
      }
    }
  }
  else //dx<0
  {
    dx=-dx;   //dx=abs(dx)
    if(dy >= 0) // dy>=0
    {
      if(dx>=dy) // 4/8 octant
      {
        e=dy-dx/2;
        while(nPosX1>=nPosX2)
        {
          GLIB_PutPalData(nPosX1,nPosY1,uData);
          if(e>0){nPosY1+=1;e-=dx;}
          nPosX1-=1;
          e+=dy;
        }
      }
      else    // 3/8 octant
      {
        e=dx-dy/2;
        while(nPosY1<=nPosY2)
        {
          GLIB_PutPalData(nPosX1,nPosY1,uData);
          if(e>0){nPosX1-=1;e-=dy;}
          nPosY1+=1;
          e+=dx;
        }
      }
    }
    else       // dy<0
    {
      dy=-dy;   // dy=abs(dy)

      if(dx>=dy) // 5/8 octant
      {
        e=dy-dx/2;
        while(nPosX1>=nPosX2)
        {
          GLIB_PutPalData(nPosX1,nPosY1,uData);
          if(e>0){nPosY1-=1;e-=dx;}
          nPosX1-=1;
          e+=dy;
        }
      }
      else    // 6/8 octant
      {
        e=dx-dy/2;
        while(nPosY1>=nPosY2)
        {
          GLIB_PutPalData(nPosX1,nPosY1,uData);
          if(e>0){nPosX1-=1;e-=dy;}
          nPosY1-=1;
          e+=dx;
        }
      }
    }
  }
}

void GLIB_ClearFrame2(u32 c)
{
  int i, j;

  for(j=0; j<oGlib.m_uVsz; j++)
    for(i=0; i<oGlib.m_uHsz; i++)
      GLIB_PutPixel2(i, j, c);

}

void GLIB_ClearPalFrame(u32 uData)
{
  int i, j;
  u32 uCnt = 0;
  for(j=0; j<oGlib.m_uVsz; j++)
    for(i=0; i<oGlib.m_uHsz; i++)
      GLIB_PutPalData(i, j, uData);

}

void GLIB_InitInstance(u32 uFbAddr, u32 uHSize, u32 uVSize, IMG_FMT eBpp)
{
  oGlib.m_uHsz = uHSize;
  oGlib.m_uVsz = uVSize;

  oGlib.m_uBpp =
    (eBpp == RGB8 || eBpp == ARGB8) ? 8 :
    (eBpp == RGB16 || eBpp == ARGB16) ? 16 :
    (eBpp == RGB24 || eBpp == ARGB24) ? 24 : 24;

  oGlib.m_uPalDataBpp =
    (eBpp == PAL1) ? 1 :
    (eBpp == PAL2) ? 2 :
    (eBpp == PAL4) ? 4 :
    (eBpp == PAL8) ? 8 : 8; 

  oGlib.m_eCSpace = eBpp;

  oGlib.m_pFrameBuf = (void*)uFbAddr;
  oGlib.m_uFrameBuf = uFbAddr; /// added in GJ

  oGlib.m_uPalFlag = 0;
  oGlib.m_uCnt = 0;
  oGlib.m_ucPreData = 0;

  oGlib.m_uPrePosX = 0;
  oGlib.m_uPrePosY = 0;

  if (eBpp == RGB8)
  {
    oGlib.m_uColorValue[C_BLACK] = 0x00;
    oGlib.m_uColorValue[C_RED]   = 0x07<<5;
    oGlib.m_uColorValue[C_GREEN] = 0x07<<2;
    oGlib.m_uColorValue[C_BLUE]  = 0x03;
    oGlib.m_uColorValue[C_WHITE] = 0xff;
  }
  // A:1 - R:2 - G:3 - B:2
  // A's init value: 0 (A is Alpha value select bit)
  // if A is 0 then alpha_0 value of LCDC is selected, else alpha_1 value is selected
  else if (eBpp == ARGB8) 
  {
    oGlib.m_uColorValue[C_BLACK] = 0x00;
    oGlib.m_uColorValue[C_RED] = 0x3<<5;
    oGlib.m_uColorValue[C_GREEN] = 0x7<<2;
    oGlib.m_uColorValue[C_BLUE] = 0x3;
    oGlib.m_uColorValue[C_WHITE] = 0x7f;
  }
  else if (eBpp == RGB16)
  {
    oGlib.m_uColorValue[C_BLACK] = 0x0;
    oGlib.m_uColorValue[C_RED]   = 0x1f<<11;
    oGlib.m_uColorValue[C_GREEN] = 0x3f<<5;
    oGlib.m_uColorValue[C_BLUE]  = 0x1f<<0;
    oGlib.m_uColorValue[C_WHITE] = 0xffff;
  }
  else if (eBpp == ARGB16) // A:1 - R:5 - G:5 - B:5 and A's init value: 0
  {
    oGlib.m_uColorValue[C_BLACK] = 0x0;
    oGlib.m_uColorValue[C_RED]   = 0x1f<<10;
    oGlib.m_uColorValue[C_GREEN] = 0x1f<<5;
    oGlib.m_uColorValue[C_BLUE]  = 0x1f<<0;
    oGlib.m_uColorValue[C_WHITE] = 0x7fff;
  }
  else if (eBpp == RGB24)
  {
    oGlib.m_uColorValue[C_BLACK] = 0x0;
    oGlib.m_uColorValue[C_RED]   = 0xff0000;
    oGlib.m_uColorValue[C_GREEN] = 0xff00;
    oGlib.m_uColorValue[C_BLUE]  = 0xff;
    oGlib.m_uColorValue[C_WHITE] = 0xffffff;
  }
  else if (eBpp == ARGB24) // A:1 - R:8 - G:8 - B:7 and A's init value: 0
  {
    oGlib.m_uColorValue[C_BLACK] = 0x0;
    oGlib.m_uColorValue[C_RED]   = 0xff<<15;
    oGlib.m_uColorValue[C_GREEN] = 0xff<<7;
    oGlib.m_uColorValue[C_BLUE]  = 0x7f;
    oGlib.m_uColorValue[C_WHITE] = 0x7fffff;
  }
  else if (eBpp == ARGB25) // A:1 - R:8 - G:8 - B:8 and A's init value: 0
  {
    oGlib.m_uColorValue[C_BLACK] = 0x0;
    oGlib.m_uColorValue[C_RED]   = 0xff<<15;
    oGlib.m_uColorValue[C_GREEN] = 0xff<<7;
    oGlib.m_uColorValue[C_BLUE]  = 0xff;
    oGlib.m_uColorValue[C_WHITE] = 0xffffff;
  }

  oGlib.m_uColorValue[C_YELLOW] = (oGlib.m_uColorValue[C_RED] | oGlib.m_uColorValue[C_GREEN]);
  oGlib.m_uColorValue[C_CYAN] = (oGlib.m_uColorValue[C_GREEN] | oGlib.m_uColorValue[C_BLUE]);
  oGlib.m_uColorValue[C_MAGENTA] = (oGlib.m_uColorValue[C_RED] | oGlib.m_uColorValue[C_BLUE]);    

  GLIB_InitFont();
}

void GLIB_InitFont(void)
{
  oGlib.m_pFont8x15 = (u8 *)font8x15;

  GLIB_SetFontColor(C_WHITE, C_BLACK, 0);
}

void GLIB_SetFontColor(COLOR eFontColor, COLOR eBgColor, bool bIsBgTransparent)
{
  oGlib.m_fontColor = eFontColor;
  oGlib.m_bgColor = eBgColor;
  oGlib.m_bIsBgTransparent = bIsBgTransparent;
}

void GLIB_DrawPattern(u32 uWidth, u32 uHeight)
{
  u32 uSubWidth, uSubHeight;

  GLIB_ClearFrame(C_WHITE);

  uSubWidth = uWidth/8;
  uSubHeight = uHeight/2;

  // Draw Color Bar
  GLIB_FillRectangle(0, 0, uSubWidth, uSubHeight, C_RED);
  GLIB_FillRectangle(uSubWidth*1, 0, uSubWidth, uSubHeight, C_GREEN);
  GLIB_FillRectangle(uSubWidth*2, 0, uSubWidth, uSubHeight, C_BLUE);  
  GLIB_FillRectangle(uSubWidth*3, 0, uSubWidth, uSubHeight, C_CYAN);    
  GLIB_FillRectangle(uSubWidth*4, 0, uSubWidth, uSubHeight, C_MAGENTA);     
  GLIB_FillRectangle(uSubWidth*5, 0, uSubWidth, uSubHeight, C_YELLOW);        
  GLIB_FillRectangle(uSubWidth*6, 0, uSubWidth, uSubHeight, C_BLACK);
  GLIB_FillRectangle(uSubWidth*7, 0, uSubWidth, uSubHeight, C_WHITE);

  // Draw Pattern
  uSubWidth = uWidth/2;
  GLIB_PutRectangle(0, uSubHeight, uWidth, uSubHeight, C_RED);
  GLIB_PutLine(0, uSubHeight, uWidth-1, uHeight-1, C_BLACK);
  GLIB_PutLine(uWidth, uSubHeight, 0, uHeight-1, C_MAGENTA);  

  uSubWidth = uWidth/2;
  GLIB_PutLine(uSubWidth, uSubHeight, uSubWidth, uHeight-1, C_GREEN);   

  uSubHeight = uHeight*3/4;
  GLIB_PutLine(0, uSubHeight, uWidth-1, uSubHeight, C_BLUE);
}

