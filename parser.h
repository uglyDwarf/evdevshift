#ifndef PARSER__H
#define PARSER__H

#include <stdlib.h>
#include <stdbool.h>
#include "ops.h"
#include "config_bison.h"

extern t_config config;
extern int edslineno;
extern YYSTYPE edslval;
extern char *edstext;

int find_control(const char *ctrl_name, t_ctrl_type *ctrl);
int parse_config(const char *fname);
void clean_up_config(void);

int find_free_button(int btn_array[]);
bool mark_available_button(int btn_array[], int button);
int add_used_ctrl(int ctrl_array[], size_t len, unsigned int button, t_ctrl_type ctrl_type);

bool sort_out_buttons(void);

#endif
