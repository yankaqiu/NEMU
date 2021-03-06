#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_si(char *args){
	int cnt=0;
	if(args!=NULL)
	  sscanf(args,"%d",&cnt);
	else
		cnt=1;
	cpu_exec(cnt);
	return 0;
}

static int cmd_info(char *args){
	int i;
	if(args==NULL){
		printf("Unknown type to print!\n");
	}
	if(args[0]=='r'){
		for(i=R_EAX;i<=R_EDI;i++)
			printf("%s\t0x%08x\n",regsl[i],reg_l(i));
		printf("eip\t0x%08x\n",cpu.eip);
	}
	if(args[0]=='w')
		print_wp();
    return 0;
}

static int cmd_x(char *args){
	int num;
	bool flag;
	swaddr_t start_address;
	char *c=strtok(args, " ");
	sscanf(c,"%d",&num);
	args=c+strlen(c)+1;
	start_address=expr(args,&flag);
	if(!flag)
		assert(1);
	printf("0x%08x: ",start_address);
	int i;
	for(i=1;i<=num;i++){
		printf("0x%08x ",swaddr_read(start_address,4));
		start_address+=4;
	}
	printf("\n");
	return 0;
}

static int cmd_p(char *args){
	if(args==NULL){
		printf("Please input the expression to calculate\n");
		return 0;
	}
	bool success=true;
	int result=expr(args,&success);
	if(!success)
		printf("Illegal\n");
	else printf("%#10x\t%d\n",result,result);
	return 0;
}

static int cmd_w(char *args){
	new_wp(args);
	return 0;
}

static int cmd_d(char *args){
	int n=0;
	if(sscanf(args,"%d",&n)==EOF)
		printf("Please input the right argument!\n");
	else
	{
		del_wp(n);
	}
	return 0;
}

static int cmd_help(char *args);


static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "Step into implementation of N instructions after the suspension of execution. When N is not given, the default is 1.", cmd_si},
	{ "info", "r for print register state, w for print watchpoint information.", cmd_info},
	{ "p", "Expression evaluation", cmd_p},
	{ "x", "Calculate the value of the expression and regard the result as the starting memory address.", cmd_x},
	{ "w", "Stop the execution of the program if the result of the expression has changed.", cmd_w},
	{ "d", "Delete the Nth watchpoint", cmd_d},

	/* TODO: Add more commands */
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
