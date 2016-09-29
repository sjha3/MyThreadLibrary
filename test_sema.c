#include<stdio.h>
#include"mythread.h" 
#include"assert.h" 

int inter_count=0;
int outer_count=0; 
int mode=0; 
MySemaphore sem; 

void t1(void * who)
{
	printf("w%d wait\n",(int)who);
	MySemaphoreWait(sem);
	printf("w%d end\n",(int)who);
	MyThreadExit(); 
}

void t2(void * who)
{
	printf("s%d signal\n",(int)who);
	MySemaphoreSignal(sem); 
	printf("s%d end\n",(int)who);
	MyThreadExit(); 
}


void t0(void *dummy)
{
	int i, j;
	int re; 
	printf("Create Semaphore\n");
	sem=MySemaphoreInit(0); 
	for(i=0;i<outer_count;i++){
		for(j=0;j<inter_count;j++){
			MyThreadCreate(t1,(void *)(j));
			MyThreadYield(); 
			
		//	t1((void*)j); 
		}	
	
		if(mode==0){		
			for(j=0;j<inter_count;j++){
				MyThreadCreate(t2,(void *)(j));
				MyThreadYield(); 
			}
		}
	}

	MyThreadYield();	
	re=MySemaphoreDestroy(sem);
	if(mode!=0){
	//	printf("re %d\n",re); 
		assert(re==-1);
	}
	printf("Destroy Semaphore\n"); 
	MyThreadExit();
}




int main(int argc, char* argv[])
{
	if(argc<3||argc>4)
	  return -1;
	inter_count=atoi(argv[1]);
	outer_count=atoi(argv[2]);
	if (argc==4)
	  mode=atoi(argv[3]); 
 
	MyThreadInit(t0,(void *)0);
}
