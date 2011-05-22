/* Sixaxis emulator

   Copyright (c) 2010 Mathieu Laurendeau

   Copyright (c) 2009 Jim Paris <jim@jtan.com>
   License: GPLv3
*/

#ifndef WIN32
#include <termio.h>
#include <sys/ioctl.h>
#include <err.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pwd.h>
#else
#include <winsock2.h>
#define MSG_DONTWAIT 0
#define sleep Sleep
#endif
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <SDL/SDL.h>
#include "sixaxis.h"
#include "dump.h"
#include "macros.h"
#include "config.h"
#include "config_writter.h"
#include "config_reader.h"
#include <math.h>

#include <pthread.h>
#include <sys/time.h>

#ifndef WIN32
#define SCREEN_WIDTH  8
#define SCREEN_HEIGHT 8
#else
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#endif
#define DEFAULT_MULTIPLIER_STEP 0.01
#define EXPONENT_STEP 0.01
#define EVENT_BUFFER_SIZE 256
#define DEFAULT_POSTPONE_COUNT 3
#define DEFAULT_MAX_AXIS_VALUE 255
#define DEFAULT_AXIS_SCALE 1

static const double pi = 3.14159265;

#ifndef WIN32
char* homedir = "";
#else
char* ip = "";
#endif

char* config_file = NULL;

int refresh = DEFAULT_REFRESH_PERIOD;
int postpone_count = DEFAULT_POSTPONE_COUNT;
int max_axis_value = DEFAULT_MAX_AXIS_VALUE;
int mean_axis_value = DEFAULT_MAX_AXIS_VALUE/2;
double multiplier_step = DEFAULT_MULTIPLIER_STEP;
double axis_scale = DEFAULT_AXIS_SCALE;
double frequency_scale;
int subpos = 0;

int rs232 = 0;
int done = 0;
int current_mouse = 0;
int current_conf = 0;
e_current_cal current_cal = NONE;
int display = 0;
static int lctrl = 0;
static int rctrl = 0;
static int lshift = 0;
static int rshift = 0;
static int lalt = 0;
static int ralt = 0;
static int grab = 1;

SDL_Surface *screen = NULL;

struct sixaxis_state state[MAX_CONTROLLERS];
int (*assemble)(uint8_t *buf, int len, struct sixaxis_state *state);
static int sockfd[MAX_CONTROLLERS];
s_controller controller[MAX_CONTROLLERS] = {};

extern s_mouse_control mouse_control[MAX_DEVICES];
extern s_mouse_cal mouse_cal[MAX_DEVICES][MAX_CONFIGURATIONS];

#ifdef WIN32
static void err(int eval, const char *fmt)
{
    fprintf(stderr, fmt);
    exit(eval);
}
#endif

char* joystickName[MAX_DEVICES] = {};
SDL_Joystick* joysticks[MAX_DEVICES] = {};
int joystickVirtualIndex[MAX_DEVICES] = {};
int joystickNbButton[MAX_DEVICES] = {};
int joystickSixaxis[MAX_DEVICES] = {};
char* mouseName[MAX_DEVICES] = {};
int mouseVirtualIndex[MAX_DEVICES] = {};
char* keyboardName[MAX_DEVICES] = {};
int keyboardVirtualIndex[MAX_DEVICES] = {};

int test_time = 1000;

#define BT_SIXAXIS_NAME "PLAYSTATION(R)3 Controller"

int initialize(int width, int height, const char *title)
{
  int i = 0;
  int j;
#ifndef WIN32
  const char* name;
#endif

  /* Init SDL */
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
  {
    fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
    return 0;
  }

#ifdef WIN32
  /* enable stdout and stderr */
  freopen( "CON", "w", stdout );
  freopen( "CON", "w", stderr );
#endif

  SDL_WM_SetCaption(title, title);

  /* Init video */
  screen = SDL_SetVideoMode(width, height, 0, SDL_HWSURFACE | SDL_ANYFORMAT);
  if (screen == NULL)
  {
    fprintf(stderr, "Unable to create video surface: %s\n", SDL_GetError());
    return 0;
  }

  if(grab)
  {
    SDL_WM_GrabInput(SDL_GRAB_ON);
  }
  SDL_ShowCursor(SDL_DISABLE);

  while((joysticks[i] = SDL_JoystickOpen(i)))
  {
      joystickName[i] = strdup(SDL_JoystickName(i));

      if(!strncmp(joystickName[i], BT_SIXAXIS_NAME, sizeof(BT_SIXAXIS_NAME)-1))
      {
        joystickName[i][sizeof(BT_SIXAXIS_NAME)-1] = '\0';
      }

      for(j=i-1; j>=0; --j)
      {
        if(!strcmp(joystickName[i], joystickName[j]))
        {
          joystickVirtualIndex[i] = joystickVirtualIndex[j]+1;
          break;
        }
      }
      if(j < 0)
      {
        joystickVirtualIndex[i] = 0;
      }
      joystickNbButton[i] = SDL_JoystickNumButtons(joysticks[i]);
      if(!strcmp(joystickName[i], "Sony PLAYSTATION(R)3 Controller"))
      {
        joystickSixaxis[i] = 1;
      }
      i++;
  }
#ifndef WIN32
  i = 0;
  while ((name = SDL_GetMouseName(i)))
  {
    mouseName[i] = strdup(name);

    for (j = i - 1; j >= 0; --j)
    {
      if (!strcmp(mouseName[i], mouseName[j]))
      {
        mouseVirtualIndex[i] = mouseVirtualIndex[j] + 1;
        break;
      }
    }
    if (j < 0)
    {
      mouseVirtualIndex[i] = 0;
    }
    i++;
  }
  i = 0;
  while ((name = SDL_GetKeyboardName(i)))
  {
    keyboardName[i] = strdup(name);

    for (j = i - 1; j >= 0; --j)
    {
      if (!strcmp(keyboardName[i], keyboardName[j]))
      {
        keyboardVirtualIndex[i] = keyboardVirtualIndex[j] + 1;
        break;
      }
    }
    if (j < 0)
    {
      keyboardVirtualIndex[i] = 0;
    }
    i++;
  }
#endif
  return 1;
}

#define TCP_PORT 21313

int tcpconnect(void)
{
    int fd;
    int i;
    int ret = -1;
    struct sockaddr_in addr;

    for(i=0; i<MAX_CONTROLLERS; ++i)
    {

#ifdef WIN32
      WSADATA wsadata;

      if (WSAStartup(MAKEWORD(1,1), &wsadata) == SOCKET_ERROR)
        err(1, "WSAStartup");

      if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err(1, "socket");
#else
      if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        err(1, "socket");
#endif
      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(TCP_PORT+i);
#ifdef WIN32
      addr.sin_addr.s_addr = inet_addr(ip);
#else
      addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#endif
      if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
      {
        fd = 0;
      }
      else
      {
        ret = 1;
        printf("connected to emu %d\n", i);
      }

#ifdef WIN32
      // Set the socket I/O mode; iMode = 0 for blocking; iMode != 0 for non-blocking
      int iMode = 1;
      ioctlsocket(fd, FIONBIO, (u_long FAR*) &iMode);
#endif

      sockfd[i] = fd;

    }

    return ret;
}

void auto_test()
{
  int i, j, k;
  int step;

  SDL_Event mouse_evt = {};

  for(k = 0; k<5; ++k)
  {
    step = 1 << k;

    for(i=0; i<500; i++)
    {
      for(j=0; j<MAX_DEVICES && mouseName[j]; ++j)
      {
        mouse_evt.motion.xrel = step;
        mouse_evt.motion.which = j;
        mouse_evt.type = SDL_MOUSEMOTION;
        SDL_PushEvent(&mouse_evt);
      }
      usleep(2000);
    }

    usleep(1000000);

    for(i=0; i<250; i++)
    {
      for(j=0; j<MAX_DEVICES && mouseName[j]; ++j)
      {
        mouse_evt.motion.xrel = -2*step;
        mouse_evt.motion.which = j;
        mouse_evt.type = SDL_MOUSEMOTION;
        SDL_PushEvent(&mouse_evt);
      }
      usleep(2000);
    }

    usleep(1000000);
  }

  /*if(mouse_cal[current_mouse][current_conf].dzx)
  {
    state[0].user.axis[1][0] = *mouse_cal[current_mouse][current_conf].dzx;
    controller[0].send_command = 1;

    usleep(test_time*1000);

    state[0].user.axis[1][0] = 0;
    controller[0].send_command = 1;
  }*/

  /*if(mouse_cal[current_mouse][current_conf].dzy)
    {
      state[0].user.axis[1][1] = *mouse_cal[current_mouse][current_conf].dzy;
      controller[0].send_command = 1;

      usleep(test_time*1000);

      state[0].user.axis[1][1] = 0;
      controller[0].send_command = 1;

      usleep(500*1000);

      state[0].user.axis[1][1] = *mouse_cal[current_mouse][current_conf].dzy;
      controller[0].send_command = 1;

      usleep(1000*1000);

      state[0].user.axis[1][1] = 0;
      controller[0].send_command = 1;

      usleep(500*1000);

      state[0].user.axis[1][1] = - *mouse_cal[current_mouse][current_conf].dzy;
      controller[0].send_command = 1;

      usleep(test_time*1000);

      state[0].user.axis[1][1] = 0;
      controller[0].send_command = 1;

      usleep(500*1000);

      state[0].user.axis[1][1] = - *mouse_cal[current_mouse][current_conf].dzy;
      controller[0].send_command = 1;

      usleep(1000*1000);

      state[0].user.axis[1][1] = 0;
      controller[0].send_command = 1;
    }*/
}

void circle_test()
{
  int i, j;
  const int step = 1;

  for(i=1; i<360; i+=step)
  {
    for(j=0; j<DEFAULT_REFRESH_PERIOD/refresh; ++j)
    {
      mouse_control[current_mouse].merge_x += mouse_cal[current_mouse][current_conf].rd*64*(cos(i*2*pi/360)-cos((i-1)*2*pi/360));
      mouse_control[current_mouse].merge_y += mouse_cal[current_mouse][current_conf].rd*64*(sin(i*2*pi/360)-sin((i-1)*2*pi/360));
      mouse_control[current_mouse].change = 1;
      usleep(refresh);
    }
  }
}

void display_calibration()
{
  if(current_cal != NONE)
  {
    printf("calibrating mouse %s %d\n", mouseName[current_mouse], mouseVirtualIndex[current_mouse]);
    printf("calibrating conf %d\n", current_conf+1);
    printf("multiplier_x:");
    if(mouse_cal[current_mouse][current_conf].mx)
    {
      printf(" %.2f\n", *mouse_cal[current_mouse][current_conf].mx);
    }
    else
    {
      printf(" NA\n");
    }
    printf("x/y_ratio:");
    if(mouse_cal[current_mouse][current_conf].mx && mouse_cal[current_mouse][current_conf].my)
    {
      printf(" %.2f\n", *mouse_cal[current_mouse][current_conf].my / *mouse_cal[current_mouse][current_conf].mx);
    }
    else
    {
      printf(" NA\n");
    }
    printf("dead_zone_x:");
    if(mouse_cal[current_mouse][current_conf].dzx)
    {
      printf(" %d\n", *mouse_cal[current_mouse][current_conf].dzx);
    }
    else
    {
      printf(" NA\n");
    }
    printf("dead_zone_y:");
    if(mouse_cal[current_mouse][current_conf].dzy)
    {
      printf(" %d\n", *mouse_cal[current_mouse][current_conf].dzy);
    }
    else
    {
      printf(" NA\n");
    }
    printf("shape:");
    if(mouse_cal[current_mouse][current_conf].dzs) {
      if(*mouse_cal[current_mouse][current_conf].dzs == E_SHAPE_CIRCLE) printf(" Circle\n");
      else printf(" Rectangle\n");
    }
    else
    {
      printf(" NA\n");
    }
    printf("exponent_x:");
    if(mouse_cal[current_mouse][current_conf].ex)
    {
      printf(" %.2f\n", *mouse_cal[current_mouse][current_conf].ex);
    }
    else
    {
      printf(" NA\n");
    }
    printf("exponent_y:");
    if(mouse_cal[current_mouse][current_conf].ey)
    {
      printf(" %.2f\n", *mouse_cal[current_mouse][current_conf].ey);
    }
    else
    {
      printf(" NA\n");
    }
    printf("radius: %.2f\n", mouse_cal[current_mouse][current_conf].rd);
    printf("time: %d\n", test_time);
  }
}

static void key(int device_id, int sym, int down)
{
  pthread_t thread;
  pthread_attr_t thread_attr;

	switch (sym)
  {
	  case SDLK_LCTRL: lctrl = down ? 1 : 0; break;
    case SDLK_RCTRL: rctrl = down ? 1 : 0; break;

    case SDLK_LSHIFT: lshift = down ? 1 : 0; break;
    case SDLK_RSHIFT: rshift = down ? 1 : 0; break;

    case SDLK_LALT: lalt = down ? 1 : 0; break;
    case SDLK_MODE: ralt = down ? 1 : 0; break;

    case SDLK_ESCAPE: if(down) done = 1; break;
  }

	if(lalt && ralt)
	{
	  if(grab)
	  {
	    SDL_WM_GrabInput(SDL_GRAB_OFF);
	    grab = 0;
	  }
	  else
	  {
	    SDL_WM_GrabInput(SDL_GRAB_ON);
      grab = 1;
	  }
	}

  switch (sym)
  {
    case SDLK_F1:
      if(down && rctrl)
      {
        if(current_cal == NONE)
        {
          current_cal = MC;
          printf("mouse selection\n");
          display_calibration();
        }
        else
        {
          current_cal = NONE;
          if(modify_file(config_file))
          {
            printf("error writting the config file %s\n", config_file);
          }
          printf("calibration done\n");
        }
      }
      break;
    case SDLK_F2:
      if(down && rctrl)
      {
        if(current_cal == CC)
        {
          current_cal = MC;
          printf("mouse selection\n");
        }
        else if(current_cal >= MC)
        {
          current_cal = CC;
          printf("config selection\n");
        }
      }
      break;
    case SDLK_F3:
      if(down && rctrl && current_cal != NONE)
      {
        if(current_conf >=0 && current_mouse >=0)
        {
          printf("calibrating multiplier x\n");
          current_cal = MX;
        }
      }
      break;
    case SDLK_F4:
      if(down && rctrl && current_cal != NONE)
      {
        if(current_conf >=0 && current_mouse >=0)
        {
          printf("calibrating x/y ratio\n");
          current_cal = MY;
        }
      }
      break;
    case SDLK_F5:
      if(down && rctrl && current_cal != NONE)
      {
        if(current_conf >=0 && current_mouse >=0)
        {
          printf("calibrating dead zone x\n");
          current_cal = DZX;
          mouse_control[current_mouse].merge_x = 1;
          mouse_control[current_mouse].merge_y = 0;
          mouse_control[current_mouse].change = 1;
        }
      }
      break;
    case SDLK_F6:
      if(down && rctrl && current_cal != NONE)
      {
        if(current_conf >=0 && current_mouse >=0)
        {
          printf("calibrating dead zone y\n");
          current_cal = DZY;
          mouse_control[current_mouse].merge_x = 0;
          mouse_control[current_mouse].merge_y = 1;
          mouse_control[current_mouse].change = 1;
        }
      }
      break;
    case SDLK_F7:
      if(down && rctrl && current_cal != NONE)
      {
        if(current_conf >=0 && current_mouse >=0)
        {
          printf("calibrating dead zone shape\n");
          current_cal = DZS;
          mouse_control[current_mouse].merge_x = 1;
          mouse_control[current_mouse].merge_y = 1;
          mouse_control[current_mouse].change = 1;
        }
      }
      break;
    case SDLK_F8:
      if(down && rctrl && current_cal != NONE)
      {
        if(current_conf >=0 && current_mouse >=0)
        {
          printf("calibrating exponent x\n");
          current_cal = EX;
        }
      }
      break;
    case SDLK_F9:
      if(down && rctrl && current_cal != NONE)
      {
        if(current_conf >=0 && current_mouse >=0)
        {
          printf("calibrating exponent y\n");
          current_cal = EY;
        }
      }
      break;
    case SDLK_F10:
      if(down && rctrl && current_cal != NONE)
      {
        if(current_conf >=0 && current_mouse >=0)
        {
          printf("adjusting circle test radius\n");
          current_cal = RD;
        }
      }
      break;
    case SDLK_F11:
      if(down && rctrl)
      {
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
        pthread_create( &thread, &thread_attr, (void*)circle_test, NULL);
      }
      break;
    case SDLK_F12:
      if(down && rctrl && current_cal != NONE)
      {
        if(current_conf >=0 && current_mouse >=0)
        {
          current_cal = TEST;
        }
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
        pthread_create( &thread, &thread_attr, (void*)auto_test, NULL);
      }
      break;
  }

	if(lshift && rshift)
  {
    if(display)
    {
      display = 0;
    }
    else
    {
      display = 1;
    }
  }

	if(down) macro_lookup(device_id, sym);
}

static void button(int which, int button)
{
  double ratio;

  switch (button)
  {
    case SDL_BUTTON_WHEELUP:
      switch(current_cal)
      {
        case MC:
          current_mouse += 1;
          if(!mouseName[current_mouse])
          {
            current_mouse -= 1;
          }
          break;
        case CC:
          current_conf += 1;
          if(current_conf > MAX_CONFIGURATIONS-1)
          {
            current_conf = MAX_CONFIGURATIONS-1;
          }
          break;
        case MX:
          if(mouse_cal[current_mouse][current_conf].mx && mouse_cal[current_mouse][current_conf].my)
          {
            ratio = *mouse_cal[current_mouse][current_conf].my / *mouse_cal[current_mouse][current_conf].mx;
            *mouse_cal[current_mouse][current_conf].mx += multiplier_step;
            *mouse_cal[current_mouse][current_conf].my = *mouse_cal[current_mouse][current_conf].mx * ratio;
          }
          break;
        case MY:
          if(mouse_cal[current_mouse][current_conf].mx && mouse_cal[current_mouse][current_conf].my)
          {
            ratio = *mouse_cal[current_mouse][current_conf].my / *mouse_cal[current_mouse][current_conf].mx;
            ratio += multiplier_step;
            *mouse_cal[current_mouse][current_conf].my = *mouse_cal[current_mouse][current_conf].mx * ratio;
          }
          break;
        case DZX:
          if(mouse_cal[current_mouse][current_conf].dzx)
          {
            *mouse_cal[current_mouse][current_conf].dzx += 1;
            if(*mouse_cal[current_mouse][current_conf].dzx > mean_axis_value)
            {
              *mouse_cal[current_mouse][current_conf].dzx = mean_axis_value;
            }
            mouse_control[current_mouse].merge_x = 1;
            mouse_control[current_mouse].merge_y = 0;
            mouse_control[current_mouse].change = 1;
          }
          break;
        case DZY:
          if(mouse_cal[current_mouse][current_conf].dzy)
          {
            *mouse_cal[current_mouse][current_conf].dzy += 1;
            if(*mouse_cal[current_mouse][current_conf].dzy > mean_axis_value)
            {
              *mouse_cal[current_mouse][current_conf].dzy = mean_axis_value;
            }
            mouse_control[current_mouse].merge_x = 0;
            mouse_control[current_mouse].merge_y = 1;
            mouse_control[current_mouse].change = 1;
          }
          break;
        case DZS:
          if(mouse_cal[current_mouse][current_conf].dzs)
          {
            if(*mouse_cal[current_mouse][current_conf].dzs == E_SHAPE_CIRCLE)
            {
              *mouse_cal[current_mouse][current_conf].dzs = E_SHAPE_RECTANGLE;
            }
            else
            {
              *mouse_cal[current_mouse][current_conf].dzs = E_SHAPE_CIRCLE;
            }
            mouse_control[current_mouse].merge_x = 1;
            mouse_control[current_mouse].merge_y = 1;
            mouse_control[current_mouse].change = 1;
          }
          break;
        case RD:
          mouse_cal[current_mouse][current_conf].rd += 0.5;
          break;
        case EX:
          if(mouse_cal[current_mouse][current_conf].ex) *mouse_cal[current_mouse][current_conf].ex += EXPONENT_STEP;
          break;
        case EY:
          if(mouse_cal[current_mouse][current_conf].ey) *mouse_cal[current_mouse][current_conf].ey += EXPONENT_STEP;
          break;
        case TEST:
          test_time += 10;
          break;
        case NONE:
          break;
      }
      break;
    case SDL_BUTTON_WHEELDOWN:
      switch(current_cal)
      {
        case MC:
          if(current_mouse > 0)
          {
            current_mouse -= 1;
          }
          break;
        case CC:
          if(current_conf > 0)
          {
            current_conf -= 1;
          }
          break;
        case MX:
          if(mouse_cal[current_mouse][current_conf].mx && mouse_cal[current_mouse][current_conf].my)
          {
            ratio = *mouse_cal[current_mouse][current_conf].my / *mouse_cal[current_mouse][current_conf].mx;
            *mouse_cal[current_mouse][current_conf].mx -= multiplier_step;
            *mouse_cal[current_mouse][current_conf].my = *mouse_cal[current_mouse][current_conf].mx * ratio;
          }
          break;
        case MY:
          if(mouse_cal[current_mouse][current_conf].mx && mouse_cal[current_mouse][current_conf].my)
          {
            ratio = *mouse_cal[current_mouse][current_conf].my / *mouse_cal[current_mouse][current_conf].mx;
            ratio -= multiplier_step;
            *mouse_cal[current_mouse][current_conf].my = *mouse_cal[current_mouse][current_conf].mx * ratio;
          }
          break;
        case DZX:
          if(mouse_cal[current_mouse][current_conf].dzx)
          {
            *mouse_cal[current_mouse][current_conf].dzx -= 1;
            if(*mouse_cal[current_mouse][current_conf].dzx < 0)
            {
              *mouse_cal[current_mouse][current_conf].dzx = 0;
            }
            mouse_control[current_mouse].merge_x = -1;
            mouse_control[current_mouse].merge_y = 0;
            mouse_control[current_mouse].change = 1;
          }
          break;
        case DZY:
          if(mouse_cal[current_mouse][current_conf].dzy)
          {
            *mouse_cal[current_mouse][current_conf].dzy -= 1;
            if(*mouse_cal[current_mouse][current_conf].dzy < 0)
            {
              *mouse_cal[current_mouse][current_conf].dzy = 0;
            }
            mouse_control[current_mouse].merge_x = 0;
            mouse_control[current_mouse].merge_y = -1;
            mouse_control[current_mouse].change = 1;
          }
          break;
        case DZS:
          if(mouse_cal[current_mouse][current_conf].dzs)
          {
            if(*mouse_cal[current_mouse][current_conf].dzs == E_SHAPE_CIRCLE)
            {
              *mouse_cal[current_mouse][current_conf].dzs = E_SHAPE_RECTANGLE;
            }
            else
            {
              *mouse_cal[current_mouse][current_conf].dzs = E_SHAPE_CIRCLE;
            }
            mouse_control[current_mouse].merge_x = -1;
            mouse_control[current_mouse].merge_y = -1;
            mouse_control[current_mouse].change = 1;
          }
          break;
        case RD:
          mouse_cal[current_mouse][current_conf].rd -= 0.5;
          break;
        case EX:
          if(mouse_cal[current_mouse][current_conf].ex) *mouse_cal[current_mouse][current_conf].ex -= EXPONENT_STEP;
          break;
        case EY:
          if(mouse_cal[current_mouse][current_conf].ey) *mouse_cal[current_mouse][current_conf].ey -= EXPONENT_STEP;
          break;
        case TEST:
          test_time -= 10;
          break;
        case NONE:
          break;
      }
      break;
  }

  display_calibration();
}

int main(int argc, char *argv[])
{
    SDL_Event events[EVENT_BUFFER_SIZE];
    SDL_Event* event;
    SDL_Event mouse_evt = {};
    int i;
    int num_evt;
    unsigned char buf[48];
    int read = 0;
    struct timeval t0, t1;
    int time_to_sleep;

#ifndef WIN32
    setlinebuf(stdout);
    homedir = getpwuid(getuid())->pw_dir;

    system("test -d ~/.emuclient || cp -r /etc/emuclient ~/.emuclient");
#endif

    for(i=1; i<argc; ++i)
    {
      if(!strcmp(argv[i], "--nograb"))
      {
        grab = 0;
      }
      else if(!strcmp(argv[i], "--config") && i<argc)
      {
        config_file = argv[++i];
        read = 1;
      }
      else if(!strcmp(argv[i], "--status"))
      {
        display = 1;
      }
      else if(!strcmp(argv[i], "--refresh"))
      {
        refresh = atoi(argv[++i])*1000;
        postpone_count = 3*DEFAULT_REFRESH_PERIOD/refresh;
      }
      else if(!strcmp(argv[i], "--precision"))
      {
        max_axis_value = (1 << atoi(argv[++i])) - 1;
        mean_axis_value = max_axis_value/2;
      }
      else if(!strcmp(argv[i], "--rs232"))
      {
        rs232 = 1;
      }
      else if(!strcmp(argv[i], "--subpos"))
      {
        subpos = 1;
      }
#ifdef WIN32
      else if(!strcmp(argv[i], "--ip") && i<argc)
      {
        ip = argv[++i];
      }
#endif
    }

    if(grab == 1)
    {
      sleep(1);//ugly stuff that needs to be cleaned...
    }

    if(display == 1)
    {
      printf("max_axis_value: %d\n", max_axis_value);//needed by sixstatus...
    }

    axis_scale = (double) max_axis_value / DEFAULT_MAX_AXIS_VALUE;
    frequency_scale = (double) DEFAULT_REFRESH_PERIOD / refresh;

    initialize_macros();

    for(i=0; i<MAX_CONTROLLERS; ++i)
    {
      sixaxis_init(state+i);
      memset(controller+i, 0x00, sizeof(s_controller));
    }

    if (!initialize(SCREEN_WIDTH, SCREEN_HEIGHT, "Sixaxis Control"))
        err(1, "can't init sdl");

    if(read == 1)
    {
      read_config_file(config_file);
    }

    if(tcpconnect() < 0)
    {
      err(1, "tcpconnect");
    }

    done = 0;
    while(!done)
    {
        gettimeofday(&t0, NULL);

        SDL_PumpEvents();
        num_evt = SDL_PeepEvents(events, sizeof(events)/sizeof(events[0]), SDL_GETEVENT, SDL_ALLEVENTS);

        if(num_evt == EVENT_BUFFER_SIZE)
        {
          printf("buffer too small!!!\n");
        }

        for(event=events; event<events+num_evt; ++event)
        {
          if(event->type != SDL_MOUSEMOTION)
          {
            //if calibration is on, all mouse wheel events are skipped
            if(current_cal == NONE || event->type != SDL_MOUSEBUTTONDOWN || (event->button.button != SDL_BUTTON_WHEELDOWN && event->button.button != SDL_BUTTON_WHEELUP))
            {
              process_event(event);
            }
          }
          else
          {
            mouse_control[event->motion.which].merge_x += event->motion.xrel;
            mouse_control[event->motion.which].merge_y += event->motion.yrel;
            mouse_control[event->motion.which].change = 1;
          }

          trigger_lookup(event);
          intensity_lookup(event);

          switch (event->type)
          {
            case SDL_QUIT:
              done = 1;
              break;
            case SDL_KEYDOWN:
              key(event->key.which, event->key.keysym.sym, 1);
              break;
            case SDL_KEYUP:
              key(event->key.which, event->key.keysym.sym, 0);
              break;
            case SDL_MOUSEBUTTONDOWN:
              button(event->button.which, event->button.button);
              break;
          }
        }

        /*
         * Process a single (merged) motion event for each mouse.
         */
        for(i=0; i<MAX_DEVICES; ++i)
        {
          if(mouse_control[i].changed || mouse_control[i].change)
          {
            if(subpos)
            {
              /*
               * Add the residual motion vector from the last iteration.
               */
              mouse_control[i].merge_x += mouse_control[i].residue_x;
              mouse_control[i].merge_y += mouse_control[i].residue_y;
              /*
               * If no motion was received this iteration, the residual motion vector from the last iteration is reset.
               */
              if(!mouse_control[i].change)
              {
                mouse_control[i].residue_x = 0;
                mouse_control[i].residue_y = 0;
              }
            }
            mouse_evt.motion.which = i;
            mouse_evt.type = SDL_MOUSEMOTION;
            process_event(&mouse_evt);
          }
          mouse_control[i].merge_x = 0;
          mouse_control[i].merge_y = 0;
          mouse_control[i].changed = mouse_control[i].change;
          mouse_control[i].change = 0;
          if(i == current_mouse && (current_cal == DZX || current_cal == DZY || current_cal == DZS))
          {
            mouse_control[i].changed = 0;
          }
        }

        /*
         * Send a command to each controller that has its status changed.
         */
        for(i=0; i<MAX_CONTROLLERS; ++i)
        {
          if(controller[i].send_command)
          {
            if(!sockfd[i]) continue;

            if(!rs232)
            {
              if (assemble_input_01(buf, sizeof(buf), state + i) < 0)
              {
                printf("can't assemble\n");
              }
              send(sockfd[i], (const char*)buf, 48, MSG_DONTWAIT);
            }
            else
            {
              send(sockfd[i], (const char*)(state + i), sizeof(struct sixaxis_state), MSG_DONTWAIT);
            }

            if (display)
            {
              sixaxis_dump_state(state+i, i);
            }

            controller[i].send_command = 0;
          }
        }

        gettimeofday(&t1, NULL);

        time_to_sleep = refresh - ((t1.tv_sec*1000000+t1.tv_usec)-(t0.tv_sec*1000000+t0.tv_usec));

        if(time_to_sleep > 0)
        {
          usleep(time_to_sleep);
        }
        else
        {
          printf("processing time higher than %dms!!\n", refresh/1000);
        }
    }

    printf("Exiting\n");

    free_macros();

    for(i=0; i<MAX_DEVICES && joystickName[i]; ++i)
    {
      free(joystickName[i]);
      SDL_JoystickClose(joysticks[i]);
    }
    for(i=0; i<MAX_DEVICES && mouseName[i]; ++i)
    {
      free(mouseName[i]);
    }
    for(i=0; i<MAX_DEVICES && keyboardName[i]; ++i)
    {
      free(keyboardName[i]);
    }

#ifndef WIN32
    SDL_FreeSurface(screen);
    SDL_Quit();
#endif

    return 0;
}
