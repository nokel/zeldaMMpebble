#include <pebble.h>
#include "pdc-transform.h"

/* =========================================================
   GLOBAL MOVEMENT SYSTEM
========================================================= */

static int MOVE_ALL_X = 0;
static int MOVE_ALL_Y = 0;

static int HEARTS_X = 0;
static int HEARTS_Y = 0;

static int HOUR_X = 0;
static int HOUR_Y = 0;

static int MINUTE_X = 0;
static int MINUTE_Y = 0;

static int DATE_X = 0;
static int DATE_Y = 0;

static int GREEN_X_OFFSET = 40;
static int GREEN_Y_OFFSET = 40;
/* NOTE: GREEN_X/Y already include HOUR_X/Y — do NOT add HOUR_X/Y again at
   layer creation, or the offset gets doubled. */
#define GREEN_X (HOUR_X + GREEN_X_OFFSET)
#define GREEN_Y (HOUR_Y + GREEN_Y_OFFSET)

static int BLUE_X_OFFSET = 80;
static int BLUE_Y_OFFSET = 78;
/* Same rule — BLUE_X/Y already include MINUTE_X/Y. */
#define BLUE_X (MINUTE_X + BLUE_X_OFFSET)
#define BLUE_Y (MINUTE_Y + BLUE_Y_OFFSET)

static int LEFTC_X  = 18;
static int LEFTC_Y  = 52;

static int RIGHTC_X = 134;
static int RIGHTC_Y = 52;

static int DOWNC_X  = 110;
static int DOWNC_Y  = 120;

static int ARCH_X   = 8;
static int ARCH_Y   = 88;

/* Desired rendered size of the arch in pixels (width = height).
   Change this variable to resize the arch at runtime.
   100 = native PDC size. 150 = 1.5× native size, etc.            */
static int ARCH_SCALE = 100;

/* =========================================================
   WINDOW
========================================================= */

static Window *s_window;

/* =========================================================
   TEXT LAYERS
========================================================= */

static TextLayer *s_hour_layer;
static TextLayer *s_minute_layer;
static TextLayer *s_date_layer;
static TextLayer *s_day_layer;

/* =========================================================
   HEARTS
========================================================= */

static BitmapLayer *s_hearts[10];

/* C BUTTONS */
static BitmapLayer *s_leftc_layer;
static BitmapLayer *s_rightc_layer;
static BitmapLayer *s_downc_layer;

static GBitmap *s_leftc_bmp;
static GBitmap *s_rightc_bmp;
static GBitmap *s_downc_bmp;

/* ARCH (vector PDC) */
static Layer              *s_arch_layer;
static GDrawCommandImage  *s_arch_cmd;

/* =========================================================
   MAIN UI
========================================================= */

static BitmapLayer *s_green_circle;
static BitmapLayer *s_blue_circle;

/* =========================================================
   BITMAPS
========================================================= */

static GBitmap *s_green_bmp;
static GBitmap *s_blue_bmp;

static GBitmap *s_heart_full_bmp;
static GBitmap *s_heart_3quarter_bmp;
static GBitmap *s_heart_half_bmp;
static GBitmap *s_heart_quarter_bmp;
static GBitmap *s_heart_empty_bmp;

/* =========================================================
   RECT HELPER
========================================================= */

static GRect rect_pos(int x, int y, int w, int h) {
  return GRect(
    x + MOVE_ALL_X,
    y + MOVE_ALL_Y,
    w,
    h);
}

/* =========================================================
   LOAD RESOURCES
========================================================= */

static void load_resources(void) {

  s_green_bmp =
      gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GREEN);

  s_blue_bmp =
      gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUE);

  s_heart_full_bmp =
      gbitmap_create_with_resource(RESOURCE_ID_HEART_FULL);

  s_heart_3quarter_bmp =
      gbitmap_create_with_resource(RESOURCE_ID_HEART_3QUARTER);

  s_heart_half_bmp =
      gbitmap_create_with_resource(RESOURCE_ID_HEART_HALF);

  s_heart_quarter_bmp =
      gbitmap_create_with_resource(RESOURCE_ID_HEART_QUARTER);

  s_heart_empty_bmp =
      gbitmap_create_with_resource(RESOURCE_ID_HEART_EMPTY);

  s_leftc_bmp  = gbitmap_create_with_resource(RESOURCE_ID_LEFTC);
  s_rightc_bmp = gbitmap_create_with_resource(RESOURCE_ID_RIGHTC);
  s_downc_bmp  = gbitmap_create_with_resource(RESOURCE_ID_CDOWN);

  s_arch_cmd =
      gdraw_command_image_create_with_resource(RESOURCE_ID_ARCH);
}

/* =========================================================
   UNLOAD RESOURCES
========================================================= */

static void unload_resources(void) {

  gbitmap_destroy(s_green_bmp);
  gbitmap_destroy(s_blue_bmp);

  gbitmap_destroy(s_heart_full_bmp);
  gbitmap_destroy(s_heart_3quarter_bmp);
  gbitmap_destroy(s_heart_half_bmp);
  gbitmap_destroy(s_heart_quarter_bmp);
  gbitmap_destroy(s_heart_empty_bmp);

  gbitmap_destroy(s_leftc_bmp);
  gbitmap_destroy(s_rightc_bmp);
  gbitmap_destroy(s_downc_bmp);

  if (s_arch_cmd) {
    gdraw_command_image_destroy(s_arch_cmd);
    s_arch_cmd = NULL;
  }
}

/* =========================================================
   ARCH DRAW
   -------------------------------------------------------
   pdc_transform_gdraw_command_image_draw_transformed()
   derives scale10 from ARCH_SCALE vs. the PDC's native
   bounding-box width so the arch renders at exactly
   ARCH_SCALE × ARCH_SCALE pixels regardless of what size
   the original .pdc was authored at.

   It makes an internal copy of s_arch_cmd each frame, so
   the master image is never mutated.
========================================================= */

static void arch_update_proc(Layer *layer, GContext *ctx) {

  if (!s_arch_cmd) return;

  /* Get the native size baked into the PDC file. */
  GSize native = gdraw_command_image_get_bounds_size(s_arch_cmd);

  /* Compute scale as ×10 integer (e.g. 1.0 → 10, 1.5 → 15).
     Guard against a zero native size just in case the asset is bad. */
  int scale10 = (native.w > 0)
    ? (ARCH_SCALE * 10) / native.w
    : 10;

  /* Draw at (0,0) relative to the layer — the layer itself is
     positioned by rect_pos() in window_load().
     Pass GColorClear for colors to keep the PDC's own colors.
     Pass 0 for stroke_width to keep the PDC's own widths.      */
  pdc_transform_gdraw_command_image_draw_transformed(
    ctx,
    s_arch_cmd,
    GPoint(0, 0),   /* offset inside the layer */
    scale10,        /* scale ×10 */
    0,              /* rotation (degrees, 0 = none) */
    GColorClear,    /* fill_color override  — Clear = use PDC original */
    GColorClear,    /* stroke_color override — Clear = use PDC original */
    0               /* stroke_width override — 0 = use PDC original */
  );
}

/* =========================================================
   HEART LOOKUP
========================================================= */

static GBitmap *heart_bitmap(int level) {

  switch (level) {
    case 4:  return s_heart_full_bmp;
    case 3:  return s_heart_3quarter_bmp;
    case 2:  return s_heart_half_bmp;
    case 1:  return s_heart_quarter_bmp;
    default: return s_heart_empty_bmp;
  }
}

/* =========================================================
   UPDATE HEARTS FROM BATTERY
========================================================= */

static void update_hearts(int battery_percent) {
  /*
     Battery expressed as 40 quarter-subunits (10 hearts × 4 levels).
     Fill order: indices 0..4 = top row left→right,
                          5..9 = bottom row left→right.

     Remainder mapping (how many quarter-subunits are in the partial heart):
       remainder 3 → quarterheart  (level 1)
       remainder 2 → halfheart     (level 2)
       remainder 1 → 3quarterheart (level 3)
       remainder 0 → empty         (level 0)
  */

  int total_subunits = (battery_percent * 40) / 100;
  int full_hearts    = total_subunits / 4;
  int remainder      = total_subunits % 4;

  for (int i = 0; i < 10; i++) {

    if (i < full_hearts) {

      bitmap_layer_set_bitmap(s_hearts[i], heart_bitmap(4));

    } else if (i == full_hearts) {

      int level;
      switch (remainder) {
        case 3:  level = 1; break;
        case 2:  level = 2; break;
        case 1:  level = 3; break;
        default: level = 0; break;
      }
      bitmap_layer_set_bitmap(s_hearts[i], heart_bitmap(level));

    } else {
      bitmap_layer_set_bitmap(s_hearts[i], heart_bitmap(0));
    }
  }
}

/* =========================================================
   BATTERY CALLBACK
========================================================= */

static void battery_callback(BatteryChargeState state) {
  update_hearts(state.charge_percent);
}

/* =========================================================
   UPDATE TIME
========================================================= */

static void update_time(void) {

  time_t       temp      = time(NULL);
  struct tm   *tick_time = localtime(&temp);

  static char hour_buffer[3];
  static char minute_buffer[3];
  static char date_buffer[16];
  static char day_buffer[16];

  int hour = tick_time->tm_hour;

  if (!clock_is_24h_style()) {
    hour = hour % 12;
    if (hour == 0) hour = 12;
  }

  snprintf(hour_buffer,   sizeof(hour_buffer),   "%d",    hour);
  snprintf(minute_buffer, sizeof(minute_buffer),  "%02d",  tick_time->tm_min);

  snprintf(date_buffer,   sizeof(date_buffer),
           "%d/%d",
           tick_time->tm_mday,
           tick_time->tm_mon + 1);

  const char *days[] = {
    "SUN","MON","TUE","WED","THU","FRI","SAT"
  };
  const char *months[] = {
    "JAN","FEB","MAR","APR","MAY","JUN",
    "JUL","AUG","SEP","OCT","NOV","DEC"
  };

  snprintf(day_buffer, sizeof(day_buffer),
           "%s %s",
           days[tick_time->tm_wday],
           months[tick_time->tm_mon]);

  text_layer_set_text(s_hour_layer,   hour_buffer);
  text_layer_set_text(s_minute_layer, minute_buffer);
  text_layer_set_text(s_date_layer,   date_buffer);
  text_layer_set_text(s_day_layer,    day_buffer);
}

/* =========================================================
   TICK
========================================================= */

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

/* =========================================================
   WINDOW LOAD
========================================================= */

static void window_load(Window *window) {

  Layer *window_layer = window_get_root_layer(window);

  load_resources();

  /* -------------------------------------------------------
     BACKGROUND
  ------------------------------------------------------- */

  window_set_background_color(window, GColorBlack);

  /* -------------------------------------------------------
     HEARTS — two rows of 5
  ------------------------------------------------------- */

  for (int i = 0; i < 5; i++) {
    s_hearts[i] = bitmap_layer_create(
        rect_pos(HEARTS_X + (i * 14), HEARTS_Y, 12, 10));
    bitmap_layer_set_compositing_mode(s_hearts[i], GCompOpSet);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_hearts[i]));
  }

  for (int i = 0; i < 5; i++) {
    s_hearts[i + 5] = bitmap_layer_create(
        rect_pos(HEARTS_X + (i * 14), HEARTS_Y + 14, 12, 10));
    bitmap_layer_set_compositing_mode(s_hearts[i + 5], GCompOpSet);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_hearts[i + 5]));
  }

  /* -------------------------------------------------------
     GREEN CIRCLE
     FIX: Use GREEN_X / GREEN_Y directly — they already expand
     to (HOUR_X + GREEN_X_OFFSET) and (HOUR_Y + GREEN_Y_OFFSET).
     Adding HOUR_X/Y again was doubling the offset.
  ------------------------------------------------------- */

  s_green_circle = bitmap_layer_create(
      rect_pos(GREEN_X, GREEN_Y, 54, 54));
  bitmap_layer_set_bitmap(s_green_circle, s_green_bmp);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_green_circle));

  /* -------------------------------------------------------
     HOUR TEXT
  ------------------------------------------------------- */

  s_hour_layer = text_layer_create(
      rect_pos(40 + HOUR_X, 48 + HOUR_Y, 54, 32));

  text_layer_set_background_color(s_hour_layer,   GColorClear);
  text_layer_set_text_color(s_hour_layer,          GColorYellow);
  text_layer_set_font(s_hour_layer,
      fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_hour_layer,      GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_hour_layer));

  /* -------------------------------------------------------
     BLUE CIRCLE
     FIX: Same double-offset fix — BLUE_X/Y already include
     MINUTE_X/Y via the macro.
  ------------------------------------------------------- */

  s_blue_circle = bitmap_layer_create(
      rect_pos(BLUE_X, BLUE_Y, 54, 54));
  bitmap_layer_set_bitmap(s_blue_circle, s_blue_bmp);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_blue_circle));

  /* -------------------------------------------------------
     C BUTTONS
  ------------------------------------------------------- */

  s_leftc_layer = bitmap_layer_create(
      rect_pos(LEFTC_X + HOUR_X, LEFTC_Y + HOUR_Y, 20, 20));
  bitmap_layer_set_bitmap(s_leftc_layer, s_leftc_bmp);
  bitmap_layer_set_compositing_mode(s_leftc_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_leftc_layer));

  s_rightc_layer = bitmap_layer_create(
      rect_pos(RIGHTC_X + MINUTE_X, RIGHTC_Y + MINUTE_Y, 20, 20));
  bitmap_layer_set_bitmap(s_rightc_layer, s_rightc_bmp);
  bitmap_layer_set_compositing_mode(s_rightc_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_rightc_layer));

  s_downc_layer = bitmap_layer_create(
      rect_pos(DOWNC_X + DATE_X, DOWNC_Y + DATE_Y, 20, 20));
  bitmap_layer_set_bitmap(s_downc_layer, s_downc_bmp);
  bitmap_layer_set_compositing_mode(s_downc_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_downc_layer));

  /* -------------------------------------------------------
     ARCH — vector PDC layer
     The layer is sized to ARCH_SCALE × ARCH_SCALE so the
     transformed drawing fits without clipping.
     The actual scaling is done inside arch_update_proc().
  ------------------------------------------------------- */

  s_arch_layer = layer_create(
      rect_pos(ARCH_X + DATE_X, ARCH_Y + DATE_Y, ARCH_SCALE, ARCH_SCALE));
  layer_set_update_proc(s_arch_layer, arch_update_proc);
  layer_add_child(window_layer, s_arch_layer);

  /* -------------------------------------------------------
     MINUTE TEXT
  ------------------------------------------------------- */

  s_minute_layer = text_layer_create(
      rect_pos(80 + MINUTE_X, 86 + MINUTE_Y, 54, 32));

  text_layer_set_background_color(s_minute_layer,  GColorClear);
  text_layer_set_text_color(s_minute_layer,         GColorYellow);
  text_layer_set_font(s_minute_layer,
      fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_minute_layer,     GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_minute_layer));

  /* -------------------------------------------------------
     DATE
  ------------------------------------------------------- */

  s_date_layer = text_layer_create(
      rect_pos(DATE_X + 32, DATE_Y + 120, 80, 24));

  text_layer_set_background_color(s_date_layer,    GColorClear);
  text_layer_set_text_color(s_date_layer,           GColorYellow);
  text_layer_set_font(s_date_layer,
      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_date_layer,       GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  /* -------------------------------------------------------
     DAY
  ------------------------------------------------------- */

  s_day_layer = text_layer_create(
      rect_pos(DATE_X + 24, DATE_Y + 142, 96, 20));

  text_layer_set_background_color(s_day_layer,     GColorClear);
  text_layer_set_text_color(s_day_layer,            GColorYellow);
  text_layer_set_font(s_day_layer,
      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_day_layer,        GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(s_day_layer));

  /* -------------------------------------------------------
     INITIAL DRAW
  ------------------------------------------------------- */

  BatteryChargeState charge = battery_state_service_peek();
  update_hearts(charge.charge_percent);

  update_time();
}

/* =========================================================
   WINDOW UNLOAD
========================================================= */

static void window_unload(Window *window) {

  text_layer_destroy(s_hour_layer);
  text_layer_destroy(s_minute_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_day_layer);

  bitmap_layer_destroy(s_green_circle);
  bitmap_layer_destroy(s_blue_circle);

  for (int i = 0; i < 10; i++) {
    bitmap_layer_destroy(s_hearts[i]);
  }

  bitmap_layer_destroy(s_leftc_layer);
  bitmap_layer_destroy(s_rightc_layer);
  bitmap_layer_destroy(s_downc_layer);

  layer_destroy(s_arch_layer);

  unload_resources();
}

/* =========================================================
   INIT
========================================================= */

static void init(void) {

  s_window = window_create();

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load   = window_load,
    .unload = window_unload,
  });

  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);
}

/* =========================================================
   DEINIT
========================================================= */

static void deinit(void) {

  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  window_destroy(s_window);
}

/* =========================================================
   MAIN
========================================================= */

int main(void) {
  init();
  app_event_loop();
  deinit();
}