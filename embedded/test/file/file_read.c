#include <stdio.h>
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

#define MAX_LEN 100
int main(){
	char *filename = "score.txt";

	FILE *fp;
	fp = fopen(filename, "r");

	char str[MAX_LEN];

	while(fgets(str, MAX_LEN, fp) != NULL){
		printf("%s\n", str);
	}
	

	fclose(fp);
}
