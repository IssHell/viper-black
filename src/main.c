#include <pebble.h>
#include <math.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define KEY_UPDATE_UNITS 2
#define KEY_UPDATE_THEME 3
#define PERSIST_KEY_UNIT_FORMAT 10
#define PERSIST_KEY_THEME 11

static Window *s_main_window;
static TextLayer *s_background_layer;
static TextLayer *s_am_pm_layer;
static TextLayer *s_hour_layer;
static TextLayer *s_main_hour_layer;
static TextLayer *s_before_hour_1_layer;
static TextLayer *s_after_hour_1_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;

static Layer *s_main_minutes_layer;

static int theme_buffer = 0;
static BatteryChargeState battery_level_buffer;
static bool connected_buffer;

static char hour_buffer[] = "00";
static char am_pm_buffer[] = "  ";
static char hour_before_1_buffer[] = "00";
static char hour_after_1_buffer[] = "00";
static char mins_buffer[] = "00";
static char date_buffer[] = "000 00";

static GFont s_time_font;
static GFont s_time_font_big;
static GFont s_time_font_medium;
static GFont s_time_font_small;
static GFont s_time_font_extra_small;

static GBitmap *s_bt_on;
static BitmapLayer *s_bt_layer;

static GBitmap *s_battery_status;
static BitmapLayer *s_battery_layer;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  
  //Set date buffer
  strftime(date_buffer, sizeof("000 00"), "%a %d", tick_time);

  // Write the current hours and minutes into the buffers
  if(clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(hour_buffer, sizeof("00"), "%H", tick_time);
    tick_time->tm_hour = tick_time->tm_hour - 1;
    strftime(hour_before_1_buffer, sizeof("00"), "%H", tick_time);
    tick_time->tm_hour = tick_time->tm_hour + 2;
    strftime(hour_after_1_buffer, sizeof("00"), "%H", tick_time);
  } else {
    //Use 12 hour format
    strftime(hour_buffer, sizeof("00"), "%I", tick_time);
    strftime(am_pm_buffer, sizeof("00"), "%p", tick_time);
    tick_time->tm_hour = tick_time->tm_hour - 1;
    strftime(hour_before_1_buffer, sizeof("00"), "%I", tick_time);
    tick_time->tm_hour = tick_time->tm_hour + 2;
    strftime(hour_after_1_buffer, sizeof("00"), "%I", tick_time);
  }
  
  if(hour_buffer[0] == '0') {
    memmove(hour_buffer, &hour_buffer[1], sizeof(hour_buffer) - 1);
  }
  
  if (hour_before_1_buffer[0] == '0') {
    memmove(hour_before_1_buffer, &hour_before_1_buffer[1], sizeof(hour_before_1_buffer) - 1);
  }
  
  if (hour_after_1_buffer[0] == '0') {
    memmove(hour_after_1_buffer, &hour_after_1_buffer[1], sizeof(hour_after_1_buffer) - 1);
  }

  // Display this time on the TextLayers
  text_layer_set_text(s_am_pm_layer, am_pm_buffer);
  text_layer_set_text(s_date_layer, date_buffer);
  text_layer_set_text(s_main_hour_layer, hour_buffer);
  text_layer_set_text(s_before_hour_1_layer, hour_before_1_buffer);
  text_layer_set_text(s_after_hour_1_layer, hour_after_1_buffer);
  
  //Set minutes buffer
  strftime(mins_buffer, sizeof("00"), "%M", tick_time);
}

static void white_line_update(GPoint first_point,GPoint second_point, GContext *ctx){
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, first_point, second_point);
}

static void black_line_update(GPoint first_point,GPoint second_point, GContext *ctx){
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, first_point, second_point);
}

static void black_thick_line_update(GPoint first_point,GPoint second_point, GContext *ctx){
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, first_point, second_point);
}

static void red_line_update(GPoint first_point,GPoint second_point, GContext *ctx){
  graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, first_point, second_point);
}

static void create_text_layer(TextLayer *layer, GColor background, GColor text_color, GFont font, GTextAlignment alignment){
  text_layer_set_background_color(layer, background);
  text_layer_set_text_color(layer, text_color);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, alignment);  
}

static void create_bt_icon_layer(){
  s_bt_on = gbitmap_create_with_resource(RESOURCE_ID_BT_ON);
  s_bt_layer = bitmap_layer_create(GRect(128, 145, 10, 20));
  bitmap_layer_set_bitmap(s_bt_layer, s_bt_on);
  bitmap_layer_set_compositing_mode(s_bt_layer, GCompOpSet);
}

static void create_battery_icon_layer(){
  s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_100);
  s_battery_layer = bitmap_layer_create(GRect(77, 143, 50, 25));
  bitmap_layer_set_bitmap(s_battery_layer, s_battery_status);
  bitmap_layer_set_compositing_mode(s_battery_layer, GCompOpOr);
}

static void bt_handler(bool connected) {
  connected_buffer = connected;
  // Show current connection state
  if (connected_buffer) {
    if (theme_buffer == 0){
      s_bt_on = gbitmap_create_with_resource(RESOURCE_ID_BT_ON);
    }else{
      s_bt_on = gbitmap_create_with_resource(RESOURCE_ID_BT_ON_RED);
    }
  } else {
    vibes_short_pulse();
    if (theme_buffer == 0){
      s_bt_on = gbitmap_create_with_resource(RESOURCE_ID_BT_OFF);
    }else{
      s_bt_on = gbitmap_create_with_resource(RESOURCE_ID_BT_OFF_RED);
    }
  }
  bitmap_layer_set_bitmap(s_bt_layer, s_bt_on);
  bitmap_layer_set_compositing_mode(s_battery_layer, GCompOpSet);
}

static void canvas_update_proc(Layer *this_layer, GContext *ctx) {
  
  // Enable antialiasing
  graphics_context_set_antialiased(ctx, true);
  int i = atoi(mins_buffer); 
  
  if (i <= 5){ // 5 min
    red_line_update(GPoint(20, 0), GPoint(20, 10), ctx); //5 min
  }else{
    if (theme_buffer == 0){
      white_line_update(GPoint(20, 0), GPoint(20, 10), ctx); //5 min
    }else{
      black_thick_line_update(GPoint(20, 0), GPoint(20, 10), ctx); //5 min
    }
  }
  if (i <= 10){ // 10 min
    red_line_update(GPoint(30, 0), GPoint(30, 10), ctx); //5 min
  }else{
    if (theme_buffer == 0){
      white_line_update(GPoint(30, 0), GPoint(30, 10), ctx); //15 min
    }else{
      black_thick_line_update(GPoint(30, 0), GPoint(30, 10), ctx); //15 min
    }
  }
  if (i <= 15){ // 15 min
    red_line_update(GPoint(40, 0), GPoint(40, 20), ctx); //15 min
  }else{
    if (theme_buffer == 0){
      white_line_update(GPoint(40, 0), GPoint(40, 20), ctx); //15 min
    }else{
      black_thick_line_update(GPoint(40, 0), GPoint(40, 20), ctx); //15 min
    }
  }
  if (i <= 20){ // 20 min
    red_line_update(GPoint(50, 0), GPoint(50, 10), ctx); //20 min
  }else{
    if (theme_buffer == 0){
      white_line_update(GPoint(50, 0), GPoint(50, 10), ctx); //20 min
    }else{
      black_thick_line_update(GPoint(50, 0), GPoint(50, 10), ctx); //20 min
    }
  }
  if (i <= 25){ // 25 min
    red_line_update(GPoint(60, 0), GPoint(60, 10), ctx); //25 min
  }else{
    if (theme_buffer == 0){
      white_line_update(GPoint(60, 0), GPoint(60, 10), ctx); //25 min
    }else{
      black_thick_line_update(GPoint(60, 0), GPoint(60, 10), ctx); //25 min
    }
  }
  if (i <= 30){ // 30 min
    red_line_update(GPoint(70, 0), GPoint(70, 40), ctx); //30 min
  }else{
    if (theme_buffer == 0){
      white_line_update(GPoint(70, 0), GPoint(70, 40), ctx); //30 min
    }else{
      black_thick_line_update(GPoint(70, 0), GPoint(70, 40), ctx); //30 min
    }
  }
  if (i <= 35){ // 35 min
    red_line_update(GPoint(80, 0), GPoint(80, 10), ctx); //35 min
  }else{
    if (theme_buffer == 0){
      white_line_update(GPoint(80, 0), GPoint(80, 10), ctx); //35 min
    }else{
      black_thick_line_update(GPoint(80, 0), GPoint(80, 10), ctx); //35 min
    }
  }
  if (i <= 40){ // 40 min
    red_line_update(GPoint(90, 0), GPoint(90, 10), ctx); //40 min
  }else{
    if (theme_buffer == 0){
      white_line_update(GPoint(90, 0), GPoint(90, 10), ctx); //40 min
    }else{
      black_thick_line_update(GPoint(90, 0), GPoint(90, 10), ctx); //40 min
    }
  }
  if (i <= 45){ // 45 min
    red_line_update(GPoint(100, 0), GPoint(100, 20), ctx); //45 min
  }else{
    if (theme_buffer == 0){
      white_line_update(GPoint(100, 0), GPoint(100, 20), ctx); //45 min
    }else{
      black_thick_line_update(GPoint(100, 0), GPoint(100, 20), ctx); //45 min
    }
  }
  if (i <= 50){ // 50 min
    red_line_update(GPoint(110, 0), GPoint(110, 10), ctx); //50 min
  }else{
    if (theme_buffer == 0){
      white_line_update(GPoint(110, 0), GPoint(110, 10), ctx); //50 min
    }else{
      black_thick_line_update(GPoint(110, 0), GPoint(110, 10), ctx); //50 min
    }
  }
  if (i <= 55){ // 55 min
    red_line_update(GPoint(120, 0), GPoint(120, 10), ctx); //55 min
  }else{
    if (theme_buffer == 0){
      white_line_update(GPoint(120, 0), GPoint(120, 10), ctx); //55 min
    }else{
      black_thick_line_update(GPoint(120, 0), GPoint(120, 10), ctx); //55 min
    }
  }
}

static void battery_handler(BatteryChargeState new_state) {
  battery_level_buffer = new_state;
  
  // Write to buffer and display
  if (new_state.is_charging){
    s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_CHARGING);
    if (theme_buffer == 0){
      s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_BLACK_CHARGING);
    } else {
      s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_BLACK_20);
    }
  } else if((new_state.charge_percent <= 20)){
    if (theme_buffer == 0){
      s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_20);
    } else {
      s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_BLACK_20);
    }
  } else if ((new_state.charge_percent == 30) || (new_state.charge_percent == 40)){
    if (theme_buffer == 0){
      s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_40);
    } else {
      s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_BLACK_40);
    }
  } else if ((new_state.charge_percent == 50) || (new_state.charge_percent == 60)){
    if (theme_buffer == 0){
      s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_60);
    } else {
      s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_BLACK_60);
    }
  } else if ((new_state.charge_percent == 70) || (new_state.charge_percent == 80)){
    if (theme_buffer == 0){
      s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_80);
    } else {
      s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_BLACK_80);
    }
  } else if ((new_state.charge_percent == 90) || (new_state.charge_percent == 100)){
    if (theme_buffer == 0){
//       APP_LOG(APP_LOG_LEVEL_INFO, "Using OPTION 1");
      s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_100);
    } else {
      s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_BLACK_100);
    }
  }
  bitmap_layer_set_bitmap(s_battery_layer, s_battery_status);
  bitmap_layer_set_compositing_mode(s_battery_layer, GCompOpSet);
}

static void update_theme(){
  if (theme_buffer == 1){
    text_layer_set_background_color(s_background_layer, GColorWhite);
    text_layer_set_text_color(s_weather_layer, GColorBlack); // Weather
    text_layer_set_text_color(s_main_hour_layer, GColorBlack); // Main hour
    text_layer_set_text_color(s_am_pm_layer, GColorBlack); // AM / PM 
    text_layer_set_text_color(s_date_layer, GColorBlack); // Date 
    text_layer_set_text_color(s_before_hour_1_layer, GColorRed); // Before
    text_layer_set_text_color(s_after_hour_1_layer, GColorRed); // After
  } else if (theme_buffer == 0) {
    text_layer_set_background_color(s_background_layer, GColorBlack);
    text_layer_set_text_color(s_weather_layer, GColorWhite); // Weather
    text_layer_set_text_color(s_main_hour_layer, GColorWhite); // Main hour
    text_layer_set_text_color(s_am_pm_layer, GColorWhite); // AM / PM
    text_layer_set_text_color(s_date_layer, GColorWhite); // Date
    text_layer_set_text_color(s_before_hour_1_layer, GColorRed); // Before
    text_layer_set_text_color(s_after_hour_1_layer, GColorRed); // After
  }
  battery_handler(battery_level_buffer);
  bt_handler(connected_buffer);
}

static void main_window_load(Window *window) {
  s_time_font_big = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MECHA_BOLD_34));
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MECHA_BOLD_24));
  s_time_font_medium = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MECHA_BOLD_18));
  s_time_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MECHA_BOLD_14));
  s_time_font_extra_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MECHA_BOLD_10));
  theme_buffer = persist_read_int(PERSIST_KEY_THEME);
  APP_LOG(APP_LOG_LEVEL_INFO, "Theme: %d", theme_buffer);
  
  s_background_layer = text_layer_create(GRect(0, 0, 144, 168)); // 144 x 166 Create background layer
  s_am_pm_layer = text_layer_create(GRect(2, 38, 144, 20)); // Create AM-PM layer
  s_hour_layer = text_layer_create(GRect(0, 40, 144, 40)); // Create Hour layer and its child layers
  s_main_hour_layer = text_layer_create(GRect(45, 0, 60, 60));
  s_before_hour_1_layer = text_layer_create(GRect(2, 10, 45, 50));
  s_after_hour_1_layer = text_layer_create(GRect(103, 10, 45, 50));
  s_main_minutes_layer = layer_create(GRect(2, 85, 144, 45)); // Create Minutes Layers and its child layers
  s_date_layer = text_layer_create(GRect(0, 145, 80, 20)); // Create Date Layers
  s_weather_layer = text_layer_create(GRect(0, 0, 144, 20)); // Weather Layer
  
  if (theme_buffer == 1){
    create_text_layer(s_background_layer, GColorWhite, GColorClear, s_time_font_small, GTextAlignmentCenter);
    create_text_layer(s_am_pm_layer, GColorClear, GColorBlack, s_time_font_small, GTextAlignmentCenter);
    create_text_layer(s_hour_layer, GColorClear, GColorClear, s_time_font_small, GTextAlignmentCenter);
    create_text_layer(s_main_hour_layer, GColorClear, GColorBlack, s_time_font_big, GTextAlignmentCenter);
    create_text_layer(s_before_hour_1_layer, GColorClear, GColorRed, s_time_font, GTextAlignmentCenter);
    create_text_layer(s_after_hour_1_layer, GColorClear, GColorRed, s_time_font, GTextAlignmentCenter);
    create_text_layer(s_date_layer, GColorClear, GColorBlack, s_time_font_small, GTextAlignmentCenter);
    create_text_layer(s_weather_layer, GColorClear, GColorBlack, s_time_font_small, GTextAlignmentCenter);
  }else{
    create_text_layer(s_background_layer, GColorBlack, GColorClear, s_time_font_small, GTextAlignmentCenter);
    create_text_layer(s_am_pm_layer, GColorClear, GColorWhite, s_time_font_small, GTextAlignmentCenter);
    create_text_layer(s_hour_layer, GColorClear, GColorClear, s_time_font_small, GTextAlignmentCenter);
    create_text_layer(s_main_hour_layer, GColorClear, GColorWhite, s_time_font_big, GTextAlignmentCenter);
    create_text_layer(s_before_hour_1_layer, GColorClear, GColorRed, s_time_font, GTextAlignmentCenter);
    create_text_layer(s_after_hour_1_layer, GColorClear, GColorRed, s_time_font, GTextAlignmentCenter);
    create_text_layer(s_date_layer, GColorClear, GColorWhite, s_time_font_small, GTextAlignmentCenter);
    create_text_layer(s_weather_layer, GColorClear, GColorWhite, s_time_font_small, GTextAlignmentCenter);
  }
  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_background_layer));  
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_am_pm_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_hour_layer));
  layer_add_child(text_layer_get_layer(s_hour_layer), text_layer_get_layer(s_main_hour_layer));
  layer_add_child(text_layer_get_layer(s_hour_layer), text_layer_get_layer(s_before_hour_1_layer));
  layer_add_child(text_layer_get_layer(s_hour_layer), text_layer_get_layer(s_after_hour_1_layer));  
  layer_add_child(window_get_root_layer(window), s_main_minutes_layer);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  text_layer_set_text(s_weather_layer, "Loading...");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
  
  // BT Layer
  create_bt_icon_layer();
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_layer));
  
  // Battery Layer
  create_battery_icon_layer();
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_battery_layer));
  
  // Set the update_proc
  layer_set_update_proc(s_main_minutes_layer, canvas_update_proc);
  // Show current connection state
  bt_handler(bluetooth_connection_service_peek());
  // Get the current battery level
  battery_handler(battery_state_service_peek());
  // Make sure the time is displayed from the start
  update_time();
}

static void main_window_unload(Window *window) {
  //Unload GFont
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_time_font_big);
  fonts_unload_custom_font(s_time_font_medium);
  fonts_unload_custom_font(s_time_font_small);
  fonts_unload_custom_font(s_time_font_extra_small);
  
  // Destroy TextLayers
  text_layer_destroy(s_background_layer);
  text_layer_destroy(s_am_pm_layer);
  text_layer_destroy(s_hour_layer);
  text_layer_destroy(s_main_hour_layer);
  text_layer_destroy(s_before_hour_1_layer);
  text_layer_destroy(s_after_hour_1_layer);
  layer_destroy(s_main_minutes_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_weather_layer);
  
  gbitmap_destroy(s_bt_on);
  bitmap_layer_destroy(s_bt_layer);
  
  gbitmap_destroy(s_battery_status);
  bitmap_layer_destroy(s_battery_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

int Round(float myfloat)
{
  double integral;
  float fraction = (float)modf(myfloat, &integral);
 
  if (fraction >= 0.5)
    integral += 1;
  if (fraction <= -0.5)
    integral -= 1;
 
  return (int)integral;
}

static void set_weather(char temperature_buffer[8], char conditions_buffer[32]){
  static char weather_layer_buffer[32];
  int x;
  static float fahrenheit;
  static float final;
  if (persist_read_int(PERSIST_KEY_UNIT_FORMAT) == 1){
    x = atoi(temperature_buffer);
    fahrenheit = (float)x;
    final = 1.8 * fahrenheit + 32;
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%dF | %s", Round(final), conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }else{
    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%sC | %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }
}

static void update_unit_setting(int unit){
  if (!(persist_exists(PERSIST_KEY_UNIT_FORMAT)) || persist_read_int(PERSIST_KEY_UNIT_FORMAT) != unit) {
    persist_write_int(PERSIST_KEY_UNIT_FORMAT, unit);
  }
}

static void update_theme_setting(int theme){
  if (!(persist_exists(PERSIST_KEY_THEME)) || persist_read_int(PERSIST_KEY_THEME) != theme) {
    persist_write_int(PERSIST_KEY_THEME, theme);
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static int unit_buffer;
  
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_UPDATE_UNITS:
      unit_buffer = (int)t->value->int32;
      update_unit_setting(unit_buffer); 
      break;
    case KEY_UPDATE_THEME:
      theme_buffer = (int)t->value->int32;
      update_theme_setting(theme_buffer);
      update_theme();
      break;
    case KEY_TEMPERATURE:
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d", (int)t->value->int32);
      break;
    case KEY_CONDITIONS:
      snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_INFO, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  
  set_weather(temperature_buffer, conditions_buffer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  // Subscribe to Bluetooth updates
  bluetooth_connection_service_subscribe(bt_handler);
  
  // Subscribe to the Battery State Service
  battery_state_service_subscribe(battery_handler);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}