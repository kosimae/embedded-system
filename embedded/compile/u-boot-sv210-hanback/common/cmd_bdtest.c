#include <common.h>
#include <command.h>
#include <regs.h>

#ifdef CONFIG_CMD_BDT
void main_menu_print(void)
{
#if defined(CONFIG_EMPOS3SV210)
	printf("\n-------------- HBE-EMPOSIII-SV210 Board Test --------------\n");
#else /* CONFIG_SM3SV210 */
  printf("\n-------------- HBE-SMIII-SV210 Board Test --------------\n");
#endif
	printf(" [1] Dip Switch Input Testing..\n");
	printf(" [2] KeyPad Input Testing..\n");
	printf(" [3] Character LCD Output Testing..\n");
	printf(" [4] LED Output Testing..\n");
	printf(" [5] 7-Segment LED Output Testing..\n");
	printf(" [6] Dot LED Testing...\n");
	printf(" [7] ST16C554A Quad Uart Testing...\n");
#if defined(CONFIG_SM3SV210)
  printf(" [8] GPIO LED Testing...\n");
  printf(" [9] GPIO 7-Segment Testing...\n");
  printf(" [a] Temporary Testing...\n");
#else /* CONFIG_EMPOS3SV210 */
  printf(" [9] Temporary Testing...\n");
#endif
	printf(" [Q or q] Quit\n\n");
	printf("---------------------------------------------------\n");
	printf("Select Menu ? ");
}

int st16c554a_test(void)
{
  ST16C554_SerialInit();
#if defined(CONFIG_EMPOS3SV210)
  ST16C554_printf("HBE-EMPOSIII-SC100 \n 16C554 Quad UART Controller A Port \n");
#else /* CONFIG_SM3SV210 */
  ST16C554_printf("HBE-SMIII-SC100 \n 16C554 Quad UART Controller A Port \n");
#endif
  return 0;
}

void temp_test(void)
{
#if 0
  LCDM_BackLightOn();
#else
  InitializeDisplay();
  //TestSimpleDisplay();
#endif
}

int do_bdtest(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char ch;
	int rtn;

	main_menu_print();
	ch = getc();
	while(1) {
		switch(ch) {
			case '1':
				printf("\nDip switch Input Testing...\n");
				dipsw_test();
				break;

			case '2':
				printf("\nKeyPad Input Testing...\n");
				key_test();
				break;

			case '3':
				printf("\nCharacter LCD Testing...\n");
				lcd_test();
				break;

			case '4':
				printf("\nLED Output Testing...\n");
				led_test();
				break;

			case '5':
				printf("\n7-Segment LED Testing...\n");
				seg_c_main();
				break;

			case '6':
				printf("\nDot-LED Testing...\n");
				dotled_test();
				break;

      case '7':
				printf("\nST16C554A Quad Uart Testing...\n");
				st16c554a_test();
				break;

      case '8':
				printf("\nGPIO LED Testing...\n");
				gpio_led_test();
				break;

#if defined(CONFIG_SM3SV210)
      case '9':
				printf("\nGPIO 7-Segment Testing...\n");
				temp_test();
				break;

      case 'a':
				printf("\nTemporary Testing...\n");
				temp_test();
				break;
#else /* CONFIG_EMPOS3SV210 */
      case '9':
				printf("\nTemporary Testing...\n");
				temp_test();
				break;
#endif

			case 'q': case 'Q':
				return 0;
		}
			main_menu_print();
			ch = getc();
	}
	return rtn;
}

/* -------------------------------------------------------------------- */

U_BOOT_CMD(
	bdtest,	1,	1,	do_bdtest,
	"bdtest  - Board test\n",
	NULL
);
#endif	/* CONFIG_CMD_BDT */
