#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <elf.h>

enum {
	NOTYPE = 256, EQ, NEQ, NUMBER, MINUS, AND, OR, POINTER, REGISTER, HEX, MARK

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
	int priority;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +", NOTYPE,0},				// spaces
	{"\\+", '+',4},					// plus
	{"==", EQ,3},						// equal
	{"-", '-',4}, //sub
	{"\\*", '*',5}, //mul
	{"/", '/',5}, //div
	{"\\t+", NOTYPE,0}, //tabs
	{"0[xX][0-9a-fA-F]+",HEX,0},     //hex numbers
	{"[0-9]+",NUMBER,0}, //numbers
	{"\\(", '(',7}, //left bracket
	{"\\)", ')',7}, //right bracket
	{"!=",NEQ,3},    //not equal
	{"!",'!',6},     //not
	{"&&",AND,2},    //and
	{"\\|\\|",OR,1}, //or
	{"\\$(eax|EAX|ebx|EBX|ecx|ECX|edx|EDX|esp|ESP|ebp|EBP|esi|ESI|edi|EDI|eip|EIP)",REGISTER,0},
	{"\\$(([ABCD][HLX])|([abcd][hlx]))",REGISTER,0},           //register
	{"[a-zA-Z][A-Za-z0-9_]*",MARK,0}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
	int priority;
} Token;
#define max_token_len 100
Token tokens[max_token_len];
int nr_token;

static bool make_token(char *e) {
	int position = 0;//now position
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				char *start2=e+position+1;
				int substr_len = pmatch.rm_eo;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case NOTYPE:break;
					case REGISTER:
						tokens[nr_token].type=rules[i].token_type;
						tokens[nr_token].priority=rules[i].priority;
						strncpy(tokens[nr_token].str,start2,substr_len-1);
						tokens[nr_token].str[substr_len-1]='\0';
						nr_token++;
						break;
					     
					default: 
						tokens[nr_token].type=rules[i].token_type;
						tokens[nr_token].priority=rules[i].priority;
						strncpy(tokens[nr_token].str,substr_start,substr_len);
						tokens[nr_token].str[substr_len]='\0';
						nr_token++;
						break;
				}
				position+=substr_len;
				//Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

bool check_parentheses(int l,int r){
	int i;
	if(tokens[l].type=='('&&tokens[r].type==')'){
		int lnum=0,rnum=0;
		for(i=l+1;i<r;i++){
			if(tokens[i].type=='(')
				lnum++;
			if(tokens[i].type==')')
				rnum++;
			if(lnum<rnum)
				return false;
		}
		if(lnum==rnum)
			return true;
	}
	return false;
}

//
int dominent_operator(int l,int r){ 
	int i;
	int find=0;
	int main_op=1;
	int main_pr=10;
	for(i=l;i<=r;i++){
		if(tokens[i].type==NUMBER||tokens[i].type==HEX||tokens[i].type==REGISTER||tokens[i].type==MARK)
			continue;
		if(tokens[i].type=='(')
		    find++;
		if(tokens[i].type==')')
		    find--;
		if(find!=0)
			continue;
		if(tokens[i].priority<=main_pr){
			main_pr=tokens[i].priority;
			main_op=i;
		}
	}
	return main_op;
}

uint32_t getAddressFromMArk(char *mark,bool *success);
uint32_t eval(int l,int r,bool *legal){
	if(!(*legal))
		return -1;
	if(l>r){
		Assert(l>r,"Unkonwn expression calculation error!\n");
		return -1;
	}
	else if(l==r){
		uint32_t num=0;
		if(tokens[l].type==NUMBER)
			sscanf(tokens[l].str,"%d",&num);
		else if(tokens[l].type==HEX)
			sscanf(tokens[l].str,"%x",&num);
		else if(tokens[l].type==REGISTER){
			if(strlen(tokens[l].str)==3){
				int i;
				for(i = R_EAX; i <= R_EDI; i ++){
					if(strcmp(tokens[i].str,regsl[i])==0){
						break;
					}
				}
				if(i>R_EDI){
					if(strcmp(tokens[i].str,"eip")==0)
						num=cpu.eip;
					else
						assert("no this register!\n");
				}
				else num=reg_l(i);
			}
			else if(strlen(tokens[l].str)==2){
				if(tokens[l].str[1]=='x'||tokens[l].str[1]=='p'||tokens[l].str[1]=='i'){
					int i;
					for(i = R_AX; i <= R_DI; i ++){
				       if(strcmp(tokens[i].str,regsw[i])==0)
					     break;
					}
					num=reg_w(i);
				}
				if(tokens[l].str[1]=='l'||tokens[l].str[1]=='h'){
					int i;
					for(i = R_AX; i <= R_BH; i ++){
				      if(strcmp(tokens[i].str,regsb[i])==0){
						  break;
					  }
					}
					num=reg_b(i);
				}
				else assert(1);
			}
		}
		else if(tokens[l].type==MARK)
		{
			num=getAddressFromMArk(tokens[l].str,legal);
			if(*legal==false)
			return 0;
		}
		else{
			*legal=false;
			return -1;
		}
		return num;		
	}
	else if(check_parentheses(l,r)==true){
		return eval(l+1,r-1,legal);
	}
	else{
		int op=dominent_operator(l,r);
		if(l==op||tokens[op].type==MINUS||tokens[op].type==POINTER||tokens[op].type=='!'){
			uint32_t val=eval(l+1,r,legal);
			switch(tokens[l].type){
				case POINTER:return swaddr_read(val,4);
				case MINUS:return -val;
				case '!':return !val;
				default:*legal=false;
						return -1;
			}
		}
		uint32_t val1=eval(l,op-1,legal);
		uint32_t val2=eval(op+1,r,legal);
		switch(tokens[op].type){
			case '+':return val1+val2;
			case '-':return val1-val2;
			case '*':return val1*val2;
			case '/':return val1/val2;
			case EQ: return val1==val2;
			case NEQ:return val1!=val2;
			case AND:return val1&&val2;
			case OR: return val1||val2;
			default:*legal=false;
					return -1;
		}
	}
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
	int i;
	for(i=0;i<nr_token;i++){
		if(tokens[i].type=='-'&&(i==0||(tokens[i-1].type!=NUMBER && tokens[i-1].type!=HEX && tokens[i-1].type!=REGISTER && tokens[i-1].type!=MARK && tokens[i-1].type!=')'))){
			tokens[i].type=MINUS;
			tokens[i].priority=6;
		}
		if(tokens[i].type=='*'&&(i==0||(tokens[i-1].type!=NUMBER && tokens[i-1].type!=HEX && tokens[i-1].type!=REGISTER && tokens[i-1].type!=MARK && tokens[i-1].type!=')'))){
			tokens[i].type=POINTER;
			tokens[i].priority=6;
		}
	}
	*success=true;
	return eval(0,nr_token-1,success);
}