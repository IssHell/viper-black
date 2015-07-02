#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define KEY_IS_RED_ON  2

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
  graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, first_point, second_point);
}

static void create_background_layer(GColor color) {
  // Create background TextLayer
  s_background_layer = text_layer_create(GRect(0, 0, 144, 168)); // 144 x 166
  text_layer_set_background_color(s_background_layer, color);
}

static void create_am_pm_layer(){
  // Create hour TextLayer
  s_am_pm_layer = text_layer_create(GRect(2, 38, 144, 20));
  text_layer_set_background_color(s_am_pm_layer, GColorClear);
  text_layer_set_text_color(s_am_pm_layer, GColorWhite);
  text_layer_set_text(s_am_pm_layer, "  ");
  //Apply to TextLayer
  text_layer_set_font(s_am_pm_layer, s_time_font_small);
  text_layer_set_text_alignment(s_am_pm_layer, GTextAlignmentCenter);
}

static void create_hour_layer(){
  // Create hour TextLayer
  s_hour_layer = text_layer_create(GRect(0, 40, 144, 40));
  text_layer_set_background_color(s_hour_layer, GColorClear);
}

static void create_minutes_layer(){
  // Create minutes Layer
  s_main_minutes_layer = layer_create(GRect(2, 85, 144, 45));
  //bitmap_layer_set_background_color(s_main_minutes_layer, GColorBlack);
}

static void create_date_layer(){
  s_date_layer = text_layer_create(GRect(0, 145, 80, 20));
  text_layer_set_background_color(s_date_layer, GColorClear);
  
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text(s_date_layer, "000 00");
  
  text_layer_set_font(s_date_layer, s_time_font_small);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
}

static void create_bt_icon_layer(){
  s_bt_on = gbitmap_create_with_resource(RESOURCE_ID_BT_ON_WHITE);
  s_bt_layer = bitmap_layer_create(GRect(128, 145, 10, 20));
  bitmap_layer_set_compositing_mode(s_bt_layer, GCompOpOr);
  bitmap_layer_set_background_color(s_bt_layer, GColorClear);
  bitmap_layer_set_bitmap(s_bt_layer, s_bt_on);
}

static void bt_handler(bool connected) {
  // Show current connection state
  if (connected) {
    s_bt_on = gbitmap_create_with_resource(RESOURCE_ID_BT_ON_WHITE);
    bitmap_layer_set_bitmap(s_bt_layer, s_bt_on);
  } else {
    s_bt_on = gbitmap_create_with_resource(RESOURCE_ID_BT_ON_BLACK);
    bitmap_layer_set_bitmap(s_bt_layer, s_bt_on);
  }
}

static void create_second_hour_layer(GColor color){ //Hour before Layer
  // Create main hour TextLayer
  s_before_hour_1_layer = text_layer_create(GRect(2, 10, 45, 50));
  text_layer_set_background_color(s_before_hour_1_layer, GColorClear);
  text_layer_set_text_color(s_before_hour_1_layer, color);
  text_layer_set_text(s_before_hour_1_layer, "00");
  //Apply to TextLayer
  text_layer_set_font(s_before_hour_1_layer, s_time_font);
  text_layer_set_text_alignment(s_before_hour_1_layer, GTextAlignmentCenter);
}

static void create_main_hour_layer(){
  // Create main hour TextLayer
  s_main_hour_layer = text_layer_create(GRect(45, 0, 60, 60));
  text_layer_set_background_color(s_main_hour_layer, GColorClear);
  text_layer_set_text_color(s_main_hour_layer, GColorWhite);
  text_layer_set_text(s_main_hour_layer, "00");
  //Apply to TextLayer
  text_layer_set_font(s_main_hour_layer, s_time_font_big);
  text_layer_set_text_alignment(s_main_hour_layer, GTextAlignmentCenter);
}

static void create_hour_after_layer(GColor color){ //Hour after Layer
  // Create main hour TextLayer
  s_after_hour_1_layer = text_layer_create(GRect(103, 10, 45, 50));
  text_layer_set_background_color(s_after_hour_1_layer, GColorClear);
  text_layer_set_text_color(s_after_hour_1_layer, color);
  text_layer_set_text(s_after_hour_1_layer, "00");
  //Apply to TextLayer
  text_layer_set_font(s_after_hour_1_layer, s_time_font);
  text_layer_set_text_alignment(s_after_hour_1_layer, GTextAlignmentCenter);
}

static void canvas_update_proc(Layer *this_layer, GContext *ctx) {
  // Enable antialiasing
  graphics_context_set_antialiased(ctx, true);
  int i = atoi(mins_buffer);
  
  if (i <= 5){
    black_line_update(GPoint(20, 0), GPoint(20, 10), ctx); //5 min
  }else{
    white_line_update(GPoint(20, 0), GPoint(20, 10), ctx); //5 min
  }
  if (i <= 10){
    black_line_update(GPoint(30, 0), GPoint(30, 10), ctx); //10 min
  }else{
    white_line_update(GPoint(30, 0), GPoint(30, 10), ctx); //10 min
  }
  if (i <= 15){
    black_line_update(GPoint(40, 0), GPoint(40, 20), ctx); //15 min
  }else{
    white_line_update(GPoint(40, 0), GPoint(40, 20), ctx); //15 min
  }
  if (i <= 20){
    black_line_update(GPoint(50, 0), GPoint(50, 10), ctx); //20 min
  }else{
    white_line_update(GPoint(50, 0), GPoint(50, 10), ctx); //20 min
  }
  if (i <= 25){
    black_line_update(GPoint(60, 0), GPoint(60, 10), ctx); //25 min
  }else{
    white_line_update(GPoint(60, 0), GPoint(60, 10), ctx); //25 min
  }
  if (i <= 30){
    black_line_update(GPoint(70, 0), GPoint(70, 40), ctx); //30 min
  }else{
    white_line_update(GPoint(70, 0), GPoint(70, 40), ctx); //30 min
  }
  if (i <= 35){
    black_line_update(GPoint(80, 0), GPoint(80, 10), ctx); //35 min
  }else{
    white_line_update(GPoint(80, 0), GPoint(80, 10), ctx); //35 min
  }
  if (i <= 40){
    black_line_update(GPoint(90, 0), GPoint(90, 10), ctx); //40 min
  }else{
    white_line_update(GPoint(90, 0), GPoint(90, 10), ctx); //40 min
  }
  if (i <= 45){
    black_line_update(GPoint(100, 0), GPoint(100, 20), ctx); //45 min
  }else{
    white_line_update(GPoint(100, 0), GPoint(100, 20), ctx); //45 min
  }
  if (i <= 50){
    black_line_update(GPoint(110, 0), GPoint(110, 10), ctx); //50 min
  }else{
    white_line_update(GPoint(110, 0), GPoint(110, 10), ctx); //50 min
  }
  if (i <= 55){
    black_line_update(GPoint(120, 0), GPoint(120, 10), ctx); //55 min
  }else{
    white_line_update(GPoint(120, 0), GPoint(120, 10), ctx); //55 min
  }
}

static void create_battery_icon_layer(){
  s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_100_WHITE);
  s_battery_layer = bitmap_layer_create(GRect(77, 143, 50, 25));
  bitmap_layer_set_compositing_mode(s_battery_layer, GCompOpOr);
  bitmap_layer_set_background_color(s_battery_layer, GColorClear);
  bitmap_layer_set_bitmap(s_battery_layer, s_battery_status);
}

static void battery_handler(BatteryChargeState new_state) {
  // Write to buffer and display
  if (new_state.is_charging){
    s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_CHARGING_WHITE);
    bitmap_layer_set_bitmap(s_battery_layer, s_battery_status);
  } else if((new_state.charge_percent <= 20)){
    s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_20_WHITE);
    bitmap_layer_set_bitmap(s_battery_layer, s_battery_status);
  } else if ((new_state.charge_percent == 30) || (new_state.charge_percent == 40)){
    s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_40_WHITE);
    bitmap_layer_set_bitmap(s_battery_layer, s_battery_status);
  } else if ((new_state.charge_percent == 50) || (new_state.charge_percent == 60)){
    s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_60_WHITE);
    bitmap_layer_set_bitmap(s_battery_layer, s_battery_status);
  } else if ((new_state.charge_percent == 70) || (new_state.charge_percent == 80)){
    s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_80_WHITE);
    bitmap_layer_set_bitmap(s_battery_layer, s_battery_status);
  } else if ((new_state.charge_percent == 90) || (new_state.charge_percent == 100)){
    s_battery_status = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_100_WHITE);
    bitmap_layer_set_bitmap(s_battery_layer, s_battery_status);
  }
}

static void create_weather_layer(){
  s_weather_layer = text_layer_create(GRect(0, 0, 144, 20));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_text(s_weather_layer, "Loading...");
  
  text_layer_set_font(s_weather_layer, s_time_font_small);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
}

static void main_window_load(Window *window) {
  s_time_font_big = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MECHA_BOLD_34));
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MECHA_BOLD_24));
  s_time_font_medium = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MECHA_BOLD_18));
  s_time_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MECHA_BOLD_14));
  s_time_font_extra_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MECHA_BOLD_10));
  // Create background layer
  create_background_layer(GColorBlack);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_background_layer));
  // Create AM-PM layer
  create_am_pm_layer();
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_am_pm_layer));
  // Create Hour layer and its child layers
  create_hour_layer();
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_hour_layer));
  
  create_main_hour_layer();
  layer_add_child(text_layer_get_layer(s_hour_layer), text_layer_get_layer(s_main_hour_layer));
  
  create_second_hour_layer(GColorRed);
  layer_add_child(text_layer_get_layer(s_hour_layer), text_layer_get_layer(s_before_hour_1_layer));
  
  create_hour_after_layer(GColorRed);
  layer_add_child(text_layer_get_layer(s_hour_layer), text_layer_get_layer(s_after_hour_1_layer));
  // Create Minutes Layers and its child layers
  create_minutes_layer();
  layer_add_child(window_get_root_layer(window), s_main_minutes_layer);
  // Create Minutes Layers
  create_date_layer();
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  // BT Layer
  create_bt_icon_layer();
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_layer));
  // Battery Layer
  create_battery_icon_layer();
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_battery_layer));
  // Weather Layer
  create_weather_layer();
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
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
  text_layer_destroy(s_am_pm_layer);
  text_layer_destroy(s_hour_layer);
  text_layer_destroy(s_background_layer);
  text_layer_destroy(s_main_hour_layer);
  text_layer_destroy(s_before_hour_1_layer);
  text_layer_destroy(s_after_hour_1_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_weather_layer);
  
  layer_destroy(s_main_minutes_layer);
  
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

static void set_color(char is_red[]){
  if (strncmp(is_red, "on", 2) == 0){
    APP_LOG(APP_LOG_LEVEL_INFO, "Color red is here");
    create_background_layer(GColorRed);
    create_second_hour_layer(GColorRed);
    create_hour_after_layer(GColorRed);
  }else{
    APP_LOG(APP_LOG_LEVEL_INFO, "Color black is here");
    create_background_layer(GColorBlack);
    create_second_hour_layer(GColorBlack);
    create_hour_after_layer(GColorBlack);
  }
}

static void set_weather(char temperature_buffer[8], char conditions_buffer[32]){
  static char weather_layer_buffer[32];
  // Assemble full string and display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s | %s", temperature_buffer, conditions_buffer);
  text_layer_set_text(s_weather_layer, weather_layer_buffer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_TEMPERATURE:
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)t->value->int32);
      break;
    case KEY_CONDITIONS:
      snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
      break;
    case KEY_IS_RED_ON:
      set_color(t->value->cstring);
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