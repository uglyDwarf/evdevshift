#ifndef OPS__H
#define OPS__H

#include <linux/input.h>
#include <stdbool.h>

//BTN_* balues from /usr/include/linux/input.h
#define BUTTON_MIN BTN_JOYSTICK
#define BUTTON_MAX  KEY_MAX
#define BUTTON_ARRAY_LEN (BUTTON_MAX - BUTTON_MIN + 1)

#define ABS_MIN ABS_X

typedef enum{
  BUTTON,
  AXIS
} t_ctrl_type;

typedef struct t_name_def t_name_def;

struct t_name_def{
  t_ctrl_type type;
  char *name;
  int val;
  t_name_def *next;
};

typedef struct{
  int button_neg;
  int button_pos;
} t_axis_map;

typedef union{
  //Target/axis button Id
  int target;
  //Target buttons when axis is negative/positive
  t_axis_map axis_map;
} t_map_op;

typedef struct t_op t_op;

typedef enum{
  BUTTON_2_BUTTON,
  AXIS_2_BUTTON,
  AXIS_2_AXIS
} t_map_type;

struct t_op{
  t_name_def *source;
  t_map_type map_type;
  t_map_op map;
  t_op *next;
};

typedef struct t_state t_state;

struct t_state{
  t_name_def *condition;
  t_op *ops;
  t_state *next;
};

typedef enum {
  INACTIVE,
  NEGATIVE,
  POSITIVE
} t_axis2btn;

typedef struct t_axis_info t_axis_info;

struct t_axis_info{
  float hysteresis;
  float center;
  t_axis2btn current_state; // state of the emulated button
  int current_value;
  bool ignore;
  int minimum;
  int maximum;
  int fuzz;
  int flat;
  int res;
  t_axis_info *next;
};

typedef struct{
  //Name of the device as reported by EVIOCGNAME
  char *device;
  //ID of the real device as reported by EVIOCGID
  int vendor;
  int product;
  int version;
  //Axis/button name to Id map
  t_name_def *ctrl_maps;
  //List of all conditions
  t_state *state;
  //List of unconditionaly mapped axess
  t_op *axis_maps;
  //Hardware information
  int real_btn_array[BUTTON_ARRAY_LEN];
  int virtual_btn_array[BUTTON_ARRAY_LEN];
  t_axis_info axes[ABS_CNT];
  int axis_array[ABS_CNT];
  bool grab;
  bool grabbed;
} t_config;


char *find_axis_name(int ctrl);
char *find_button_name(int ctrl);
int find_button_number(const char *name);
void print_config(void);

#endif
