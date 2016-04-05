#include <stdio.h>
#include <string.h>
#include "parser.h"

int edslineno;
YYSTYPE edslval;
char *edstext;
const char *parsed_file;
FILE *edsin;
int edslex_destroy(void);

t_config config;

void edserror(char const *s)
{
  printf("Config parser: %s in file %s, line %d near '%s'.\n",
      s, parsed_file, edslineno, edstext);
}

int parse_config(const char *fname)
{
    parsed_file = fname;

    if((edsin = fopen(parsed_file, "r")) == NULL){
      printf("Can't open file '%s'\n", parsed_file);
      return -1;
    }

    printf("Parsing file '%s'.\n", parsed_file);
    //edsdebug = 1;
    int res = edsparse();
    fclose(edsin);
    edsin = NULL;
    edslex_destroy();
    parsed_file = NULL;
    return res;
}


int find_control(const char *ctrl_name, t_ctrl_type *ctrl)
{
    t_name_def *tmp = config.ctrl_maps;
    while(tmp != NULL){
      if(strcmp(ctrl_name, tmp->name) == 0){
        if(ctrl != NULL){
          *ctrl = tmp->type;
        }
        return tmp->val;
      }
      tmp = tmp->next;
    }
    printf("Unable to find control named '%s'.", ctrl_name);
    return -1;
}

void clean_up_name_chain(t_name_def *n)
{
  void *to_free;
  t_name_def *next = n;
  while(next != 0){
    free(next->name);
    to_free = next;
    next = next->next;
    free(to_free);
  }
}

void clean_up_op_chain(t_op *op)
{
  t_op *tmp2 = op;
  void *to_free;
  while(tmp2 != NULL){
    clean_up_name_chain(tmp2->source);
    to_free = (void*) tmp2;
    tmp2 = tmp2->next;
    free(to_free);
  }
}

void clean_up_config(void)
{
  free(config.device);
  clean_up_name_chain(config.ctrl_maps);
  void *to_free;

  t_state *tmp1 = config.state;
  while(tmp1 != NULL){
    clean_up_name_chain(tmp1->condition);
    clean_up_op_chain(tmp1->ops);
    to_free = (void*) tmp1;
    tmp1 = tmp1->next;
    free(to_free);
  }
  clean_up_op_chain(config.axis_maps);
}


int find_free_button(int btn_array[])
{
  size_t i;
  for(i = 0; i < BUTTON_ARRAY_LEN; ++i){
    if(btn_array[i] == 0){
      add_used_button(btn_array, i + BUTTON_MIN, true);
      return i + BUTTON_MIN;
    }
  }
  return -1;
}

bool mark_available_button(int btn_array[], int button)
{
  if((button < BUTTON_MIN) || (button > BUTTON_MAX)){
    printf("Can't add button with Id %d; only values between %d and %d are alowed.\n", 
      button, BUTTON_MIN, BUTTON_MAX);
    return false;
  }
  btn_array[button - BUTTON_MIN] = 0;
  return true;
}

bool add_used_button(int btn_array[], int button, bool unique)
{
  if((button < BUTTON_MIN) || (button > BUTTON_MAX)){
    printf("Can't add button with Id %d; only values between %d and %d are alowed.\n", 
      button, BUTTON_MIN, BUTTON_MAX);
    return false;
  }
  if(unique && (btn_array[button - BUTTON_MIN] != 0)){
    printf("Button %d is already taken!\n", button);
    return false;
  }
  btn_array[button - BUTTON_MIN] = -1;
  return true;
}

bool sort_out_buttons(void)
{
  //start by copying the real buttons to virtual ones
  memcpy(config.virtual_btn_array, config.real_btn_array, sizeof(config.virtual_btn_array));
  //free the ones used in conditions (they won't propagate to the virtual device)
  t_state *p_state = config.state;
  while(p_state){
    mark_available_button(config.virtual_btn_array, p_state->condition->val);
    p_state = p_state->next;
  }
  //Assign virtual buttons
  p_state = config.state;
  t_op *p_op;
  while(p_state){
    p_op = p_state->ops;
    while(p_op){
      switch(p_op->source->type){
        case BUTTON:
          p_op->map.target = find_free_button(config.virtual_btn_array);
          break;
        case AXIS:
          config.axes[p_op->source->val].ignore = true;
          p_op->map.axis_map.button_neg = find_free_button(config.virtual_btn_array);
          p_op->map.axis_map.button_pos = find_free_button(config.virtual_btn_array);
          break;
      }
      p_op = p_op->next;
    }
    p_state = p_state->next;
  }
  //Assign axes
  p_op = config.axis_maps;
  while(p_op){
    config.axes[p_op->source->val].ignore = true;
    p_op->map.axis_map.button_neg = find_free_button(config.virtual_btn_array);
    p_op->map.axis_map.button_pos = find_free_button(config.virtual_btn_array);
    p_op = p_op->next;
  }

  return true;
}


