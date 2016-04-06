#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "parser.h"
#include "ev_process.h"

#define NAME_LENGTH 256

static struct option long_opts[] = {
  {"template", required_argument, NULL, 't'},
  {"device", required_argument, NULL, 'd'},
  {"config", required_argument, NULL, 'c'},
  {0, 0, 0, 0}
};

#define DEF2INIT(D) \
          { (D), #D }

typedef struct {
  int ctrl;
  char *desc;
} t_map;

t_map axis_map[] = {
  DEF2INIT(ABS_X),
  DEF2INIT(ABS_Y),
  DEF2INIT(ABS_Z),
  DEF2INIT(ABS_RX),
  DEF2INIT(ABS_RY),
  DEF2INIT(ABS_RZ),
  DEF2INIT(ABS_THROTTLE),
  DEF2INIT(ABS_RUDDER),
  DEF2INIT(ABS_WHEEL),
  DEF2INIT(ABS_GAS),
  DEF2INIT(ABS_BRAKE),
  DEF2INIT(ABS_HAT0X),
  DEF2INIT(ABS_HAT0Y),
  DEF2INIT(ABS_HAT1X),
  DEF2INIT(ABS_HAT1Y),
  DEF2INIT(ABS_HAT2X),
  DEF2INIT(ABS_HAT2Y),
  DEF2INIT(ABS_HAT3X),
  DEF2INIT(ABS_HAT3Y),
  DEF2INIT(ABS_PRESSURE),
  DEF2INIT(ABS_DISTANCE),
  DEF2INIT(ABS_TILT_X),
  DEF2INIT(ABS_TILT_Y),
  DEF2INIT(ABS_TOOL_WIDTH),
  DEF2INIT(ABS_VOLUME),
  DEF2INIT(ABS_MISC),
  {-1, NULL}
};

t_map button_map[] = {
  DEF2INIT(KEY_ESC),
  DEF2INIT(KEY_1),
  DEF2INIT(KEY_2),
  DEF2INIT(KEY_3),
  DEF2INIT(KEY_4),
  DEF2INIT(KEY_5),
  DEF2INIT(KEY_6),
  DEF2INIT(KEY_7),
  DEF2INIT(KEY_8),
  DEF2INIT(KEY_9),
  DEF2INIT(KEY_0),
  DEF2INIT(KEY_MINUS),
  DEF2INIT(KEY_EQUAL),
  DEF2INIT(KEY_BACKSPACE),
  DEF2INIT(KEY_TAB),
  DEF2INIT(KEY_Q),
  DEF2INIT(KEY_W),
  DEF2INIT(KEY_E),
  DEF2INIT(KEY_R),
  DEF2INIT(KEY_T),
  DEF2INIT(KEY_Y),
  DEF2INIT(KEY_U),
  DEF2INIT(KEY_I),
  DEF2INIT(KEY_O),
  DEF2INIT(KEY_P),
  DEF2INIT(KEY_LEFTBRACE),
  DEF2INIT(KEY_RIGHTBRACE),
  DEF2INIT(KEY_ENTER),
  DEF2INIT(KEY_LEFTCTRL),
  DEF2INIT(KEY_A),
  DEF2INIT(KEY_S),
  DEF2INIT(KEY_D),
  DEF2INIT(KEY_F),
  DEF2INIT(KEY_G),
  DEF2INIT(KEY_H),
  DEF2INIT(KEY_J),
  DEF2INIT(KEY_K),
  DEF2INIT(KEY_L),
  DEF2INIT(KEY_SEMICOLON),
  DEF2INIT(KEY_APOSTROPHE),
  DEF2INIT(KEY_GRAVE),
  DEF2INIT(KEY_LEFTSHIFT),
  DEF2INIT(KEY_BACKSLASH),
  DEF2INIT(KEY_Z),
  DEF2INIT(KEY_X),
  DEF2INIT(KEY_C),
  DEF2INIT(KEY_V),
  DEF2INIT(KEY_B),
  DEF2INIT(KEY_N),
  DEF2INIT(KEY_M),
  DEF2INIT(KEY_COMMA),
  DEF2INIT(KEY_DOT),
  DEF2INIT(KEY_SLASH),
  DEF2INIT(KEY_RIGHTSHIFT),
  DEF2INIT(KEY_KPASTERISK),
  DEF2INIT(KEY_LEFTALT),
  DEF2INIT(KEY_SPACE),
  DEF2INIT(KEY_CAPSLOCK),
  DEF2INIT(KEY_F1),
  DEF2INIT(KEY_F2),
  DEF2INIT(KEY_F3),
  DEF2INIT(KEY_F4),
  DEF2INIT(KEY_F5),
  DEF2INIT(KEY_F6),
  DEF2INIT(KEY_F7),
  DEF2INIT(KEY_F8),
  DEF2INIT(KEY_F9),
  DEF2INIT(KEY_F10),
  DEF2INIT(KEY_NUMLOCK),
  DEF2INIT(KEY_SCROLLLOCK),
  DEF2INIT(KEY_KP7),
  DEF2INIT(KEY_KP8),
  DEF2INIT(KEY_KP9),
  DEF2INIT(KEY_KPMINUS),
  DEF2INIT(KEY_KP4),
  DEF2INIT(KEY_KP5),
  DEF2INIT(KEY_KP6),
  DEF2INIT(KEY_KPPLUS),
  DEF2INIT(KEY_KP1),
  DEF2INIT(KEY_KP2),
  DEF2INIT(KEY_KP3),
  DEF2INIT(KEY_KP0),
  DEF2INIT(KEY_KPDOT),
  DEF2INIT(KEY_ZENKAKUHANKAKU),
  DEF2INIT(KEY_102ND),
  DEF2INIT(KEY_F11),
  DEF2INIT(KEY_F12),
  DEF2INIT(KEY_RO),
  DEF2INIT(KEY_KATAKANA),
  DEF2INIT(KEY_HIRAGANA),
  DEF2INIT(KEY_HENKAN),
  DEF2INIT(KEY_KATAKANAHIRAGANA),
  DEF2INIT(KEY_MUHENKAN),
  DEF2INIT(KEY_KPJPCOMMA),
  DEF2INIT(KEY_KPENTER),
  DEF2INIT(KEY_RIGHTCTRL),
  DEF2INIT(KEY_KPSLASH),
  DEF2INIT(KEY_SYSRQ),
  DEF2INIT(KEY_RIGHTALT),
  DEF2INIT(KEY_LINEFEED),
  DEF2INIT(KEY_HOME),
  DEF2INIT(KEY_UP),
  DEF2INIT(KEY_PAGEUP),
  DEF2INIT(KEY_LEFT),
  DEF2INIT(KEY_RIGHT),
  DEF2INIT(KEY_END),
  DEF2INIT(KEY_DOWN),
  DEF2INIT(KEY_PAGEDOWN),
  DEF2INIT(KEY_INSERT),
  DEF2INIT(KEY_DELETE),
  DEF2INIT(KEY_MACRO),
  DEF2INIT(KEY_MUTE),
  DEF2INIT(KEY_VOLUMEDOWN),
  DEF2INIT(KEY_VOLUMEUP),
  DEF2INIT(KEY_POWER),
  DEF2INIT(KEY_KPEQUAL),
  DEF2INIT(KEY_KPPLUSMINUS),
  DEF2INIT(KEY_PAUSE),
  DEF2INIT(KEY_SCALE),
  DEF2INIT(KEY_KPCOMMA),
  DEF2INIT(KEY_HANGUEL),
  DEF2INIT(KEY_HANJA),
  DEF2INIT(KEY_YEN),
  DEF2INIT(KEY_LEFTMETA),
  DEF2INIT(KEY_RIGHTMETA),
  DEF2INIT(KEY_COMPOSE),
  DEF2INIT(KEY_STOP),
  DEF2INIT(KEY_AGAIN),
  DEF2INIT(KEY_PROPS),
  DEF2INIT(KEY_UNDO),
  DEF2INIT(KEY_FRONT),
  DEF2INIT(KEY_COPY),
  DEF2INIT(KEY_OPEN),
  DEF2INIT(KEY_PASTE),
  DEF2INIT(KEY_FIND),
  DEF2INIT(KEY_CUT),
  DEF2INIT(KEY_HELP),
  DEF2INIT(KEY_MENU),
  DEF2INIT(KEY_CALC),
  DEF2INIT(KEY_SETUP),
  DEF2INIT(KEY_SLEEP),
  DEF2INIT(KEY_WAKEUP),
  DEF2INIT(KEY_FILE),
  DEF2INIT(KEY_SENDFILE),
  DEF2INIT(KEY_DELETEFILE),
  DEF2INIT(KEY_XFER),
  DEF2INIT(KEY_PROG1),
  DEF2INIT(KEY_PROG2),
  DEF2INIT(KEY_WWW),
  DEF2INIT(KEY_MSDOS),
  DEF2INIT(KEY_SCREENLOCK),
  DEF2INIT(KEY_DIRECTION),
  DEF2INIT(KEY_CYCLEWINDOWS),
  DEF2INIT(KEY_MAIL),
  DEF2INIT(KEY_BOOKMARKS),
  DEF2INIT(KEY_COMPUTER),
  DEF2INIT(KEY_BACK),
  DEF2INIT(KEY_FORWARD),
  DEF2INIT(KEY_CLOSECD),
  DEF2INIT(KEY_EJECTCD),
  DEF2INIT(KEY_EJECTCLOSECD),
  DEF2INIT(KEY_NEXTSONG),
  DEF2INIT(KEY_PLAYPAUSE),
  DEF2INIT(KEY_PREVIOUSSONG),
  DEF2INIT(KEY_STOPCD),
  DEF2INIT(KEY_RECORD),
  DEF2INIT(KEY_REWIND),
  DEF2INIT(KEY_PHONE),
  DEF2INIT(KEY_ISO),
  DEF2INIT(KEY_CONFIG),
  DEF2INIT(KEY_HOMEPAGE),
  DEF2INIT(KEY_REFRESH),
  DEF2INIT(KEY_EXIT),
  DEF2INIT(KEY_MOVE),
  DEF2INIT(KEY_EDIT),
  DEF2INIT(KEY_SCROLLUP),
  DEF2INIT(KEY_SCROLLDOWN),
  DEF2INIT(KEY_KPLEFTPAREN),
  DEF2INIT(KEY_KPRIGHTPAREN),
  DEF2INIT(KEY_NEW),
  DEF2INIT(KEY_REDO),
  DEF2INIT(KEY_F13),
  DEF2INIT(KEY_F14),
  DEF2INIT(KEY_F15),
  DEF2INIT(KEY_F16),
  DEF2INIT(KEY_F17),
  DEF2INIT(KEY_F18),
  DEF2INIT(KEY_F19),
  DEF2INIT(KEY_F20),
  DEF2INIT(KEY_F21),
  DEF2INIT(KEY_F22),
  DEF2INIT(KEY_F23),
  DEF2INIT(KEY_F24),
  DEF2INIT(KEY_PLAYCD),
  DEF2INIT(KEY_PAUSECD),
  DEF2INIT(KEY_PROG3),
  DEF2INIT(KEY_PROG4),
  DEF2INIT(KEY_DASHBOARD),
  DEF2INIT(KEY_SUSPEND),
  DEF2INIT(KEY_CLOSE),
  DEF2INIT(KEY_PLAY),
  DEF2INIT(KEY_FASTFORWARD),
  DEF2INIT(KEY_BASSBOOST),
  DEF2INIT(KEY_PRINT),
  DEF2INIT(KEY_HP),
  DEF2INIT(KEY_CAMERA),
  DEF2INIT(KEY_SOUND),
  DEF2INIT(KEY_QUESTION),
  DEF2INIT(KEY_EMAIL),
  DEF2INIT(KEY_CHAT),
  DEF2INIT(KEY_SEARCH),
  DEF2INIT(KEY_CONNECT),
  DEF2INIT(KEY_FINANCE),
  DEF2INIT(KEY_SPORT),
  DEF2INIT(KEY_SHOP),
  DEF2INIT(KEY_ALTERASE),
  DEF2INIT(KEY_CANCEL),
  DEF2INIT(KEY_BRIGHTNESSDOWN),
  DEF2INIT(KEY_BRIGHTNESSUP),
  DEF2INIT(KEY_MEDIA),
  DEF2INIT(KEY_SWITCHVIDEOMODE),
  DEF2INIT(KEY_KBDILLUMTOGGLE),
  DEF2INIT(KEY_KBDILLUMDOWN),
  DEF2INIT(KEY_KBDILLUMUP),
  DEF2INIT(KEY_SEND),
  DEF2INIT(KEY_REPLY),
  DEF2INIT(KEY_FORWARDMAIL),
  DEF2INIT(KEY_SAVE),
  DEF2INIT(KEY_DOCUMENTS),
  DEF2INIT(KEY_BATTERY),
  DEF2INIT(KEY_BLUETOOTH),
  DEF2INIT(KEY_WLAN),
  DEF2INIT(KEY_UWB),
  DEF2INIT(KEY_UNKNOWN),
  DEF2INIT(KEY_VIDEO_NEXT),
  DEF2INIT(KEY_VIDEO_PREV),
  DEF2INIT(KEY_BRIGHTNESS_CYCLE),
  DEF2INIT(KEY_BRIGHTNESS_ZERO),
  DEF2INIT(KEY_DISPLAY_OFF),
  DEF2INIT(KEY_WIMAX),
  DEF2INIT(KEY_RFKILL),
  DEF2INIT(KEY_MICMUTE),
  DEF2INIT(BTN_0),
  DEF2INIT(BTN_1),
  DEF2INIT(BTN_2),
  DEF2INIT(BTN_3),
  DEF2INIT(BTN_4),
  DEF2INIT(BTN_5),
  DEF2INIT(BTN_6),
  DEF2INIT(BTN_7),
  DEF2INIT(BTN_8),
  DEF2INIT(BTN_9),
  DEF2INIT(BTN_LEFT),
  DEF2INIT(BTN_RIGHT),
  DEF2INIT(BTN_MIDDLE),
  DEF2INIT(BTN_SIDE),
  DEF2INIT(BTN_EXTRA),
  DEF2INIT(BTN_FORWARD),
  DEF2INIT(BTN_BACK),
  DEF2INIT(BTN_TASK),
  DEF2INIT(BTN_TRIGGER),
  DEF2INIT(BTN_THUMB),
  DEF2INIT(BTN_THUMB2),
  DEF2INIT(BTN_TOP),
  DEF2INIT(BTN_TOP2),
  DEF2INIT(BTN_PINKIE),
  DEF2INIT(BTN_BASE),
  DEF2INIT(BTN_BASE2),
  DEF2INIT(BTN_BASE3),
  DEF2INIT(BTN_BASE4),
  DEF2INIT(BTN_BASE5),
  DEF2INIT(BTN_BASE6),
  DEF2INIT(BTN_DEAD),
  DEF2INIT(BTN_A),
  DEF2INIT(BTN_B),
  DEF2INIT(BTN_C),
  DEF2INIT(BTN_X),
  DEF2INIT(BTN_Y),
  DEF2INIT(BTN_Z),
  DEF2INIT(BTN_TL),
  DEF2INIT(BTN_TR),
  DEF2INIT(BTN_TL2),
  DEF2INIT(BTN_TR2),
  DEF2INIT(BTN_SELECT),
  DEF2INIT(BTN_START),
  DEF2INIT(BTN_MODE),
  DEF2INIT(BTN_THUMBL),
  DEF2INIT(BTN_THUMBR),
  DEF2INIT(BTN_TOOL_PEN),
  DEF2INIT(BTN_TOOL_RUBBER),
  DEF2INIT(BTN_TOOL_BRUSH),
  DEF2INIT(BTN_TOOL_PENCIL),
  DEF2INIT(BTN_TOOL_AIRBRUSH),
  DEF2INIT(BTN_TOOL_FINGER),
  DEF2INIT(BTN_TOOL_MOUSE),
  DEF2INIT(BTN_TOOL_LENS),
  DEF2INIT(BTN_TOOL_QUINTTAP),
  DEF2INIT(BTN_TOUCH),
  DEF2INIT(BTN_STYLUS),
  DEF2INIT(BTN_STYLUS2),
  DEF2INIT(BTN_TOOL_DOUBLETAP),
  DEF2INIT(BTN_TOOL_TRIPLETAP),
  DEF2INIT(BTN_TOOL_QUADTAP),
  DEF2INIT(BTN_GEAR_DOWN),
  DEF2INIT(BTN_GEAR_UP),
  DEF2INIT(KEY_OK),
  DEF2INIT(KEY_SELECT),
  DEF2INIT(KEY_GOTO),
  DEF2INIT(KEY_CLEAR),
  DEF2INIT(KEY_POWER2),
  DEF2INIT(KEY_OPTION),
  DEF2INIT(KEY_INFO),
  DEF2INIT(KEY_TIME),
  DEF2INIT(KEY_VENDOR),
  DEF2INIT(KEY_ARCHIVE),
  DEF2INIT(KEY_PROGRAM),
  DEF2INIT(KEY_CHANNEL),
  DEF2INIT(KEY_FAVORITES),
  DEF2INIT(KEY_EPG),
  DEF2INIT(KEY_PVR),
  DEF2INIT(KEY_MHP),
  DEF2INIT(KEY_LANGUAGE),
  DEF2INIT(KEY_TITLE),
  DEF2INIT(KEY_SUBTITLE),
  DEF2INIT(KEY_ANGLE),
  DEF2INIT(KEY_ZOOM),
  DEF2INIT(KEY_MODE),
  DEF2INIT(KEY_KEYBOARD),
  DEF2INIT(KEY_SCREEN),
  DEF2INIT(KEY_PC),
  DEF2INIT(KEY_TV),
  DEF2INIT(KEY_TV2),
  DEF2INIT(KEY_VCR),
  DEF2INIT(KEY_VCR2),
  DEF2INIT(KEY_SAT),
  DEF2INIT(KEY_SAT2),
  DEF2INIT(KEY_CD),
  DEF2INIT(KEY_TAPE),
  DEF2INIT(KEY_RADIO),
  DEF2INIT(KEY_TUNER),
  DEF2INIT(KEY_PLAYER),
  DEF2INIT(KEY_TEXT),
  DEF2INIT(KEY_DVD),
  DEF2INIT(KEY_AUX),
  DEF2INIT(KEY_MP3),
  DEF2INIT(KEY_AUDIO),
  DEF2INIT(KEY_VIDEO),
  DEF2INIT(KEY_DIRECTORY),
  DEF2INIT(KEY_LIST),
  DEF2INIT(KEY_MEMO),
  DEF2INIT(KEY_CALENDAR),
  DEF2INIT(KEY_RED),
  DEF2INIT(KEY_GREEN),
  DEF2INIT(KEY_YELLOW),
  DEF2INIT(KEY_BLUE),
  DEF2INIT(KEY_CHANNELUP),
  DEF2INIT(KEY_CHANNELDOWN),
  DEF2INIT(KEY_FIRST),
  DEF2INIT(KEY_LAST),
  DEF2INIT(KEY_AB),
  DEF2INIT(KEY_NEXT),
  DEF2INIT(KEY_RESTART),
  DEF2INIT(KEY_SLOW),
  DEF2INIT(KEY_SHUFFLE),
  DEF2INIT(KEY_BREAK),
  DEF2INIT(KEY_PREVIOUS),
  DEF2INIT(KEY_DIGITS),
  DEF2INIT(KEY_TEEN),
  DEF2INIT(KEY_TWEN),
  DEF2INIT(KEY_VIDEOPHONE),
  DEF2INIT(KEY_GAMES),
  DEF2INIT(KEY_ZOOMIN),
  DEF2INIT(KEY_ZOOMOUT),
  DEF2INIT(KEY_ZOOMRESET),
  DEF2INIT(KEY_WORDPROCESSOR),
  DEF2INIT(KEY_EDITOR),
  DEF2INIT(KEY_SPREADSHEET),
  DEF2INIT(KEY_GRAPHICSEDITOR),
  DEF2INIT(KEY_PRESENTATION),
  DEF2INIT(KEY_DATABASE),
  DEF2INIT(KEY_NEWS),
  DEF2INIT(KEY_VOICEMAIL),
  DEF2INIT(KEY_ADDRESSBOOK),
  DEF2INIT(KEY_MESSENGER),
  DEF2INIT(KEY_BRIGHTNESS_TOGGLE),
  DEF2INIT(KEY_SPELLCHECK),
  DEF2INIT(KEY_LOGOFF),
  DEF2INIT(KEY_DOLLAR),
  DEF2INIT(KEY_EURO),
  DEF2INIT(KEY_FRAMEBACK),
  DEF2INIT(KEY_FRAMEFORWARD),
  DEF2INIT(KEY_CONTEXT_MENU),
  DEF2INIT(KEY_MEDIA_REPEAT),
  DEF2INIT(KEY_10CHANNELSUP),
  DEF2INIT(KEY_10CHANNELSDOWN),
  DEF2INIT(KEY_IMAGES),
  DEF2INIT(KEY_DEL_EOL),
  DEF2INIT(KEY_DEL_EOS),
  DEF2INIT(KEY_INS_LINE),
  DEF2INIT(KEY_DEL_LINE),
  DEF2INIT(KEY_FN),
  DEF2INIT(KEY_FN_ESC),
  DEF2INIT(KEY_FN_F1),
  DEF2INIT(KEY_FN_F2),
  DEF2INIT(KEY_FN_F3),
  DEF2INIT(KEY_FN_F4),
  DEF2INIT(KEY_FN_F5),
  DEF2INIT(KEY_FN_F6),
  DEF2INIT(KEY_FN_F7),
  DEF2INIT(KEY_FN_F8),
  DEF2INIT(KEY_FN_F9),
  DEF2INIT(KEY_FN_F10),
  DEF2INIT(KEY_FN_F11),
  DEF2INIT(KEY_FN_F12),
  DEF2INIT(KEY_FN_1),
  DEF2INIT(KEY_FN_2),
  DEF2INIT(KEY_FN_D),
  DEF2INIT(KEY_FN_E),
  DEF2INIT(KEY_FN_F),
  DEF2INIT(KEY_FN_S),
  DEF2INIT(KEY_FN_B),
  DEF2INIT(KEY_BRL_DOT1),
  DEF2INIT(KEY_BRL_DOT2),
  DEF2INIT(KEY_BRL_DOT3),
  DEF2INIT(KEY_BRL_DOT4),
  DEF2INIT(KEY_BRL_DOT5),
  DEF2INIT(KEY_BRL_DOT6),
  DEF2INIT(KEY_BRL_DOT7),
  DEF2INIT(KEY_BRL_DOT8),
  DEF2INIT(KEY_BRL_DOT9),
  DEF2INIT(KEY_BRL_DOT10),
  DEF2INIT(KEY_NUMERIC_0),
  DEF2INIT(KEY_NUMERIC_1),
  DEF2INIT(KEY_NUMERIC_2),
  DEF2INIT(KEY_NUMERIC_3),
  DEF2INIT(KEY_NUMERIC_4),
  DEF2INIT(KEY_NUMERIC_5),
  DEF2INIT(KEY_NUMERIC_6),
  DEF2INIT(KEY_NUMERIC_7),
  DEF2INIT(KEY_NUMERIC_8),
  DEF2INIT(KEY_NUMERIC_9),
  DEF2INIT(KEY_NUMERIC_STAR),
  DEF2INIT(KEY_NUMERIC_POUND),
  DEF2INIT(KEY_NUMERIC_A),
  DEF2INIT(KEY_NUMERIC_B),
  DEF2INIT(KEY_NUMERIC_C),
  DEF2INIT(KEY_NUMERIC_D),
  DEF2INIT(KEY_CAMERA_FOCUS),
  DEF2INIT(KEY_WPS_BUTTON),
  DEF2INIT(KEY_TOUCHPAD_TOGGLE),
  DEF2INIT(KEY_TOUCHPAD_ON),
  DEF2INIT(KEY_TOUCHPAD_OFF),
  DEF2INIT(KEY_CAMERA_ZOOMIN),
  DEF2INIT(KEY_CAMERA_ZOOMOUT),
  DEF2INIT(KEY_CAMERA_UP),
  DEF2INIT(KEY_CAMERA_DOWN),
  DEF2INIT(KEY_CAMERA_LEFT),
  DEF2INIT(KEY_CAMERA_RIGHT),
  DEF2INIT(KEY_ATTENDANT_ON),
  DEF2INIT(KEY_ATTENDANT_OFF),
  DEF2INIT(KEY_ATTENDANT_TOGGLE),
  DEF2INIT(KEY_LIGHTS_TOGGLE),
  DEF2INIT(BTN_DPAD_UP),
  DEF2INIT(BTN_DPAD_DOWN),
  DEF2INIT(BTN_DPAD_LEFT),
  DEF2INIT(BTN_DPAD_RIGHT),
  DEF2INIT(KEY_ALS_TOGGLE),
  DEF2INIT(KEY_BUTTONCONFIG),
  DEF2INIT(KEY_TASKMANAGER),
  DEF2INIT(KEY_JOURNAL),
  DEF2INIT(KEY_CONTROLPANEL),
  DEF2INIT(KEY_APPSELECT),
  DEF2INIT(KEY_SCREENSAVER),
  DEF2INIT(KEY_VOICECOMMAND),
  DEF2INIT(KEY_BRIGHTNESS_MIN),
  DEF2INIT(KEY_BRIGHTNESS_MAX),
  DEF2INIT(KEY_KBDINPUTASSIST_PREV),
  DEF2INIT(KEY_KBDINPUTASSIST_NEXT),
  DEF2INIT(KEY_KBDINPUTASSIST_PREVGROUP),
  DEF2INIT(KEY_KBDINPUTASSIST_NEXTGROUP),
  DEF2INIT(KEY_KBDINPUTASSIST_ACCEPT),
  DEF2INIT(KEY_KBDINPUTASSIST_CANCEL),
  DEF2INIT(BTN_TRIGGER_HAPPY1),
  DEF2INIT(BTN_TRIGGER_HAPPY2),
  DEF2INIT(BTN_TRIGGER_HAPPY3),
  DEF2INIT(BTN_TRIGGER_HAPPY4),
  DEF2INIT(BTN_TRIGGER_HAPPY5),
  DEF2INIT(BTN_TRIGGER_HAPPY6),
  DEF2INIT(BTN_TRIGGER_HAPPY7),
  DEF2INIT(BTN_TRIGGER_HAPPY8),
  DEF2INIT(BTN_TRIGGER_HAPPY9),
  DEF2INIT(BTN_TRIGGER_HAPPY10),
  DEF2INIT(BTN_TRIGGER_HAPPY11),
  DEF2INIT(BTN_TRIGGER_HAPPY12),
  DEF2INIT(BTN_TRIGGER_HAPPY13),
  DEF2INIT(BTN_TRIGGER_HAPPY14),
  DEF2INIT(BTN_TRIGGER_HAPPY15),
  DEF2INIT(BTN_TRIGGER_HAPPY16),
  DEF2INIT(BTN_TRIGGER_HAPPY17),
  DEF2INIT(BTN_TRIGGER_HAPPY18),
  DEF2INIT(BTN_TRIGGER_HAPPY19),
  DEF2INIT(BTN_TRIGGER_HAPPY20),
  DEF2INIT(BTN_TRIGGER_HAPPY21),
  DEF2INIT(BTN_TRIGGER_HAPPY22),
  DEF2INIT(BTN_TRIGGER_HAPPY23),
  DEF2INIT(BTN_TRIGGER_HAPPY24),
  DEF2INIT(BTN_TRIGGER_HAPPY25),
  DEF2INIT(BTN_TRIGGER_HAPPY26),
  DEF2INIT(BTN_TRIGGER_HAPPY27),
  DEF2INIT(BTN_TRIGGER_HAPPY28),
  DEF2INIT(BTN_TRIGGER_HAPPY29),
  DEF2INIT(BTN_TRIGGER_HAPPY30),
  DEF2INIT(BTN_TRIGGER_HAPPY31),
  DEF2INIT(BTN_TRIGGER_HAPPY32),
  DEF2INIT(BTN_TRIGGER_HAPPY33),
  DEF2INIT(BTN_TRIGGER_HAPPY34),
  DEF2INIT(BTN_TRIGGER_HAPPY35),
  DEF2INIT(BTN_TRIGGER_HAPPY36),
  DEF2INIT(BTN_TRIGGER_HAPPY37),
  DEF2INIT(BTN_TRIGGER_HAPPY38),
  DEF2INIT(BTN_TRIGGER_HAPPY39),
  DEF2INIT(BTN_TRIGGER_HAPPY40),
  {-1, NULL}
};

char *find_axis_name(int ctrl)
{
  int i = 0;
  while(axis_map[i].ctrl >= 0){
    if(axis_map[i].ctrl == ctrl){
      return strdup(axis_map[i].desc);
    }
    ++i;
  }
  char *new_name = NULL;
  asprintf(&new_name, "ABS_%03X", ctrl);
  return new_name;
}

char *find_button_name(int ctrl)
{
  int i = 0;
  while(button_map[i].ctrl >= 0){
    if(button_map[i].ctrl == ctrl){
      return strdup(button_map[i].desc);
    }
    ++i;
  }
  char *new_name = NULL;
  asprintf(&new_name, "BUTTON_%02X", ctrl);
  return new_name;
}


int find_device_by_name(const char *path, const char *devName)
{
  char *nameStart = "event";
  DIR *input = opendir(path);
  if(input == NULL){
    perror("opendir");
    return -1;
  }

  struct dirent *de;
  int fd = -1;
  while((de = readdir(input)) != NULL){
    if(strncmp(nameStart, de->d_name, strlen(nameStart)) != 0){
      continue;
    }
    char *fname = NULL;
    if(asprintf(&fname, "%s/%s", path, de->d_name) < 0){
      continue;
    }

    fd = open(fname, O_RDONLY);
    if(fd < 0){
      perror("open");
      //printf("Problem opening '%s'.\n", fname);
      free(fname);
      fname = NULL;
      continue;
    }
    //printf("Opened '%s'.\n", fname);
    free(fname);
    fname = NULL;

    char name[NAME_LENGTH];
    if(ioctl(fd, EVIOCGNAME(NAME_LENGTH), name) < 0){
      perror("ioctl EVIOCGNAME");
      return 1;
    }
    if(strncmp(name, devName, strlen(devName)) == 0){
      break;
    }

    close(fd);
    fd = -1;
  }
  closedir(input);
  return fd;
}

int explore_device(int fd, FILE *templ_file)
{
  int i;
  //Initialize the button array (all buttons are marked free)
  for(i = 0; i < BUTTON_ARRAY_LEN; ++i){
    config.real_btn_array[i] = 0;
  }
  //Initialize all axes as ignored
  for(i = 0; i < ABS_CNT; ++i){
    config.axes[i].ignore = true;
  }

  char name[NAME_LENGTH];
  if(ioctl(fd, EVIOCGNAME(NAME_LENGTH), name) < 0){
    perror("ioctl EVIOCGNAME");
    return 1;
  }
  if(templ_file){
    fprintf(templ_file, "//Automaticaly generated section\n");
    fprintf(templ_file, "device \"%s\"\n\n", name);
  }

  if(config.grab){
    int grab = ioctl(fd, EVIOCGRAB, (void*)1);
    if(grab < 0){
      config.grabbed = false;
    }else{
      config.grabbed = true;
    }
  }else{
    config.grabbed = false;
  }

  struct input_id id;
  if(ioctl(fd, EVIOCGID, &id) < 0){
    perror("ioctl EVIOCGID");
    return 1;
  }
  //No need to record the bustype as it will be virtual anyway
  config.vendor = id.vendor;
  config.product = id.product;
  config.version = id.version;

  uint8_t ev_bit[KEY_MAX / 8 + 1];
  uint8_t bit[KEY_MAX / 8 + 1];
  memset(ev_bit, 0, sizeof(ev_bit));
  if(ioctl(fd, EVIOCGBIT(0, EV_MAX), ev_bit) < 0){
    perror("ioctl EVIOCGBIT");
    return 1;
  }

  int ev, code;

  for(ev = 0; ev < EV_MAX; ++ev){
    if(ev_bit[0] & (1 << ev)){
      if(ev == EV_SYN){
        continue;
      }
      uint8_t state[KEY_MAX / 8 + 1];
      if(ev == EV_KEY){
        memset(state, 0, sizeof(state));
        if(ioctl(fd, EVIOCGKEY(KEY_MAX), state) < 0){
          perror("ioctl EVIOCGKEY");
          return 1;
        }
      }
      memset(bit, 0, sizeof(bit));
      if(ioctl(fd, EVIOCGBIT(ev, KEY_MAX), bit) < 0){
        perror("ioctl EVIOCGBIT");
        return 1;
      }
      for(code = 0; code < KEY_MAX; ++code){
        if(bit[code / 8] & (1 << (code % 8))){//event is valid
          if(ev == EV_KEY){
            add_used_button(config.real_btn_array, code, true);
            if(templ_file){
              char *name = find_button_name(code);
              fprintf(templ_file, "  button %s = %d\n", name, code);
              free(name);
            }
          }
          if(ev == EV_ABS){
            if(templ_file){
              char *name = find_axis_name(code);
              fprintf(templ_file, "  axis %s = %d\n", name, code);
	      free(name);
	    }
            struct input_absinfo absinfo;
            if(ioctl(fd, EVIOCGABS(code), &absinfo) < 0){
              perror("ioctl EVIOCGABS");
              return 1;
            }
            config.axes[code].center = (absinfo.minimum + absinfo.maximum) / 2.0;
            //Hysteresis 1/4 of the full scale
            config.axes[code].hysteresis = (absinfo.maximum - config.axes[code].center) / 2.0;
            config.axes[code].current_state = INACTIVE;
            config.axes[code].ignore = false;
            config.axes[code].minimum = absinfo.minimum;
            config.axes[code].maximum = absinfo.maximum;
            config.axes[code].fuzz = absinfo.fuzz;
            config.axes[code].flat = absinfo.flat;
            config.axes[code].res = absinfo.resolution;
          }
        }
      }
    }
  }

  if(templ_file){
    fprintf(templ_file, "\n//Your configuration follows\n");
  }

  return 0;
}


static int virt_dev = -1;

int send_event(struct input_event *ev)
{
  if(virt_dev < 0){
    printf("NULL virt_dev.\n");
    return -1;
  }
  int res = write(virt_dev, ev, sizeof(struct input_event));
  if(res < 0){
    printf("Problem sending out an event!\n");
  }
  return res;
}

void print_help()
{
  printf("Usage examples:\n");
  printf("To create config template:\n");
  printf("evdevshift --device /dev/input/event12 --template x52.conf\n");
  printf("To create a virtual device based on the config:\n");
  printf("evdevshift --config x52.conf\n");
}


int main(int argc, char *argv[])
{
  int c;
  int index;
  char *template = NULL;
  char *dev = NULL;
  char *conf_file = NULL;

  while(1){
    c = getopt_long(argc, argv, "t:d:c:h", long_opts, &index);
    if(c < 0){
      break;
    }
    switch(c){
      case 't':
        if(optarg != NULL){
          template = optarg;
        }
        break;
      case 'd':
        if(optarg != NULL){
          dev = optarg;
        }
        break;
      case 'c':
        if(optarg != NULL){
          conf_file = optarg;
        }
        break;
      case 'h':
      default:
        print_help();
        break;
    }
  }

  if((dev && template && !conf_file) || (conf_file && !dev && !template)){
    //valid combinations
  }else{
    print_help();
    return 0;
  }

  int fd;

  if(conf_file){
    parse_config(conf_file);
  }

  if(dev){
    fd = open(dev, O_RDWR);
    if(fd < 0){
      perror("open");
      printf("Device '%s' NOT opened.\n", dev);
      return 1;
    }
  }else{
    fd = find_device_by_name("/dev/input", config.device);
    if(fd < 0){
      perror("open");
      printf("Device named '%s' NOT opened.\n", config.device);
      return 1;
    }
  }

  FILE *templ_file = NULL;
  if(template != NULL){
    if((templ_file = fopen(template, "w")) == NULL){
      printf("Can't open template file '%s'.\n", template);
      return 1;
    }
  }

  explore_device(fd, templ_file);
  
  if(templ_file){
    fclose(templ_file);
    close(fd);
    return 0;
  }

  sort_out_buttons();

  print_config();

  int ui = open("/dev/uinput", O_RDWR);
  if(ui < 0){
    perror("open");
    return 1;
  }
  printf("File '%s' opened.\n", argv[2]);

  int res = 0;
  struct uinput_user_dev ud;
  memset(&ud, 0, sizeof(ud));
  ud.id.bustype = BUS_VIRTUAL;
  ud.id.vendor = config.vendor;
  ud.id.product = config.product;
  ud.id.version = config.version;
  snprintf(ud.name, UINPUT_MAX_NAME_SIZE,
                   "evdevshift: %s", config.device);

  res |= (ioctl(ui, UI_SET_EVBIT, EV_SYN) == -1);
  res |= (ioctl(ui, UI_SET_EVBIT, EV_KEY) == -1);
  int i;
  for(i = 0; i < BUTTON_ARRAY_LEN; ++i){
    if(config.virtual_btn_array[i] == -1){
      res |= (ioctl(ui, UI_SET_KEYBIT, i + BUTTON_MIN) == -1);
    }
  }
  res |= (ioctl(ui, UI_SET_EVBIT, EV_ABS) == -1);
  for(i = 0; i < ABS_CNT; ++i){
    if(!config.axes[i].ignore){
      res |= (ioctl(ui, UI_SET_ABSBIT, i) == -1);
      ud.absmin[i] = config.axes[i].minimum;
      ud.absmax[i] = config.axes[i].maximum;
      ud.absfuzz[i] = config.axes[i].fuzz;
      ud.absflat[i] = config.axes[i].flat;
    }
  }
  res |= (write(ui, &ud, sizeof(ud)) == -1);

  if(ioctl(ui, UI_DEV_CREATE, 0) < 0){
    perror("ioctl UI_DEV_CREATE");
    return 1;
  }

  printf("New device created.\n");
  struct input_event event[16];
  ssize_t read_in;

  for(i = 0; i < BUTTON_ARRAY_LEN; ++i){
    config.real_btn_array[i] = 0;
    config.virtual_btn_array[i] = 0;
  }
  virt_dev = ui;
  while(1){
    read_in = read(fd, &event, sizeof(event));
    if((read_in < 0) && (errno == EAGAIN)){
      printf("Continuing.\n");
      continue;
    }
    if(read_in % sizeof(struct input_event) != 0){
      printf("Read wrong number of bytes (%d)!\n", (int)read_in);
      return 1;
    }
    size_t n;
    for(n = 0; n < read_in / sizeof(struct input_event); ++n){
      process_event(&(event[n]));
    }
  }

  if(config.grabbed){
    ioctl(fd, EVIOCGRAB, (void*)1);
  }
  clean_up_config();

  return 0;
}

