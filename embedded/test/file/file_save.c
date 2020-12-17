#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> 	/* for O_RDONLY */
#include <sys/mman.h> 	/* for mmap */
#include <linux/fb.h> 	/* for fb_var_screeninfo, FBIOGET_VSCREENINFO*/
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>  
#include <linux/input.h>
#include <string.h> 
#include <errno.h>

#define FBDEV_FILE "/dev/fb0"
#define EVENT_BUF_NUM 64

struct _player{
	char name[20];
	int score;
}player;

int main(){
	char *filename = "score.txt";

	FILE *fp;
	fp = fopen(filename, "w");
	char str[20];

	int j;
	for(j=0; j<10; j++){
		sprintf(str, "%d", j);
		fputs(str, fp);
	}


	fclose(fp);
}
