#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include <stdlib.h>
#include "sixaxis.h"
#include <math.h>
#include "conversion.h"
#include <unistd.h>
#include <sys/time.h>

extern struct sixaxis_state state[MAX_CONTROLLERS];
extern s_controller controller[MAX_CONTROLLERS];
extern int display;

extern int joystickNbButton[255];
extern int joystickSixaxis[MAX_DEVICES];

extern int current_mouse;
extern e_current_cal current_cal;

extern double axis_scale;
extern double frequency_scale;
extern int mean_axis_value;
extern int refresh;
extern int postpone_count;

/*
 * This tells what's the current config of each controller.
 */
static int current_config[MAX_CONTROLLERS];

/*
 * This tells what's the next config of each controller.
 */
static int next_config[MAX_CONTROLLERS];
static int config_delay[MAX_CONTROLLERS];

/*
 * This tells what's the previous config of each controller.
 * Hackish way to switch back to previous config.
 */
static int previous_config[MAX_CONTROLLERS];

/*
 * This lists config triggers for all controllers.
 */
s_trigger triggers[MAX_CONTROLLERS][MAX_CONFIGURATIONS];

/*
 * This lists controller stick intensity modifiers.
 */
s_intensity left_intensity[MAX_CONTROLLERS][MAX_CONFIGURATIONS];
s_intensity right_intensity[MAX_CONTROLLERS][MAX_CONFIGURATIONS];

/*
 * This lists controls of each controller configuration for all keyboards.
 */
s_mapper* keyboard_buttons[MAX_DEVICES][MAX_CONTROLLERS][MAX_CONFIGURATIONS];

/*
 * This lists controls of each controller configuration for all mice.
 */
s_mapper* mouse_buttons[MAX_DEVICES][MAX_CONTROLLERS][MAX_CONFIGURATIONS];

s_mapper* mouse_axis[MAX_DEVICES][MAX_CONTROLLERS][MAX_CONFIGURATIONS];

/*
 * Used to tweak mouse controls.
 */
s_mouse_control mouse_control[MAX_DEVICES];

/*
 * Used to calibrate mouse controls.
 */
s_mouse_cal mouse_cal[MAX_DEVICES][MAX_CONFIGURATIONS] = {};

/*
 * This lists controls of each controller configuration for all joysticks.
 */
s_mapper* joystick_buttons[MAX_DEVICES][MAX_CONTROLLERS][MAX_CONFIGURATIONS];
s_mapper* joystick_axis[MAX_DEVICES][MAX_CONTROLLERS][MAX_CONFIGURATIONS];

/*
 * This updates the stick according to the intensity shape.
 */
static void update_stick(s_intensity* intensity, int control, int ts)
{
  int value = intensity->value;

  if (intensity->shape == E_SHAPE_CIRCLE)
  {
    if (state[control].user.axis[ts][0] && state[control].user.axis[ts][1])
    {
      value = sqrt(value * value / 2);
    }
  }

  if (state[control].user.axis[ts][0] > 0)
  {
    state[control].user.axis[ts][0] = round(value);
    controller[control].send_command = 1;
  }
  else if (state[control].user.axis[ts][0] < 0)
  {
    state[control].user.axis[ts][0] = -round(value);
    controller[control].send_command = 1;
  }
  if (state[control].user.axis[ts][1] > 0)
  {
    state[control].user.axis[ts][1] = round(value);
    controller[control].send_command = 1;
  }
  else if (state[control].user.axis[ts][1] < 0)
  {
    state[control].user.axis[ts][1] = -round(value);
    controller[control].send_command = 1;
  }

}

/*
 * Update a stick intensity.
 */
static int update_intensity(s_intensity* intensity, int device_type, int device_id, int button, int control, int config, int ts)
{
  int ret = 0;

  if (intensity->device_up_type == device_type && device_id == intensity->device_up_id && button == intensity->up_button)
  {
    intensity->value += intensity->step;
    if (intensity->value > mean_axis_value)
    {
      if (intensity->down_button != -1)
      {
        intensity->value = mean_axis_value;
      }
      else
      {
        intensity->value = (double) intensity->dead_zone
            + intensity->step;
      }
    }
    update_stick(intensity, control, ts);
    ret = 1;
  }
  else if (intensity->device_down_type == device_type && device_id == intensity->device_down_id && button == intensity->down_button)
  {
    intensity->value -= intensity->step;
    if (intensity->value < intensity->dead_zone + intensity->step)
    {
      if (intensity->up_button != -1)
      {
        intensity->value = (double) intensity->dead_zone + intensity->step;
      }
      else
      {
        intensity->value = mean_axis_value;
      }
    }
    update_stick(intensity, control, ts);
    ret = 1;
  }


  return ret;
}

/*
 * Check if stick intensities need to be updated.
 */
void intensity_lookup(SDL_Event* e)
{
  int i;
  int device_type;
  int button_id;
  unsigned int device_id = ((SDL_KeyboardEvent*)e)->which;

  switch( e->type )
  {
    case SDL_JOYBUTTONDOWN:
      device_type = E_DEVICE_TYPE_JOYSTICK;
      button_id = e->jbutton.button;
      break;
    case SDL_KEYDOWN:
      device_type = E_DEVICE_TYPE_KEYBOARD;
      button_id = e->key.keysym.sym;
      break;
    case SDL_MOUSEBUTTONDOWN:
      device_type = E_DEVICE_TYPE_MOUSE;
      button_id = e->button.button;
      break;
    default:
      return;
  }

  for(i=0; i<MAX_CONTROLLERS; ++i)
  {
    if(update_intensity(&left_intensity[i][current_config[i]], device_type, device_id, button_id, i, current_config[i], 0))
    {
      if (display)
      {
        printf("controller %d configuration %d left intensity: %.0f\n", i, current_config[i], left_intensity[i][current_config[i]].value);
      }
    }
    if(update_intensity(&right_intensity[i][current_config[i]], device_type, device_id, button_id, i, current_config[i], 1))
    {
      if (display)
      {
        printf("controller %d configuration %d left intensity: %.0f\n", i, current_config[i], right_intensity[i][current_config[i]].value);
      }
    }
  }
}

/*
 * Check if current configurations of controllers need to be updated.
 */
void trigger_lookup(SDL_Event* e)
{
  int i, j;
  int device_type;
  int button;
  int up = 0;
  unsigned int device_id = ((SDL_KeyboardEvent*)e)->which;
  int selected;
  int current;

  switch( e->type )
  {
    case SDL_JOYBUTTONUP:
      up = 1;
    case SDL_JOYBUTTONDOWN:
      device_type = E_DEVICE_TYPE_JOYSTICK;
      button = e->jbutton.button;
      break;
    case SDL_KEYUP:
      up = 1;
    case SDL_KEYDOWN:
      device_type = E_DEVICE_TYPE_KEYBOARD;
      button = e->key.keysym.sym;
      break;
    case SDL_MOUSEBUTTONUP:
      up = 1;
    case SDL_MOUSEBUTTONDOWN:
      device_type = E_DEVICE_TYPE_MOUSE;
      button = e->button.button;
      break;
    default:
      return;
  }

  for(i=0; i<MAX_CONTROLLERS; ++i)
  {
    selected = -1;
    if(next_config[i] > -1)
    {
      current = next_config[i];
    }
    else
    {
      current = current_config[i];
    }
    for(j=0; j<MAX_CONFIGURATIONS; ++j)
    {
      if (triggers[i][j].device_type != device_type || device_id
          != triggers[i][j].device_id)
      {
        continue;
      }
      if (button == triggers[i][j].button)
      {
        if (!up)
        {
          if(current == j)
          {
            continue;
          }
          if(selected < 0)
          {
            selected = j;
          }
          if(selected < current && j > current)
          {
            selected = j;
          }
        }
        else if(triggers[i][j].switch_back)
        {
          selected = previous_config[i];
          break;
        }
      }
    }
    if(selected > -1)
    {
      next_config[i] = selected;
      config_delay[i] = triggers[i][selected].delay / (refresh / 1000);
      break;
    }
  }
}

/*
 * Check if a config activation has to be performed.
 */
void config_activation()
{
  int i;
  struct timeval tv;

  for(i=0; i<MAX_CONTROLLERS; ++i)
  {
    if(next_config[i] > -1)
    {
      if(!config_delay[i])
      {
        if(display)
        {
          gettimeofday(&tv, NULL);

          printf("%d %ld.%06ld controller %d is switched from configuration %d to %d\n", i, tv.tv_sec, tv.tv_usec, i, current_config[i], next_config[i]);
        }
        previous_config[i] = current_config[i];
        current_config[i] = next_config[i];
        update_stick(&left_intensity[i][current_config[i]], i, 0);
        update_stick(&right_intensity[i][current_config[i]], i, 1);
        next_config[i] = -1;
      }
      else
      {
        config_delay[i]--;
      }
    }
  }
}

/*
 * Specific stuff to postpone some SDL_MOUSEBUTTONUP events
 * that come too quickly after corresponding SDL_MOUSEBUTTONDOWN events.
 * If we don't do that, the PS3 will miss events.
 * 
 * This function also postpones mouse button up events in case a delayed config toggle is triggered.
 */
static int postpone_event(unsigned int device, SDL_Event* event)
{
  int i;
  int ret = 0;
  if (event->button.button == SDL_BUTTON_WHEELUP)
  {
    if (mouse_control[device].postpone_wheel_up < postpone_count)
    {
      SDL_PushEvent(event);
      mouse_control[device].postpone_wheel_up++;
      ret = 1;
    }
    else
    {
      mouse_control[device].postpone_wheel_up = 0;
    }
  }
  else if (event->button.button == SDL_BUTTON_WHEELDOWN)
  {
    if (mouse_control[device].postpone_wheel_down < postpone_count)
    {
      SDL_PushEvent(event);
      mouse_control[device].postpone_wheel_down++;
      ret = 1;
    }
    else
    {
      mouse_control[device].postpone_wheel_down = 0;
    }
  }
  else if (event->button.button == SDL_BUTTON_X1)
  {
    if (mouse_control[device].postpone_button_x1 < postpone_count)
    {
      SDL_PushEvent(event);
      mouse_control[device].postpone_button_x1++;
      ret = 1;
    }
    else
    {
      mouse_control[device].postpone_button_x1 = 0;
    }
  }
  else if (event->button.button == SDL_BUTTON_X2)
  {
    if (mouse_control[device].postpone_button_x2 < postpone_count)
    {
      SDL_PushEvent(event);
      mouse_control[device].postpone_button_x2++;
      ret = 1;
    }
    else
    {
      mouse_control[device].postpone_button_x2 = 0;
    }
  }

  for(i=0; i<MAX_CONTROLLERS; ++i)
  {
    if(next_config[i] > -1)
    {
      if(triggers[i][next_config[i]].device_type == E_DEVICE_TYPE_MOUSE
          && triggers[i][next_config[i]].device_id == device
          && triggers[i][next_config[i]].button == event->button.button)
      {
        SDL_PushEvent(event);
        ret = 1;
        break;
      }
    }
  }

  return ret;
}

static double mouse2axis(int device, struct sixaxis_state* state, int which, double x, double y, int ts, int ts_axis, double exp, double multiplier, int dead_zone, e_shape shape)
{
  double z = 0;
  double dz = dead_zone;
  double motion_residue = 0;
  double ztrunk = 0;
  double val = 0;

  multiplier *= axis_scale;
  dz *= axis_scale;

  if(which == AXIS_X)
  {
    val = x * frequency_scale;
    if(x && y && shape == E_SHAPE_CIRCLE)
    {
      dz = dz*cos(atan(fabs(y/x)));
    }
    if(device == current_mouse && (current_cal == DZX || current_cal == DZS))
    {
      if(val > 0)
      {
        state->user.axis[ts][ts_axis] = dz;
      }
      else
      {
        state->user.axis[ts][ts_axis] = -dz;
      }
      return 0;
    }
  }
  else if(which == AXIS_Y)
  {
    val = y * frequency_scale;
    if(x && y && shape == E_SHAPE_CIRCLE)
    {
      dz = dz*sin(atan(fabs(y/x)));
    }
    if(device == current_mouse && (current_cal == DZY || current_cal == DZS))
    {
      if(val > 0)
      {
        state->user.axis[ts][ts_axis] = dz;
      }
      else
      {
        state->user.axis[ts][ts_axis] = -dz;
      }
      return 0;
    }
  }

  if(val != 0)
  {
    z = multiplier * (val/fabs(val)) * pow(fabs(val), exp);
    /*
     * Subtract the first position to the dead zone (useful for high multipliers).
     */
    dz = dz - multiplier;// * pow(1, exp);
  }

  if(z > 0)
  {
    state->user.axis[ts][ts_axis] = dz + z;
    /*
     * max stick position => no residue
     */
    if(state->user.axis[ts][ts_axis] < mean_axis_value)
    {
      ztrunk = state->user.axis[ts][ts_axis] - dz;
    }
  }
  else if(z < 0)
  {
    state->user.axis[ts][ts_axis] = z - dz;
    /*
     * max stick position => no residue
     */
    if(state->user.axis[ts][ts_axis] > -mean_axis_value)
    {
      ztrunk = state->user.axis[ts][ts_axis] + dz;
    }
  }
  else state->user.axis[ts][ts_axis] = 0;

  if(val != 0 && ztrunk != 0)
  {
    //printf("ztrunk: %.4f\n", ztrunk);
    /*
     * Compute the motion that wasn't applied due to the double to integer conversion.
     */
    motion_residue = (val/fabs(val)) * ( fabs(val) - pow(fabs(ztrunk)/multiplier, 1/exp) );
    if(fabs(motion_residue) < 0.0039)//allow 256 subpositions
    {
      motion_residue = 0;
    }
    //printf("motion_residue: %.4f\n", motion_residue);
  }

  return motion_residue;
}

/*
 * Updates the state table.
 * Too long function, but not hard to understand.
 */
void process_event(SDL_Event* event)
{
  s_mapper* mapper;
  int button;
  int ts;
  unsigned int ts_axis;
  unsigned int config;
  unsigned int c_id;
  unsigned int control;
  unsigned int threshold;
  double multiplier;
  double exp;
  int dead_zone;
  e_shape shape;
  int value = 0;
  unsigned int nb_controls = 0;
  SDL_Event event_jb;
  double mx;
  double my;
  double residue;

  /*
   * 'which' should always be at that place
   * There is no need to check the value, since it's stored as an uint8_t, and MAX_CONTROLLERS is 256.
   */
  unsigned int device = ((SDL_KeyboardEvent*)event)->which;

  for(c_id=0; c_id<MAX_CONTROLLERS; ++c_id)
  {
    config = current_config[c_id];

    nb_controls = 0;

    switch(event->type)
    {
      case SDL_JOYHATMOTION:
      event_jb.jbutton.which = event->jhat.which;
      if(event->jhat.value & SDL_HAT_UP)
      {
        event_jb.jbutton.type = SDL_JOYBUTTONDOWN;
        event_jb.jbutton.button=joystickNbButton[event->jhat.which]+4*event->jhat.hat;
        process_event(&event_jb);
      }
      else
      {
        event_jb.jbutton.type = SDL_JOYBUTTONUP;
        event_jb.jbutton.button=joystickNbButton[event->jhat.which]+4*event->jhat.hat;
        process_event(&event_jb);
      }
      if(event->jhat.value & SDL_HAT_RIGHT)
      {
        event_jb.jbutton.type = SDL_JOYBUTTONDOWN;
        event_jb.jbutton.button=joystickNbButton[event->jhat.which]+4*event->jhat.hat+1;
        process_event(&event_jb);
      }
      else
      {
        event_jb.jbutton.type = SDL_JOYBUTTONUP;
        event_jb.jbutton.button=joystickNbButton[event->jhat.which]+4*event->jhat.hat+1;
        process_event(&event_jb);
      }
      if(event->jhat.value & SDL_HAT_DOWN)
      {
        event_jb.jbutton.type = SDL_JOYBUTTONDOWN;
        event_jb.jbutton.button=joystickNbButton[event->jhat.which]+4*event->jhat.hat+2;
        process_event(&event_jb);
      }
      else
      {
        event_jb.jbutton.type = SDL_JOYBUTTONUP;
        event_jb.jbutton.button=joystickNbButton[event->jhat.which]+4*event->jhat.hat+2;
        process_event(&event_jb);
      }
      if(event->jhat.value & SDL_HAT_LEFT)
      {
        event_jb.jbutton.type = SDL_JOYBUTTONDOWN;
        event_jb.jbutton.button=joystickNbButton[event->jhat.which]+4*event->jhat.hat+3;
        process_event(&event_jb);
      }
      else
      {
        event_jb.jbutton.type = SDL_JOYBUTTONUP;
        event_jb.jbutton.button=joystickNbButton[event->jhat.which]+4*event->jhat.hat+3;
        process_event(&event_jb);
      }
      break;
      case SDL_JOYBUTTONDOWN:
      case SDL_JOYBUTTONUP:
      if(joystick_buttons[device][c_id][config])
      {
        nb_controls = joystick_buttons[device][c_id][config]->nb_mappers;
      }
      break;
      case SDL_JOYAXISMOTION:
      if(joystickSixaxis[device] && event->jaxis.axis > 3)
      {
        event->jaxis.value = (event->jaxis.value + 32767) / 2;
      }
      if(joystick_axis[device][c_id][config])
      {
        nb_controls = joystick_axis[device][c_id][config]->nb_mappers;
      }
      break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
      if(keyboard_buttons[device][c_id][config])
      {
        nb_controls = keyboard_buttons[device][c_id][config]->nb_mappers;
      }
      break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
      if(mouse_buttons[device][c_id][config])
      {
        nb_controls = mouse_buttons[device][c_id][config]->nb_mappers;
      }
      break;
      case SDL_MOUSEMOTION:
      if(mouse_axis[device][c_id][config])
      {
        nb_controls = mouse_axis[device][c_id][config]->nb_mappers;
      }
      break;
    }

    for(control=0; control<nb_controls; ++control)
    {
      switch(event->type)
      {
        case SDL_JOYBUTTONDOWN:
          mapper = joystick_buttons[device][c_id][config]+control;
          /*
           * Check that it's the right button.
           */
          if(!mapper || mapper->button != event->jbutton.button)
          {
            continue;
          }
          controller[c_id].send_command = 1;
          /*
           * Button to button.
           */
          button = mapper->controller_button;
          if(button >= 0)
          {
            state[c_id].user.button[button].pressed = 1;
            state[c_id].user.button[button].value = 255;
          }
          /*
           * Button to axis.
           */
          ts = mapper->controller_thumbstick;
          ts_axis = mapper->controller_thumbstick_axis;
          if(ts >= 0)
          {
            if(mapper->controller_thumbstick_axis_value < 0)
            {
              state[c_id].user.axis[ts][ts_axis] = -mean_axis_value;
            }
            else
            {
              state[c_id].user.axis[ts][ts_axis] = mean_axis_value;
            }
          }
          break;
        case SDL_JOYBUTTONUP:
          mapper = joystick_buttons[device][c_id][config]+control;
          /*
           * Check that it's the right button.
           */
          if(!mapper || mapper->button != event->jbutton.button)
          {
            continue;
          }
          controller[c_id].send_command = 1;
          /*
           * Button to button.
           */
          button = mapper->controller_button;
          if(button >= 0)
          {
            state[c_id].user.button[button].pressed = 0;
            state[c_id].user.button[button].value = 0;
          }
          /*
           * Button to axis.
           */
          ts = mapper->controller_thumbstick;
          ts_axis = mapper->controller_thumbstick_axis;
          if(ts >= 0)
          {
            state[c_id].user.axis[ts][ts_axis] = 0;
          }
          break;
        case SDL_JOYAXISMOTION:
          mapper = joystick_axis[device][c_id][config]+control;
          /*
           * Check that it's the right axis.
           */
          if(!mapper || mapper->axis != event->jaxis.axis)
          {
            continue;
          }
          controller[c_id].send_command = 1;
          /*
           * Axis to button.
           */
          button = mapper->controller_button;
          threshold = mapper->threshold;
          if(button >= 0)
          {
            if(threshold > 0 && event->jaxis.value > threshold)
            {
              state[c_id].user.button[button].pressed = 1;
              state[c_id].user.button[button].value = 255;
            }
            else if(threshold < 0 && event->jaxis.value < threshold)
            {
              state[c_id].user.button[button].pressed = 1;
              state[c_id].user.button[button].value = 255;
            }
            else
            {
              state[c_id].user.button[button].pressed = 0;
              state[c_id].user.button[button].value = 0;
            }
          }
          /*
           * Axis to button axis.
           */
          button = mapper->controller_button_axis;
          multiplier = mapper->multiplier;
          exp = mapper->exponent;
          dead_zone = mapper->dead_zone;
          if(button >= 0)
          {
            value = event->jaxis.value;
            if(value)
            {
              value = value/abs(value)*multiplier*pow(abs(value), exp);
            }
            if(value > 0)
            {
              value += dead_zone;
              state[c_id].user.button[button].pressed = 1;
              state[c_id].user.button[button].value = clamp(0, value , 255);
            }
            else
            {
              state[c_id].user.button[button].pressed = 0;
              state[c_id].user.button[button].value = 0;
            }
          }
          /*
           * Axis to axis.
           */
          ts = mapper->controller_thumbstick;
          ts_axis = mapper->controller_thumbstick_axis;
          multiplier = mapper->multiplier * axis_scale;
          exp = mapper->exponent;
          dead_zone = mapper->dead_zone;
          if(ts >= 0)
          {
            value = event->jaxis.value;
            if(value)
            {
              value = value/abs(value)*multiplier*pow(abs(value), exp);
            }
            if(value > 0)
            {
              value += dead_zone;
            }
            else if(value < 0)
            {
              value -= dead_zone;
            }
            state[c_id].user.axis[ts][ts_axis] = clamp(-mean_axis_value, value , mean_axis_value);
          }
          break;
        case SDL_KEYDOWN:
          mapper = keyboard_buttons[device][c_id][config]+control;
          /*
           * Check that it's the right button.
           */
          if(!mapper || mapper->button != event->key.keysym.sym)
          {
            continue;
          }
          controller[c_id].send_command = 1;
          /*
           * Button to button.
           */
          button = mapper->controller_button;
          if(button >= 0)
          {
            state[c_id].user.button[button].pressed = 1;
            state[c_id].user.button[button].value = 255;
          }
          /*
           * Button to axis.
           */
          ts = mapper->controller_thumbstick;
          ts_axis = mapper->controller_thumbstick_axis;
          if(ts >= 0)
          {
            if(mapper->controller_thumbstick_axis_value < 0)
            {
              if(ts == 0)
              {
                state[c_id].user.axis[ts][ts_axis] = - left_intensity[c_id][config].value;
              }
              else
              {
                state[c_id].user.axis[ts][ts_axis] = - right_intensity[c_id][config].value;
              }
            }
            else
            {
              if(ts == 0)
              {
                state[c_id].user.axis[ts][ts_axis] = left_intensity[c_id][config].value;
              }
              else
              {
                state[c_id].user.axis[ts][ts_axis] = right_intensity[c_id][config].value;
              }
            }
            if(ts == 0)
            {
              update_stick(&left_intensity[c_id][config], c_id, ts);
            }
            else
            {
              update_stick(&left_intensity[c_id][config], c_id, ts);
            }
            /*
             * Specific code for issue 15.
             */
            if(mapper->controller_thumbstick_axis_value > 0)
            {
              controller[c_id].ts_axis[ts][ts_axis][0] = 1;
            }
            else if(mapper->controller_thumbstick_axis_value < 0)
            {
              controller[c_id].ts_axis[ts][ts_axis][1] = 1;
            }
          }
          break;
        case SDL_KEYUP:
          mapper = keyboard_buttons[device][c_id][config]+control;
          /*
           * Check that it's the right button.
           */
          if(!mapper || mapper->button != event->key.keysym.sym)
          {
            continue;
          }
          controller[c_id].send_command = 1;
          /*
           * Button to button.
           */
          button = mapper->controller_button;
          if(button >= 0)
          {
            state[c_id].user.button[button].pressed = 0;
            state[c_id].user.button[button].value = 0;
          }
          /*
           * Button to axis.
           */
          ts = mapper->controller_thumbstick;
          ts_axis = mapper->controller_thumbstick_axis;
          if(ts >= 0)
          {
            state[c_id].user.axis[ts][ts_axis] = 0;
            /*
             * Specific code for issue 15.
             */
            if(mapper->controller_thumbstick_axis_value > 0)
            {
              controller[c_id].ts_axis[ts][ts_axis][0] = 0;
              if(controller[c_id].ts_axis[ts][ts_axis][1] == 1)
              {
                if(mapper->controller_thumbstick_axis_value < 0)
                {
                  state[c_id].user.axis[ts][ts_axis] = mean_axis_value;
                }
                else
                {
                  state[c_id].user.axis[ts][ts_axis] = -mean_axis_value;
                }
              }
            }
            else if(mapper->controller_thumbstick_axis_value < 0)
            {
              controller[c_id].ts_axis[ts][ts_axis][1] = 0;
              if(controller[c_id].ts_axis[ts][ts_axis][0] == 1)
              {
                if(mapper->controller_thumbstick_axis_value < 0)
                {
                  state[c_id].user.axis[ts][ts_axis] = mean_axis_value;
                }
                else
                {
                  state[c_id].user.axis[ts][ts_axis] = -mean_axis_value;
                }
              }
            }
            if(ts == 0)
            {
              update_stick(&left_intensity[c_id][config], c_id, ts);
            }
            else
            {
              update_stick(&left_intensity[c_id][config], c_id, ts);
            }
          }
          break;
        case SDL_MOUSEMOTION:
          mapper = mouse_axis[device][c_id][config]+control;
          /*
           * Check the mouse axis.
           */
          if(!mapper)
          {
            continue;
          }
          else if (mapper->axis == AXIS_X)
          {
            value = event->motion.xrel;
          }
          else if(mapper->axis == AXIS_Y)
          {
            value = event->motion.yrel;
          }
          controller[c_id].send_command = 1;
          /*
           * Axis to button.
           */
          button = mapper->controller_button;
          threshold = mapper->threshold;
          if(button >= 0)
          {
            //TODO: many sixaxis buttons are axis too... value may have other values than 0 and 255!
            if(threshold > 0 && value > threshold)
            {
              state[c_id].user.button[button].pressed = 1;
              state[c_id].user.button[button].value = 255;
            }
            else if(threshold < 0 && value < threshold)
            {
              state[c_id].user.button[button].pressed = 1;
              state[c_id].user.button[button].value = 255;
            }
            else
            {
              state[c_id].user.button[button].pressed = 0;
              state[c_id].user.button[button].value = 0;
            }
          }
          /*
           * Axis to axis.
           */
          ts = mapper->controller_thumbstick;
          ts_axis = mapper->controller_thumbstick_axis;
          multiplier = mapper->multiplier;
          exp = mapper->exponent;
          dead_zone = mapper->dead_zone;
          shape = mapper->shape;
          if(mouse_control[device].change)
          {
            mx = mouse_control[device].merge_x;
            my = mouse_control[device].merge_y;
          }
          else
          {
            mx = 0;
            my = 0;
          }
          if(ts >= 0)
          {
            residue = mouse2axis(device, state+c_id, mapper->axis, mx, my, ts, ts_axis, exp, multiplier, dead_zone, shape);
            if(mapper->axis == AXIS_X)
            {
              mouse_control[device].residue_x = residue;
            }
            else if(mapper->axis == AXIS_Y)
            {
              mouse_control[device].residue_y = residue;
            }
          }
          break;
        case SDL_MOUSEBUTTONDOWN:
          mapper = mouse_buttons[device][c_id][config]+control;
          /*
           * Check that it's the right button.
           */
          if(!mapper || mapper->button != event->button.button)
          {
            continue;
          }
          controller[c_id].send_command = 1;
          /*
           * Button to button.
           */
          button = mapper->controller_button;
          if(button >= 0)
          {
            state[c_id].user.button[button].pressed = 1;
            state[c_id].user.button[button].value = 255;
          }
          /*
           * Button to axis.
           */
          ts = mapper->controller_thumbstick;
          ts_axis = mapper->controller_thumbstick_axis;
          if(ts >= 0)
          {
            if(mapper->controller_thumbstick_axis_value < 0)
            {
              state[c_id].user.axis[ts][ts_axis] = -mean_axis_value;
            }
            else
            {
              state[c_id].user.axis[ts][ts_axis] = mean_axis_value;
            }
          }
          break;
        case SDL_MOUSEBUTTONUP:
          mapper = mouse_buttons[device][c_id][config]+control;
          /*
           * Check that it's the right button.
           */
          if(!mapper || mapper->button != event->button.button)
          {
            continue;
          }
          /*
           * Check if this event needs to be postponed.
           */
          if(postpone_event(device, event))
          {
            return; //no need to do something more
          }
          controller[c_id].send_command = 1;
          /*
           * Button to button.
           */
          button = mapper->controller_button;
          if(button >= 0)
          {
            state[c_id].user.button[button].pressed = 0;
            state[c_id].user.button[button].value = 0;
          }
          /*
           * Button to axis.
           */
          ts = mapper->controller_thumbstick;
          ts_axis = mapper->controller_thumbstick_axis;
          if(ts >= 0)
          {
            state[c_id].user.axis[ts][ts_axis] = 0;
          }
          break;
      }
    }
  }
}
