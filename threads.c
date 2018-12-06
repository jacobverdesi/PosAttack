#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
pthread_mutex_t sharedLock;
int scrWidth,scrHeight,defenseHeight;
typedef struct readData{
	int maxMissles;
	int defenseHeight;
	char* player1;
	char* player2; 	
	int city[256];
}readData;
typedef struct Missle{
	int speed;
	int row;
	int col;
}Missle;
struct readData createData(){
	readData data;
	data.player1=malloc(80);
	data.player2=malloc(80);
	//data.city=malloc(scrWidth);
	data.maxMissles=0;
	for(int i=0;i<scrWidth;i++){
		data.city[i]=2;
	}
	return data;
}

void initScreen(){
	initscr();
	cbreak();
	noecho();
	scrWidth=getmaxx(stdscr);
	scrHeight=getmaxy(stdscr);
	defenseHeight=scrHeight;
	clear();
	char* quitMsg="Enter 'q' to quit at end of attack, or control-C";
	mvaddstr(0,scrWidth/2-strlen(quitMsg)/2,quitMsg);
	refresh();
}
readData readFile(FILE* fp){
	char *line=NULL;
	size_t len=0;
	size_t read;
	int row=0;
	struct readData data=createData();
	int counter=0;
	while((signed)(read=getline(&line,&len,fp))!=-1){
		strtok(line,"\n");
		if(line[0]!='#'){
			row++;
			if(row==1){
				data.player1=line;
			}
			else if( row==2){
				data.player2=line;
			}
			else if( row==3){
				data.maxMissles=atoi(line);
			}
			else{
				char* ptr;
				ptr=strtok(line," ");
				while(ptr!=NULL&&counter<scrWidth){
					if(scrHeight-atoi(ptr)-3<defenseHeight)
						defenseHeight=scrHeight-atoi(ptr)-3;		
					data.city[counter]=atoi(ptr);
					counter++;
					ptr=strtok(NULL," ");
				}
			}
		}
	}
	return data;
}
int makeCity(readData data){
	for(int i=0;i<scrWidth;i++){
		if(data.city[i-1] >= data.city[i]&&data.city[i+1]>=data.city[i])
			mvaddch(scrHeight-data.city[i],i,'_');
		for(int j=1;j<data.city[i]-1;j++){	
			if(!(data.city[i-1] >= data.city[i])||!(data.city[i+1]>=data.city[i])){
				mvaddch(scrHeight-data.city[i]+j,i,'|');
			}		
		}
		refresh();
	}
	
	return -1;
}
void* runDefender(){
	
	int pos=scrWidth/2;
        char* paddle="#####";
	int ch;	
	int row=defenseHeight;
        
	pthread_mutex_lock(&sharedLock);
        mvaddstr(row,pos,paddle);
        pthread_mutex_unlock( &sharedLock );
        
	while(getch()!='q'){
		ch=getch();
                pthread_mutex_lock(&sharedLock);
                mvaddstr(row,pos,"     ");
                if(ch==68&&pos>0){
            		pos-=1;
                }
                if(ch==67&&pos+5<scrWidth){
                        pos+=1;
                }
		mvaddstr(row,pos,paddle);
                refresh();
                pthread_mutex_unlock( &sharedLock );
        }
	return NULL;
}
Missle* make_missle(){
	
	Missle* missle=malloc(sizeof(Missle));

	missle->col=rand()%scrWidth;

	missle->speed=(rand()%100)+100;

	missle->row=1;
	
	return missle;
}
char ahead(Missle* curr){
	return mvinch(curr->row+1,curr->col);
}
void* runMissle(void* missle){
	Missle *curr=missle;
 	pthread_mutex_lock( &sharedLock );
	mvaddch(curr->row,curr->col,'|');
	pthread_mutex_unlock( &sharedLock );	
	while(ahead(curr)==' '||(ahead(curr)=='?'&& mvinch(curr->row+2,curr->col)==' ')){
		curr->row++;
		pthread_mutex_lock( &sharedLock );
	        mvaddch(curr->row,curr->col,'|');
		mvaddch(curr->row-1,curr->col,' ');
	        refresh();
	        pthread_mutex_unlock( &sharedLock );	
		usleep(1000*curr->speed);
	}
	if(ahead(curr)=='?'&&curr->row+2!=defenseHeight&& curr->row<=scrHeight-4){
		curr->row++;
		pthread_mutex_lock( &sharedLock );
	        mvaddch(curr->row,curr->col,'|');
		mvaddch(curr->row-1,curr->col,' ');
	        refresh();
	        pthread_mutex_unlock( &sharedLock );	
		usleep(1000*curr->speed);
		curr->row++;
	        mvaddch(curr->row,curr->col,'|');
		pthread_mutex_lock( &sharedLock );
		mvaddch(curr->row-1,curr->col,' ');
	        refresh();
	        pthread_mutex_unlock( &sharedLock );	
		usleep(1000*curr->speed);
		curr->row--;
	}	
	if(ahead(curr)=='#')
		curr->row--;
	
	pthread_mutex_lock( &sharedLock );
	mvaddch(curr->row+1,curr->col,'?');
	mvaddch(curr->row+2,curr->col,'*');
	mvaddch(curr->row,curr->col,' ');
	pthread_mutex_unlock( &sharedLock );	
	refresh();
	return NULL;
}
int main(int argc, char* argv[]){
	
	if(argc!=2){
		printf("Usage: ./threads config-file\n");
		exit(EXIT_FAILURE);
	}

	initScreen();

	FILE *fp;
	fp=fopen(argv[1],"r");
	if(fp==NULL){
		printf("File not created, errno=%d\n",errno);
		exit(EXIT_FAILURE);
	}
	readData rf=readFile(fp);
	fclose(fp);
	/*if(rf==NULL){
		printf("Invalid config file");
		exit(EXIT_FAILURE);
	}*/
	makeCity(rf);
	srand(time(NULL));	
	void *retval;
	
	Missle* missles[rf.maxMissles];
	for(int t=0;t<rf.maxMissles;t++){
	
	}
	pthread_mutex_init(&sharedLock,NULL);
	pthread_t threads[rf.maxMissles];
	pthread_t defense;	
	pthread_create(&defense,NULL,runDefender,NULL);
	
	for(int t=0;t<rf.maxMissles;t++){
		sleep(1);
		missles[t]=make_missle();
		pthread_create(&threads[t],NULL,runMissle,(void*) missles[t]); 
		
	}
	
	for(int t=0; t< rf.maxMissles;t++){
		pthread_join(threads[t],&retval);
	}
	pthread_join(defense,&retval);
	
	while(getch()!='q'){
	}
	
	clear();
	endwin();
	
	return 0;
}
