#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/ioctl.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <unistd.h> 
#include <linux/input.h>
#define EVENT_BUF_NUM 64
int main(void)
{
    int i, quit = 1;
    int fd = -1;                                 /* the file descriptor for the device */
    size_t read_bytes;                           // 몇 bytes read 했느냐
    struct input_event event_buf[EVENT_BUF_NUM]; // 몇개의 event 까지 한꺼번에 읽느냐
    // /dev/input/event3 파일을 읽기 전용 모드로 열어 // fd 변수에 파일 기술자를 저장한다.
    if ((fd = open("/dev/input/event3", O_RDONLY)) < 0)
    {
        printf("application : keypad driver open fail!\n");
        exit(1);
    }
    printf("press the key button!\n");
    printf("press the key 16 to exit!\n");
    while (quit)
    {
        // event 발생을 EVENT_BUF_NUM 만큼 읽어들인다.
        read_bytes = read(fd, event_buf, (sizeof(struct input_event) * EVENT_BUF_NUM));

	printf("\n Button key : %d\n", event_buf[i].code);
        if (event_buf[i].code == 16){
            printf("\napplication : Exit Program!! (key = %d)\n", event_buf[i].code);
            quit = 0;
        }
    }
    close(fd);
    return 0;
}
//파일을 닫는다.
