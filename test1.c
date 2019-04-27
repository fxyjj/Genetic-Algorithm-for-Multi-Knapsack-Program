#include<stdio.h>
#include<string.h>
#include<stdlib.h>

int main(int argc, char const *argv[])
{
	char prob_set[20];
    char output_set[20];
    int MAX_TIME;
    for(int i = 1;i<argc-1;i+=2){
        if(strcmp(argv[i],"-o") == 0){
            strcpy(output_set,argv[i+1]);
           
        }else
       	if(strcmp(argv[i],"-s") == 0){
            strcpy(prob_set,argv[i+1]);
           
        }else
        if(strcmp(argv[i],"-t") == 0){
            MAX_TIME = atoi(argv[i+1]);
        }else{
        	continue;
        }
    }
	printf("%s,%s\n",argv[1],prob_set);
	printf("%s,%s\n",argv[3],output_set);
	printf("%s,%d\n",argv[5],MAX_TIME);
	//printf("%s\n",argv[4]);
	
	return 0;
}