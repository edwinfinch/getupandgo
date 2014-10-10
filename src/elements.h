#define SETTINGS_KEY 0
#define TIMER_KEY 1
	
Window *watchface_window, *menu_window, *wf_selector_window, *timer_selector_window, *aboot_window, *settings_window;

TextLayer *aboot_edwin, *aboot_doug, *aboot_version;
TextLayer *timer_layer, *battery_text_layer, *digital_time_layer, *digital_date_layer;
TextLayer *timer_set_layer_main, *timer_set_layer_des;

char timer_buffer[] = "00:00:00";
char digital_time_buffer[] = "00:00:00";
char date_buffer[] = "September 90th, ...";
char timer_set_buffer[] = "24.";

ActionBarLayer *ab;

InverterLayer *aboot_theme, *wf_theme, *main_menu_theme, *settings_theme, *wf_sel_theme;

GBitmap *aboot, *battery, *bluetooth, *menu_icon_digital, *menu_icon_simplicity, *menu_icon_textwatch;
GBitmap *onclose, *pause, *play, *settings_b, *theme, *timer_b, *watchface, *next_b, *up_b, *down_b;
GBitmap *connected_b, *disconnected_b, *disconnected_invert, *connected_invert;

bool shouldVibrate = false;

BitmapLayer *bluetooth_layer;

GFont *digital_f, *digital_f_small;

typedef struct timer {
	int hours;
	int minutes;
	int seconds;
	bool isRunning;
}timer;

timer mTimer = {
	.hours = 1,
	.minutes = 30,
	.seconds = 0,
	.isRunning = 0,
};

typedef struct persist {
	uint8_t watchface;
	bool bluetooth;
	bool battery;
	bool theme;
	bool onclose;
	int defaultHours;
	int defaultMinutes;
	int defaultSeconds;
	bool firstboot;
}persist;

persist settings = {
	.watchface = 1,
	.bluetooth = 1,
	.battery = 1,
	.theme = 0,
	.onclose = 0,
	.defaultHours = 1,
	.defaultMinutes = 30,
	.defaultSeconds = 0,
};

int value = 0;
int set_value = 0;
int timer_sel_section = 1;
int vibes_call = 2;

TextLayer *text_date_layer, *text_time_layer;
Layer *line_layer;

#define NUM_MENU_SECTIONS 1
#define NUM_FIRST_MENU_ITEMS 5

SimpleMenuLayer *main_menu_layer;
SimpleMenuSection menu_sections[NUM_MENU_SECTIONS];
SimpleMenuItem first_menu_items[NUM_FIRST_MENU_ITEMS];

#define NUM_WF_MENU_SECTIONS 1
#define NUM_WF_FIRST_MENU_ITEMS 3

SimpleMenuLayer *wf_menu_layer;
SimpleMenuSection menu_wf_sections[NUM_WF_MENU_SECTIONS];
SimpleMenuItem first_wf_menu_items[NUM_WF_FIRST_MENU_ITEMS];

#define NUM_SET_MENU_SECTIONS 1
#define NUM_SET_FIRST_MENU_ITEMS 4

SimpleMenuLayer *set_menu_layer;
SimpleMenuSection menu_set_sections[NUM_SET_MENU_SECTIONS];
SimpleMenuItem first_set_menu_items[NUM_SET_FIRST_MENU_ITEMS];

#define BUFFER_SIZE 44

typedef struct {
	TextLayer *currentLayer;
	TextLayer *nextLayer;	
	PropertyAnimation *currentAnimation;
	PropertyAnimation *nextAnimation;
} Line;


static Line line1;
static Line line2;
static Line line3;

static struct tm *t;
static GFont lightFont;
static GFont boldFont;

static char line1Str[2][BUFFER_SIZE];
static char line2Str[2][BUFFER_SIZE];
static char line3Str[2][BUFFER_SIZE];