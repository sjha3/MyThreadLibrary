#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>

typedef ucontext_t* MyThread;
int queue_size;
ucontext_t mythread_init_context;
struct c_node{
     int id;
     struct c_node *next;
};

static int t_id;
struct t_node{
	struct ucontext_t* uc;
	struct t_node *next;
	struct t_node *parent;
	int num_child;
	int block_on_join;
        int block_on_sema;
        int block_par;
	int is_main;
        int is_yield;
        int th_id;
        //struct c_node *ch_ll;
        struct c_node* child_arr;
        struct c_node *child_ptr;
        //struct t_node *child_arr;
};

struct t_node main_node;
struct t_node *exe_node,*main_exe_node;
static int id_cnt;
struct Queue{
	struct t_node* head;
	struct t_node* tail;
}; 
struct Queue* readyQueue;
static struct Queue* blockedQueue;
static int is_main_calling_exit=0;

struct sema{
   int val;
   struct Queue * semaQueue;
};  
 
typedef struct sema* MySemaphore;

struct sema_ll{
    MySemaphore mysema;
    struct sema_ll *next;
};

static struct sema_ll *sema_head=NULL;
static int sema_count = 0;

//=============================================================
void deleteNode(MySemaphore temp_sema)
{
    // When node to be deleted is head node
    if(sema_head->mysema == temp_sema)
    {
        if(sema_head->next == NULL)
        {
          //  printf("There is only one node. The list can't be made empty ");
            return;
        }
 
        // Copy the data of next node to head 
        sema_head->mysema = sema_head->next->mysema;
 
        // store address of next node
        struct sema_ll* n = sema_head->next;
 
        // Remove the link of next node
        sema_head->next = sema_head->next->next;
 
        // free memory
        free(n);
 
        return;
    }
    // When not first node, follow the normal deletion process
     // find the previous node
    struct sema_ll* prev = sema_head;
    while(prev->next != NULL && prev->next->mysema != temp_sema)
        prev = prev->next;
 
    // Check if node really exists in Linked List
    if(prev->next == NULL)
    {
        //printf("\n Given node is not present in Linked List");
        return;
    }
    struct sema_ll* n = prev->next;
    // Remove node from Linked List
    prev->next = prev->next->next;
 
    // Free memory
    free(n);
 
    return; 
}

void print_semalist(){
struct sema_ll *new_node = sema_head;
while(new_node!=NULL){
//printf(" sema is %x and value is %d and queue is %x\n",new_node->mysema,new_node->mysema->val,new_node->mysema->semaQueue);
new_node=new_node->next;
  }
}
 

// Utility function to insert a node at the begining 
void push(MySemaphore temp_sema)
{
   struct sema_ll *new_node =(struct sema_ll*)malloc(sizeof(struct sema_ll));
    new_node->mysema = temp_sema;
    new_node->next = sema_head;
    sema_head = new_node; //  need to check whethr to use *sema_list or only sema_list
  // printf("=========  Inserted node : %x, head_node : %x\n",new_node,sema_head);
}

//===============================================================


void enqueue(struct Queue *queue, struct t_node* node){
//	printf("inside enqueue()\n");
	if(queue->head == NULL && queue->tail==NULL){
  //              printf("empty Queue with head as %x\n",queue->head);
		queue->head = node;
		queue->tail = node;
		node ->next = NULL;
                //printf(" element : %x \n",queue->head);
	}
	else{
                //printf("Non empty Queue\n");
		queue->tail->next = node;
		queue->tail=node;
		node->next = NULL;
                //printf(" element : %x \n",queue->head);
	}	
	return;
}

int size_queue(struct Queue *queue){
if(queue==NULL)
return 0;
int cnt=0;
struct t_node* head=queue;
while(head!=queue->tail){
cnt++;
head=head->next;
}
return cnt;
}

struct t_node* dequeue(){
	//printf(" deque op from queue : %x\n",readyQueue);
	if(readyQueue->head == NULL && readyQueue->tail==NULL){
        //printf("queue is empty");
	return NULL;
        }
	
	struct t_node *node = readyQueue->head;
	if(readyQueue->head == readyQueue->tail){
          //printf("queue has only one element");
		readyQueue->head=NULL;
		readyQueue->tail=NULL;		
	}
	else{
        //printf("queue has more than one element");
	readyQueue->head = readyQueue->head->next;
        }
	
       node->next = NULL;	
       return node;
       
}

/* deque from blocked, will need to merge both dequeue()*/
struct t_node* dequeue_blocked(){
	//printf(" deque op from queue : %x\n",blockedQueue);
	if(blockedQueue->head == NULL && blockedQueue->tail==NULL){
        //printf("queue is empty");
	return NULL;
        }
	
	struct t_node *node = blockedQueue->head;
	if(blockedQueue->head == blockedQueue->tail){
          //printf("queue has only one element");
		blockedQueue->head=NULL;
		blockedQueue->tail=NULL;		
	}
	else{
        //printf("queue has more than one element");
	blockedQueue->head = blockedQueue->head->next;
        }
	
       node->next = NULL;	
       return node;
       
}




struct t_node* peek()
{
        struct t_node *node = readyQueue->head; 
	return node;	
}

void remove_from_queue(struct Queue *queue, struct t_node* node)
{
if(queue==NULL || queue->head==NULL)
return;
struct t_node *temp=queue->head;
struct t_node *prev=queue->head;
if(queue->head==node){  //head is same as node, so simply dequeu this queue
  	if(queue->head == queue->tail){
          //printf("queue has only one element");
	  queue->head=NULL;
	  queue->tail=NULL;		
	}
	else{
        //printf("queue has more than one element");
	queue->head = queue->head->next;
        }
	
       temp->next = NULL;
       return;
}
else {
   while(temp!=NULL)
    {
     if(temp==node){
      prev->next=temp->next;
      break;
     }
     prev=temp;
     temp=temp->next;
   }
}
return;
}


void unblock(struct t_node *node){
   node->block_on_join--;
   if(node->block_on_join==0){
   remove_from_queue(blockedQueue,node);
   enqueue(readyQueue,node);
   }
}

struct t_node* getNode(struct ucontext_t *th, int is_mn, struct t_node *par)
{
	struct t_node *node1 = (struct t_node*)malloc(sizeof(struct t_node));	
	node1 -> uc = th;
	node1->next = NULL;
	node1->parent = par;
	node1->num_child = 0;
	node1->block_on_join = 0;
        node1->block_par=0;
        node1->is_main=is_mn;
        node1->block_on_join=0;
        node1->block_par=0;
        node1->is_yield=0;
        node1->block_on_sema=0;
        node1->th_id=id_cnt++;
        node1->child_arr = (struct c_node*)malloc(sizeof(struct c_node));
        node1->child_ptr=node1->child_arr;
        if(par!=NULL){
        //push(par->child_arr,node1->th_id);
        par->num_child++;
        } 

	return node1;
}

MyThread MyThreadCreate(void(*start_funct)(void *), void *args)
{
	MyThread t_new = (MyThread)malloc(sizeof(ucontext_t));
        //printf("****************** CREATE()***************\n");
	char *thread_stack = malloc(8*1024);
	struct t_node *node,*cur_node;
	int ret = getcontext(t_new);
	//printf("get context\n");
	if(ret == -1){
	//printf("getcontext failed\n");
    return;
   }

	t_new->uc_stack.ss_sp = thread_stack;
	t_new->uc_stack.ss_size = 8192;
	t_new->uc_stack.ss_flags = 0;
	t_new->uc_link = NULL; //killer_thread;

      //node1 = getNode(t_new,0,NULL);
        //printf("before peek\n");
	cur_node = peek();
        
        if(cur_node!=NULL){
        cur_node->num_child++;
        //printf("front of queue is %x and num_child is %d\n",cur_node,cur_node->num_child);
        }
        getcontext(t_new);
	makecontext(t_new,start_funct,1,args);
	node = getNode(t_new,0,exe_node);  // check whether it's being passed well or not
        
        //node->id=id_cnt++;
        //printf("do enqueu: %x node id: %d\n",node,node->th_id);
     
	enqueue(readyQueue, node);
        //printf("after enqueu:\n");
        //setcontext(t_new);
	return t_new;		
}

void MyThreadInit (void(*start_funct)(void *), void *args)
{	
	readyQueue = (struct Queue*)malloc(sizeof(struct Queue));
	blockedQueue = (struct Queue*)malloc(sizeof(struct Queue));
        
	readyQueue->head = NULL;
	readyQueue->tail = NULL;
	blockedQueue->head = NULL;
	blockedQueue->tail = NULL;
	//printf("**** ThreadInit()*****\n");
        MyThread t_main = (MyThread)malloc(sizeof(ucontext_t));
        MyThread t_child = (MyThread)malloc(sizeof(ucontext_t));


	char *main_thread_stack = malloc(8*1024);
	char *child_thread_stack = malloc(8*1024);
	struct t_node *main_node, *child_node;
	
	t_child -> uc_stack.ss_sp = child_thread_stack;
	t_child -> uc_stack.ss_size = 8192;
	t_child -> uc_stack.ss_flags = 0;
	t_child ->uc_link = NULL;// killer_thread;
	main_node = getNode(t_child,0,NULL);
        main_node->is_main=1;
        main_node->block_on_join=0;
        main_node->block_par=0;
        main_node->is_yield=0;
        main_node->block_on_sema=0;
        main_node->child_arr = (struct t_node*)malloc(sizeof(struct t_node));
        exe_node=main_node;
        main_exe_node = main_node;

        getcontext(t_child);
        //printf("before insertion queue is %x\n",readyQueue);	
	makecontext(t_child,start_funct,1,args);

        swapcontext(&mythread_init_context,t_child);

	return;	
}


int MyThreadJoin(MyThread th)
{
        //printf("$$$$$$$$$$$$$$$$$$ inside join()\n");
	struct t_node *temp_node;
        int found_ready=0,found_blocked=0;
	struct t_node *frnt = readyQueue->head;
        struct t_node *b_front;
	while(frnt!= NULL){
		if(frnt->uc==th){
                  //printf("FOund the thread in readyqueue\n");
		  found_ready = 1;
                  if(frnt->parent != exe_node)
                    return -1;
                  else
		   break;
	   }		  
		frnt = frnt->next;
	}

if(found_ready==0){   //Thread not found in readyQueue, need to check blockedQueue
b_front=blockedQueue->head;
	while(b_front!= NULL){
		if(b_front->uc==th){
                  //printf("FOund the thread in bloeckedqueue\n");
		  found_blocked = 1;
                 if(b_front->parent != exe_node)
                    return -1;
		 else 
                    break;
	   }		  
		b_front = b_front->next;
	}

 }

  if(found_ready==0 && found_blocked==0){
   //printf("Thread already exited\n");
   return 0;   //need to check whether -1 or 0 will be returned
   }
// Put a check for returnig -1 as per requirment

    if(found_ready==1)
    frnt->block_par=1;
    else if(found_blocked==1)
    b_front->block_par=1;

    exe_node->block_on_join = 1;
    temp_node = exe_node;
    enqueue(blockedQueue, exe_node);
    exe_node = dequeue();

    swapcontext(temp_node->uc,exe_node->uc);  
   
    return 0; 
    
}

void MyThreadJoinAll(){

//printf("******* MythreadJoinALL()***********\n");
if(exe_node->num_child==0){
   return;
  }

int block_cnt=0;
//check in readyQueue
struct t_node* front=readyQueue->head;
while(front!=NULL){
if(front->parent==exe_node){
  //printf("found chld in readyqeueu\n");
  block_cnt++;
  front->block_par=1;
  }
  front=front->next;
 }

//check in blockedQueue as well
struct t_node* b_front=blockedQueue->head;
while(b_front!=NULL){
if(b_front->parent==exe_node){
  //printf("found chld in blockedqeueu\n");
  block_cnt++;
  b_front->block_par=1;
  }
  b_front=b_front->next;
 }


 if(block_cnt > 0){   // children are waiting on readyQueue
   //printf("setting blocked count of parent as %d\n",block_cnt);
   struct t_node* temp_node = exe_node;
   exe_node->block_on_join=block_cnt;
   enqueue(blockedQueue, exe_node);
   exe_node = dequeue();
   //printf("exe node : %x prev exe node : %x\n",exe_node,temp_node);
   swapcontext(temp_node->uc,exe_node->uc); 
   }
}


void MyThreadYield(void){
   //printf("**********MyThreadYield()********* Queue : %x front %x \n",readyQueue,peek());
   struct t_node *next_node, *front_node,*temp_node;
   front_node = peek();
   if(front_node==NULL){  
      return;
   }

    temp_node = exe_node;
    enqueue(readyQueue, temp_node);
    exe_node = dequeue();
    //printf("exe node : %x prev exe node : %x\n",exe_node,temp_node);
    swapcontext(temp_node->uc,exe_node->uc);

}

void print_child_parent(){
struct t_node *node = exe_node;
//printf("*********** inside print child and parent()**********\n");
//printf("exe_node : %x thread id : %d num of child : %d\n",exe_node,node->th_id,node->num_child);
struct c_node *child=exe_node->child_arr;
//for(int i=0;i<num_child;i++)
while(child!=NULL){
//printf("child : %x child id: %d\n",child,child->id);
child = child->next;
}
return;
}

void MyThreadExit(){
   //printf("************ MyThreadExit()***********\n");
   //print_child_parent();	
   struct t_node *next_node;
   // if exe node has called exit, we need to check if it has parent, if yes then if parent was in blocked queue due to join, 
   // we need to dequeue parent from blocked queue and enqueue it to reqdyqueue
   if(exe_node->block_par==1)
    unblock(exe_node->parent);

   struct t_node *front_node=peek();
   if(front_node!=NULL){
    //printf("front node is not NULL\n");
    front_node=dequeue();


    exe_node=front_node;

    setcontext(exe_node->uc);
   }
  else{
  //printf("front node is NULL\n");
  setcontext(&mythread_init_context);
   // setcontext(main_exe_node);
   }
    return;
} 

MySemaphore dequeue_sema(struct Queue *queue){
	//printf(" deque op from queue : %x\n",readyQueue);
	if(queue->head == NULL && queue->tail==NULL){
        //printf("queue is empty");
	return NULL;
        }
	
	struct t_node *node = queue->head;
	if(queue->head == queue->tail){
          //printf("queue has only one element");
		queue->head=NULL;
		queue->tail=NULL;		
	}
	else{
        //printf("queue has more than one element");
	queue->head = queue->head->next;
        }
	
       node->next = NULL;	
       return node;
}

MySemaphore MySemaphoreInit(int initialValue){
//printf("MMMMMMM SemaInit() MMMMMMMM \n");
        if(initialValue < 0 )
         return NULL;
	MySemaphore temp_sema = (MySemaphore)malloc(sizeof(MySemaphore));
	temp_sema->val=initialValue;
	temp_sema->semaQueue = (struct Queue*)malloc(sizeof(struct Queue));
	temp_sema->semaQueue->head=NULL;
	temp_sema->semaQueue->tail=NULL;
    //printf(" sema queue is %x\n",temp_sema->semaQueue);
      push(temp_sema);
      //printf(" sema created : %x\n",temp_sema);
print_semalist();
      return temp_sema;
}

void MySemaphoreSignal(MySemaphore sem){
   //printf("MMMMMMMMMM Semaphore Signal () with val as %d MMMMMMMMM\n",sem->val);
     if(sem==NULL)
      return;
   MySemaphore front;
     sem->val = sem->val+1; 

     if(sem->val >= 0){
       //printf("val >= 0 , so take 1st elem and put into readyqueu\n");
       front = sem->semaQueue->head;
       if(front==NULL)
          return;
       front=dequeue_sema(sem->semaQueue);
       enqueue(readyQueue,front);

     }
}

void MySemaphoreWait(MySemaphore sem){
   //printf("MMMMMMMMMMm Inside Wait() with val as %d MMMMMMMMMMM \n",sem->val);
      if(sem==NULL)
      return;
   MySemaphore front;  
     if(sem->val-- <=0){
    //printf("val < = 0 , so take exe_node and into semaQueue\n");
   struct t_node *temp_node;
    exe_node->block_on_sema = 1;
    temp_node = exe_node;

    enqueue(sem->semaQueue, exe_node);
    exe_node = dequeue();

    swapcontext(temp_node->uc,exe_node->uc);
    }
}

int MySemaphoreDestroy(MySemaphore sem){

if(sem==NULL || sem->semaQueue->head==NULL)
return -1;
int blocked_node_found=0; 
struct t_node* front = sem->semaQueue->head;
while(front!=NULL){
  if(front->block_on_sema ==1){
   blocked_node_found=1;
    break;
  }
  front=front->next;
}

if(!blocked_node_found){
deleteNode(sem);
free(sem->semaQueue);
free(sem);

return 0;
}
return -1;
}
