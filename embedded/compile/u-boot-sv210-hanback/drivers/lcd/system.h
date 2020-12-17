#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <common.h>
#include "./def.h"

extern volatile u32 g_uAPLL, g_uMPLL, g_uEPLL, g_uVPLL; // SCLKapll,mpll,epll,vpll
extern volatile u32 g_uARMCLK, g_uHclkMsys, g_uHclkDsys, g_uHclkPsys, g_uPclkMsys, g_uPclkDsys, g_uPclkPsys, g_uOneDRAM; // Core & Bus clocks


#endif
