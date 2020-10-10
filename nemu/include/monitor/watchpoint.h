#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"
#define MAX_LENGTH 100

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	/* TODO: Add more members if necessary */
    char expr[MAX_LENGTH];
	int value;

} WP;

void init_wp_pool();

WP* new_wp(char *args);

void free_wp(WP* wp);

void del_wp(int index);

bool check_wp();

void print_wp();

#endif
