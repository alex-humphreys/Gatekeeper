/*

  Because ponies.

 */

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID {0x0A, 0x3B, 0x2A, 0x35, 0x3A, 0x40, 0x4B, 0xDC, 0xD3, 0x69, 0xDE, 0x9F, 0xF4, 0x4A, 0x81, 0x88}
PBL_APP_INFO(MY_UUID, "Gatekeeper", "Mr. Humphreys", 0x1, 0x0, RESOURCE_ID_IMAGE_MENU_ICON, APP_INFO_WATCH_FACE);


Window window;


BmpContainer background_image_container;

RotBmpContainer hour_hand_image_container;
RotBmpContainer minute_hand_image_container;
RotBmpContainer second_hand_image_container;

GFont dice;
TextLayer date_layer;
static char date_text[] = "12";

Layer second_display_layer;


RotBmpPairContainer center_circle_image_container;



// from src/core/util/misc.h

#define MAX(a,b) (((a)>(b))?(a):(b))

// From src/fw/ui/rotate_bitmap_layer.c

//! newton's method for floor(sqrt(x)) -> should always converge
static int32_t integer_sqrt(int32_t x) {
  if (x < 0) {
    ////    PBL_LOG(LOG_LEVEL_ERROR, "Looking for sqrt of negative number");
    return 0;
  }

  int32_t last_res = 0;
  int32_t res = (x + 1)/2;
  while (last_res != res) {
    last_res = res;
    res = (last_res + x / last_res) / 2;
  }
  return res;
}

void second_display_layer_update_callback(Layer *me, GContext* ctx) {
  (void)me;

  PblTm t;
  get_time(&t);

  int32_t second_angle = t.tm_sec * (0xffff/60);
  int second_hand_length = 70;

  graphics_context_set_fill_color(ctx, GColorWhite);

  GPoint center = grect_center_point(&me->frame);
  GPoint second = GPoint(center.x + second_hand_length * sin_lookup(second_angle)/0xffff,
        center.y + (-second_hand_length) * cos_lookup(second_angle)/0xffff);

  graphics_draw_line(ctx, center, second);
}


void rot_bitmap_set_src_ic(RotBitmapLayer *image, GPoint ic) {
  image->src_ic = ic;

  // adjust the frame so the whole image will still be visible
  const int32_t horiz = MAX(ic.x, abs(image->bitmap->bounds.size.w - ic.x));
  const int32_t vert = MAX(ic.y, abs(image->bitmap->bounds.size.h - ic.y));

  GRect r = layer_get_frame(&image->layer);
  //// const int32_t new_dist = integer_sqrt(horiz*horiz + vert*vert) * 2;
  const int32_t new_dist = (integer_sqrt(horiz*horiz + vert*vert) * 2) + 1; //// Fudge to deal with non-even dimensions--to ensure right-most and bottom-most edges aren't cut off.

  r.size.w = new_dist;
  r.size.h = new_dist;
  layer_set_frame(&image->layer, r);

  r.origin = GPoint(0, 0);
  ////layer_set_bounds(&image->layer, r);
  image->layer.bounds = r;

  image->dest_ic = GPoint(new_dist / 2, new_dist / 2);

  layer_mark_dirty(&(image->layer));
}

/* ------------------------------------------------------------------------- */


void set_hand_angle(RotBmpContainer *hand_image_container, unsigned int hand_angle) {

  signed short x_fudge = 0;
  signed short y_fudge = 0;


  hand_image_container->layer.rotation =  TRIG_MAX_ANGLE * hand_angle / 360;

  //
  // Due to rounding/centre of rotation point/other issues of fitting
  // square pixels into round holes by the time hands get to 6 and 9
  // o'clock there's off-by-one pixel errors.
  //
  // The `x_fudge` and `y_fudge` values enable us to ensure the hands
  // look centred on the minute marks at those points. (This could
  // probably be improved for intermediate marks also but they're not
  // as noticable.)
  //
  // I think ideally we'd only ever calculate the rotation between
  // 0-90 degrees and then rotate again by 90 or 180 degrees to
  // eliminate the error.
  //
  if (hand_angle == 180) {
    x_fudge = -1;
  } else if (hand_angle == 270) {
    y_fudge = -1;
  }

  // (144 = screen width, 168 = screen height)
  hand_image_container->layer.layer.frame.origin.x = (144/2) - (hand_image_container->layer.layer.frame.size.w/2) + x_fudge;
  hand_image_container->layer.layer.frame.origin.y = (168/2) - (hand_image_container->layer.layer.frame.size.h/2) + y_fudge;

  layer_mark_dirty(&hand_image_container->layer.layer);
}


void update_hand_positions() {

  PblTm t;

  get_time(&t);

  set_hand_angle(&hour_hand_image_container, ((t.tm_hour % 12) * 30) + (t.tm_min/2)); // ((((t.tm_hour % 12) * 6) + (t.tm_min / 10))) / (12 * 6));

  set_hand_angle(&minute_hand_image_container, t.tm_min * 6);

}


void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)t;
  (void)ctx;

  update_hand_positions(); // TODO: Pass tick event

 PblTm ti;
 get_time(&ti);
  
 string_format_time(date_text, sizeof(date_text), "%d", &ti);
 text_layer_set_text(&date_layer, date_text);
  layer_mark_dirty(&second_display_layer);

}




void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "Brains Watch");
  window_stack_push(&window, true);

  resource_init_current_app(&APP_RESOURCES);

//    rotbmp_pair_init_container(RESOURCE_ID_IMAGE_PANDA_WHITE, RESOURCE_ID_IMAGE_PANDA_BLACK, &bitmap_container);


  // Set up a layer for the static watch face background
  bmp_init_container(RESOURCE_ID_IMAGE_BACKGROUND, &background_image_container);
  layer_add_child(&window.layer, &background_image_container.layer.layer);


dice = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_VISITOR_16));

  text_layer_init(&date_layer, GRect(53, 105, 40, 40));
  text_layer_set_text_alignment(&date_layer, GTextAlignmentCenter);
  text_layer_set_text_color(&date_layer, GColorWhite);
  text_layer_set_background_color(&date_layer, GColorClear);
  text_layer_set_font(&date_layer, dice);
  layer_add_child(&window.layer, &date_layer.layer);

 PblTm ti;
 get_time(&ti);
  
 string_format_time(date_text, sizeof(date_text), "%d", &ti);
 text_layer_set_text(&date_layer, date_text);




  // Set up a layer for the hour hand
  rotbmp_init_container(RESOURCE_ID_IMAGE_HOUR_HAND, &hour_hand_image_container);

    //  hour_hand_image_container.layer.compositing_mode = GCompOpClear;

  rot_bitmap_set_src_ic(&hour_hand_image_container.layer, GPoint(3, 42));

  layer_add_child(&window.layer, &hour_hand_image_container.layer.layer);


  // Set up a layer for the minute hand
  rotbmp_init_container(RESOURCE_ID_IMAGE_MINUTE_HAND, &minute_hand_image_container);

 // minute_hand_image_container.layer.compositing_mode = GCompOpClear;

  rot_bitmap_set_src_ic(&minute_hand_image_container.layer, GPoint(3, 58));

  layer_add_child(&window.layer, &minute_hand_image_container.layer.layer);






  update_hand_positions();


  // Setup the black and white circle in the centre of the watch face
  // (We use a bitmap rather than just drawing it because it means not having
  // to stuff around with working out the circle center etc.)
  rotbmp_pair_init_container(RESOURCE_ID_IMAGE_CENTER_CIRCLE_WHITE, RESOURCE_ID_IMAGE_CENTER_CIRCLE_BLACK,
			     &center_circle_image_container);

  // TODO: Do this properly with a GRect().
  // (144 = screen width, 168 = screen height)
  center_circle_image_container.layer.layer.frame.origin.x = (144/2) - (center_circle_image_container.layer.layer.frame.size.w/2);
  center_circle_image_container.layer.layer.frame.origin.y = (168/2) - (center_circle_image_container.layer.layer.frame.size.h/2);


  layer_add_child(&window.layer, &center_circle_image_container.layer.layer);

  layer_init(&second_display_layer, window.layer.frame);
  second_display_layer.update_proc = &second_display_layer_update_callback;
  layer_add_child(&window.layer, &second_display_layer);

}


void handle_deinit(AppContextRef ctx) {
  (void)ctx;

  bmp_deinit_container(&background_image_container);
  rotbmp_deinit_container(&hour_hand_image_container);
  rotbmp_deinit_container(&minute_hand_image_container);
  rotbmp_pair_deinit_container(&center_circle_image_container);

  fonts_unload_custom_font(dice);

}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,

    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }

  };
  app_event_loop(params, &handlers);
}
