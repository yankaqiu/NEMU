#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(){
	WP* f= free_;
	WP* h;
	free_=free_->next;
	f->next=NULL;
	h=head;
	if(h==NULL){
		head=f;
		h=head;
	}
	else{
		while(h->next!=NULL)
		 h=h->next;
		h->next=f;
	}
	return f;
}

void free_wp(WP *wp){
	if(!head||!wp)
	  return;
	bool flag=false;
	WP* p=head;
	if(head->NO==wp->NO){
		head=head->next;
		p->next=free_;
		free_=p;
		flag=true;
	}else{
		while(p->next!=NULL){
			if(p->next->NO==wp->NO){
				p->next=p->next->next;
				wp->next=free_;
				free_=wp;
				flag=true;
				break;
			}
		}
	}
	if(flag){

	}
	else
	{
		printf("The watchpoint of No.%d doesn't exist, delete watchpoint failed!\n",wp->NO);
	}
}

