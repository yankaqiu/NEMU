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
WP* new_wp(char* args){
	bool success=true;
	int ans=expr(args,&success);
	if(!success){
		printf("Set watchpoitn failed!");
		return NULL;
	}
	if(!free_){
		printf("There is no space left for a new watchpoint!");
		return NULL;
	}
	WP* re=free_;
	free_=free_->next;
	re->next=head;
	head=re;
	strcpy(re->expr,args);
	re->value=ans;
	return re;
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
		printf("The watchpoint of NO.%d(%s) has been ddeleted!\n",wp->NO,(wp->expr));
		(wp->expr)[0]='\0';
		wp->value=0;
	}
	else
	{
		printf("The watchpoint of No.%d doesn't exist, delete watchpoint failed!\n",wp->NO);
	}
}

void del_wp(int index){
	if(index>=0 && index<NR_WP)
		free_wp(&wp_pool[index]);
	else
	{
		printf("Index %d out of range(0<=index<%d\n",index,NR_WP);
	}
	
}

bool check_wp(){
	WP* p=head;
	bool ischanged=false;
	if(!head)
		return ischanged;
	while(p){
		bool success=true;
		int ans=expr(p->expr, &success);
		if(ans!=p->value){
			printf("EXPR: %s\n",p->expr);
			printf("Previous\t\t\tNow\n");
			printf("%#8x	%d",p->value,p->value);
			printf("<==>");
			printf("%#8x	%d\n",ans,ans);
			p->value=ans;
			ischanged=true;
		}
		p=p->next;
	}
	return ischanged;

}

void print_wp(){
	WP* p=head;
	if(!p){
		printf("No watchpoint to print\n");
		return ;
	}
	printf("NO\tEXPR\t\tValue\t");
	while(p){
		printf("%-d\t%-16s%#08x	%d\n",p->NO,p->expr,p->value,p->value);
		p=p->next;
	}
}
