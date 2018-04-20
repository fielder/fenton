#ifndef __APPIO_H__
#define __APPIO_H__

/*
 * SDL keycodes range from 0 to around 1<<30. We want our keys to be
 * usable as an index. So, we remap them down to a straight sequence.
 * The first 128 should still be ASCII codes.
 */
enum
{
	FK_UNKNOWN = 0,

	FK_RETURN = '\r',
	FK_ESCAPE = '\033',
	FK_BACKSPACE = '\b',
	FK_TAB = '\t',
	FK_SPACE = ' ',
	FK_EXCLAIM = '!',
	FK_QUOTEDBL = '"',
	FK_HASH = '#',
	FK_PERCENT = '%',
	FK_DOLLAR = '$',
	FK_AMPERSAND = '&',
	FK_QUOTE = '\'',
	FK_LEFTPAREN = '(',
	FK_RIGHTPAREN = ')',
	FK_ASTERISK = '*',
	FK_PLUS = '+',
	FK_COMMA = ',',
	FK_MINUS = '-',
	FK_PERIOD = '.',
	FK_SLASH = '/',
	FK_0 = '0',
	FK_1 = '1',
	FK_2 = '2',
	FK_3 = '3',
	FK_4 = '4',
	FK_5 = '5',
	FK_6 = '6',
	FK_7 = '7',
	FK_8 = '8',
	FK_9 = '9',
	FK_COLON = ':',
	FK_SEMICOLON = ';',
	FK_LESS = '<',
	FK_EQUALS = '=',
	FK_GREATER = '>',
	FK_QUESTION = '?',
	FK_AT = '@',
	/*
		Skip uppercase letters
	 */
	FK_LEFTBRACKET = '[',
	FK_BACKSLASH = '\\',
	FK_RIGHTBRACKET = ']',
	FK_CARET = '^',
	FK_UNDERSCORE = '_',
	FK_BACKQUOTE = '`',
	FK_a = 'a',
	FK_b = 'b',
	FK_c = 'c',
	FK_d = 'd',
	FK_e = 'e',
	FK_f = 'f',
	FK_g = 'g',
	FK_h = 'h',
	FK_i = 'i',
	FK_j = 'j',
	FK_k = 'k',
	FK_l = 'l',
	FK_m = 'm',
	FK_n = 'n',
	FK_o = 'o',
	FK_p = 'p',
	FK_q = 'q',
	FK_r = 'r',
	FK_s = 's',
	FK_t = 't',
	FK_u = 'u',
	FK_v = 'v',
	FK_w = 'w',
	FK_x = 'x',
	FK_y = 'y',
	FK_z = 'z',

	FK_CAPSLOCK = 0x80,

	FK_F1,
	FK_F2,
	FK_F3,
	FK_F4,
	FK_F5,
	FK_F6,
	FK_F7,
	FK_F8,
	FK_F9,
	FK_F10,
	FK_F11,
	FK_F12,

	FK_PRINTSCREEN,
	FK_SCROLLLOCK,
	FK_PAUSE,
	FK_INSERT,
	FK_HOME,
	FK_PAGEUP,
	FK_DELETE,
	FK_END,
	FK_PAGEDOWN,
	FK_RIGHT,
	FK_LEFT,
	FK_DOWN,
	FK_UP,

	FK_NUMLOCKCLEAR,
	FK_KP_DIVIDE,
	FK_KP_MULTIPLY,
	FK_KP_MINUS,
	FK_KP_PLUS,
	FK_KP_ENTER,
	FK_KP_1,
	FK_KP_2,
	FK_KP_3,
	FK_KP_4,
	FK_KP_5,
	FK_KP_6,
	FK_KP_7,
	FK_KP_8,
	FK_KP_9,
	FK_KP_0,
	FK_KP_PERIOD,

	FK_APPLICATION,
	FK_POWER,
	FK_KP_EQUALS,
	FK_F13,
	FK_F14,
	FK_F15,
	FK_F16,
	FK_F17,
	FK_F18,
	FK_F19,
	FK_F20,
	FK_F21,
	FK_F22,
	FK_F23,
	FK_F24,
	FK_EXECUTE,
	FK_HELP,
	FK_MENU,
	FK_SELECT,
	FK_STOP,
	FK_AGAIN,
	FK_UNDO,
	FK_CUT,
	FK_COPY,
	FK_PASTE,
	FK_FIND,
	FK_MUTE,
	FK_VOLUMEUP,
	FK_VOLUMEDOWN,
	FK_KP_COMMA,
	FK_KP_EQUALSAS400,

	FK_ALTERASE,
	FK_SYSREQ,
	FK_CANCEL,
	FK_CLEAR,
	FK_PRIOR,
	FK_RETURN2,
	FK_SEPARATOR,
	FK_OUT,
	FK_OPER,
	FK_CLEARAGAIN,
	FK_CRSEL,
	FK_EXSEL,

	FK_KP_00,
	FK_KP_000,
	FK_THOUSANDSSEPARATOR,
	FK_DECIMALSEPARATOR,
	FK_CURRENCYUNIT,
	FK_CURRENCYSUBUNIT,
	FK_KP_LEFTPAREN,
	FK_KP_RIGHTPAREN,
	FK_KP_LEFTBRACE,
	FK_KP_RIGHTBRACE,
	FK_KP_TAB,
	FK_KP_BACKSPACE,
	FK_KP_A,
	FK_KP_B,
	FK_KP_C,
	FK_KP_D,
	FK_KP_E,
	FK_KP_F,
	FK_KP_XOR,
	FK_KP_POWER,
	FK_KP_PERCENT,
	FK_KP_LESS,
	FK_KP_GREATER,
	FK_KP_AMPERSAND,
	FK_KP_DBLAMPERSAND,
	FK_KP_VERTICALBAR,
	FK_KP_DBLVERTICALBAR,
	FK_KP_COLON,
	FK_KP_HASH,
	FK_KP_SPACE,
	FK_KP_AT,
	FK_KP_EXCLAM,
	FK_KP_MEMSTORE,
	FK_KP_MEMRECALL,
	FK_KP_MEMCLEAR,
	FK_KP_MEMADD,
	FK_KP_MEMSUBTRACT,
	FK_KP_MEMMULTIPLY,
	FK_KP_MEMDIVIDE,
	FK_KP_PLUSMINUS,
	FK_KP_CLEAR,
	FK_KP_CLEARENTRY,
	FK_KP_BINARY,
	FK_KP_OCTAL,
	FK_KP_DECIMAL,
	FK_KP_HEXADECIMAL,

	FK_LCTRL,
	FK_LSHIFT,
	FK_LALT,
	FK_LGUI,
	FK_RCTRL,
	FK_RSHIFT,
	FK_RALT,
	FK_RGUI,

	FK_MODE,

	FK_AUDIONEXT,
	FK_AUDIOPREV,
	FK_AUDIOSTOP,
	FK_AUDIOPLAY,
	FK_AUDIOMUTE,
	FK_MEDIASELECT,
	FK_WWW,
	FK_MAIL,
	FK_CALCULATOR,
	FK_COMPUTER,
	FK_AC_SEARCH,
	FK_AC_HOME,
	FK_AC_BACK,
	FK_AC_FORWARD,
	FK_AC_STOP,
	FK_AC_REFRESH,
	FK_AC_BOOKMARKS,

	FK_BRIGHTNESSDOWN,
	FK_BRIGHTNESSUP,
	FK_DISPLAYSWITCH,
	FK_KBDILLUMTOGGLE,
	FK_KBDILLUMDOWN,
	FK_KBDILLUMUP,
	FK_EJECT,
	FK_SLEEP,

	FK_LAST,
};

enum
{
	MBUTTON_LEFT,
	MBUTTON_MIDDLE,
	MBUTTON_RIGHT,
	MBUTTON_WHEEL_UP,
	MBUTTON_WHEEL_DOWN,
	MBUTTON_LAST,
};

struct input_s
{
	struct
	{
		int delta[2];
		struct
		{
			int state[MBUTTON_LAST];
			int press[MBUTTON_LAST];
			int release[MBUTTON_LAST];
		} button;
	} mouse;

	struct
	{
		int state[FK_LAST];
		int press[FK_LAST];
		int release[FK_LAST];
	} key;
};

struct video_s
{
	int w, h, bpp;

	int bytes_pp;
	void *buf;
	void **rows;

	int red, green, blue;
};

extern struct video_s video;
extern struct input_s input;

extern void
IO_Print (const char *s);

extern void
IO_Init (void);

extern void
IO_SetMode (int w, int h, int bpp, int scale, int fullscreen);

extern void
IO_Shutdown (void);

/* terminates the app */
extern void
IO_Terminate (void);

extern unsigned int
IO_MSecs (void);

extern void
IO_FetchInput (void);

extern void
IO_SetGrab (int grab);

extern void
IO_ToggleGrab (void);

extern void
IO_Swap (void);

#endif /* __APPIO_H__ */
