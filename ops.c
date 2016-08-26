#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ops.h"
#include "parser.h"


void print_name_def(t_name_def *v, const char *prefix)
{
  printf("%s%s %s => %d\n", prefix, (v->type == AXIS)?"Axis":"Button", v->name, v->val);
}

void print_op(t_op *op, const char *prefix)
{
  switch(op->source->type){
    case BUTTON:
      printf("%s  Button %s (%d) will be mapped to %d.\n", prefix, op->source->name,
             op->source->val, op->map.target);
      break;
    case AXIS:
      if(op->map_type == AXIS_2_BUTTON){
        printf("%s  Axis %s (%d) will be mapped to buttons %d/%d.\n", prefix, op->source->name,
               op->source->val, op->map.axis_map.button_neg, op->map.axis_map.button_pos);
      }else{
        printf("%s  Axis %s (%d) will be mapped to axis %d.\n", prefix, op->source->name,
               op->source->val, op->map.target);
      }
      break;
  }
}

void print_config(void)
{
  printf("Device: '%s'.\n", config.device);
  t_name_def *tmp = config.ctrl_maps;
  printf("Names defined:\n");
  while(tmp != NULL){
    print_name_def(tmp, "Name def: ");
    tmp = tmp->next;
  }

  t_state *tmp1 = config.state;
  t_op *tmp2;
  while(tmp1 != NULL){
    printf("State:\n");
    print_name_def(tmp1->condition, "      ");
    tmp2 = tmp1->ops;
    while(tmp2 != NULL){
      print_op(tmp2, "      ");
      tmp2 = tmp2->next;
    }
    tmp1 = tmp1->next;
  }

  printf("Axis Maps:\n");
  tmp2 = config.axis_maps;
  while(tmp2 != NULL){
    print_op(tmp2, "      ");
    tmp2 = tmp2->next;
  }
}

#ifdef STANDALONE
int main(int argc, char *argv[])
{
  if(argc != 2){
    printf("Specify a file to parse.\n");
    return 0;
  }

  parse_config(argv[1]);

  print_config();

  clean_up_config();

  return 0;
}
#endif
