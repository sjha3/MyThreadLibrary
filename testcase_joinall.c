#include <stdio.h>
#include "mythread.h"
#include "assert.h" 

int count; 
int mode; 
MyThread T; 

void t1(void * who)
{
  int i;
  int re; 

  printf("t%d start\n",(int)who);
 
  if((int)who==0){
      for(i=0;i<count;i++){
      T = MyThreadCreate(t1,(void *)(i+1)); 
      }
      
      if (mode==1){
        printf("JoinAll\n");
        MyThreadJoinAll();
        printf("JoinAll finish\n"); 
       }
      else if (mode==2){
	MyThreadYield();
	printf("JoinAll\n"); 
	MyThreadJoinAll();
	printf("JoinAll finish\n");  
      }

   }

  else if((int)who==1&&mode==3){
        re= MyThreadJoin(T);
//	printf("re %d\n",re); 
        assert(re==-1);
    }

    printf("t%d end\n",(int)who);
    MyThreadExit();
}

int main(int argc, char *argv[])
{
  if(argc<2 || argc > 3)
    return -1; 
  count=atoi(argv[1]); 
  if (argc==3)
    mode=atoi(argv[2]);
  MyThreadInit(t1,(void*)0);
} 



