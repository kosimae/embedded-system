#include <config.h>
#include <pmic.h>

void Delay(void)
{
	unsigned long i,j;
	for(i=0;i<DELAY;i++);
}

void SCLH_SDAH()
{
	IIC_ESCL_Hi;
	IIC_ESDA_Hi;
	Delay();
}

void SCLH_SDAL()
{
	IIC_ESCL_Hi;
	IIC_ESDA_Lo;
	Delay();
}

void SCLL_SDAH()
{
	IIC_ESCL_Lo;
	IIC_ESDA_Hi;
	Delay();
}

void SCLL_SDAL()
{
	IIC_ESCL_Lo;
	IIC_ESDA_Lo;
	Delay();
}

void IIC_ELow()
{
	SCLL_SDAL();
	SCLH_SDAL();
	SCLH_SDAL();
	SCLL_SDAL();
}

void IIC_EHigh()
{
	SCLL_SDAH();
	SCLH_SDAH();
	SCLH_SDAH();
	SCLL_SDAH();
}

void IIC_EStart()
{
	SCLH_SDAH();
	SCLH_SDAL();
	Delay();
	SCLL_SDAL();
}

void IIC_EEnd()
{
	SCLL_SDAL();
	SCLH_SDAL();
	Delay();
	SCLH_SDAH();
}

void IIC_EAck()
{
	unsigned long ack;

	IIC_ESDA_INP;			// Function <- Input

	IIC_ESCL_Lo;
	Delay();
	IIC_ESCL_Hi;
	Delay();
	ack = GPD1DAT;
	IIC_ESCL_Hi;
	Delay();
	IIC_ESCL_Hi;
	Delay();

	IIC_ESDA_OUTP;			// Function <- Output (SDA)

	ack = (ack>>4)&0x1;
	while(ack!=0);

	SCLL_SDAL();
}

void IIC_ESetport(void)
{
	GPD1PUD &= ~(0xf<<8);	// Pull Up/Down Disable	SCL, SDA

	IIC_ESCL_Hi;
	IIC_ESDA_Hi;

	IIC_ESCL_OUTP;		// Function <- Output (SCL)
	IIC_ESDA_OUTP;		// Function <- Output (SDA)

	Delay();
}

void IIC_EWrite (unsigned char ChipId, unsigned char IicAddr, unsigned char IicData)
{
	unsigned long i;

	IIC_EStart();

////////////////// write chip id //////////////////
	for(i = 7; i>0; i--)
	{
		if((ChipId >> (i-1)) & 0x0001)
			IIC_EHigh();
		else
			IIC_ELow();
	}

	IIC_ELow();	// write 'W'

	IIC_EAck();	// ACK

////////////////// write reg. addr. //////////////////
	for(i = 8; i>0; i--)
	{
		if((IicAddr >> (i-1)) & 0x0001)
			IIC_EHigh();
		else
			IIC_ELow();
	}

	IIC_EAck();	// ACK

////////////////// write reg. data. //////////////////
	for(i = 8; i>0; i--)
	{
		if((IicData >> (i-1)) & 0x0001)
			IIC_EHigh();
		else
			IIC_ELow();
	}

	IIC_EAck();	// ACK

	IIC_EEnd();
}

void PMIC_InitIp(void)
{
	IIC_ESetport();

#if defined(CONFIG_EMPOS3SV210) || defined(CONFIG_ADKSV210) || defined(CONFIG_EDKSV210)
	IIC_EWrite(MAX8698_ADDR, 0x00, 0xDE); // Turn BUCK3 off : EN3   : Bit5 => 0
	IIC_EWrite(MAX8698_ADDR, 0x01, 0xB0); // Turn LDO7 off  : ELDO7 : Bit6 => 0
#else
	IIC_EWrite(MAX8698_ADDR, 0x00, 0x3E);
	IIC_EWrite(MAX8698_ADDR, 0x01, 0xF0);
#endif

	// VDD_ARM
#if defined(CONFIG_CLK_1200_200_166_133)
	IIC_EWrite(MAX8698_ADDR, 0x04, 0xBB);	// default:0x99=>1.2v, 0xBB=>1.30v
	IIC_EWrite(MAX8698_ADDR, 0x05, 0xBB);	// default:0x99=>1.2v, 0xBB=>1.30v
#elif defined (CONFIG_CLK_1000_200_166_133)
#if defined(CONFIG_EMPOS3SV210) || defined(CONFIG_ADKSV210) || defined(CONFIG_EDKSV210)
	IIC_EWrite(MAX8698_ADDR, 0x04, 0xAA);	// BUCK1 default:0x99=>1.2v, 0xAA=>1.25v
	IIC_EWrite(MAX8698_ADDR, 0x05, 0xAA);	// BUCK1 default:0x99=>1.2v, 0xAA=>1.25v
#elif defined (CONFIG_SM3SV210)
  IIC_EWrite(MAX8698_ADDR, 0x04, 0x99);	// default:0x99=>1.2v, 0x99=>1.2v
	IIC_EWrite(MAX8698_ADDR, 0x05, 0x99);	// default:0x99=>1.2v, 0x99=>1.2v
#else
	IIC_EWrite(MAX8698_ADDR, 0x04, 0xAA);	// default:0x99=>1.2v, 0xAA=>1.25v
	IIC_EWrite(MAX8698_ADDR, 0x05, 0xAA);	// default:0x99=>1.2v, 0xAA=>1.25v
#endif
#else
#if defined(CONFIG_EMPOS3SV210) || defined(CONFIG_ADKSV210) || defined(CONFIG_EDKSV210)
	IIC_EWrite(MAX8698_ADDR, 0x04, 0xAA);	// BUCK1 default:0x99=>1.2v, 0xAA=>1.25v
	IIC_EWrite(MAX8698_ADDR, 0x05, 0xAA);	// BUCK1 default:0x99=>1.2v, 0xAA=>1.25v
#else /* HBE-SM3-SV210 */
  IIC_EWrite(MAX8698_ADDR, 0x04, 0x99);	// default:0x99=>1.2v, 0x99=>1.2v
	IIC_EWrite(MAX8698_ADDR, 0x05, 0x99);	// default:0x99=>1.2v, 0x99=>1.2v
#endif
#endif

	// VDD_INT
#if defined(CONFIG_CLK_1200_200_166_133)
#if defined(CONFIG_EMPOS3SV210) || defined(CONFIG_ADKSV210) || defined(CONFIG_EDKSV210)
	IIC_EWrite(MAX8698_ADDR, 0x06, 0x77);	// BUCK2 default:0x99=>1.2v, 0x77=>1.1v
#elif defined (CONFIG_SM3SV210)
	IIC_EWrite(MAX8698_ADDR, 0x06, 0x77);	// default:0x99=>1.2v, 0x77=>1.1v
#else
	IIC_EWrite(MAX8698_ADDR, 0x06, 0x88);	// default:0x99=>1.2v, 0x88=>1.15v
#endif
#else
#if defined(CONFIG_EMPOS3SV210) || defined(CONFIG_ADKSV210) || defined(CONFIG_EDKSV210)
	IIC_EWrite(MAX8698_ADDR, 0x06, 0x77);	// BUCK2 default:0x99=>1.2v, 0x77=>1.1v
#else /* HBE-SM3-SV210 */
	IIC_EWrite(MAX8698_ADDR, 0x06, 0x77);	// default:0x99=>1.2v, 0x77=>1.1v
#endif

#if defined(CONFIG_EMPOS3SV210) || defined(CONFIG_ADKSV210) || defined(CONFIG_EDKSV210)
	IIC_EWrite(MAX8698_ADDR, 0x07, 0x11); // BUCK3 default:0x02=>1.8v, 0v
	IIC_EWrite(MAX8698_ADDR, 0x08, 0x66); // LDO2/LDO3 default:0x88=>1.2v, 0x66=>1.1v
	IIC_EWrite(MAX8698_ADDR, 0x09, 0x02); // LDO4 default:0x02=>1.8v, 0x02=>1.8v
	IIC_EWrite(MAX8698_ADDR, 0x0A, 0x0C); // LDO5 default:0x0C=>2.8v, 0x0C=>2.8v
	IIC_EWrite(MAX8698_ADDR, 0x0B, 0x11); // LDO6 default:0x0A=>2.6v, 0x11=>3.3v
	IIC_EWrite(MAX8698_ADDR, 0x0C, 0x11); // LDO7 default:0x0E=>3.0v, 0v
	IIC_EWrite(MAX8698_ADDR, 0x0D, 0x33); // LDO8/BKCHR default:0x33=>3.3v/3.2v, 0x33=>3.3v/3.2v
	IIC_EWrite(MAX8698_ADDR, 0x0E, 0x0E); // LDO9 default:0x0E=>3.0v, 0x0E=>3.0v
#else /* HBE-SM3-SV210 */
	IIC_EWrite(MAX8698_ADDR, 0x07, 0x02); // BUCK3 default:0x02=>1.8v,
	IIC_EWrite(MAX8698_ADDR, 0x08, 0x66); // LDO2/LDO3 default:0x88=>1.2v,
	IIC_EWrite(MAX8698_ADDR, 0x09, 0x02); // LDO4 default:0x02=>1.8v, 
	IIC_EWrite(MAX8698_ADDR, 0x0A, 0x0C); // LDO5 default:0x0C=>2.8v,
	IIC_EWrite(MAX8698_ADDR, 0x0B, 0x0A); // LDO6 default:0x0A=>2.6v,
	IIC_EWrite(MAX8698_ADDR, 0x0C, 0x0E); // LDO7 default:0x0E=>3.0v,
	IIC_EWrite(MAX8698_ADDR, 0x0D, 0x33); // LDO8/BKCHR default:0x33=>3.3v/3.2v,
	IIC_EWrite(MAX8698_ADDR, 0x0E, 0x0E); // LDO9 default:0x0E=>3.0v,
#endif
#endif
}

