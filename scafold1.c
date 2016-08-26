#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "parser.h"
#include "ev_process.h"

#define NAME_LENGTH 256

#define INVALID (EV_CNT + 1)
#define EV_STACK_CNT 10
static int virt_dev = -1;
static struct input_event ev_stack[EV_STACK_CNT];
static int problems = 0;
void init_ev_stack(void)
{
  int i;
  for(i = 0; i < EV_STACK_CNT; ++i){
    ev_stack[i].type = INVALID;
  }
}

int ev_stack_add(struct input_event *ev)
{
  int i;
  for(i = 0; i < EV_STACK_CNT; ++i){
    if(ev_stack[i].type == INVALID){
      printf("Passing event(%d, %d, %d)\n", ev->type, ev->code, ev->value);
      ev_stack[i] = *ev;
      return sizeof(struct input_event);
    }
  }
  printf("There is not enough space in the evnet stack!\n");
  ++problems;
  return -1;
}

bool ev_equal(struct input_event *ev1, struct input_event *ev2)
{
  return (ev1->type == ev2->type) && (ev1->code == ev2->code) && (ev1->value == ev2->value);
}

bool ev_stack_check(struct input_event *ev)
{
  int i;
  for(i = 0; i < EV_STACK_CNT; ++i){
    if((ev_stack[i].type != INVALID) && (ev_equal(ev, &(ev_stack[i])))){
      ev_stack[i].type = INVALID;
      return true;
    }
  }
  printf("There is no similar event!\n");
  printf("  Expected: (%d, %d, %d)\n", ev->type, ev->code, ev->value);
  for(i = 0; i < EV_STACK_CNT; ++i){
    if(ev_stack[i].type != INVALID){
      printf("  On stack: (%d, %d, %d)\n", ev_stack[i].type, ev_stack[i].code, ev_stack[i].value);
    }
  }
  init_ev_stack();
  ++problems;
  return true;
}

bool ev_stack_check_empty(void)
{
  int i;
  bool empty = true;
  for(i = 0; i < EV_STACK_CNT; ++i){
    if(ev_stack[i].type != INVALID){
      empty = false;
    }
  }
  if(empty){
    return true;
  }
  printf(">>> Stack not empty!\n");
  for(i = 0; i < EV_STACK_CNT; ++i){
    if(ev_stack[i].type != INVALID){
      printf("  On stack: (%d, %d, %d)\n", ev_stack[i].type, ev_stack[i].code, ev_stack[i].value);
    }
  }
  init_ev_stack();
  ++problems;
  return false;
}

struct input_event* create_event(struct input_event *ev, int type, int code, int value)
{
  ev->type = type;
  ev->code = code;
  ev->value = value;
  return ev;
}

int send_event(struct input_event *ev)
{
  if(virt_dev < 0){
    printf("NULL virt_dev.\n");
    return -1;
  }
  //int res = write(virt_dev, &ev, sizeof(struct input_event));
  int res = ev_stack_add(ev);
  if(res < 0){
    printf("Problem sending out an event!\n");
  }
  return res;
}

void print_btns(void)
{
  int i;
  printf("R:");
  for(i = 0; i < 80; ++i){
    printf("%d", config.real_btn_array[i]);
  }
  printf("\n");
  printf("C:");
  for(i = 0; i < 80; ++i){
    printf("%d", config.virtual_btn_array[i]);
  }
  printf("\n");
}



int main(int argc, char *argv[])
{
  (void) argv;
  (void) argc;
  init_ev_stack();
  virt_dev = 1;
  printf("Hello world!\n");
  parse_config("test.conf");
  t_name_def *names = config.ctrl_maps;
  while(names){
    if(names->type == BUTTON){
      config.real_btn_array[names->val] = 1;
    }else{
      config.axes[names->val].center = 0;
      config.axes[names->val].hysteresis = 0.25;
      config.axes[names->val].current_state = INACTIVE;
      config.axes[names->val].ignore = false;
      config.axes[names->val].minimum = -1;
      config.axes[names->val].maximum = 1;
      config.axis_array[names->val] = 1;
    }
    names = names->next;
  }
  sort_out_buttons();
  print_config();
  struct input_event ev, res1;

  int i;
  for(i = 0; i < BUTTON_ARRAY_LEN; ++i){
    config.real_btn_array[i] = 0;
    config.virtual_btn_array[i] = 0;
  }

  int btn_cond_trigger = 292;
  int btn_cond_thumb = 293;
  int cond_ax_x_neg = 290;
  int cond_ax_x_pos = 291;
  int ax_y_neg = 296;
  int ax_y_pos = 297;
  int ax_hat0x_neg = 294;
  int ax_hat0x_pos = 295;
  int cond_ax_hat0x_neg = 288;
  int cond_ax_hat0x_pos = 289;
  int cond_ax_rz = 3;

  printf("===========================================================\n");
  //Nothing
  process_event(create_event(&ev, EV_REL, 0, 0));
  process_event(create_event(&ev, EV_KEY, 1024 ,1));
  process_event(create_event(&ev, EV_ABS, 200 ,1));
  ev_stack_check_empty();
  printf("===========================================================\n");
  //SYN
  process_event(create_event(&ev, EV_SYN, 0, 0));
  ev_stack_check(create_event(&res1, EV_SYN, 0, 0));
  ev_stack_check_empty();
  printf("===========================================================\n");
  //Just a button press/release
  process_event(create_event(&ev, EV_KEY, BTN_THUMB, 1));
  ev_stack_check(create_event(&res1, EV_KEY, BTN_THUMB, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_THUMB, 0));
  ev_stack_check(create_event(&res1, EV_KEY, BTN_THUMB, 0));
  ev_stack_check_empty();
  printf("===========================================================\n");
  //condition activation/deactivation
  process_event(create_event(&ev, EV_KEY, BTN_TOP, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_TOP, 0));
  ev_stack_check_empty();

  printf("===========================================================\n");
  printf("//      testing of contition (buttons only)\n");
  printf("//  Pressing/releasing trigger outside the condition\n");
  process_event(create_event(&ev, EV_KEY, BTN_TRIGGER, 1));
  ev_stack_check(create_event(&res1, EV_KEY, 288, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_TRIGGER, 0));
  ev_stack_check(create_event(&res1, EV_KEY, 288, 0));
  ev_stack_check_empty();

  printf("//  Activate condition\n");
  process_event(create_event(&ev, EV_KEY, BTN_TOP, 1));
  ev_stack_check_empty();

  printf("//  Pressing/releasing a button not handled by this condition\n");
  process_event(create_event(&ev, EV_KEY, BTN_BASE4, 1));
  ev_stack_check(create_event(&res1, EV_KEY, BTN_BASE4, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_BASE4, 0));
  ev_stack_check(create_event(&res1, EV_KEY, BTN_BASE4, 0));
  ev_stack_check_empty();

  printf("//  Pressing/releasing a button handled by this condition\n");
  process_event(create_event(&ev, EV_KEY, BTN_TRIGGER, 1));
  ev_stack_check(create_event(&res1, EV_KEY, btn_cond_trigger, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_TRIGGER, 0));
  ev_stack_check(create_event(&res1, EV_KEY, btn_cond_trigger, 0));
  ev_stack_check_empty();

  printf("//  Deactivate the condition\n");
  process_event(create_event(&ev, EV_KEY, BTN_TOP, 0));
  ev_stack_check_empty();
  printf("===========================================================\n");

//Axes
  printf("===========================================================\n");
  //Axis test (passthrough)
  //  Normal movement
  process_event(create_event(&ev, EV_ABS, ABS_Z, 1.0));
  ev_stack_check(create_event(&res1, EV_ABS, ABS_Z, 1.0));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_Z, 0.0));
  ev_stack_check(create_event(&res1, EV_ABS, ABS_Z, 0.0));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_Z, -1.0));
  ev_stack_check(create_event(&res1, EV_ABS, ABS_Z, -1.0));
  ev_stack_check_empty();
  //  Directly to the other extreme
  process_event(create_event(&ev, EV_ABS, ABS_Z, 1.0));
  ev_stack_check(create_event(&res1, EV_ABS, ABS_Z, 1.0));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_Z, -1.0));
  ev_stack_check(create_event(&res1, EV_ABS, ABS_Z, -1.0));
  ev_stack_check_empty();
  //  Back to zero
  process_event(create_event(&ev, EV_ABS, ABS_Z, 0.0));
  ev_stack_check(create_event(&res1, EV_ABS, ABS_Z, 0.0));
  ev_stack_check_empty();

  printf("===========================================================\n");
  //Axis test (mapped)
  //  Normal movement
  process_event(create_event(&ev, EV_ABS, ABS_Y, 1));
  ev_stack_check(create_event(&res1, EV_KEY, ax_y_pos, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_Y, 0));
  ev_stack_check(create_event(&res1, EV_KEY, ax_y_pos, 0));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_Y, -1));
  ev_stack_check(create_event(&res1, EV_KEY, ax_y_neg, 1));
  ev_stack_check_empty();
  //  Directly to the other extreme
  process_event(create_event(&ev, EV_ABS, ABS_Y, 1));
  ev_stack_check(create_event(&res1, EV_KEY, ax_y_neg, 0));
  ev_stack_check(create_event(&res1, EV_KEY, ax_y_pos, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_Y, -1));
  ev_stack_check(create_event(&res1, EV_KEY, ax_y_neg, 1));
  ev_stack_check(create_event(&res1, EV_KEY, ax_y_pos, 0));
  ev_stack_check_empty();
  //  Back to zero
  process_event(create_event(&ev, EV_ABS, ABS_Y, 0));
  ev_stack_check(create_event(&res1, EV_KEY, ax_y_neg, 0));
  ev_stack_check_empty();

  printf("===========================================================\n");
  //Axis test (mapped)
  //  Normal movement (outside condition)
  process_event(create_event(&ev, EV_ABS, ABS_X, 1));
  ev_stack_check(create_event(&res1, EV_ABS, ABS_X, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_X, 0));
  ev_stack_check(create_event(&res1, EV_ABS, ABS_X, 0));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_X, -1));
  ev_stack_check(create_event(&res1, EV_ABS, ABS_X, -1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_X, 0));
  ev_stack_check(create_event(&res1, EV_ABS, ABS_X, 0));
  ev_stack_check_empty();

  //  Normal movement (inside condition)
  process_event(create_event(&ev, EV_KEY, BTN_TOP2, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_X, 1));
  ev_stack_check(create_event(&res1, EV_KEY, cond_ax_x_pos, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_X, 0));
  ev_stack_check(create_event(&res1, EV_KEY, cond_ax_x_pos, 0));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_X, -1));
  ev_stack_check(create_event(&res1, EV_KEY, cond_ax_x_neg, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_X, 0));
  ev_stack_check(create_event(&res1, EV_KEY, cond_ax_x_neg, 0));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_TOP2, 0));
  ev_stack_check(create_event(&ev, EV_ABS, ABS_X, 0));
  ev_stack_check_empty();

  printf("===========================================================\n");
  //Button changing throughout the condition
  process_event(create_event(&ev, EV_KEY, BTN_TRIGGER, 1));
  ev_stack_check(create_event(&res1, EV_KEY, BTN_TRIGGER, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_TOP, 1));
  ev_stack_check(create_event(&res1, EV_KEY, btn_cond_trigger, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_TRIGGER, 0));
  ev_stack_check(create_event(&res1, EV_KEY, btn_cond_trigger, 0));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_TOP, 0));
  ev_stack_check(create_event(&res1, EV_KEY, BTN_TRIGGER, 0));
  ev_stack_check_empty();

  printf("===========================================================\n");
  //Axis changing throughout the condition
  process_event(create_event(&ev, EV_ABS, ABS_X, 1));
  ev_stack_check(create_event(&res1, EV_ABS, ABS_X, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_TOP2, 1));
  ev_stack_check(create_event(&res1, EV_KEY, cond_ax_x_pos, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_X, -1));
  ev_stack_check(create_event(&res1, EV_KEY, cond_ax_x_pos, 0));
  ev_stack_check(create_event(&res1, EV_KEY, cond_ax_x_neg, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_TOP2, 0));
  ev_stack_check(create_event(&res1, EV_ABS, ABS_X, -1));
  ev_stack_check_empty();

  printf("===========================================================\n");
  //Axis changing throughout the condition
  process_event(create_event(&ev, EV_ABS, ABS_HAT0X, 1));
  ev_stack_check(create_event(&res1, EV_KEY, ax_hat0x_pos, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_BASE6, 1));
  ev_stack_check(create_event(&res1, EV_KEY, cond_ax_hat0x_pos, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_HAT0X, -1));
  ev_stack_check(create_event(&res1, EV_KEY, cond_ax_hat0x_pos, 0));
  ev_stack_check(create_event(&res1, EV_KEY, cond_ax_hat0x_neg, 1));
  ev_stack_check_empty();
  
  process_event(create_event(&ev, EV_KEY, BTN_BASE6, 0));
  ev_stack_check(create_event(&res1, EV_KEY, ax_hat0x_pos, 0));
  ev_stack_check(create_event(&res1, EV_KEY, ax_hat0x_neg, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_BASE6, 1));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_ABS, ABS_HAT0X, 1));
  ev_stack_check(create_event(&res1, EV_KEY, cond_ax_hat0x_pos, 1));
  ev_stack_check(create_event(&res1, EV_KEY, cond_ax_hat0x_neg, 0));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_BASE6, 0));
  ev_stack_check(create_event(&res1, EV_KEY, ax_hat0x_pos, 1));
  ev_stack_check(create_event(&res1, EV_KEY, ax_hat0x_neg, 0));
  ev_stack_check_empty();

  printf("===========================================================\n");
  // new axis test
  process_event(create_event(&ev, EV_ABS, ABS_RZ, 123));
  ev_stack_check(create_event(&res1, EV_ABS, ABS_RZ, 123));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_BASE5, 1));
  ev_stack_check(create_event(&res1, EV_ABS, cond_ax_rz, 123));
  ev_stack_check_empty();
  
  process_event(create_event(&ev, EV_ABS, ABS_RZ, 13));
  ev_stack_check(create_event(&res1, EV_ABS, cond_ax_rz, 13));
  ev_stack_check_empty();

  process_event(create_event(&ev, EV_KEY, BTN_BASE5, 0));
  ev_stack_check(create_event(&res1, EV_ABS, ABS_RZ, 13));
  ev_stack_check_empty();

  if(problems > 0){
    printf("%d problems found!\n", problems);
  }else{
    printf("Tests passed\n");
  }
  return 0;
}
