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
    printf("Unable to find control named '%s'.\n", ctrl_name);
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


int get_free_control(int ctrl_array[], size_t len, t_ctrl_type ctrl_type)
{
  size_t i;
  for(i = 0; i < len; ++i){
    if(ctrl_array[i] == 0){
      return add_used_ctrl(ctrl_array, len, i, ctrl_type);
    }
  }
  return -1;
}


int get_free_button(int btn_array[])
{
  int res = get_free_control(btn_array, BUTTON_ARRAY_LEN, BUTTON);
  if(res < 0){
    return res;
  }
  return res + BUTTON_MIN;
}

int get_free_axis(int axis_array[])
{
  return get_free_control(axis_array, ABS_CNT, AXIS);
}


bool mark_available_button(int btn_array[], int button)
{
  return mark_ctrl(btn_array, BUTTON_ARRAY_LEN, button, BUTTON_MIN, BUTTON_MAX, 0);
}

bool mark_ctrl(int ctrl_array[], size_t len, int ctrl, int min, int max, int val)
{
  if((ctrl < BUTTON_MIN) || (ctrl > BUTTON_MAX)){
    printf("Can't add button with Id %d; only values between %d and %d are alowed.\n", 
      ctrl, BUTTON_MIN, BUTTON_MAX);
    return false;
  }
  ctrl_array[ctrl - BUTTON_MIN] = val;
  return true;
}




int add_used_ctrl(int ctrl_array[], size_t len, unsigned int ctrl, t_ctrl_type ctrl_type)
{
  if(ctrl > len - 1){
    if(ctrl_type == BUTTON){
      printf("Button Id. %d out of bounds(%d, %d inclusive)!", ctrl, BUTTON_MIN, BUTTON_MAX - 1);
    }else{
      printf("Axis Id. %d out of bounds(%d, %d inclusive)!", ctrl, ABS_MIN, ABS_MAX);
    }
    return -1;
  }
  if(ctrl_array[ctrl] != 0){
    if(ctrl_type == BUTTON){
      printf("Button Id. %d is already taken!", ctrl);
    }else{
      printf("Axis Id. %d is already taken!", ctrl);
    }
    return -2;
  }
  ctrl_array[ctrl] = -1;
  return ctrl;
}



bool sort_out_buttons(void)
{
  //start by copying the real buttons to virtual ones
  for(int i = 0; i < BUTTON_ARRAY_LEN; ++i){
    if(config.virtual_btn_array[i] == 0){
      config.virtual_btn_array[i] = config.real_btn_array[i];
    } else {
      if(config.real_btn_array[i] == -1){
        printf("Button %d on the source device in the avoid spec.\nIgnoring the avoid spec.\n", BUTTON_MIN + i);
      }
    }
  }

  //free the ones used in conditions (they won't propagate to the virtual device),
  //  except for the buttons both in the condition and the condition's body
  //  (when user wants to know the modifier is pressed)
  t_state *p_state = config.state;
  while(p_state){
    bool propagate = false;
    t_op *p_op = p_state->ops;
    while(p_op){
      if(p_op->source->val == p_state->condition->val){
        propagate = true;
        p_op->map.target = add_used_ctrl(config.virtual_btn_array, BUTTON_ARRAY_LEN, p_op->source->val, BUTTON);
      }
      p_op = p_op->next;
    }
    if(!propagate){
      mark_available_button(config.virtual_btn_array, p_state->condition->val);
    }
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
          if(p_op->map.target == 0){
            p_op->map.target = get_free_button(config.virtual_btn_array);
          }
          break;
        case AXIS:
          if(p_op->map_type == AXIS_2_BUTTON){
            config.axes[p_op->source->val].ignore = true;
            p_op->map.axis_map.button_neg = get_free_button(config.virtual_btn_array);
            p_op->map.axis_map.button_pos = get_free_button(config.virtual_btn_array);
          }else{
            p_op->map.target = get_free_axis(config.axis_array);
            config.axes[p_op->map.target] = config.axes[p_op->source->val];
          }
          break;
      }
      p_op = p_op->next;
    }
    p_state = p_state->next;
  }
  //Assign axes
  p_op = config.axis_maps;
  while(p_op){
    if(p_op->map_type == AXIS_2_BUTTON){
    config.axes[p_op->source->val].ignore = true;
    p_op->map.axis_map.button_neg = get_free_button(config.virtual_btn_array);
    p_op->map.axis_map.button_pos = get_free_button(config.virtual_btn_array);
    }else{
      printf("Unconditional axis-to-axis mapping ignored.\n");
    }
    p_op = p_op->next;
  }

  return true;
}


