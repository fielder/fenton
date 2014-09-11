#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#include <SDL_keycode.h>

#include "fenton.h"
#include "appio.h"

//static SDL_Surface *sdl_surf = NULL;

struct video_s video;
struct input_s input;


void
IO_Print (const char *s)
{
	fputs (s, stdout);
	fflush (stdout);
}


void
IO_Init (void)
{
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
		F_Error ("video init failed: %s", SDL_GetError());
}


static void
DestroyWindow (void)
{
	/*
	if (sdl_surf != NULL)
	{
		SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		IO_SetGrab (0);
		SDL_FreeSurface (sdl_surf);
		sdl_surf = NULL;
	}
	*/

#if 0
	if (video.bouncebuf != NULL)
	{
		free (video.bouncebuf);
		video.bouncebuf = NULL;
	}

	if (video.rows != NULL)
	{
		free (video.rows);
		video.rows = NULL;
	}
#endif
}


void
IO_SetMode (int w, int h, int bpp, int scale)
{
//	Uint32 flags;
#if 0
	int y;
#endif

//	IO_Print ("Setting mode %dx%dx%d\n", w, h, bpp);
//	printf ("Scaling %dx\n", scale);

	DestroyWindow ();

//	flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_HWPALETTE;
#if 0
	if ((sdl_surf = SDL_SetVideoMode(video.w * video.scale, video.h * video.scale, sizeof(pixel_t) * 8, flags)) == NULL)
	{
		fprintf (stderr, "ERROR: failed setting video mode\n");
		Quit ();
	}
	printf ("Set %dx%dx%d real video mode\n", sdl_surf->w, sdl_surf->h, sdl_surf->format->BytesPerPixel * 8);
	printf ("RGB mask: 0x%x 0x%x 0x%x\n", sdl_surf->format->Rmask, sdl_surf->format->Gmask, sdl_surf->format->Bmask);

	video.rows = malloc (video.h * sizeof(*video.rows));

	if (video.scale == 1)
	{
		for (y = 0; y < video.h; y++)
			video.rows[y] = (pixel_t *)((uintptr_t)sdl_surf->pixels + y * sdl_surf->pitch);
	}
	else
	{
		video.bouncebuf = malloc (video.w * video.h * sizeof(pixel_t));
		for (y = 0; y < video.h; y++)
			video.rows[y] = video.bouncebuf + y * video.w;
	}
#endif
}


void
IO_Shutdown (void)
{
	DestroyWindow ();

	SDL_QuitSubSystem (SDL_INIT_VIDEO);
}


void
IO_Terminate (void)
{
	SDL_QuitSubSystem (SDL_INIT_TIMER);
	SDL_Quit ();
	exit (EXIT_SUCCESS);
}


unsigned int
IO_MSecs (void)
{
	return SDL_GetTicks ();
}


static int
XlateSDLKey (int sdlk)
{
	switch (sdlk)
	{
		case SDLK_UNKNOWN: return FK_UNKNOWN;
		case SDLK_RETURN: return FK_RETURN;
		case SDLK_ESCAPE: return FK_ESCAPE;
		case SDLK_BACKSPACE: return FK_BACKSPACE;
		case SDLK_TAB: return FK_TAB;
		case SDLK_SPACE: return FK_SPACE;
		case SDLK_EXCLAIM: return FK_EXCLAIM;
		case SDLK_QUOTEDBL: return FK_QUOTEDBL;
		case SDLK_HASH: return FK_HASH;
		case SDLK_PERCENT: return FK_PERCENT;
		case SDLK_DOLLAR: return FK_DOLLAR;
		case SDLK_AMPERSAND: return FK_AMPERSAND;
		case SDLK_QUOTE: return FK_QUOTE;
		case SDLK_LEFTPAREN: return FK_LEFTPAREN;
		case SDLK_RIGHTPAREN: return FK_RIGHTPAREN;
		case SDLK_ASTERISK: return FK_ASTERISK;
		case SDLK_PLUS: return FK_PLUS;
		case SDLK_COMMA: return FK_COMMA;
		case SDLK_MINUS: return FK_MINUS;
		case SDLK_PERIOD: return FK_PERIOD;
		case SDLK_SLASH: return FK_SLASH;
		case SDLK_0: return FK_0;
		case SDLK_1: return FK_1;
		case SDLK_2: return FK_2;
		case SDLK_3: return FK_3;
		case SDLK_4: return FK_4;
		case SDLK_5: return FK_5;
		case SDLK_6: return FK_6;
		case SDLK_7: return FK_7;
		case SDLK_8: return FK_8;
		case SDLK_9: return FK_9;
		case SDLK_COLON: return FK_COLON;
		case SDLK_SEMICOLON: return FK_SEMICOLON;
		case SDLK_LESS: return FK_LESS;
		case SDLK_EQUALS: return FK_EQUALS;
		case SDLK_GREATER: return FK_GREATER;
		case SDLK_QUESTION: return FK_QUESTION;
		case SDLK_AT: return FK_AT;
		case SDLK_LEFTBRACKET: return FK_LEFTBRACKET;
		case SDLK_BACKSLASH: return FK_BACKSLASH;
		case SDLK_RIGHTBRACKET: return FK_RIGHTBRACKET;
		case SDLK_CARET: return FK_CARET;
		case SDLK_UNDERSCORE: return FK_UNDERSCORE;
		case SDLK_BACKQUOTE: return FK_BACKQUOTE;
		case SDLK_a: return FK_a;
		case SDLK_b: return FK_b;
		case SDLK_c: return FK_c;
		case SDLK_d: return FK_d;
		case SDLK_e: return FK_e;
		case SDLK_f: return FK_f;
		case SDLK_g: return FK_g;
		case SDLK_h: return FK_h;
		case SDLK_i: return FK_i;
		case SDLK_j: return FK_j;
		case SDLK_k: return FK_k;
		case SDLK_l: return FK_l;
		case SDLK_m: return FK_m;
		case SDLK_n: return FK_n;
		case SDLK_o: return FK_o;
		case SDLK_p: return FK_p;
		case SDLK_q: return FK_q;
		case SDLK_r: return FK_r;
		case SDLK_s: return FK_s;
		case SDLK_t: return FK_t;
		case SDLK_u: return FK_u;
		case SDLK_v: return FK_v;
		case SDLK_w: return FK_w;
		case SDLK_x: return FK_x;
		case SDLK_y: return FK_y;
		case SDLK_z: return FK_z;
		case SDLK_CAPSLOCK: return FK_CAPSLOCK;
		case SDLK_F1: return FK_F1;
		case SDLK_F2: return FK_F2;
		case SDLK_F3: return FK_F3;
		case SDLK_F4: return FK_F4;
		case SDLK_F5: return FK_F5;
		case SDLK_F6: return FK_F6;
		case SDLK_F7: return FK_F7;
		case SDLK_F8: return FK_F8;
		case SDLK_F9: return FK_F9;
		case SDLK_F10: return FK_F10;
		case SDLK_F11: return FK_F11;
		case SDLK_F12: return FK_F12;
		case SDLK_PRINTSCREEN: return FK_PRINTSCREEN;
		case SDLK_SCROLLLOCK: return FK_SCROLLLOCK;
		case SDLK_PAUSE: return FK_PAUSE;
		case SDLK_INSERT: return FK_INSERT;
		case SDLK_HOME: return FK_HOME;
		case SDLK_PAGEUP: return FK_PAGEUP;
		case SDLK_DELETE: return FK_DELETE;
		case SDLK_END: return FK_END;
		case SDLK_PAGEDOWN: return FK_PAGEDOWN;
		case SDLK_RIGHT: return FK_RIGHT;
		case SDLK_LEFT: return FK_LEFT;
		case SDLK_DOWN: return FK_DOWN;
		case SDLK_UP: return FK_UP;
		case SDLK_NUMLOCKCLEAR: return FK_NUMLOCKCLEAR;
		case SDLK_KP_DIVIDE: return FK_KP_DIVIDE;
		case SDLK_KP_MULTIPLY: return FK_KP_MULTIPLY;
		case SDLK_KP_MINUS: return FK_KP_MINUS;
		case SDLK_KP_PLUS: return FK_KP_PLUS;
		case SDLK_KP_ENTER: return FK_KP_ENTER;
		case SDLK_KP_1: return FK_KP_1;
		case SDLK_KP_2: return FK_KP_2;
		case SDLK_KP_3: return FK_KP_3;
		case SDLK_KP_4: return FK_KP_4;
		case SDLK_KP_5: return FK_KP_5;
		case SDLK_KP_6: return FK_KP_6;
		case SDLK_KP_7: return FK_KP_7;
		case SDLK_KP_8: return FK_KP_8;
		case SDLK_KP_9: return FK_KP_9;
		case SDLK_KP_0: return FK_KP_0;
		case SDLK_KP_PERIOD: return FK_KP_PERIOD;
		case SDLK_APPLICATION: return FK_APPLICATION;
		case SDLK_POWER: return FK_POWER;
		case SDLK_KP_EQUALS: return FK_KP_EQUALS;
		case SDLK_F13: return FK_F13;
		case SDLK_F14: return FK_F14;
		case SDLK_F15: return FK_F15;
		case SDLK_F16: return FK_F16;
		case SDLK_F17: return FK_F17;
		case SDLK_F18: return FK_F18;
		case SDLK_F19: return FK_F19;
		case SDLK_F20: return FK_F20;
		case SDLK_F21: return FK_F21;
		case SDLK_F22: return FK_F22;
		case SDLK_F23: return FK_F23;
		case SDLK_F24: return FK_F24;
		case SDLK_EXECUTE: return FK_EXECUTE;
		case SDLK_HELP: return FK_HELP;
		case SDLK_MENU: return FK_MENU;
		case SDLK_SELECT: return FK_SELECT;
		case SDLK_STOP: return FK_STOP;
		case SDLK_AGAIN: return FK_AGAIN;
		case SDLK_UNDO: return FK_UNDO;
		case SDLK_CUT: return FK_CUT;
		case SDLK_COPY: return FK_COPY;
		case SDLK_PASTE: return FK_PASTE;
		case SDLK_FIND: return FK_FIND;
		case SDLK_MUTE: return FK_MUTE;
		case SDLK_VOLUMEUP: return FK_VOLUMEUP;
		case SDLK_VOLUMEDOWN: return FK_VOLUMEDOWN;
		case SDLK_KP_COMMA: return FK_KP_COMMA;
		case SDLK_KP_EQUALSAS400: return FK_KP_EQUALSAS400;
		case SDLK_ALTERASE: return FK_ALTERASE;
		case SDLK_SYSREQ: return FK_SYSREQ;
		case SDLK_CANCEL: return FK_CANCEL;
		case SDLK_CLEAR: return FK_CLEAR;
		case SDLK_PRIOR: return FK_PRIOR;
		case SDLK_RETURN2: return FK_RETURN2;
		case SDLK_SEPARATOR: return FK_SEPARATOR;
		case SDLK_OUT: return FK_OUT;
		case SDLK_OPER: return FK_OPER;
		case SDLK_CLEARAGAIN: return FK_CLEARAGAIN;
		case SDLK_CRSEL: return FK_CRSEL;
		case SDLK_EXSEL: return FK_EXSEL;
		case SDLK_KP_00: return FK_KP_00;
		case SDLK_KP_000: return FK_KP_000;
		case SDLK_THOUSANDSSEPARATOR: return FK_THOUSANDSSEPARATOR;
		case SDLK_DECIMALSEPARATOR: return FK_DECIMALSEPARATOR;
		case SDLK_CURRENCYUNIT: return FK_CURRENCYUNIT;
		case SDLK_CURRENCYSUBUNIT: return FK_CURRENCYSUBUNIT;
		case SDLK_KP_LEFTPAREN: return FK_KP_LEFTPAREN;
		case SDLK_KP_RIGHTPAREN: return FK_KP_RIGHTPAREN;
		case SDLK_KP_LEFTBRACE: return FK_KP_LEFTBRACE;
		case SDLK_KP_RIGHTBRACE: return FK_KP_RIGHTBRACE;
		case SDLK_KP_TAB: return FK_KP_TAB;
		case SDLK_KP_BACKSPACE: return FK_KP_BACKSPACE;
		case SDLK_KP_A: return FK_KP_A;
		case SDLK_KP_B: return FK_KP_B;
		case SDLK_KP_C: return FK_KP_C;
		case SDLK_KP_D: return FK_KP_D;
		case SDLK_KP_E: return FK_KP_E;
		case SDLK_KP_F: return FK_KP_F;
		case SDLK_KP_XOR: return FK_KP_XOR;
		case SDLK_KP_POWER: return FK_KP_POWER;
		case SDLK_KP_PERCENT: return FK_KP_PERCENT;
		case SDLK_KP_LESS: return FK_KP_LESS;
		case SDLK_KP_GREATER: return FK_KP_GREATER;
		case SDLK_KP_AMPERSAND: return FK_KP_AMPERSAND;
		case SDLK_KP_DBLAMPERSAND: return FK_KP_DBLAMPERSAND;
		case SDLK_KP_VERTICALBAR: return FK_KP_VERTICALBAR;
		case SDLK_KP_DBLVERTICALBAR: return FK_KP_DBLVERTICALBAR;
		case SDLK_KP_COLON: return FK_KP_COLON;
		case SDLK_KP_HASH: return FK_KP_HASH;
		case SDLK_KP_SPACE: return FK_KP_SPACE;
		case SDLK_KP_AT: return FK_KP_AT;
		case SDLK_KP_EXCLAM: return FK_KP_EXCLAM;
		case SDLK_KP_MEMSTORE: return FK_KP_MEMSTORE;
		case SDLK_KP_MEMRECALL: return FK_KP_MEMRECALL;
		case SDLK_KP_MEMCLEAR: return FK_KP_MEMCLEAR;
		case SDLK_KP_MEMADD: return FK_KP_MEMADD;
		case SDLK_KP_MEMSUBTRACT: return FK_KP_MEMSUBTRACT;
		case SDLK_KP_MEMMULTIPLY: return FK_KP_MEMMULTIPLY;
		case SDLK_KP_MEMDIVIDE: return FK_KP_MEMDIVIDE;
		case SDLK_KP_PLUSMINUS: return FK_KP_PLUSMINUS;
		case SDLK_KP_CLEAR: return FK_KP_CLEAR;
		case SDLK_KP_CLEARENTRY: return FK_KP_CLEARENTRY;
		case SDLK_KP_BINARY: return FK_KP_BINARY;
		case SDLK_KP_OCTAL: return FK_KP_OCTAL;
		case SDLK_KP_DECIMAL: return FK_KP_DECIMAL;
		case SDLK_KP_HEXADECIMAL: return FK_KP_HEXADECIMAL;
		case SDLK_LCTRL: return FK_LCTRL;
		case SDLK_LSHIFT: return FK_LSHIFT;
		case SDLK_LALT: return FK_LALT;
		case SDLK_LGUI: return FK_LGUI;
		case SDLK_RCTRL: return FK_RCTRL;
		case SDLK_RSHIFT: return FK_RSHIFT;
		case SDLK_RALT: return FK_RALT;
		case SDLK_RGUI: return FK_RGUI;
		case SDLK_MODE: return FK_MODE;
		case SDLK_AUDIONEXT: return FK_AUDIONEXT;
		case SDLK_AUDIOPREV: return FK_AUDIOPREV;
		case SDLK_AUDIOSTOP: return FK_AUDIOSTOP;
		case SDLK_AUDIOPLAY: return FK_AUDIOPLAY;
		case SDLK_AUDIOMUTE: return FK_AUDIOMUTE;
		case SDLK_MEDIASELECT: return FK_MEDIASELECT;
		case SDLK_WWW: return FK_WWW;
		case SDLK_MAIL: return FK_MAIL;
		case SDLK_CALCULATOR: return FK_CALCULATOR;
		case SDLK_COMPUTER: return FK_COMPUTER;
		case SDLK_AC_SEARCH: return FK_AC_SEARCH;
		case SDLK_AC_HOME: return FK_AC_HOME;
		case SDLK_AC_BACK: return FK_AC_BACK;
		case SDLK_AC_FORWARD: return FK_AC_FORWARD;
		case SDLK_AC_STOP: return FK_AC_STOP;
		case SDLK_AC_REFRESH: return FK_AC_REFRESH;
		case SDLK_AC_BOOKMARKS: return FK_AC_BOOKMARKS;
		case SDLK_BRIGHTNESSDOWN: return FK_BRIGHTNESSDOWN;
		case SDLK_BRIGHTNESSUP: return FK_BRIGHTNESSUP;
		case SDLK_DISPLAYSWITCH: return FK_DISPLAYSWITCH;
		case SDLK_KBDILLUMTOGGLE: return FK_KBDILLUMTOGGLE;
		case SDLK_KBDILLUMDOWN: return FK_KBDILLUMDOWN;
		case SDLK_KBDILLUMUP: return FK_KBDILLUMUP;
		case SDLK_EJECT: return FK_EJECT;
		case SDLK_SLEEP: return FK_SLEEP;
	}
	return FK_UNKNOWN;
}


static int _mouse_grabbed = 0;
static int _mouse_ignore_move = 1;

void
IO_FetchInput (void)
{
	SDL_Event sdlev;

	memset (input.key.press, 0, sizeof(input.key.press));
	memset (input.key.release, 0, sizeof(input.key.release));

	input.mouse.delta[0] = 0;
	input.mouse.delta[1] = 0;
	memset (input.mouse.button.press, 0, sizeof(input.mouse.button.press));
	memset (input.mouse.button.release, 0, sizeof(input.mouse.button.release));

	while (SDL_PollEvent(&sdlev))
	{
		switch (sdlev.type)
		{
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				int fk = XlateSDLKey(sdlev.key.keysym.sym);
				if (fk >= 0 && fk < (sizeof(input.key.state) / sizeof(input.key.state[0])))
				{
					if (sdlev.type == SDL_KEYDOWN)
					{
						input.key.state[fk] = 1;
						input.key.press[fk] = 1;
					}
					else
					{
						input.key.state[fk] = 0;
						input.key.release[fk] = 1;
					}
				}
				break;
			}

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				int b;
				switch (sdlev.button.button)
				{
					case 1: b = MBUTTON_LEFT; break;
					case 2: b = MBUTTON_MIDDLE; break;
					case 3: b = MBUTTON_RIGHT; break;
					case 4: b = MBUTTON_WHEEL_UP; break;
					case 5: b = MBUTTON_WHEEL_DOWN; break;
					default: b = -1; break;
				}

				if (b != -1)
				{
					if (sdlev.type == SDL_MOUSEBUTTONDOWN)
					{
						input.mouse.button.state[b] = 1;
						input.mouse.button.press[b] = 1;
					}
					else
					{
						input.mouse.button.state[b] = 0;
						input.mouse.button.release[b] = 1;
					}
				}

				break;
			}

			case SDL_MOUSEMOTION:
			{
				if (_mouse_grabbed)
				{
					/* when grabbing the mouse, the initial
					 * delta we get can be HUGE, so ignore
					 * the first mouse movement after grabbing
					 * input */
					if (_mouse_ignore_move == 0)
					{
						//FIXME: a grabbed mouse under vmware gives jacked deltas
						input.mouse.delta[0] = sdlev.motion.xrel;
						input.mouse.delta[1] = sdlev.motion.yrel;
					}
					else
					{
						_mouse_ignore_move = 0;
					}
				}
				break;
			}

			case SDL_QUIT:
				F_Quit ();
				break;

			default:
				break;
		}
	}
}


void
IO_SetGrab (int grab)
{
	if (grab && !_mouse_grabbed)
	{
//		SDL_WM_GrabInput (SDL_GRAB_ON);
		SDL_ShowCursor (SDL_DISABLE);
		_mouse_grabbed = 1;
		_mouse_ignore_move = 1;
	}
	else if (!grab && _mouse_grabbed)
	{
//		SDL_WM_GrabInput (SDL_GRAB_OFF);
		SDL_ShowCursor (SDL_ENABLE);
		_mouse_grabbed = 0;
		_mouse_ignore_move = 1;
	}
}


void
IO_ToggleGrab (void)
{
	IO_SetGrab (!_mouse_grabbed);
}


void
IO_Swap (void)
{
	//SDL_UpdateTexture (tex, NULL, pix, 640 * sizeof(*pix));
	//SDL_RenderCopy (rend, tex, NULL, NULL);
	//SDL_RenderPresent (rend);
}


extern void
F_Init (void);

extern void
F_RunTime (int msecs);


int
main (int argc, const char **argv)
{
	unsigned int last, duration, now;

	if (SDL_Init(SDL_INIT_TIMER) != 0)
	{
		fprintf (stderr, "ERROR: SDL init failed: %s\n", SDL_GetError());
		exit (EXIT_FAILURE);
	}

//	F_Init ();

	last = IO_MSecs ();
	for (;;)
	{
		do
		{
			now = IO_MSecs ();
			duration = now - last;
		} while (duration < 1);
		last = now;

		F_RunTime (duration);
	}

	return 0;
}
