/*
Get Up and Go
Edwin Finch

Under MIT license.
*/

#include <pebble.h>
#include "elements.h"
#include "num2words-en.h"
void tick_handler(struct tm *t, TimeUnits unit_changed);
void handle_battery(BatteryChargeState charge_state);
void bt_handler(bool connected);

// Animation handler
static void animationStoppedHandler(struct Animation *animation, bool finished, void *context)
{
	Layer *textLayer = text_layer_get_layer((TextLayer *)context);
	GRect rect = layer_get_frame(textLayer);
	rect.origin.x = 144;
	layer_set_frame(textLayer, rect);
}

// Animate line
static void makeAnimationsForLayers(Line *line, TextLayer *current, TextLayer *next)
{
	GRect fromRect = layer_get_frame(text_layer_get_layer(next));
	GRect toRect = fromRect;
	toRect.origin.x -= 144;

	line->nextAnimation = property_animation_create_layer_frame(text_layer_get_layer(next), &fromRect, &toRect);
	animation_set_duration((Animation *)line->nextAnimation, 400);
	animation_set_curve((Animation *)line->nextAnimation, AnimationCurveEaseOut);
	animation_schedule((Animation *)line->nextAnimation);

	GRect fromRect2 = layer_get_frame(text_layer_get_layer(current));
	GRect toRect2 = fromRect2;
	toRect2.origin.x -= 144;

	line->currentAnimation = property_animation_create_layer_frame(text_layer_get_layer(current), &fromRect2, &toRect2);
	animation_set_duration((Animation *)line->currentAnimation, 400);
	animation_set_curve((Animation *)line->currentAnimation, AnimationCurveEaseOut);

	animation_set_handlers((Animation *)line->currentAnimation, (AnimationHandlers) {
		.stopped = (AnimationStoppedHandler)animationStoppedHandler
	}, current);

	animation_schedule((Animation *)line->currentAnimation);
}

// Update line
static void updateLineTo(Line *line, char lineStr[2][BUFFER_SIZE], char *value)
{
	TextLayer *next, *current;

	GRect rect = layer_get_frame(text_layer_get_layer(line->currentLayer));
	current = (rect.origin.x == 0) ? line->currentLayer : line->nextLayer;
	next = (current == line->currentLayer) ? line->nextLayer : line->currentLayer;

	// Update correct text only
	if (current == line->currentLayer) {
		memset(lineStr[1], 0, BUFFER_SIZE);
		memcpy(lineStr[1], value, strlen(value));
		text_layer_set_text(next, lineStr[1]);
	} else {
		memset(lineStr[0], 0, BUFFER_SIZE);
		memcpy(lineStr[0], value, strlen(value));
		text_layer_set_text(next, lineStr[0]);
	}

	makeAnimationsForLayers(line, current, next);
}

// Check to see if the current line needs to be updated
static bool needToUpdateLine(Line *line, char lineStr[2][BUFFER_SIZE], char *nextValue)
{
	char *currentStr;
	GRect rect = layer_get_frame(text_layer_get_layer(line->currentLayer));
	currentStr = (rect.origin.x == 0) ? lineStr[0] : lineStr[1];

	if (memcmp(currentStr, nextValue, strlen(nextValue)) != 0 ||
		(strlen(nextValue) == 0 && strlen(currentStr) != 0)) {
		return true;
	}
	return false;
}

// Update screen based on new time
static void display_time(struct tm *t)
{
	// The current time text will be stored in the following 3 strings
	char textLine1[BUFFER_SIZE];
	char textLine2[BUFFER_SIZE];
	char textLine3[BUFFER_SIZE];

	time_to_3words(t->tm_hour, t->tm_min, textLine1, textLine2, textLine3, BUFFER_SIZE);

	if (needToUpdateLine(&line1, line1Str, textLine1)) {
		updateLineTo(&line1, line1Str, textLine1);	
	}
	if (needToUpdateLine(&line2, line2Str, textLine2)) {
		updateLineTo(&line2, line2Str, textLine2);	
	}
	if (needToUpdateLine(&line3, line3Str, textLine3)) {
		updateLineTo(&line3, line3Str, textLine3);	
	}
}

// Update screen without animation first time we start the watchface
static void display_initial_time(struct tm *t)
{
	time_to_3words(t->tm_hour, t->tm_min, line1Str[0], line2Str[0], line3Str[0], BUFFER_SIZE);

	text_layer_set_text(line1.currentLayer, line1Str[0]);
	text_layer_set_text(line2.currentLayer, line2Str[0]);
	text_layer_set_text(line3.currentLayer, line3Str[0]);
}

// Configure the first line of text
static void configureBoldLayer(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, boldFont);
	text_layer_set_text_color(textlayer, GColorWhite);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, GTextAlignmentLeft);
}

// Configure for the 2nd and 3rd lines
static void configureLightLayer(TextLayer *textlayer)
{
	text_layer_set_font(textlayer, lightFont);
	text_layer_set_text_color(textlayer, GColorWhite);
	text_layer_set_background_color(textlayer, GColorClear);
	text_layer_set_text_alignment(textlayer, GTextAlignmentLeft);
}

void line_layer_update_callback(Layer *layer, GContext* ctx) {
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}
	
void select(ClickRecognizerRef click, void *context){
	window_stack_push(menu_window, true);
}

void up(ClickRecognizerRef oi_m8, void *context){
	mTimer.isRunning = 1;
}

void down(ClickRecognizerRef oi_m8, void *context){
	mTimer.hours = settings.defaultHours;
	mTimer.minutes = settings.defaultMinutes;
	mTimer.seconds = settings.defaultSeconds;
	mTimer.isRunning = 1;
	shouldVibrate = false;
}

static TextLayer* text_layer_init(GRect location, GColor background, GTextAlignment alignment, int font)
{
	TextLayer *layer = text_layer_create(location);
	text_layer_set_text_color(layer, GColorBlack);
	text_layer_set_background_color(layer, background);
	text_layer_set_text_alignment(layer, alignment);
	if(font == 1){
		text_layer_set_font(layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	}
	else if(font == 2){
		text_layer_set_font(layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
	}
	else if(font == 3){
		text_layer_set_font(layer, digital_f);
	}
	else if(font == 4){
		text_layer_set_font(layer, digital_f_small);
	}
	else if(font == 5){
		text_layer_set_font(layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
	}
	else if(font == 6){
		text_layer_set_font(layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	}
	return layer;
}

void click_rec(void *context){
	window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) up);
	window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) down);
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) select);
}

void window_load_wf(Window *w){
	window_set_click_config_provider(w, click_rec);
	Layer *window_layer = window_get_root_layer(w);
	window_set_background_color(w, GColorBlack);
	
	//Simplicity
	if(settings.watchface == 0){
		text_date_layer = text_layer_create(GRect(8, 68, 144-8, 168-68));
		text_layer_set_text_color(text_date_layer, GColorWhite);
  		text_layer_set_background_color(text_date_layer, GColorClear);
		text_layer_set_font(text_date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
		layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

		text_time_layer = text_layer_create(GRect(7, 92, 144-7, 168-92));
		text_layer_set_text_color(text_time_layer, GColorWhite);
		text_layer_set_background_color(text_time_layer, GColorClear);
		text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
		layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

		GRect line_frame = GRect(8, 97, 139, 2);
		line_layer = layer_create(line_frame);
		layer_set_update_proc(line_layer, line_layer_update_callback);
		layer_add_child(window_layer, line_layer);
	}
	else if(settings.watchface == 1){
		// 1st line layers
		line1.currentLayer = text_layer_create(GRect(0, 48, 144, 50));
		line1.nextLayer = text_layer_create(GRect(144, 48, 144, 50));
		configureBoldLayer(line1.currentLayer);
		configureBoldLayer(line1.nextLayer);

		// 2nd layers
		line2.currentLayer = text_layer_create(GRect(0, 85, 144, 50));
		line2.nextLayer = text_layer_create(GRect(144, 85, 144, 50));
		configureLightLayer(line2.currentLayer);
		configureLightLayer(line2.nextLayer);

		// 3rd layers
		line3.currentLayer = text_layer_create(GRect(0, 115, 144, 50));
		line3.nextLayer = text_layer_create(GRect(144, 115, 144, 50));
		configureLightLayer(line3.currentLayer);
		configureLightLayer(line3.nextLayer);
		
		// Load layers
		layer_add_child(window_layer, text_layer_get_layer(line1.currentLayer));
		layer_add_child(window_layer, text_layer_get_layer(line1.nextLayer));
		layer_add_child(window_layer, text_layer_get_layer(line2.currentLayer));
		layer_add_child(window_layer, text_layer_get_layer(line2.nextLayer));
		layer_add_child(window_layer, text_layer_get_layer(line3.currentLayer));
		layer_add_child(window_layer, text_layer_get_layer(line3.nextLayer));
		
		time_t now = time(NULL);
  		struct tm *ti = localtime(&now);
		display_initial_time(ti);
	}
	else if(settings.watchface == 2){
		digital_time_layer = text_layer_init(GRect(0, 65, 144, 168), GColorClear, GTextAlignmentCenter, 3);
		text_layer_set_text_color(digital_time_layer, GColorWhite);
		layer_add_child(window_layer, text_layer_get_layer(digital_time_layer));
		
		digital_date_layer = text_layer_init(GRect(0, 110, 144, 168), GColorClear, GTextAlignmentCenter, 4);
		text_layer_set_text_color(digital_date_layer, GColorWhite);
		layer_add_child(window_layer, text_layer_get_layer(digital_date_layer));
	}
	timer_layer = text_layer_init(GRect(0, 0, 144, 168), GColorClear, GTextAlignmentCenter, 1);
	text_layer_set_text_color(timer_layer, GColorWhite);
	layer_add_child(window_layer, text_layer_get_layer(timer_layer));
	
	battery_text_layer = text_layer_init(GRect(-50, 20, 144, 168), GColorClear, GTextAlignmentCenter, 2);
	text_layer_set_text_color(battery_text_layer, GColorWhite);
	layer_add_child(window_layer, text_layer_get_layer(battery_text_layer));
	layer_set_hidden(text_layer_get_layer(battery_text_layer), settings.battery);
	
	wf_theme = inverter_layer_create(GRect(0, 0, 144, 168));
	layer_add_child(window_layer, inverter_layer_get_layer(wf_theme));
	layer_set_hidden(inverter_layer_get_layer(wf_theme), settings.theme);
	
	bluetooth_layer = bitmap_layer_create(GRect(50, -49, 144, 168));
	bitmap_layer_set_background_color(bluetooth_layer, GColorClear);
	bitmap_layer_set_alignment(bluetooth_layer, GAlignCenter);
	layer_add_child(window_layer, bitmap_layer_get_layer(bluetooth_layer));
	layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), settings.bluetooth);
	
	struct tm *ti;
  	time_t temp;        
  	temp = time(NULL);        
  	ti = localtime(&temp);
	
	tick_handler(ti, SECOND_UNIT);
	
	bool connected = bluetooth_connection_service_peek();
	bt_handler(connected);
	BatteryChargeState charge_state = battery_state_service_peek();
	handle_battery(charge_state);
}

void window_unload_wf(Window *w){
	switch(settings.watchface){
		case 0:
			text_layer_destroy(text_date_layer);
			text_layer_destroy(text_time_layer);
			layer_destroy(line_layer);
			break;
		case 1:
			text_layer_destroy(line1.currentLayer);
			text_layer_destroy(line1.nextLayer);
			text_layer_destroy(line2.currentLayer);
			text_layer_destroy(line2.nextLayer);
			text_layer_destroy(line3.currentLayer);
			text_layer_destroy(line3.nextLayer);
			break;
		case 2:
			text_layer_destroy(digital_time_layer);
			text_layer_destroy(digital_date_layer);
			break;
	}
	text_layer_destroy(timer_layer);
	bitmap_layer_destroy(bluetooth_layer);
	text_layer_destroy(battery_text_layer);
	inverter_layer_destroy(wf_theme);
}

void bt_handler(bool connected){
	if(settings.theme == 0){
		if(connected){
			bitmap_layer_set_bitmap(bluetooth_layer, connected_b);
		}
		else{
			bitmap_layer_set_bitmap(bluetooth_layer, disconnected_b);
		}
	}
	else{
		if(connected){
			bitmap_layer_set_bitmap(bluetooth_layer, connected_invert);
		}
		else{
			bitmap_layer_set_bitmap(bluetooth_layer, disconnected_invert);
		}
	}
}

void tick_handler(struct tm *tick_time, TimeUnits unit_changed){
	if(settings.watchface == 0){
		static char time_text[] = "00:00";
		static char date_text[] = "Xxxxxxxxx 00";

		char *time_format;

		if (!tick_time) {
		  time_t now = time(NULL);
		  tick_time = localtime(&now);
		}

		strftime(date_text, sizeof(date_text), "%B %e", tick_time);
		text_layer_set_text(text_date_layer, date_text);


		if (clock_is_24h_style()) {
		  time_format = "%R";
		} else {
		  time_format = "%I:%M";
		}

		strftime(time_text, sizeof(time_text), time_format, tick_time);

		if (!clock_is_24h_style() && (time_text[0] == '0')) {
		  memmove(time_text, &time_text[1], sizeof(time_text) - 1);
		}

		text_layer_set_text(text_time_layer, time_text);
	}
	else if(settings.watchface == 1){
		t = tick_time;
		display_time(tick_time);
	}
	else if(settings.watchface == 2){
		if(clock_is_24h_style()){
		  strftime(digital_time_buffer, sizeof(digital_time_buffer), "%H:%M:%S", tick_time);
		}
		else{
		  strftime(digital_time_buffer, sizeof(digital_time_buffer),"%I:%M:%S", tick_time);
		}
		text_layer_set_text(digital_time_layer, digital_time_buffer);
		
		strftime(date_buffer, sizeof(date_buffer), "%B %e", tick_time);
		text_layer_set_text(digital_date_layer, date_buffer);
	}
	
	if(mTimer.isRunning){
		mTimer.seconds--;
			if(mTimer.seconds == 0 && mTimer.minutes == 0 && mTimer.hours == 0) 
			{
				shouldVibrate = true;
				mTimer.hours = settings.defaultHours;
				mTimer.minutes = settings.defaultMinutes;
				mTimer.seconds = settings.defaultSeconds;
				return;
			}
			if(mTimer.seconds < 0)
			{
				if(mTimer.minutes > 0){
					mTimer.minutes--;
					mTimer.seconds = 59;
				}
				else if(mTimer.minutes == 0 && mTimer.hours > 0){
					mTimer.seconds = 59;
					mTimer.minutes = 59;
					mTimer.hours--;
					
				}
			}
		if(mTimer.hours > 0){
			if(mTimer.seconds < 10){
				if(mTimer.minutes < 10){
					snprintf(timer_buffer, sizeof(timer_buffer), "%d:0%d:0%d", mTimer.hours, mTimer.minutes, mTimer.seconds);
				}
				else{
					snprintf(timer_buffer, sizeof(timer_buffer), "%d:%d:0%d", mTimer.hours, mTimer.minutes, mTimer.seconds);
				}
			}
			else{
				if(mTimer.minutes < 10){
					snprintf(timer_buffer, sizeof(timer_buffer), "%d:0%d:%d", mTimer.hours, mTimer.minutes, mTimer.seconds);
				}
				else{
					snprintf(timer_buffer, sizeof(timer_buffer), "%d:%d:%d", mTimer.hours, mTimer.minutes, mTimer.seconds);
				}
			}
		}
		else{
			if(mTimer.seconds < 10){
				snprintf(timer_buffer, sizeof(timer_buffer), "%d:0%d", mTimer.minutes, mTimer.seconds);
			}
			else{
				snprintf(timer_buffer, sizeof(timer_buffer), "%d:%d", mTimer.minutes, mTimer.seconds);
			}
		}
		text_layer_set_text(timer_layer, timer_buffer);
	}
	if(shouldVibrate){
		vibes_short_pulse();
	}
}

void aboot_callback(int index, void *ctx){
	window_stack_push(aboot_window, true);
}

void timer_callback(int index, void *ctx){
	window_stack_push(timer_selector_window, true);
}

void watchface_callback(int index, void *ctx){
	window_stack_push(wf_selector_window, true);
}

void settings_callback(int in, void *ctx){
	window_stack_push(settings_window, true);
}

void pause_callback(int index, void *ctx){
	mTimer.isRunning = !mTimer.isRunning;
	if(mTimer.isRunning){
		first_menu_items[1].title = "Pause Timer";
		first_menu_items[1].icon = pause;
	}
	else{
		first_menu_items[1].title = "Resume Timer";
		first_menu_items[1].icon = play;
	}
	layer_mark_dirty(simple_menu_layer_get_layer(main_menu_layer));
}

void window_load_menu(Window *window){
	first_menu_items[0] = (SimpleMenuItem){
    	.title = "Timer",
		.callback = timer_callback,
		.icon = timer_b,
    };
	first_menu_items[1] = (SimpleMenuItem){
    	.title = "Pause Timer",
		.callback = pause_callback,
    };
	first_menu_items[2] = (SimpleMenuItem){
    	.title = "Watchface",
		.callback = watchface_callback,
		.icon = watchface,
    };
	first_menu_items[3] = (SimpleMenuItem){
    	.title = "About",
		.callback = aboot_callback,
		.icon = aboot,
    };
	first_menu_items[4] = (SimpleMenuItem){
    	.title = "Settings",
		.callback = settings_callback,
		.icon = settings_b,
    };

	menu_sections[0] = (SimpleMenuSection){
		.num_items = NUM_FIRST_MENU_ITEMS,
    	.items = first_menu_items,
    };
	
	if(mTimer.isRunning){
		first_menu_items[1].title = "Pause Timer";
		first_menu_items[1].icon = pause;
	}
	else{
		first_menu_items[1].title = "Resume Timer";
		first_menu_items[1].icon = play;
	}

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  main_menu_layer = simple_menu_layer_create(bounds, window, menu_sections, NUM_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(main_menu_layer));
	
	main_menu_theme = inverter_layer_create(GRect(0, 0, 144, 168));
	layer_add_child(window_layer, inverter_layer_get_layer(main_menu_theme));
	layer_set_hidden(inverter_layer_get_layer(main_menu_theme), settings.theme);	
}

void window_unload_menu(Window *window){
	simple_menu_layer_destroy(main_menu_layer);
	inverter_layer_destroy(main_menu_theme);
}

void window_load_aboot(Window *w){
	Layer *window_layer = window_get_root_layer(w);
	aboot_edwin = text_layer_init(GRect(0, 10, 144, 168), GColorClear, GTextAlignmentCenter, 1);
	aboot_doug = text_layer_init(GRect(0, 70, 144, 168), GColorClear, GTextAlignmentCenter, 2);
	aboot_version = text_layer_init(GRect(0, 110, 144, 168), GColorClear, GTextAlignmentCenter, 2);
	aboot_theme = inverter_layer_create(GRect(0, 0, 144, 168));
	
	text_layer_set_text(aboot_edwin, "Created by Edwin Finch");
	text_layer_set_text(aboot_doug, "Idea by Doug German");
	text_layer_set_text(aboot_version, "v. 1.2-b2");
	
	layer_add_child(window_layer, text_layer_get_layer(aboot_edwin));
	layer_add_child(window_layer, text_layer_get_layer(aboot_doug));
	layer_add_child(window_layer, text_layer_get_layer(aboot_version));
	layer_add_child(window_layer, inverter_layer_get_layer(aboot_theme));
}

void window_unload_aboot(Window *m8){
	text_layer_destroy(aboot_edwin);
	text_layer_destroy(aboot_doug);
	text_layer_destroy(aboot_version);
	inverter_layer_destroy(aboot_theme);
}

void theme_callback(int index, void *u){
	settings.theme = !settings.theme;
	if(settings.theme){
		first_set_menu_items[0].subtitle = "Default";
	}
	else{
		first_set_menu_items[0].subtitle = "Inverted";
	}
	layer_mark_dirty(simple_menu_layer_get_layer(set_menu_layer));
	
	layer_set_hidden(inverter_layer_get_layer(wf_theme), settings.theme);
	layer_set_hidden(inverter_layer_get_layer(main_menu_theme), settings.theme);
	layer_set_hidden(inverter_layer_get_layer(settings_theme), settings.theme);
	
	bool i = bluetooth_connection_service_peek();
	bt_handler(i);
}

void battery_callback(int index, void *wot){
	settings.battery = !settings.battery;
	if(settings.battery){
		first_set_menu_items[2].subtitle = "Will hide";
	}
	else{
		first_set_menu_items[2].subtitle = "Will show";
	}
	layer_set_hidden(text_layer_get_layer(battery_text_layer), settings.battery);
	layer_mark_dirty(simple_menu_layer_get_layer(set_menu_layer));
}

void bt_callback(int INDEX, void *m8){
	settings.bluetooth = !settings.bluetooth;
	if(settings.bluetooth){
		first_set_menu_items[1].subtitle = "Will hide";
	}
	else{
		first_set_menu_items[1].subtitle = "Will show";
	}
	layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), settings.bluetooth);
	layer_mark_dirty(simple_menu_layer_get_layer(set_menu_layer));
	
	bool i = bluetooth_connection_service_peek();
	bt_handler(i);
}

void onclose_callback(int index, void *ctz){
	settings.onclose = !settings.onclose;
	if(settings.onclose){
		first_set_menu_items[3].subtitle = "Save timer";
	}
	else{
		first_set_menu_items[3].subtitle = "Discard timer";
	}
	layer_mark_dirty(simple_menu_layer_get_layer(set_menu_layer));
}

/*
Coming soon
*/
void vibrate_callback(int index, void *ctx){
	
}

void window_load_settings(Window *oi){
	first_set_menu_items[0] = (SimpleMenuItem){
    	.title = "Theme",
		.callback = theme_callback,
		.icon = theme,
    };
	first_set_menu_items[1] = (SimpleMenuItem){
    	.title = "Bluetooth",
		.callback = bt_callback,
		.icon = bluetooth,
    };
	first_set_menu_items[2] = (SimpleMenuItem){
    	.title = "Battery",
		.callback = battery_callback,
		.icon = battery,
    };
	first_set_menu_items[3] = (SimpleMenuItem){
    	.title = "On close:",
		.callback = onclose_callback,
		.icon = onclose,
    };

	menu_set_sections[0] = (SimpleMenuSection){
		.num_items = NUM_SET_FIRST_MENU_ITEMS,
    	.items = first_set_menu_items,
    };

	if(settings.bluetooth){
		first_set_menu_items[1].subtitle = "Will hide";
	}
	else{
		first_set_menu_items[1].subtitle = "Will show";
	}
	if(settings.battery){
		first_set_menu_items[2].subtitle = "Will hide";
	}
	else{
		first_set_menu_items[2].subtitle = "Will show";
	}
	if(settings.theme){
		first_set_menu_items[0].subtitle = "Default";
	}
	else{
		first_set_menu_items[0].subtitle = "Inverted";
	}
	if(settings.onclose){
		first_set_menu_items[3].subtitle = "Save timer";
	}
	else{
		first_set_menu_items[3].subtitle = "Discard timer";
	}
	
  Layer *window_layer = window_get_root_layer(oi);
  GRect bounds = layer_get_frame(window_layer);

  set_menu_layer = simple_menu_layer_create(bounds, oi, menu_set_sections, NUM_SET_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(set_menu_layer));
	
	settings_theme = inverter_layer_create(GRect(0, 0, 144, 168));
	layer_add_child(window_layer, inverter_layer_get_layer(settings_theme));
	layer_set_hidden(inverter_layer_get_layer(settings_theme), settings.theme);
}

void window_unload_settings(Window *m8){
	simple_menu_layer_destroy(set_menu_layer);
	inverter_layer_destroy(settings_theme);
}

void wf_callback(int index, void *ctx){
	int i;
	for(i = 0; i < 3; i++){
		first_wf_menu_items[i].subtitle = "";
	}
	first_wf_menu_items[index].subtitle = "Selected";
	layer_mark_dirty(simple_menu_layer_get_layer(wf_menu_layer));
	
	window_unload_wf(watchface_window);
	settings.watchface = index;
	window_load_wf(watchface_window);
}

void window_load_wf_sel(Window *hello){
	first_wf_menu_items[0] = (SimpleMenuItem){
    	.title = "Simplicity",
		.callback = wf_callback,
		//.icon = menu_icon_simplicity,
    };
	first_wf_menu_items[1] = (SimpleMenuItem){
    	.title = "Text watch",
		.callback = wf_callback,
		//.icon = menu_icon_textwatch,
    };
	first_wf_menu_items[2] = (SimpleMenuItem){
    	.title = "Digital",
		.callback = wf_callback,
		//.icon = menu_icon_digital,
    };

	menu_wf_sections[0] = (SimpleMenuSection){
		.num_items = NUM_WF_FIRST_MENU_ITEMS,
    	.items = first_wf_menu_items,
    };
	
	int i;
	for(i = 0; i < 3; i++){
		first_wf_menu_items[i].subtitle = "";
	}
	first_wf_menu_items[settings.watchface].subtitle = "Selected";
	
  Layer *window_layer = window_get_root_layer(hello);
  GRect bounds = layer_get_frame(window_layer);

  wf_menu_layer = simple_menu_layer_create(bounds, hello, menu_wf_sections, NUM_WF_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(wf_menu_layer));
	
	wf_sel_theme = inverter_layer_create(GRect(0, 0, 144, 168));
	layer_add_child(window_layer, inverter_layer_get_layer(wf_sel_theme));
	layer_set_hidden(inverter_layer_get_layer(wf_sel_theme), settings.theme);
}

void window_unload_wf_sel(Window *windowwindowwindow){
	simple_menu_layer_destroy(wf_menu_layer);
	inverter_layer_destroy(wf_sel_theme);
}

void handle_battery(BatteryChargeState charge_state) {
	static char battery_text[] = "100%";
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
    text_layer_set_text(battery_text_layer, battery_text);
}

void timer_add(ClickRecognizerRef add, void *context){
	set_value++;
	if(set_value > 59){
		set_value = 0;
	}
	snprintf(timer_set_buffer, sizeof(timer_set_buffer), "%d", set_value);
	text_layer_set_text(timer_set_layer_main, timer_set_buffer);
}

void timer_minus(ClickRecognizerRef minus, void *context){
	set_value--;
	if(set_value < 0){
		set_value = 59;
	}
	snprintf(timer_set_buffer, sizeof(timer_set_buffer), "%d", set_value);
	text_layer_set_text(timer_set_layer_main, timer_set_buffer);
}

void timer_next(ClickRecognizerRef next, void *context){
	if(timer_sel_section == 1){
		text_layer_set_text(timer_set_layer_main, "0");
		text_layer_set_text(timer_set_layer_des, "Minutes");
		timer_sel_section = 2;
		settings.defaultHours = set_value;
	}
	else if(timer_sel_section == 2){
		text_layer_set_text(timer_set_layer_main, "0");
		text_layer_set_text(timer_set_layer_des, "Seconds");
		timer_sel_section = 3;
		settings.defaultMinutes = set_value;
	}
	else if(timer_sel_section == 3){
		settings.defaultSeconds = set_value;
		//Why would you not want to animate it
		window_stack_pop(true);
		
		if(settings.defaultSeconds + settings.defaultMinutes + settings.defaultHours == 0){
			settings.defaultHours = 1;
			settings.defaultMinutes = 30;
		}
	}
	set_value = 0;
}

void click_rec_2(void *context){
	window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, (ClickHandler) timer_add);
	window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 50, (ClickHandler) timer_minus);
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) timer_next);
}

void window_load_timer(Window *window){
	ab = action_bar_layer_create();
	action_bar_layer_set_icon(ab, BUTTON_ID_SELECT, next_b);
	action_bar_layer_set_icon(ab, BUTTON_ID_UP, up_b);
	action_bar_layer_set_icon(ab, BUTTON_ID_DOWN, down_b);
	action_bar_layer_add_to_window(ab, window);
	
	window_set_click_config_provider(window, click_rec_2);
	
	timer_set_layer_main = text_layer_init(GRect(0, 30, 144, 168), GColorClear, GTextAlignmentCenter, 5);
	timer_set_layer_des = text_layer_init(GRect(0, 80, 144, 168), GColorClear, GTextAlignmentCenter, 6);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(timer_set_layer_main));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(timer_set_layer_des));
	
	text_layer_set_text(timer_set_layer_main, "0");
	set_value = 0;
	timer_sel_section = 1;
	text_layer_set_text(timer_set_layer_des, "Hours");
}

void window_unload_timer(Window *window){
	text_layer_destroy(timer_set_layer_main);
	text_layer_destroy(timer_set_layer_des);
}

void init(){
	if(persist_exists(SETTINGS_KEY)){
		value = persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
		APP_LOG(APP_LOG_LEVEL_INFO, "Fetched %d bytes of data from settings.", value);
		value = persist_read_data(TIMER_KEY, &mTimer, sizeof(mTimer));
		APP_LOG(APP_LOG_LEVEL_INFO, "Fetched %d bytes of data from timer data.", value);
	}
	else{
		settings.watchface = 0;
		settings.bluetooth = 1;
		settings.battery = 1;
		settings.theme = 1;
		settings.onclose = 1;
		settings.defaultHours = 1;
		settings.defaultMinutes = 30;
		settings.defaultSeconds = 0;
		
		mTimer.hours = 1;
		mTimer.minutes = 30;
		mTimer.seconds = 0;
		mTimer.isRunning = 0;
	}
	
	menu_window = window_create();
	aboot_window = window_create();
	settings_window = window_create();
	watchface_window = window_create();
	wf_selector_window = window_create();
	timer_selector_window = window_create();
	
	window_set_window_handlers(watchface_window, (WindowHandlers){
		.load = window_load_wf,
		.unload = window_unload_wf,
	});
	window_set_window_handlers(menu_window, (WindowHandlers){
		.load = window_load_menu,
		.unload = window_unload_menu,
	});
	window_set_window_handlers(aboot_window, (WindowHandlers){
		.load = window_load_aboot,
		.unload = window_unload_aboot,
	});
	window_set_window_handlers(settings_window, (WindowHandlers){
		.load = window_load_settings,
		.unload = window_unload_settings,
	});
	window_set_window_handlers(wf_selector_window, (WindowHandlers){
		.load = window_load_wf_sel,
		.unload = window_unload_wf_sel,
	});
	window_set_window_handlers(timer_selector_window, (WindowHandlers){
		.load = window_load_timer,
		.unload = window_unload_timer,
	});
	
	tick_timer_service_subscribe(SECOND_UNIT, &tick_handler);
	bluetooth_connection_service_subscribe(&bt_handler);
	battery_state_service_subscribe(&handle_battery);
	
	lightFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GOTHAM_LIGHT_31));
	boldFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GOTHAM_BOLD_36));
	digital_f = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_40));
	digital_f_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_28));
	
	if(settings.defaultHours + settings.defaultMinutes + settings.defaultSeconds == 0){
		settings.defaultHours = 1;
		settings.defaultMinutes = 30;
		settings.defaultSeconds = 0;
	}
	
	if(settings.onclose != 1){
		mTimer.hours = settings.defaultHours;
		mTimer.minutes = settings.defaultMinutes;
		mTimer.seconds = settings.defaultSeconds;
	}
	
	aboot = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ABOOT);
	battery = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
	bluetooth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
	
	//#NeverForget
	//menu_icon_digital = gbitmap_create_with_resource(RESOURCE_ID_MENU_ICON_DIGITAL);
	//menu_icon_simplicity = gbitmap_create_with_resource(RESOURCE_ID_MENU_ICON_SIMPLICITY);
	//menu_icon_textwatch = gbitmap_create_with_resource(RESOURCE_ID_MENU_ICON_TEXTWATCH);
	
	onclose = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ONCLOSE);
	pause = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PAUSE);
	play = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PLAY);
	settings_b = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SETTINGS);
	theme = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_THEME);
	timer_b = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TIMER);
	watchface = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WATCHFACE);
	next_b = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NEXT);
	up_b = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PLUS);
	down_b = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MINUS);
	connected_b = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONNECTED);
	disconnected_b = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECTED);
	connected_invert = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONNECTED_INVERTED);
	disconnected_invert = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECTED_INVERTED);
	
	window_set_fullscreen(watchface_window, true);
	window_stack_push(watchface_window, true);
}

void deinit(){
	window_destroy(menu_window);
	window_destroy(aboot_window);
	window_destroy(settings_window);
	window_destroy(watchface_window);
	window_destroy(wf_selector_window);
	window_destroy(timer_selector_window);
	value = persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
	APP_LOG(APP_LOG_LEVEL_INFO, "Wrote %d bytes to settings.", value);
	value = persist_write_data(TIMER_KEY, &mTimer, sizeof(mTimer));
	APP_LOG(APP_LOG_LEVEL_INFO, "Wrote %d bytes to timer data.", value);
	
	gbitmap_destroy(aboot);
	gbitmap_destroy(battery);
	gbitmap_destroy(bluetooth);
	//gbitmap_destroy(menu_icon_digital);
	//gbitmap_destroy(menu_icon_simplicity);
	//gbitmap_destroy(menu_icon_textwatch);
	gbitmap_destroy(onclose);
	gbitmap_destroy(pause);
	gbitmap_destroy(play);
	gbitmap_destroy(settings_b);
	gbitmap_destroy(theme);
	gbitmap_destroy(timer_b);
	gbitmap_destroy(watchface);
	gbitmap_destroy(next_b);
	gbitmap_destroy(up_b);
	gbitmap_destroy(down_b);
	gbitmap_destroy(connected_b);
	gbitmap_destroy(disconnected_b);
	gbitmap_destroy(connected_invert);
	gbitmap_destroy(disconnected_invert);
	
	fonts_unload_custom_font(digital_f);
	fonts_unload_custom_font(digital_f_small);
	fonts_unload_custom_font(lightFont);
	fonts_unload_custom_font(boldFont);
}

int main(){
	init();
	app_event_loop();
	deinit();
}