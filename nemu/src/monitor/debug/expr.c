#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ, NEQ, NUMBER, MINUS, AND, OR, POINTER

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
	{"==", EQ,3}						// equal
	{"-", '-',4} //sub
	{"\\*", '*',5} //mul
	{"/", '/',5} //div
	{"\\t+", NOTYPE,0} //tabs
	{"\\b[0-9]+\\b",NUMBER,0} //numbers
	{"\\(", '(',7} //left bracket
	{"\\)", ')',7} //right bracket
	{"!=",NEQ,3}    //not equal
	{"!",'!',6}     //not
	{"&&",AND,2}
	{"\\|\\|",OR,1}
	
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
} Token;

Token tokens[32];
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
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case NOTYPE:break;
					
					default: 
						tokens[nr_token].type=rules[i].token_type;
						strncpy(tokens[nr_token].str,substr_start,substr_len);
						tokens[nr_token].str[substr_len-1]='\0';
						nr_token++;
				}
				position+=substr_len;
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
	if(token[l].type=='('&&token[r].type==')'){
		int lnum=0,rnum=0;
		for(i=l+1;i<r;i++){
			if(token[i].type=='(')
				lnum++;
			if(token[i].type==')')
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
	int main_op=-1;
	int main_pr=10;
	for(i=l;i<=r;i++){
		if(tokens[i].type==NUMBER)
			continue;
		if(tokens[i].type=='(')
		     find++;
		if(tokens[i].type==')')
		     find--;
		if(find!=0)
			continue;
		if(tokens[i].priority<=main_pr){
			min_pr=tokens[i].priority;
			mian_op=i;
		}
	}
	return main_op;
}

uint32_t eval(int l,int r,bool *legal){
	if(!(*legal))
		return -1;
	if(l>r){
		assert(l>r,"Unkonwn expression calculation error!\n");
		return -1;
	}
	else if(l==r){
		uint32_t num=0;
		if(tokens[i].type==NUMBER)
			sscanf(tokens[i].str,"%d",&num);


		return num;		
	}
	else if(check_parentheses(l,r)==true){
		return eval(l+1,r-1,legal);
	}
	else{
		int op=dominent_operator(l,r);
		if(l==op||tokens[op].type==MINUS){
			uint32_t val eval(l+1,r,legal);
			switch(tokens[i].type){
				case MINUS:return -val;
				case '!':return !val;
				default:*success=false;
						return -1;
			}
		}
		uint32_t val1=eval(l,op-1,legal);
		uint32_t val2=eval(op+1,r,legal);
		switch(tokens[l].type){
			case '+':return val1+val2;
			case '-':return val1-val2;
			case '*':return val1*val2;
			case '/':return val1/val2;
			case EQ: return val1==val2;
			case NEQ:return val1!=val2;
			case AND:return val1&&val2;
			case OR: return val1||val2;
			default:
					*legal=flase;
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
		if(tokens[i].type=='-'&&(i==0||tokens[i-1].type!=NUMBER && token[i-1].type!=')')){
			tokens[i].type=MINUS;
		}
	}
	for(i=0;i<nr_token;i++){
		if(tokens[i].type=='*'&&(i==0||tokens[i-1].type!=NUMBER && tokens[i-1].type!=')')){
			tokens[i].type=POINTER;
		}
	}
	*success=true;
	return eval(0,nr_token-1,success);
}

