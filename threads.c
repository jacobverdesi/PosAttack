#include <stdlib.h>
#include <curses.h>
#include <stdio.h>
#include <errno.h>
int scrWidth,scrHeight;
void startScreen(){
	initscr();
	cbreak();
	noecho();
	scrWidth=getmaxx(stdscr);
	scrHeight=getmaxy(stdscr);
	clear();
	refresh();
}
int readFile(FILE* fp){
	char* line=NULL;
	size_t len=0;
	ssize_t read;
	while((read=getline(&line,&len,fp))!=-1){
		if(line[0]!='#'){
			for(int i=0;i<read;i++){
			


			}
		}
	}
}
int main(int argc, char* argv[]){
	if(argc!=2){
		printf("Usage: ./threads config-file\n");
		exit(EXIT_FAILURE);
	}

	FILE *fp;
	fp=fopen(argv[1],"w");
	if(fp==NULL){
		printf("File not created, errno=%d\n",errno);
		exit(EXIT_FAILURE);
	}
	
	int rf=readFile(fp);
	if(rf==-1){
		printf("Invalid config file");
		exit(EXIT_FAILURE);
	}

	}
	startScreen();

	getch();
	endwin();
	return 0;
}
