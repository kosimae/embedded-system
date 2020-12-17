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
#include <errno.h>

#define FBDEV_FILE "/dev/fb0"
#define EVENT_BUF_NUM 64
#define MAX_LEN 16

struct _node{
	char name[MAX_LEN];
	int score;
	struct _node *next;
}node;

struct _node *head, *tail;

void init_list(){
	head = (struct _node *)calloc(1,sizeof(struct _node));
	tail = (struct _node *)calloc(1,sizeof(struct _node));
	head->next = tail;
	tail->next = tail;
}

struct _node *insert_node(char *file_name, char *file_score){
	struct _node *s, *p, *r;
	p = head;
	s = p->next;
	
	int score = atoi(file_score);

	while(s->score >= score && s!=tail){
		p = p->next;
		s = s->next;
	}

	r = (struct _node *)calloc(1, sizeof(struct _node));
	r->score = score;
	
	int k;
	for(k=0; k<sizeof(file_name); k++){
		r->name[k] = file_name[k];
	}

	p->next = r;
	r->next = s;
	
	return r;
}

void cut_node(int num){
	struct _node *s, *p;
	p = head;
	s = p->next;
	int flag = 0;

	int j;
	for(j=0; j<num; j++){
		if(s==tail){
			flag = 1;
			break;
		}
		
		p = p->next;
		s = s->next;
	} 

	if(flag == 0){
		p->next = tail;
	}
}

void print_node(){
	struct _node *p;
	p = head->next;

	while(p!=tail){
		printf("%s : %d\n", p->name, p->score);
		p = p->next;
	}
}

int main(){
	char *filename = "score.txt";

	FILE *fp;
	fp = fopen(filename, "r");

	char *str;
	char *file_name;
	char *file_score;
	
	int j, k;

	init_list();

	str = (char *)calloc(16, sizeof(char));
	file_name = (char *)calloc(16, sizeof(char));
	file_score = (char *)calloc(16, sizeof(char));

	for(j=0; j<5; j++){
		if(fgets(str, MAX_LEN, fp) == NULL){
			break;
		}
		
		for(k=0; k<sizeof(str); k++){
			file_name[k] = str[k];
		}

		file_name[sizeof(str)-3] = '\0';

		if(fgets(str, MAX_LEN, fp) == NULL){
			break;
		}

		for(k=0; k<sizeof(str); k++){
			file_score[k] = str[k];
		}

		file_score[sizeof(str)-2] = '\0';

		insert_node(file_name, file_score);
	}

	print_node();

	file_name = "X";
	file_score = "24";

	printf("\n");

	insert_node(file_name, file_score);
	print_node();


	printf("\n");
	cut_node(5);
	print_node();

	printf("\n");

	fclose(fp);

	struct _node *p;
	p = head->next;
	fp = fopen(filename, "w");

	
	for(j=0; j<5; j++){
		if(p==tail){
			break;
		}
	
		fputs(p->name, fp);
		fputs("\n", fp);
		sprintf(str, "%d", p->score);
		fputs(str, fp);
		fputs("\n", fp);

		//printf("\t%s : %d\n", p->name, p->score);

		p = p->next;

	}

	print_node();
	fclose(fp);
}
