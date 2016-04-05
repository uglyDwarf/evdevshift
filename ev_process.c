#include <stdio.h>

#include "ops.h"
#include "parser.h"
#include "ev_process.h"

int send_key_event(struct input_event *template, int key, int state)
{
  struct input_event ev = *template;
  ev.type = EV_KEY;
  ev.code = key;
  ev.value = state;
  int res = send_event(&ev);
  if(res > 0){
    config.virtual_btn_array[key - BUTTON_MIN] = state;
  }
  return res;
}

t_axis2btn axis_state(struct input_event *event)
{
  int axis = event->code;
  float val = event->value;
  t_axis_info info = config.axes[axis];
  if(val > (info.center + info.hysteresis)){
    return POSITIVE;
  }
  if(val < (info.center - info.hysteresis)){
    return NEGATIVE;
  }
  return INACTIVE;
}

t_axis2btn axis_state_from_buttons(int neg, int pos)
{
  int n = config.virtual_btn_array[neg - BUTTON_MIN];
  int p = config.virtual_btn_array[pos - BUTTON_MIN];
  if((n == 0) && (p == 1)){
    return POSITIVE;
  }else if((n == 1) && (p == 0)){
    return NEGATIVE;
  }
  return INACTIVE;
}

int handle_axis_change(struct input_event *event, int axis, t_axis2btn new_state, int neg, int pos)
{
  switch(config.axes[axis].current_state){
    case INACTIVE:
      config.axes[axis].current_state = new_state;
      if(new_state == POSITIVE){
        return send_key_event(event, pos, 1);
      }else if(new_state == NEGATIVE){
        return send_key_event(event, neg, 1);
      }
      //break; //unreachable
    case POSITIVE:
      config.axes[axis].current_state = new_state;
      if(new_state == INACTIVE){
        return send_key_event(event, pos, 0);
      }else if(new_state == NEGATIVE){
        int res = send_key_event(event, pos, 0);
        if(res < 0){
          return res;
        }
        return send_key_event(event, neg, 1);
      }
      //break; //unreachable
    case NEGATIVE:
      config.axes[axis].current_state = new_state;
      if(new_state == POSITIVE){
        int res = send_key_event(event, neg, 0);
        if(res < 0){
          return res;
        }
        return send_key_event(event, pos, 1);
      }else if(new_state == INACTIVE){
        return send_key_event(event, neg, 0);
      }
      //break; //unreachable
  }
  return -1; //unreachable
}

bool handle_condition_change(struct input_event *event)
{
  int key = event->code;
  int key_state = event->value;

  //Check that the key is not being used as a condition trigger
  bool condition_triggered = false;
  t_state *state = config.state;
  t_op *op;
  while(state){
    if(key == state->condition->val){
      //handle the condition entry/leave
      op = state->ops;
      while(op){
        if(op->source->type == BUTTON){
          //If the state of the real button differs from the state of the virtual one being mapped to,
          //  send out an event to get them in line
          if(key_state){
            //condition activated
            //  Target button state differs from the last record
            if(config.real_btn_array[op->source->val - BUTTON_MIN] !=
               config.virtual_btn_array[op->map.target - BUTTON_MIN]){
              send_key_event(event, op->map.target, config.real_btn_array[op->source->val - BUTTON_MIN]);
            }
          }else{
            //condition deactivated
            if(config.real_btn_array[op->source->val - BUTTON_MIN] !=
               config.virtual_btn_array[op->source->val - BUTTON_MIN]){
              send_key_event(event, op->source->val, config.real_btn_array[op->source->val - BUTTON_MIN]);
            }
          }
        }else if(op->source->type == AXIS){
          if(key_state){
            //check if axis state changed and act accordingly
            if(config.axes[op->source->val].current_state !=
               axis_state_from_buttons(op->map.axis_map.button_neg, op->map.axis_map.button_pos)){
              t_axis2btn new_state = config.axes[op->source->val].current_state;
              config.axes[op->source->val].current_state = 
                axis_state_from_buttons(op->map.axis_map.button_neg, op->map.axis_map.button_pos);
              handle_axis_change(event, op->source->val, new_state,
                                 op->map.axis_map.button_neg, op->map.axis_map.button_pos);
            }
          }else{
            //leaving the condition
            t_op *ax = config.axis_maps;
            bool has_mapping = false;
            // check if the axis has mapping outside the condition
            while(ax){
              if(ax->source->val != op->source->val){
                //not our axis, not interested
                ax = ax->next;
                continue;
              }
              if(config.axes[ax->source->val].current_state !=
                 axis_state_from_buttons(ax->map.axis_map.button_neg, ax->map.axis_map.button_pos)){
                t_axis2btn new_state = config.axes[op->source->val].current_state;
                config.axes[op->source->val].current_state = 
                  axis_state_from_buttons(ax->map.axis_map.button_neg, ax->map.axis_map.button_pos);
                handle_axis_change(event, ax->source->val, new_state,
                                   ax->map.axis_map.button_neg, ax->map.axis_map.button_pos);
                has_mapping = true;
              }
              ax = ax->next;
            }
            if(!has_mapping){
              struct input_event a = *event;
              a.type = EV_ABS;
              a.code = op->source->val;
              a.value = config.axes[a.code].current_value;
              send_event(&a);
            }
          }
        }
        op = op->next;
      }
      condition_triggered = true;
      break;
    }
    state = state->next;
  }

  return condition_triggered;
}



//need two sets of states real/virtual!!!
bool process_key_event(struct input_event *event)
{
  int key = event->code;
  int key_state = event->value;
  bool handled = false;
  if((key < BUTTON_MIN) || (key > BUTTON_MAX)){
    printf("Button number %d is out of bounds <%d, %d>.\n", key, BUTTON_MIN, BUTTON_MAX);
    return false;
  }
  config.real_btn_array[key - BUTTON_MIN] = key_state;

  //Check if the event doesn't trigger/release any conditions
  if(handle_condition_change(event)){
    return true;
  }
  //Go through all conditions and see if there is an action to perform
  t_state *state = config.state;
  t_op *op;
  while(state){
    if(config.real_btn_array[state->condition->val - BUTTON_MIN]){
      op = state->ops;
      while(op){
        if((op->source->type == BUTTON) && (op->source->val == key)){
          if(send_key_event(event, op->map.target, key_state) > 0){
            handled = true;
          }
        }
        op = op->next;
      }
    }
    state = state->next;
  }
  //If no remapping, send out the original event
  if(!handled){
    send_key_event(event, key, key_state);
  }
  return true;
}

bool process_axis_maps(struct input_event *event, t_op *op, bool *handled)
{
  int axis = event->code;
  while(op){
    if((op->source->type == AXIS) && (op->source->val == axis)){
      t_axis2btn new_state = axis_state(event);
      if(new_state != config.axes[axis].current_state){
        handle_axis_change(event, axis, new_state, op->map.axis_map.button_neg, op->map.axis_map.button_pos);
        if(handled){
          *handled = true;
        }
      }
    }
    op = op->next;
  }
  return true;
}

bool process_axis_event(struct input_event *event)
{
  int axis = event->code;
  float val = event->value;
  if((axis < 0) || (axis > ABS_MAX)){
    printf("Axis Id %d is out of range <0, %d>.\n", axis, ABS_MAX);
    return false;
  }
  config.axes[axis].current_value = val;
  t_state *state = config.state;
  bool handled = false;
  while(state){
    if(config.real_btn_array[state->condition->val - BUTTON_MIN]){
      process_axis_maps(event, state->ops, &handled);
    }
    state = state->next;
  }
  if(handled){
    return true;
  }
  process_axis_maps(event, config.axis_maps, &handled);
  if(handled){
    return true;
  }
  //pass through

  config.axes[axis].current_value = val;
  config.axes[axis].current_state = axis_state(event);
  send_event(event);
  return true;
}

bool process_event(struct input_event *event)
{
  switch(event->type){
    case EV_SYN:
      send_event(event);
      return true;
      break;
    case EV_KEY:
      return process_key_event(event);
      break;
    case EV_ABS:
      return process_axis_event(event);
      break;
    default:
      return false;
      break;
  }
  return false;
}
