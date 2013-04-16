#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include <stdlib.h>

#define MY_UUID { 0x53, 0x0D, 0x98, 0xCE, 0x47, 0x8D, 0x4D, 0x17, 0xA8, 0x6F, 0xC6, 0xA2, 0x5A, 0x54, 0xB9, 0xBC }
PBL_APP_INFO(MY_UUID,
             "baWrista", "Overpunch",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);

Window window;
TextLayer steep_text_layer;
TextLayer press_text_layer;
TextLayer instr_text_layer;
Layer image_layer;

typedef enum { SET_STEEP, SET_PRESS, STEEP, PRESS, DONE } AppState;
AppState state;

char steep_time_buffer[12 + 5] = "Steep time: 0";
char press_time_buffer[12 + 5] = "Press time: 0";
// char instr_buffer[24] = "\0";
char steep_msg[] = "Set time to Steep";
char press_msg[] = "Set time to Press";
char enjoy_msg[] = "Enjoy!";

int steep_interval; // interval*15 seconds
int press_interval; // interval*15 seconds

RotBmpPairContainer base_bitmap;
// RotBmpPairContainer plunger_bitmap;
// BmpContainer base_bitmap;
BmpContainer plunger_bitmap;
PropertyAnimation prop_animation;

// Yet, another good itoa implementation
void itoa(int value, char *sp, int radix)
{
    char tmp[16];// be careful with the length of the buffer
    char *tp = tmp;
    int i;
    unsigned v;
    int sign;

    sign = (radix == 10 && value < 0);
    if (sign)   v = -value;
    else    v = (unsigned)value;

    while (v || tp == tmp)
    {
        i = v % radix;
        v /= radix; // v/=radix uses less CPU clocks than v=v/radix does
        if (i < 10)
          *tp++ = i+'0';
        else
          *tp++ = i + 'a' - 10;
    }

    if (sign)
    *sp++ = '-';
    while (tp > tmp)
    *sp++ = *--tp;

    *sp++ = '\0';
}

void update(void) {
    if (state == SET_STEEP) {
        itoa(steep_interval*15, steep_time_buffer+12, 10);
        text_layer_set_text(&steep_text_layer, steep_time_buffer);
    } else if (state == SET_PRESS) {
        itoa(press_interval*15, press_time_buffer+12, 10);
        text_layer_set_text(&press_text_layer, press_time_buffer);
    }
    
    char* msg;
    switch (state) {
      case SET_STEEP:
        msg = steep_msg;
        break;
      case SET_PRESS:
        msg = press_msg;
        break;
      case STEEP:
        msg = steep_msg + 12;
        break;
      case PRESS:
        msg = press_msg + 12;
        break;
      case DONE:
        msg = enjoy_msg;
        break;
    }
    text_layer_set_text(&instr_text_layer, msg);
}

void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  int* var = NULL;
  if (state == SET_STEEP) {
      var = &steep_interval;
  } else if (state == SET_PRESS) {
      var = &press_interval;
  }

  if (var != NULL) {
      if (*var > 0) {
        --*var;
        update();
      }
  }
}


void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  int* var = NULL;
  if (state == SET_STEEP) {
      var = &steep_interval;
  } else if (state == SET_PRESS) {
      var = &press_interval;
  }

  if (var != NULL) {
      if (*var < 20) {
        ++*var;
        update();
      }
  }
}

void select_click_handler(ClickRecognizerRef recognizer, Window *window) {
  if (state <= SET_PRESS) {
    ++state;
    update();
  }
}


void click_config_provider(ClickConfig **config, Window *window) {
  (void)window;

  config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
  config[BUTTON_ID_UP]->click.repeat_interval_ms = 10000;
  
  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_click_handler;
  config[BUTTON_ID_SELECT]->click.repeat_interval_ms = 10000;

  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
  config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 10000;
}

void handle_plunger_animation_done(struct Animation *animation, bool finished, void *context) {
  vibes_short_pulse();
}

#define GRECT_OFFSET(r, dx, dy) GRect((r).origin.x+(dx), (r).origin.y+(dy), (r).size.w, (r).size.h)

void handle_init(AppContextRef ctx) {
  (void)ctx;
  
  steep_interval = 0;
  press_interval = 0;
  
  state = SET_STEEP;

  window_init(&window, "baWrista");
  window_stack_push(&window, true);
  
  resource_init_current_app(&FONT_DEMO_RESOURCES);

  text_layer_init(&steep_text_layer, window.layer.frame);
  text_layer_set_text_alignment(&steep_text_layer, GTextAlignmentCenter);
  text_layer_set_text(&steep_text_layer, steep_time_buffer);
  text_layer_set_font(&steep_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  layer_add_child(&window.layer, &steep_text_layer.layer);
  
  text_layer_init(&press_text_layer, GRECT_OFFSET(window.layer.frame, 0, 15));
  text_layer_set_text_alignment(&press_text_layer, GTextAlignmentCenter);
  text_layer_set_text(&press_text_layer, press_time_buffer);
  text_layer_set_font(&press_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  layer_add_child(&window.layer, &press_text_layer.layer);
  
  text_layer_init(&instr_text_layer, GRECT_OFFSET(window.layer.frame, 0, 30));
  text_layer_set_text_alignment(&instr_text_layer, GTextAlignmentCenter);
  text_layer_set_text(&instr_text_layer, steep_msg);
  text_layer_set_font(&instr_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(&window.layer, &instr_text_layer.layer);
  
  layer_init(&image_layer, GRECT_OFFSET(window.layer.frame, 0, 22));
  
  // rotbmp_pair_init_container(RESOURCE_ID_IMAGE_PLUNGER_WHITE, 
  //                            RESOURCE_ID_IMAGE_PLUNGER_BLACK, &plunger_bitmap);
  bmp_init_container(RESOURCE_ID_IMAGE_PLUNGER, &plunger_bitmap);
  rotbmp_pair_init_container(RESOURCE_ID_IMAGE_BASE_WHITE, 
                             RESOURCE_ID_IMAGE_BASE_BLACK, &base_bitmap);
  //bmp_init_container(RESOURCE_ID_IMAGE_BASE, &base_bitmap);
                        
  layer_add_child(&image_layer, &plunger_bitmap.layer.layer);
  layer_add_child(&image_layer, &base_bitmap.layer.layer);
  
  layer_add_child(&window.layer, &image_layer);
  
  GRect from_rect = GRECT_OFFSET(layer_get_frame(&image_layer), 0, -28);
  GRect to_rect = GRect(from_rect.origin.x, from_rect.origin.y+40, from_rect.size.w, from_rect.size.h);
  property_animation_init_layer_frame(&prop_animation, &plunger_bitmap.layer.layer, &from_rect, &to_rect);
  
  AnimationHandlers animation_handlers = {
    .stopped = &handle_plunger_animation_done
  };
  
  animation_set_duration(&prop_animation.animation, 5000);
  animation_set_handlers(&prop_animation.animation, animation_handlers, NULL);
  
  animation_schedule(&prop_animation.animation);
  
  window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);
}

void handle_deinit(AppContextRef ctx) {
  (void)ctx;

  // rotbmp_pair_deinit_container(&plunger_bitmap);
  rotbmp_pair_deinit_container(&base_bitmap);
  bmp_deinit_container(&plunger_bitmap);
  //bmp_deinit_container(&base_bitmap);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit
  };
  app_event_loop(params, &handlers);
}
