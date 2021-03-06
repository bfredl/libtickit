#include "tickit.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int still_running = 1;

static void sigint(int sig)
{
  still_running = 0;
}

static void render_modifier(TickitTerm *tt, int mod)
{
  if(!mod)
    return;

  int pipe = 0;

  tickit_term_erasech(tt, 3, 1);
  tickit_term_print(tt, "<");

  if(mod & TICKIT_MOD_SHIFT)
    tickit_term_print(tt, pipe++ ? "|SHIFT" : "SHIFT");
  if(mod & TICKIT_MOD_ALT)
    tickit_term_print(tt, pipe++ ? "|ALT" : "ALT");
  if(mod & TICKIT_MOD_CTRL)
    tickit_term_print(tt, pipe++ ? "|CTRL" : "CTRL");

  tickit_term_print(tt, ">");
}

static void render_key(TickitTerm *tt, TickitKeyEventType type, const char *str, int mod)
{
  tickit_term_goto(tt, 2, 2);
  tickit_term_print(tt, "Key:");

  tickit_term_goto(tt, 4, 4);
  switch(type) {
    case TICKIT_KEYEV_TEXT: tickit_term_print(tt, "text "); break;
    case TICKIT_KEYEV_KEY:  tickit_term_print(tt, "key  "); break;
    default: return;
  }
  tickit_term_print(tt, str);
  render_modifier(tt, mod);
  tickit_term_erasech(tt, 30, -1);
}

static void render_mouse(TickitTerm *tt, TickitMouseEventType type, int button, int line, int col, int mod)
{
  tickit_term_goto(tt, 8, 2);
  tickit_term_print(tt, "Mouse:");

  tickit_term_goto(tt, 10, 4);
  switch(type) {
    case TICKIT_MOUSEEV_PRESS:   tickit_term_print(tt, "press   "); break;
    case TICKIT_MOUSEEV_DRAG:    tickit_term_print(tt, "drag    "); break;
    case TICKIT_MOUSEEV_RELEASE: tickit_term_print(tt, "release "); break;
    case TICKIT_MOUSEEV_WHEEL:   tickit_term_print(tt, "wheel ");   break;
    default: return;
  }

  if(type == TICKIT_MOUSEEV_WHEEL) {
    tickit_term_printf(tt, "%s at (%d,%d)", button == TICKIT_MOUSEWHEEL_DOWN ? "down" : "up", line, col);
  }
  else {
    tickit_term_printf(tt, "button %d at (%d,%d)", button, line, col);
  }
  render_modifier(tt, mod);
  tickit_term_erasech(tt, 20, -1);
}

static void event(TickitTerm *tt, TickitEventType ev, TickitEvent *args, void *data)
{
  switch(ev) {
  case TICKIT_EV_RESIZE:
  case TICKIT_EV_CHANGE:
  case TICKIT_EV_UNBIND:
    break;

  case TICKIT_EV_KEY:
    if(args->type == TICKIT_KEYEV_KEY && strcmp(args->str, "C-c") == 0) {
      still_running = 0;
      return;
    }

    render_key(tt, args->type, args->str, args->mod);
    break;

  case TICKIT_EV_MOUSE:
    render_mouse(tt, args->type, args->button, args->line, args->col, args->mod);
    break;
  }
}

int main(int argc, char *argv[])
{
  TickitTerm *tt;

  tt = tickit_term_new();
  if(!tt) {
    fprintf(stderr, "Cannot create TickitTerm - %s\n", strerror(errno));
    return 1;
  }

  tickit_term_set_input_fd(tt, STDIN_FILENO);
  tickit_term_set_output_fd(tt, STDOUT_FILENO);
  tickit_term_await_started(tt, &(const struct timeval){ .tv_sec = 0, .tv_usec = 50000 });

  tickit_term_setctl_int(tt, TICKIT_TERMCTL_ALTSCREEN, 1);
  tickit_term_setctl_int(tt, TICKIT_TERMCTL_CURSORVIS, 0);
  tickit_term_setctl_int(tt, TICKIT_TERMCTL_MOUSE, TICKIT_TERM_MOUSEMODE_DRAG);
  tickit_term_setctl_int(tt, TICKIT_TERMCTL_KEYPAD_APP, 1);
  tickit_term_clear(tt);

  tickit_term_bind_event(tt, TICKIT_EV_KEY|TICKIT_EV_MOUSE, event, NULL);

  render_key(tt, -1, "", 0);
  render_mouse(tt, -1, 0, 0, 0, 0);

  signal(SIGINT, sigint);

  while(still_running)
    tickit_term_input_wait(tt, NULL);

  tickit_term_destroy(tt);

  return 0;
}
