%{
  #include <stdio.h>

  #include "parser.h"
  int edslex (void);
  void edserror (char const *);


  extern t_config config;
%}

%debug
%error-verbose

%union {
  char *str;
  int num;
  t_name_def *name;
  t_op *op;
}

%token TOKEN_BUTTON
%token TOKEN_AXIS
%token <str> TOKEN_STRING
%token TOKEN_EQ
%token <num> TOKEN_NUMBER
%token TOKEN_IF
%token TOKEN_LPAREN
%token TOKEN_RPAREN
%token TOKEN_LBRACE
%token TOKEN_RBRACE
%token TOKEN_COMA
%token TOKEN_DEVICE
%token TOKEN_GRAB

%type <name> condition
%type <op> condition_block map_item maps
%type <state> condition_spec

%%
input:          /* empty */
                | input line
;

line:           device_spec
                |name_def
                | axis_map
                | condition_spec
;

device_spec:    TOKEN_DEVICE TOKEN_STRING {
                  config.device = $2;
                  config.grab = false;
                }
                | TOKEN_GRAB TOKEN_DEVICE TOKEN_STRING {
                  config.device = $3;
                  config.grab = true;
                }

name_def:       TOKEN_BUTTON TOKEN_STRING TOKEN_EQ TOKEN_NUMBER {
                  t_name_def *tmp = (t_name_def*)malloc(sizeof(t_name_def));
                  tmp->next = config.ctrl_maps;
                  config.ctrl_maps = tmp;
                  tmp->type = BUTTON;
                  tmp->name = $2;
                  tmp->val = $4;
                }
                | TOKEN_AXIS TOKEN_STRING TOKEN_EQ TOKEN_NUMBER {
                  t_name_def *tmp = (t_name_def*)malloc(sizeof(t_name_def));
                  tmp->next = config.ctrl_maps;
                  config.ctrl_maps = tmp;
                  tmp->type = AXIS;
                  tmp->name = $2;
                  tmp->val = $4;
                }
;

axis_map:       TOKEN_AXIS TOKEN_STRING {
                  t_ctrl_type type;
                  int num = find_control($2, &type);
                  if((num >= 0) && (type == AXIS)){
                    t_name_def *name = (t_name_def*)malloc(sizeof(t_name_def));
                    name->type = AXIS;
                    name->name = $2;
                    name->val = num;
                    name->next = NULL;

                    t_op *op = (t_op*)malloc(sizeof(t_op));
                    op->next = config.axis_maps;
                    op->source = name;
                    op->map.axis_map.button_neg = 0;
                    op->map.axis_map.button_pos = 0;
                    config.axis_maps = op;
                  }else{
                    printf("Axis remapping is available only for axes.\n");
                  }
                }
;

condition_spec: TOKEN_IF condition condition_block {
                  if($3 != NULL){
                    t_state *tmp = (t_state*)malloc(sizeof(t_state));
                    tmp->next = config.state;
                    tmp->condition = $2;
                    tmp->ops = $3;
                    config.state = tmp;
                  }else{
                    free($2->name);
                    free($2);
                  }
                }
;

condition:      TOKEN_LPAREN TOKEN_STRING TOKEN_RPAREN {
                  t_ctrl_type type;
                  int num = find_control($2, &type);
                  if(num < 0){
                    $$ = NULL;
                  }else{
                    t_name_def *name = (t_name_def*)malloc(sizeof(t_name_def));
                    name->type = BUTTON;
                    name->name = $2;
                    name->val = num;
                    name->next = NULL;
                    $$ = name;
                  }
                }
;

condition_block:    TOKEN_LBRACE maps TOKEN_RBRACE {
                      $$ = $2;
                    }
                    | TOKEN_LBRACE TOKEN_RBRACE {
                      $$ = NULL;
                    }
;

maps:           map_item {
                  $$ = $1;
                }
                | maps map_item {
                  $$ = $2;
                  $$->next = $1;
                }
;
map_item:       TOKEN_BUTTON TOKEN_STRING {
                  t_ctrl_type type;
                  int num = find_control($2, &type);
                  if((num < 0) || (type != BUTTON)){
                    free($2);
                    $$ = NULL;
                  }else{
                    t_name_def *name = (t_name_def*)malloc(sizeof(t_name_def));
                    name->type = type;
                    name->name = $2;
                    name->val = num;
                    name->next = NULL;

                    t_op *op = (t_op*)malloc(sizeof(t_op));
                    op->next = NULL;
                    op->source = name;
                    op->map.target = 0;
                    $$ = op;
                  }
                }
                | TOKEN_AXIS TOKEN_STRING /* axis name */ {
                  t_ctrl_type type;
                  int num = find_control($2, &type);
                  if(num < 0){
                    free($2);
                    $$ = NULL;
                  }else{
                    t_name_def *name = (t_name_def*)malloc(sizeof(t_name_def));
                    name->type = type;
                    name->name = $2;
                    name->val = num;
                    name->next = NULL;

                    t_op *op = (t_op*)malloc(sizeof(t_op));
                    op->next = NULL;
                    op->source = name;
                    op->map.axis_map.button_neg = 0;
                    op->map.axis_map.button_pos = 0;
                    $$ = op;
                  }
                }

//                | condition_spec
;
%%
