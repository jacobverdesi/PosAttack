// threads.c
// A Posix Thread Game
// City Defender- Defend the city from the oncoming missles
// Author Jacob Verdesi
//
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
/*
 * DataStructure for reading the file
 * containts a number of max missles
 * highest building height
 * both players
 * the city data
 */
typedef struct readData{
	int maxMissles;
	char* player1;
	char* player2; 	
	int city[1024];
}readData;
/*
 * Missle data structer
 * containts a missles:
 * speed row col
 * if its in infinite state
 */
typedef struct Missle{
	int speed;
	int row;
	int col;
	int infinite;
}Missle;

/*
 * initilizes a readData structure
 */
struct readData createData(){
	readData data;
	data.player1=malloc(80);
	data.player2=malloc(80);
	//data.city=malloc(scrWidth);
	data.maxMissles=-1;
	for(int i=0;i<1024;i++){
		data.city[i]=2;
	}
	data.city[0]=-1;
	return data;
}
/**
 * initilizes curses enviroment
 * gets screenwith and height
 * makes message
 **/
void initScreen(){
	initscr();
	cbreak();
	noecho();
	scrWidth=getmaxx(stdscr);
	scrHeight=getmaxy(stdscr);
	clear();
	char* quitMsg="Enter 'q' to quit at end of attack, or control-C";
	mvaddstr(0,scrWidth/2-strlen(quitMsg)/2,quitMsg);
	refresh();
}
/*
 * reads the file and creates a datastrucure
 * parses file into types
 * line by line remove \n 
 * handles errors
 */
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
			if(row==0){
				strcpy(data.player1,line);
			}
			else if(row==1){
				strcpy(data.player2,line);
			}
			else if( row==2){
				if(atoi(line)==0){
					fprintf(stderr,"Error: missing missile specification.\n");	
					exit(EXIT_FAILURE);
				}
				data.maxMissles=atoi(line);
			}
			else{
				printf("%s\n",line);
				char* ptr;
				if(row==4&&(line==NULL||line[0]=='\0')){
					fprintf(stderr,"missing city layout\n");
					exit(EXIT_FAILURE);
				}
				ptr=strtok(line," ");
				while(ptr!=NULL&&counter<10){
					if(atoi(ptr)>defenseHeight)
						defenseHeight=atoi(ptr);
					data.city[row*10+counter-30]=atoi(ptr);
					counter++;
					ptr=strtok(NULL," ");
				}
			}
			row++;
			counter=0;
		}
	}
	printf("Test");
	if(data.player1==NULL){
		fprintf(stderr,"Error: missing defender name.\n");	
		exit(EXIT_FAILURE);
	}
	if(data.player2==NULL){
		fprintf(stderr,"Error: missing attacker name.\n");	
		exit(EXIT_FAILURE);
	}
	if(data.maxMissles==-1){
		fprintf(stderr,"Error: missing missile specification.\n");	
		exit(EXIT_FAILURE);
	}
	if(data.city[0]==-1){
		fprintf(stderr,"Error: missing city layout\n");
		exit(EXIT_FAILURE);
	}
	return data;
}
/*
 * draws the initial city 
 */
int makeCity(readData data){
	for(int i=0;i<scrWidth;i++){
		/*
		if(i==0&&data.city[i+1]>=data.city[i])
			mvaddch(scrHeight-data.city[i],i,'_');
		if(i==scrWidth-1&&data.city[i-1]>=data.city[i]){
			mvaddch(scrHeight-data.city[i],i,'_');
		}*/
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
/*
 *run method for the defender thread
 */
void* runDefender(){
	
	int pos=scrWidth/2;
        char* paddle="#####";
	int ch;	
	int row=scrHeight-defenseHeight-3;
        
	pthread_mutex_lock(&sharedLock);
        mvaddstr(row,pos,paddle);
	refresh();
        pthread_mutex_unlock( &sharedLock );
        
	while((ch=getch())!='q'){
                pthread_mutex_lock(&sharedLock);
                mvaddstr(row,pos,"     ");
                refresh();
                pthread_mutex_unlock( &sharedLock );
                if(ch==68&&pos>0){
            		pos-=1;
                }
                if(ch==67&&pos+5<scrWidth){
                        pos+=1;
                }
                pthread_mutex_lock(&sharedLock);
		mvaddstr(row,pos,paddle);
                refresh();
                pthread_mutex_unlock( &sharedLock );
        }
	return NULL;
}
/*
 *creates a missle
 */
Missle* make_missle(){
	
	Missle* missle=malloc(sizeof(Missle));

	missle->col=rand()%scrWidth;

	missle->speed=(rand()%100)+100;
	missle->infinite=0;

	missle->row=2;
	
	return missle;
}
/*
 * checks whats ahead of them missle
 */
char ahead(Missle* curr){
	return mvinch(curr->row+1,curr->col);
}
/*
 * run method for the missle
 */
void* runMissle(void* missle){
	Missle *curr=missle;
 	pthread_mutex_lock( &sharedLock );
	mvaddch(curr->row,curr->col,'|');
	refresh();
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
	if(curr->infinite==1){
		curr->row=1;
		curr->col=rand()%scrWidth;
		curr->speed=(rand()%200)+100;
		runMissle(curr);
	}
	return NULL;
}
/*
 * displays the end screen msgs
 */
void endScreen(readData rf){
	
	pthread_mutex_lock( &sharedLock );
	mvaddstr(2,scrWidth/2-strlen(rf.player2)-4,"The ");
	mvaddstr(2,scrWidth/2-strlen(rf.player2),rf.player2);
	mvaddstr(2,scrWidth/2," attacker has ended");
	pthread_mutex_unlock( &sharedLock );	
	refresh();
	while(getch()!='q'){
	}
	pthread_mutex_lock( &sharedLock );
	mvaddstr(4,scrWidth/2-strlen(rf.player1)-4,"The ");
	mvaddstr(4,scrWidth/2-strlen(rf.player1),rf.player1);
	mvaddstr(4,scrWidth/2," defender has ended");
	mvaddstr(5,scrWidth/2-strlen(rf.player1)-4,"hit enter to close...");
	pthread_mutex_unlock( &sharedLock );	
	refresh();
}
/*
 * main runs screen functions / runs file reader
 * makes the city
 * starts making missles
 * create and run threads for missles and defender
 */
int main(int argc, char* argv[]){
	
	if(argc!=2){
		fprintf(stderr,"Usage: ./threads config-file\n");
		exit(EXIT_FAILURE);
	}

	FILE *fp;
	fp=fopen(argv[1],"r");
	if(fp==NULL){
		clear();
		endwin();	
		fprintf(stderr,"File not created, errno=%d\n",errno);
		exit(EXIT_FAILURE);
	}
	readData rf=readFile(fp);
	fclose(fp);
	
	initScreen();
	makeCity(rf);
	
	srand(time(NULL));	
	void *retval;
	int infinite=0;
	
	if(rf.maxMissles==0){
		rf.maxMissles=10;
		infinite=1;
	}
	Missle* missles[rf.maxMissles];
	pthread_mutex_init(&sharedLock,NULL);
	pthread_t threads[rf.maxMissles];
	pthread_t defense;	
	pthread_create(&defense,NULL,runDefender,NULL);
	for(int t=0;t<rf.maxMissles;t++){
		usleep((rand()%1000+500)*1000);
		missles[t]=make_missle();
		if(infinite==1)
			missles[t]->infinite=1;
		pthread_create(&threads[t],NULL,runMissle,(void*) missles[t]); 
	}
	
	for(int t=0; t< rf.maxMissles;t++){
		pthread_join(threads[t],&retval);
	}
		
	endScreen(rf);	
	pthread_join(defense,&retval);
	clear();
	endwin();
	return 0;
}
