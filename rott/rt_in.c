/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "rt_main.h"
#include "rt_spbal.h"
#include "rt_def.h"
#include "rt_in.h"
#include "_rt_in.h"
#include "isr.h"
#include "rt_util.h"
#include "rt_swift.h"
#include "rt_cfg.h"
#include "rt_msg.h"
#include "rt_net.h"
#include "rt_com.h"
#include "keyb.h"

#define MAXMESSAGELENGTH      (COM_MAXTEXTSTRINGLENGTH-1)

//****************************************************************************
//
// GLOBALS
//
//****************************************************************************]

//
// Used by menu routines that need to wait for a button release.
// Sometimes the mouse driver misses an interrupt, so you can't wait for
// a button to be released.  Instead, you must ignore any buttons that
// are pressed.
//
int IgnoreMouse = 0;

// configuration variables
//
boolean SpaceBallPresent;
boolean CybermanPresent;
boolean AssassinPresent;
boolean MousePresent;
boolean JoysPresent[MaxJoys];
boolean JoyPadPresent = 0;

//    Global variables
//
boolean Paused;
char LastASCII;
volatile int LastScan;

byte Joy_xb,
	Joy_yb,
	Joy_xs,
	Joy_ys;
word Joy_x,
	Joy_y;

int LastLetter = 0;
char LetterQueue[MAXLETTERS];
ModemMessage MSG;

static SDL_Joystick * sdl_joysticks[MaxJoys];
static int sdl_mouse_delta_x = 0;
static int sdl_mouse_delta_y = 0;
static word sdl_mouse_button_mask = 0;
static int sdl_total_sticks = 0;
static word * sdl_stick_button_state = NULL;
static word sdl_sticks_joybits = 0;
static int sdl_mouse_grabbed = 0;
extern boolean sdl_fullscreen;

//   'q','w','e','r','t','y','u','i','o','p','[',']','\\', 0 ,'a','s',

const char ScanChars[128] =    // Scan code names with single chars
	{
		0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
		'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0, 'a', 's',
		'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
		'b', 'n', 'm', ',', '.', '/', 0, 0, 0, ' ', 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, '5', 0, '+', 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

const char ShiftedScanChars[128] =    // Shifted Scan code names with single chars
	{
		0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0, 0, 0, ' ', 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, '5', 0, '+', 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

#if 0
const char ScanChars[128] =    // Scan code names with single chars
{
   '?','?','1','2','3','4','5','6','7','8','9','0','-','+','?','?',
   'Q','W','E','R','T','Y','U','I','O','P','[',']','|','?','A','S',
   'D','F','G','H','J','K','L',';','\'','?','?','?','Z','X','C','V',
   'B','N','M',',','.','/','?','?','?',' ','?','?','?','?','?','?',
   '?','?','?','?','?','?','?','?','?','?','-','?','5','?','+','?',
   '?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?',
   '?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?',
   '?','?','?','?','?','?','?','?','?','?','?','?','?','?','?','?'
};
#endif



//****************************************************************************
//
// LOCALS
//
//****************************************************************************]

static KeyboardDef KbdDefs = { 0x1d, 0x38, 0x47, 0x48, 0x49, 0x4b, 0x4d, 0x4f, 0x50, 0x51 };
static JoystickDef JoyDefs[MaxJoys];
static ControlType Controls[MAXPLAYERS];

static boolean IN_Started;

static Direction DirTable[] =      // Quick lookup for total direction
	{
		dir_NorthWest, dir_North, dir_NorthEast,
		dir_West, dir_None, dir_East,
		dir_SouthWest, dir_South, dir_SouthEast
	};

static char * ParmStrings[] = { "nojoys", "nomouse", "spaceball", "cyberman", "assassin", NULL };

static int TranslateSdlKey( int key ) {
	switch ( key ) {
	case SDLK_BACKSLASH: return 0x2B;
	case SDLK_COMMA: return sc_Comma;
	case SDLK_PERIOD: return sc_Period;
	case SDLK_RETURN:
	case SDLK_KP_ENTER: return sc_Return;
	case SDLK_ESCAPE: return sc_Escape;
	case SDLK_SPACE: return sc_Space;
	case SDLK_BACKSPACE: return sc_BackSpace;
	case SDLK_TAB: return sc_Tab;
	case SDLK_ALTERASE: return sc_Alt;
	case SDLK_LCTRL:
	case SDLK_RCTRL: return sc_Control;
	case SDLK_CAPSLOCK: return sc_CapsLock;
	case SDLK_LSHIFT:
	case SDLK_RSHIFT: return sc_LShift;
	case SDLK_UP: return sc_UpArrow;
	case SDLK_DOWN: return sc_DownArrow;
	case SDLK_LEFT: return sc_LeftArrow;
	case SDLK_RIGHT: return sc_RightArrow;
	case SDLK_INSERT: return sc_Insert;
	case SDLK_DELETE: return sc_Delete;
	case SDLK_HOME: return sc_Home;
	case SDLK_END: return sc_End;
	case SDLK_PAGEUP: return sc_PgUp;
	case SDLK_PAGEDOWN: return sc_PgDn;
	case SDLK_F1: return sc_F1;
	case SDLK_F2: return sc_F2;
	case SDLK_F3: return sc_F3;
	case SDLK_F4: return sc_F4;
	case SDLK_F5: return sc_F5;
	case SDLK_F6: return sc_F6;
	case SDLK_F7: return sc_F7;
	case SDLK_F8: return sc_F8;
	case SDLK_F9: return sc_F9;
	case SDLK_F10: return sc_F10;
	case SDLK_F11: return sc_F11;
	case SDLK_F12: return sc_F12;
	case SDLK_PRINTSCREEN: return sc_PrintScreen;
	case SDLK_1: return sc_1;
	case SDLK_2: return sc_2;
	case SDLK_3: return sc_3;
	case SDLK_4: return sc_4;
	case SDLK_5: return sc_5;
	case SDLK_6: return sc_6;
	case SDLK_7: return sc_7;
	case SDLK_8: return sc_8;
	case SDLK_9: return sc_9;
	case SDLK_0: return sc_0;
	case SDLK_MINUS:
	case SDLK_KP_MINUS: return sc_Minus;
	case SDLK_EQUALS:
	case SDLK_KP_EQUALS: return sc_Equals;
	case SDLK_PLUS:
	case SDLK_KP_PLUS: return sc_Plus;
	case SDLK_a: return sc_A;
	case SDLK_b: return sc_B;
	case SDLK_c: return sc_C;
	case SDLK_d: return sc_D;
	case SDLK_e: return sc_E;
	case SDLK_f: return sc_F;
	case SDLK_g: return sc_G;
	case SDLK_h: return sc_H;
	case SDLK_i: return sc_I;
	case SDLK_j: return sc_J;
	case SDLK_k: return sc_K;
	case SDLK_l: return sc_L;
	case SDLK_m: return sc_M;
	case SDLK_n: return sc_N;
	case SDLK_o: return sc_O;
	case SDLK_p: return sc_P;
	case SDLK_q: return sc_Q;
	case SDLK_r: return sc_R;
	case SDLK_s: return sc_S;
	case SDLK_t: return sc_T;
	case SDLK_u: return sc_U;
	case SDLK_v: return sc_V;
	case SDLK_w: return sc_W;
	case SDLK_x: return sc_X;
	case SDLK_y: return sc_Y;
	case SDLK_z: return sc_Z;
	default: return '\0';
	}
}

static int sdl_mouse_button_filter( SDL_Event const * event ) {
	/*
	 * What DOS games expect:
	 *  0	left button pressed if 1
	 *  1	right button pressed if 1
	 *  2	middle button pressed if 1
	 *
	 *   (That is, this is what Int 33h (AX=0x05) returns...)
	 */

	Uint8 bmask = SDL_GetMouseState(NULL, NULL);
	sdl_mouse_button_mask = 0;  /* this is a static var. */
	if ( bmask & SDL_BUTTON_LMASK)
		sdl_mouse_button_mask |= 1;
	if ( bmask & SDL_BUTTON_RMASK)
		sdl_mouse_button_mask |= 2;
	if ( bmask & SDL_BUTTON_MMASK)
		sdl_mouse_button_mask |= 4;
	return ( 0 );
} /* sdl_mouse_up_filter */


static int sdl_mouse_motion_filter( SDL_Event const * event ) {
	static int mouse_x = 0;
	static int mouse_y = 0;
	int mouse_relative_x = 0;
	int mouse_relative_y = 0;

	if ( event->type == SDL_JOYBALLMOTION ) {
		mouse_relative_x = event->jball.xrel / 100;
		mouse_relative_y = event->jball.yrel / 100;
		mouse_x += mouse_relative_x;
		mouse_y += mouse_relative_y;
	} /* if */
	else {
		if ( sdl_mouse_grabbed || sdl_fullscreen ) {
			mouse_relative_x = event->motion.xrel;
			mouse_relative_y = event->motion.yrel;
			mouse_x += mouse_relative_x;
			mouse_y += mouse_relative_y;
		} /* if */
		else {
			mouse_relative_x = event->motion.x - mouse_x;
			mouse_relative_y = event->motion.y - mouse_y;
			mouse_x = event->motion.x;
			mouse_y = event->motion.y;
		} /* else */
	} /* else */

#if 0
	if (mouse_x < 0) mouse_x = 0;
	if (mouse_x > surface->w) mouse_x = surface->w;
	if (mouse_y < 0) mouse_y = 0;
	if (mouse_y > surface->h) mouse_y = surface->h;
#endif

	/* set static vars... */
	sdl_mouse_delta_x += mouse_relative_x;
	sdl_mouse_delta_y += mouse_relative_y;

	return ( 0 );
} /* sdl_mouse_motion_filter */

/*
 * The windib driver can't alert us to the keypad enter key, which
 *  Ken's code depends on heavily. It sends it as the same key as the
 *  regular return key. These users will have to hit SHIFT-ENTER,
 *  which we check for explicitly, and give the engine a keypad enter
 *  enter event.
 */
static int handle_keypad_enter_hack( const SDL_Event * event ) {
	static int kp_enter_hack = 0;
	int retval = 0;

	if ( event->key.keysym.sym == SDLK_RETURN ) {
		if ( event->key.state == SDL_PRESSED ) {
			if ( event->key.keysym.mod & KMOD_SHIFT) {
				kp_enter_hack = 1;
				retval = TranslateSdlKey( SDLK_KP_ENTER );
			} /* if */
		} /* if */

		else  /* key released */
		{
			if ( kp_enter_hack ) {
				kp_enter_hack = 0;
				retval = TranslateSdlKey( SDLK_KP_ENTER );
			} /* if */
		} /* if */
	} /* if */

	return ( retval );
} /* handle_keypad_enter_hack */


static int sdl_key_filter( const SDL_Event * event ) {
	int k;
	int keyon;
	int strippedkey;
	int extended;

#if 0 // todo ?
	SDL_GrabMode grab_mode = SDL_GRAB_OFF;
	if (( event->key.keysym.sym == SDLK_g ) &&
		( event->key.state == SDL_PRESSED ) &&
		( event->key.keysym.mod & KMOD_CTRL)) {
		if ( !sdl_fullscreen ) {
			sdl_mouse_grabbed = (( sdl_mouse_grabbed ) ? 0 : 1 );
			if ( sdl_mouse_grabbed )
				grab_mode = SDL_GRAB_ON;
			SDL_WM_GrabInput( grab_mode );
		}
		return ( 0 );
	} /* if */

	else if ((( event->key.keysym.sym == SDLK_RETURN ) ||
		( event->key.keysym.sym == SDLK_KP_ENTER )) &&
		( event->key.state == SDL_PRESSED ) &&
		( event->key.keysym.mod & KMOD_ALT)) {
		if ( SDL_WM_ToggleFullScreen( SDL_GetVideoSurface()))
			sdl_fullscreen ^= 1;
		return ( 0 );
	} /* if */
#endif

	/* HDG: put this above the scancode lookup otherwise it is never reached */
	if (( event->key.keysym.sym == SDLK_PAUSE ) &&
		( event->key.state == SDL_PRESSED )) {
		PausePressed = true;
		return ( 0 );
	}

	k = handle_keypad_enter_hack( event );
	if ( !k ) {
		k = TranslateSdlKey( event->key.keysym.sym );
		if ( !k )   /* No DOS equivalent defined. */
			return ( 0 );
	} /* if */

	/* Fix elweirdo SDL capslock/numlock handling, always treat as press */
	if (( event->key.keysym.sym != SDLK_CAPSLOCK ) &&
		( event->key.keysym.sym != SDLK_NUMLOCKCLEAR ) &&
		( event->key.state == SDL_RELEASED ))
		k += 128;  /* +128 signifies that the key is released in DOS. */

	if ( event->key.keysym.sym == SDLK_SCROLLLOCK )
		PanicPressed = true;

	else {
		extended = (( k & 0xFF00 ) >> 8 );

		keyon = k & 0x80;
		strippedkey = k & 0x7f;

		if ( extended != 0 ) {
			KeyboardQueue[Keytail] = extended;
			Keytail = ( Keytail + 1 ) & ( KEYQMAX - 1 );
			k = TranslateSdlKey( event->key.keysym.sym ) & 0xFF;
			if ( event->key.state == SDL_RELEASED )
				k += 128;  /* +128 signifies that the key is released in DOS. */
		}

		if ( keyon )        // Up event
			Keystate[strippedkey] = 0;
		else                 // Down event
		{
			Keystate[strippedkey] = 1;
			LastScan = k;
		}

		KeyboardQueue[Keytail] = k;
		Keytail = ( Keytail + 1 ) & ( KEYQMAX - 1 );
	}

	return ( 0 );
} /* sdl_key_filter */

static int root_sdl_event_filter( const SDL_Event * event ) {
	switch ( event->type ) {
	default: break;

	case SDL_WINDOWEVENT:
		switch ( event->window.type ) {
		default: break;

		case SDL_WINDOWEVENT_FOCUS_LOST: SDL_SetRelativeMouseMode( false );
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED: SDL_SetRelativeMouseMode( true );
			break;
		case SDL_WINDOWEVENT_MINIMIZED: SDL_SetRelativeMouseMode( false );
			break;
		case SDL_WINDOWEVENT_RESTORED: SDL_SetRelativeMouseMode( true );
			break;
		case SDL_WINDOWEVENT_CLOSE: break;
		}
		break;

	case SDL_KEYUP:
	case SDL_KEYDOWN: return ( sdl_key_filter( event ));
	case SDL_JOYBALLMOTION:
	case SDL_MOUSEMOTION: return ( sdl_mouse_motion_filter( event ));
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEBUTTONDOWN: return ( sdl_mouse_button_filter( event ));
	case SDL_QUIT:
		/* !!! rcg TEMP */
		fprintf( stderr, "\n\n\nSDL_QUIT!\n\n\n" );
		SDL_Quit();
		exit( 42 );
	} /* switch */

	return ( 1 );
} /* root_sdl_event_filter */

static void sdl_handle_events( void ) {
	SDL_Event event;
	while ( SDL_PollEvent( &event ))
		root_sdl_event_filter( &event );
} /* sdl_handle_events */

//******************************************************************************
//
// IN_PumpEvents () - Let platform process an event queue.
//
//******************************************************************************
void IN_PumpEvents( void ) {
	sdl_handle_events();
}



//******************************************************************************
//
// INL_GetMouseDelta () - Gets the amount that the mouse has moved from the
//                        mouse driver
//
//******************************************************************************

void INL_GetMouseDelta( int * x, int * y ) {
	IN_PumpEvents();

	*x = sdl_mouse_delta_x;
	*y = sdl_mouse_delta_y;

	sdl_mouse_delta_x = sdl_mouse_delta_y = 0;
}



//******************************************************************************
//
// IN_GetMouseButtons () - Gets the status of the mouse buttons from the
//                         mouse driver
//
//******************************************************************************

word IN_GetMouseButtons
	(
		void
	) {
	word buttons = 0;

	IN_PumpEvents();

	buttons = sdl_mouse_button_mask;

// Used by menu routines that need to wait for a button release.
// Sometimes the mouse driver misses an interrupt, so you can't wait for
// a button to be released.  Instead, you must ignore any buttons that
// are pressed.

	IgnoreMouse &= buttons;
	buttons &= ~IgnoreMouse;

	return ( buttons );
}


//******************************************************************************
//
// IN_IgnoreMouseButtons () -
//    Tells the mouse to ignore the currently pressed buttons.
//
//******************************************************************************

void IN_IgnoreMouseButtons( void ) {
	IgnoreMouse |= IN_GetMouseButtons();
}


//******************************************************************************
//
// IN_GetJoyAbs () - Reads the absolute position of the specified joystick
//
//******************************************************************************

void IN_GetJoyAbs( word joy, word * xp, word * yp ) {
	Joy_x = Joy_y = 0;
	Joy_xs = joy ? 2 : 0;       // Find shift value for x axis
	Joy_xb = 1 << Joy_xs;      // Use shift value to get x bit mask
	Joy_ys = joy ? 3 : 1;       // Do the same for y axis
	Joy_yb = 1 << Joy_ys;

	if ( joy < sdl_total_sticks ) {
		Joy_x = SDL_JoystickGetAxis( sdl_joysticks[joy], 0 );
		Joy_y = SDL_JoystickGetAxis( sdl_joysticks[joy], 1 );
	} else {
		Joy_x = 0;
		Joy_y = 0;
	}

	*xp = Joy_x;
	*yp = Joy_y;
}

void JoyStick_Vals( void ) {

}


//******************************************************************************
//
// INL_GetJoyDelta () - Returns the relative movement of the specified
//                     joystick (from +/-127)
//
//******************************************************************************

void INL_GetJoyDelta( word joy, int * dx, int * dy ) {
	word x, y;
	JoystickDef * def;
	static longword lasttime;

	IN_GetJoyAbs( joy, &x, &y );
	def = JoyDefs + joy;

	if ( x < def->threshMinX ) {
		if ( x < def->joyMinX )
			x = def->joyMinX;

		x = -( x - def->threshMinX );
		x *= def->joyMultXL;
		x >>= JoyScaleShift;
		*dx = ( x > 127 ) ? -127 : -x;
	} else if ( x > def->threshMaxX ) {
		if ( x > def->joyMaxX )
			x = def->joyMaxX;

		x = x - def->threshMaxX;
		x *= def->joyMultXH;
		x >>= JoyScaleShift;
		*dx = ( x > 127 ) ? 127 : x;
	} else
		*dx = 0;

	if ( y < def->threshMinY ) {
		if ( y < def->joyMinY )
			y = def->joyMinY;

		y = -( y - def->threshMinY );
		y *= def->joyMultYL;
		y >>= JoyScaleShift;
		*dy = ( y > 127 ) ? -127 : -y;
	} else if ( y > def->threshMaxY ) {
		if ( y > def->joyMaxY )
			y = def->joyMaxY;

		y = y - def->threshMaxY;
		y *= def->joyMultYH;
		y >>= JoyScaleShift;
		*dy = ( y > 127 ) ? 127 : y;
	} else
		*dy = 0;

	lasttime = GetTicCount();
}



//******************************************************************************
//
// INL_GetJoyButtons () - Returns the button status of the specified
//                        joystick
//
//******************************************************************************

word INL_GetJoyButtons( word joy ) {
	word result = 0;

	if ( joy < sdl_total_sticks )
		result = sdl_stick_button_state[joy];

	return result;
}

#if 0
//******************************************************************************
//
// IN_GetJoyButtonsDB () - Returns the de-bounced button status of the
//                         specified joystick
//
//******************************************************************************

word IN_GetJoyButtonsDB (word joy)
{
   longword lasttime;
   word result1,result2;

   do
   {
	  result1 = INL_GetJoyButtons (joy);
	  lasttime = GetTicCount();
	  while (GetTicCount() == lasttime)
		 ;
	  result2 = INL_GetJoyButtons (joy);
   } while (result1 != result2);

   return(result1);
}
#endif

//******************************************************************************
//
// INL_StartMouse () - Detects and sets up the mouse
//
//******************************************************************************

boolean INL_StartMouse( void ) {
	// todo: useless function, remove...
	return true;
}



//******************************************************************************
//
// INL_SetJoyScale () - Sets up scaling values for the specified joystick
//
//******************************************************************************

void INL_SetJoyScale( word joy ) {
	JoystickDef * def;

	def = &JoyDefs[joy];
	def->joyMultXL = JoyScaleMax / ( def->threshMinX - def->joyMinX );
	def->joyMultXH = JoyScaleMax / ( def->joyMaxX - def->threshMaxX );
	def->joyMultYL = JoyScaleMax / ( def->threshMinY - def->joyMinY );
	def->joyMultYH = JoyScaleMax / ( def->joyMaxY - def->threshMaxY );
}



//******************************************************************************
//
// IN_SetupJoy () - Sets up thresholding values and calls INL_SetJoyScale()
//                  to set up scaling values
//
//******************************************************************************

void IN_SetupJoy( word joy, word minx, word maxx, word miny, word maxy ) {
	word d, r;
	JoystickDef * def;

	def = &JoyDefs[joy];

	def->joyMinX = minx;
	def->joyMaxX = maxx;
	r = maxx - minx;
	d = r / 3;
	def->threshMinX = (( r / 2 ) - d ) + minx;
	def->threshMaxX = (( r / 2 ) + d ) + minx;

	def->joyMinY = miny;
	def->joyMaxY = maxy;
	r = maxy - miny;
	d = r / 3;
	def->threshMinY = (( r / 2 ) - d ) + miny;
	def->threshMaxY = (( r / 2 ) + d ) + miny;

	INL_SetJoyScale( joy );
}


//******************************************************************************
//
// INL_StartJoy () - Detects & auto-configures the specified joystick
//                   The auto-config assumes the joystick is centered
//
//******************************************************************************


boolean INL_StartJoy( word joy ) {
	word x, y;

	if ( !SDL_WasInit( SDL_INIT_JOYSTICK )) {
		SDL_Init( SDL_INIT_JOYSTICK );
		sdl_total_sticks = SDL_NumJoysticks();
		if ( sdl_total_sticks > MaxJoys )
			sdl_total_sticks = MaxJoys;

		if (( sdl_stick_button_state == NULL) && ( sdl_total_sticks > 0 )) {
			sdl_stick_button_state = ( word * ) malloc( sizeof( word ) * sdl_total_sticks );
			if ( sdl_stick_button_state == NULL)
				SDL_QuitSubSystem( SDL_INIT_JOYSTICK );
			else
				memset( sdl_stick_button_state, '\0', sizeof( word ) * sdl_total_sticks );
		}
		SDL_JoystickEventState( SDL_ENABLE );
	}

	if ( joy >= sdl_total_sticks )
		return ( false );
	sdl_joysticks[joy] = SDL_JoystickOpen( joy );

	IN_GetJoyAbs( joy, &x, &y );

	if
		(
		(( x == 0 ) || ( x > MaxJoyValue - 10 ))
			|| (( y == 0 ) || ( y > MaxJoyValue - 10 ))
		)
		return ( false );
	else {
		IN_SetupJoy( joy, 0, x * 2, 0, y * 2 );
		return ( true );
	}
}



//******************************************************************************
//
// INL_ShutJoy() - Cleans up the joystick stuff
//
//******************************************************************************

void INL_ShutJoy( word joy ) {
	JoysPresent[joy] = false;

	if ( joy < sdl_total_sticks )
		SDL_JoystickClose( sdl_joysticks[joy] );
}



//******************************************************************************
//
// IN_Startup() - Starts up the Input Mgr
//
//******************************************************************************


void IN_Startup( void ) {
	boolean checkjoys,
		checkmouse,
		checkspaceball;

	word i;

	if ( IN_Started == true )
		return;

	/*
	  all keys are now mapped to the wolf3d-style names,
	  except where no such name is available.
	 */
#if 0 // todo
	scancodes[SDLK_LEFTBRACKET] = sc_OpenBracket;
	scancodes[SDLK_RIGHTBRACKET] = sc_CloseBracket;
	scancodes[SDLK_RETURN] = sc_Return;
	scancodes[SDLK_LCTRL] = sc_Control;
	scancodes[SDLK_SEMICOLON] = 0x27;
	scancodes[SDLK_QUOTE] = 0x28;
	scancodes[SDLK_BACKQUOTE] = 0x29;

	scancodes[SDLK_z] = sc_Z;
	scancodes[SDLK_x] = sc_X;
	scancodes[SDLK_c] = sc_C;
	scancodes[SDLK_v] = sc_V;
	scancodes[SDLK_b] = sc_B;
	scancodes[SDLK_n] = sc_N;
	scancodes[SDLK_m] = sc_M;
	scancodes[SDLK_COMMA] = sc_Comma;
	scancodes[SDLK_PERIOD] = sc_Period;
	scancodes[SDLK_SLASH] = 0x35;
	scancodes[SDLK_RSHIFT] = sc_RShift;
	scancodes[SDLK_KP_DIVIDE] = 0x35;

	/* 0x37 is printscreen */
	//scancodes[SDLK_KP_MULTIPLY]     = 0x37;

	scancodes[SDLK_LALT] = sc_Alt;
	scancodes[SDLK_RALT] = sc_Alt;
	scancodes[SDLK_MODE] = sc_Alt;

	scancodes[SDLK_NUMLOCK] = 0x45;
	scancodes[SDLK_SCROLLOCK] = 0x46;

	//scancodes[SDLK_MINUS]           = 0x4A;
	scancodes[SDLK_MINUS] = sc_Minus;

	scancodes[SDLK_KP7] = sc_Home;
	scancodes[SDLK_KP8] = sc_UpArrow;
	scancodes[SDLK_KP9] = sc_PgUp;
	scancodes[SDLK_HOME] = sc_Home;
	scancodes[SDLK_UP] = sc_UpArrow;
	scancodes[SDLK_PAGEUP] = sc_PgUp;
	// Make this a normal minus, for viewport changing
	//scancodes[SDLK_KP_MINUS]        = 0xE04A;
	scancodes[SDLK_KP_MINUS] = sc_Minus;
	scancodes[SDLK_KP4] = sc_LeftArrow;
	scancodes[SDLK_KP5] = 0x4C;
	scancodes[SDLK_KP6] = sc_RightArrow;
	scancodes[SDLK_LEFT] = sc_LeftArrow;
	scancodes[SDLK_RIGHT] = sc_RightArrow;

	//scancodes[SDLK_KP_PLUS]         = 0x4E;
	scancodes[SDLK_KP_PLUS] = sc_Plus;

	scancodes[SDLK_KP1] = sc_End;
	scancodes[SDLK_KP2] = sc_DownArrow;
	scancodes[SDLK_KP3] = sc_PgDn;
	scancodes[SDLK_END] = sc_End;
	scancodes[SDLK_DOWN] = sc_DownArrow;
	scancodes[SDLK_PAGEDOWN] = sc_PgDn;
	scancodes[SDLK_DELETE] = sc_Delete;
	scancodes[SDLK_KP0] = sc_Insert;
	scancodes[SDLK_INSERT] = sc_Insert;
	scancodes[SDLK_KP_ENTER] = sc_Return;
#endif

	checkjoys = true;
	checkmouse = true;
	SpaceBallPresent = false;
	CybermanPresent = false;
	AssassinPresent = false;

	for ( i = 1; i < _argc; i++ ) {
		switch ( US_CheckParm( _argv[i], ParmStrings )) {
		case 0:checkjoys = false;
			break;

		case 1:checkmouse = false;
			break;

		case 2:break;

		case 3: checkmouse = false;
			break;

		case 4: checkmouse = false;
			break;
		}
	}

	MousePresent = checkmouse ? INL_StartMouse() : false;

	if ( !MousePresent )
		mouseenabled = false;
	else {
		if ( !quiet )
			printf( "IN_Startup: Mouse Present\n" );
	}

	for ( i = 0; i < MaxJoys; i++ ) {
		JoysPresent[i] = checkjoys ? INL_StartJoy( i ) : false;
		if ( INL_StartJoy( i )) {
			if ( !quiet )
				printf( "IN_Startup: Joystick Present\n" );
		}
	}

	IN_Started = true;
}

#if 0
//******************************************************************************
//
// IN_Default() - Sets up default conditions for the Input Mgr
//
//******************************************************************************

void IN_Default (boolean gotit, ControlType in)
{
   if
   (
	  (!gotit)
   ||    ((in == ctrl_Joystick1) && !JoysPresent[0])
   ||    ((in == ctrl_Joystick2) && !JoysPresent[1])
   ||    ((in == ctrl_Mouse) && !MousePresent)
   )
	  in = ctrl_Keyboard1;
   IN_SetControlType (0, in);
}
#endif

//******************************************************************************
//
// IN_Shutdown() - Shuts down the Input Mgr
//
//******************************************************************************

void IN_Shutdown( void ) {
	if ( IN_Started == false )
		return;

//   INL_ShutMouse();

	for ( unsigned int i = 0; i < MaxJoys; i++ )
		INL_ShutJoy( i );

	CloseSpaceBall();

	IN_Started = false;
}


//******************************************************************************
//
// IN_ClearKeysDown() - Clears the keyboard array
//
//******************************************************************************

void IN_ClearKeysDown( void ) {
	LastScan = sc_None;
	memset(( void * ) Keyboard, 0, sizeof( Keyboard ));
}


//******************************************************************************
//
// IN_ReadControl() - Reads the device associated with the specified
//    player and fills in the control info struct
//
//******************************************************************************

void IN_ReadControl( int player, ControlInfo * info ) {
	boolean realdelta;
	word buttons;
	int dx, dy;
	Motion mx, my;
	ControlType type;

	KeyboardDef * def;

	dx = dy = 0;
	mx = my = motion_None;
	buttons = 0;

	switch ( type = Controls[player] ) {
	case ctrl_Keyboard:def = &KbdDefs;

#if 0
		if (Keyboard[def->upleft])
		   mx = motion_Left,my = motion_Up;
		else if (Keyboard[def->upright])
		   mx = motion_Right,my = motion_Up;
		else if (Keyboard[def->downleft])
		   mx = motion_Left,my = motion_Down;
		else if (Keyboard[def->downright])
		   mx = motion_Right,my = motion_Down;
#endif
		if ( Keyboard[sc_UpArrow] )
			my = motion_Up;
		else if ( Keyboard[sc_DownArrow] )
			my = motion_Down;

		if ( Keyboard[sc_LeftArrow] )
			mx = motion_Left;
		else if ( Keyboard[sc_RightArrow] )
			mx = motion_Right;

		if ( Keyboard[def->button0] )
			buttons += 1 << 0;
		if ( Keyboard[def->button1] )
			buttons += 1 << 1;
		realdelta = false;
		break;

#if 0
		case ctrl_Joystick1:
		case ctrl_Joystick2:
		   INL_GetJoyDelta (type - ctrl_Joystick, &dx, &dy);
		   buttons = INL_GetJoyButtons (type - ctrl_Joystick);
		   realdelta = true;
		break;

		case ctrl_Mouse:
		   INL_GetMouseDelta (&dx,&dy);
		   buttons = IN_GetMouseButtons ();
		   realdelta = true;
		break;

#endif
	default:;
	}

	if ( realdelta ) {
		mx = ( dx < 0 ) ? motion_Left : (( dx > 0 ) ? motion_Right : motion_None );
		my = ( dy < 0 ) ? motion_Up : (( dy > 0 ) ? motion_Down : motion_None );
	} else {
		dx = mx * 127;
		dy = my * 127;
	}

	info->x = dx;
	info->xaxis = mx;
	info->y = dy;
	info->yaxis = my;
	info->button0 = buttons & ( 1 << 0 );
	info->button1 = buttons & ( 1 << 1 );
	info->button2 = buttons & ( 1 << 2 );
	info->button3 = buttons & ( 1 << 3 );
	info->dir = DirTable[(( my + 1 ) * 3 ) + ( mx + 1 )];
}


//******************************************************************************
//
// IN_WaitForKey() - Waits for a scan code, then clears LastScan and
//    returns the scan code
//
//******************************************************************************

ScanCode IN_WaitForKey( void ) {
	ScanCode result;

	while ( !( result = LastScan ))
		IN_PumpEvents();
	LastScan = 0;
	return ( result );
}


//******************************************************************************
//
// IN_Ack() - waits for a button or key press.  If a button is down, upon
// calling, it must be released for it to be recognized
//
//******************************************************************************

boolean btnstate[8];

void IN_StartAck( void ) {
	unsigned i,
		buttons = 0;

//
// get initial state of everything
//
	LastScan = 0;

	IN_ClearKeysDown();
	memset( btnstate, 0, sizeof( btnstate ));

	IN_PumpEvents();

	buttons = IN_JoyButtons() << 4;

	buttons |= IN_GetMouseButtons();

	if ( SpaceBallPresent && spaceballenabled ) {
		buttons |= GetSpaceBallButtons();
	}

	for ( i = 0; i < 8; i++, buttons >>= 1 )
		if ( buttons & 1 )
			btnstate[i] = true;
}



//******************************************************************************
//
// IN_CheckAck ()
//
//******************************************************************************

boolean IN_CheckAck( void ) {
	unsigned i,
		buttons = 0;

//
// see if something has been pressed
//
	if ( LastScan )
		return true;

	IN_PumpEvents();

	buttons = IN_JoyButtons() << 4;

	buttons |= IN_GetMouseButtons();

	for ( i = 0; i < 8; i++, buttons >>= 1 )
		if ( buttons & 1 ) {
			if ( !btnstate[i] )
				return true;
		} else
			btnstate[i] = false;

	return false;
}



//******************************************************************************
//
// IN_Ack ()
//
//******************************************************************************

void IN_Ack( void ) {
	IN_StartAck();

	while ( !IN_CheckAck());
}



//******************************************************************************
//
// IN_UserInput() - Waits for the specified delay time (in ticks) or the
//    user pressing a key or a mouse button. If the clear flag is set, it
//    then either clears the key or waits for the user to let the mouse
//    button up.
//
//******************************************************************************

boolean IN_UserInput( long delay ) {
	long lasttime;

	lasttime = GetTicCount();

	IN_StartAck();
	do {
		if ( IN_CheckAck())
			return true;
	} while (( GetTicCount() - lasttime ) < delay );

	return ( false );
}

//===========================================================================


/*
===================
=
= IN_JoyButtons
=
===================
*/

byte IN_JoyButtons( void ) {
	return ( byte ) sdl_sticks_joybits;
}


//******************************************************************************
//
// IN_UpdateKeyboard ()
//
//******************************************************************************

/* HACK HACK HACK */
static int queuegotit = 0;

void IN_UpdateKeyboard( void ) {
	int tail;
	int key;

	if ( !queuegotit )
		IN_PumpEvents();

	queuegotit = 0;

	if ( Keytail != Keyhead ) {
		tail = Keytail;

		while ( Keyhead != tail ) {
			if ( KeyboardQueue[Keyhead] & 0x80 )        // Up event
			{
				key = KeyboardQueue[Keyhead] & 0x7F;   // AND off high bit

//            if (keysdown[key])
//            {
//               KeyboardQueue[Keytail] = KeyboardQueue[Keyhead];
//               Keytail = (Keytail+1)&(KEYQMAX-1);
//            }
//            else
				Keyboard[key] = 0;
			} else                                      // Down event
			{
				Keyboard[KeyboardQueue[Keyhead]] = 1;
//            keysdown[KeyboardQueue[Keyhead]] = 1;
			}

			Keyhead = ( Keyhead + 1 ) & ( KEYQMAX - 1 );

		}        // while
	}           // if

	// Carry over movement keys from the last refresh
//   keysdown[sc_RightArrow] = Keyboard[sc_RightArrow];
//   keysdown[sc_LeftArrow]  = Keyboard[sc_LeftArrow];
//   keysdown[sc_UpArrow]    = Keyboard[sc_UpArrow];
//   keysdown[sc_DownArrow]  = Keyboard[sc_DownArrow];
}



//******************************************************************************
//
// IN_InputUpdateKeyboard ()
//
//******************************************************************************

int IN_InputUpdateKeyboard( void ) {
	int key;
	int returnval = 0;
	boolean done = false;

//   _disable ();

	if ( Keytail != Keyhead ) {
		int tail = Keytail;

		while ( !done && ( Keyhead != tail )) {
			if ( KeyboardQueue[Keyhead] & 0x80 )        // Up event
			{
				key = KeyboardQueue[Keyhead] & 0x7F;   // AND off high bit

				Keyboard[key] = 0;
			} else                                      // Down event
			{
				Keyboard[KeyboardQueue[Keyhead]] = 1;
				returnval = KeyboardQueue[Keyhead];
				done = true;
			}

			Keyhead = ( Keyhead + 1 ) & ( KEYQMAX - 1 );
		}
	}           // if

//   _enable ();

	return ( returnval );
}


//******************************************************************************
//
// IN_ClearKeyboardQueue ()
//
//******************************************************************************

void IN_ClearKeyboardQueue( void ) {
//   IN_ClearKeysDown ();

//   Keytail = Keyhead = 0;
//   memset (KeyboardQueue, 0, sizeof (KeyboardQueue));
//   I_SendKeyboardData(0xf6);
//   I_SendKeyboardData(0xf4);
}

#if 0
//******************************************************************************
//
// IN_DumpKeyboardQueue ()
//
//******************************************************************************

void IN_DumpKeyboardQueue (void)
{
   int head = Keyhead;
   int tail = Keytail;
   int key;

   if (tail != head)
   {
	  SoftError( "START DUMP\n");

	  while (head != tail)
	  {
		 if (KeyboardQueue[head] & 0x80)        // Up event
		 {
			key = KeyboardQueue[head] & 0x7F;   // AND off high bit

//            if (keysdown[key])
//            {
//               SoftError( "%s - was put in next refresh\n",
//                                 IN_GetScanName (key));
//            }
//            else
//            {
			   if (Keyboard[key] == 0)
				  SoftError( "%s %ld - was lost\n", IN_GetScanName (key), key);
			   else
				  SoftError( "%s %ld - up\n", IN_GetScanName (key), key);
//            }
		 }
		 else                                      // Down event
			SoftError( "%s %ld - down\n", IN_GetScanName (KeyboardQueue[head]), KeyboardQueue[head]);

		 head = (head+1)&(KEYQMAX-1);
	  }        // while

	  SoftError( "END DUMP\n");

	}           // if
}
#endif


//******************************************************************************
//
// QueueLetterInput ()
//
//******************************************************************************

void QueueLetterInput( void ) {
	int head = Keyhead;
	int tail = Keytail;
	char c;
	int scancode;
	boolean send = false;

	/* HACK HACK HACK */
	/*
	  OK, we want the new keys NOW, and not when the update gets them.
	  Since this called before IN_UpdateKeyboard in PollKeyboardButtons,
	  we shall update here.  The hack is there to prevent IN_UpdateKeyboard
	  from stealing any keys... - SBF
	 */
	IN_PumpEvents();
	head = Keyhead;
	tail = Keytail;
	queuegotit = 1;
	/* HACK HACK HACK */

	while ( head != tail ) {
		if ( !( KeyboardQueue[head] & 0x80 ))        // Down event
		{
			scancode = KeyboardQueue[head];

			if ( Keyboard[sc_RShift] || Keyboard[sc_LShift] ) {
				c = ShiftedScanChars[scancode];
			} else {
				c = ScanChars[scancode];
			}

			// If "is printable char", queue the character
			if ( c ) {
				LetterQueue[LastLetter] = c;
				LastLetter = ( LastLetter + 1 ) & ( MAXLETTERS - 1 );

				// If typing a message, update the text with 'c'

				if ( MSG.messageon ) {
					Keystate[scancode] = 0;
					KeyboardQueue[head] = 0;
					if ( MSG.inmenu ) {
						if (( c == 'A' ) || ( c == 'a' )) {
							MSG.towho = MSG_DIRECTED_TO_ALL;
							send = true;
						}

						if (( gamestate.teamplay ) &&
							(( c == 'T' ) || ( c == 't' ))) {
							MSG.towho = MSG_DIRECTED_TO_TEAM;
							send = true;
						}

						if (( c >= '0' ) && ( c <= '9' )) {
							int who;

							if ( c == '0' ) {
								who = 10;
							} else {
								who = c - '1';
							}

							// Skip over local player
							if ( who >= consoleplayer ) {
								who++;
							}

							if ( who < numplayers ) {
								MSG.towho = who;
								send = true;
							}
						}

						if ( send ) {
							MSG.messageon = false;
							KeyboardQueue[head] = 0;
							Keyboard[scancode] = 0;
							LastScan = 0;
							FinishModemMessage( MSG.textnum, true );
						}
					} else if (( scancode >= sc_1 ) && ( scancode <= sc_0 ) &&
						( Keyboard[sc_Alt] )) {
						int msg;

						msg = scancode - sc_1;

						if ( CommbatMacros[msg].avail ) {
							MSG.length = strlen( CommbatMacros[msg].macro ) + 1;
							strcpy( Messages[MSG.textnum].text,
									CommbatMacros[msg].macro );

							MSG.messageon = false;
							FinishModemMessage( MSG.textnum, true );
							KeyboardQueue[head] = 0;
							Keyboard[sc_Enter] = 0;
							Keyboard[sc_Escape] = 0;
							LastScan = 0;
						} else {
							MSG.messageon = false;
							MSG.directed = false;

							FinishModemMessage( MSG.textnum, false );
							AddMessage( "No macro.", MSG_MACRO);
							KeyboardQueue[head] = 0;
							Keyboard[sc_Enter] = 0;
							Keyboard[sc_Escape] = 0;
							LastScan = 0;
						}
					} else if ( MSG.length < MAXMESSAGELENGTH) {
						UpdateModemMessage( MSG.textnum, c );
					}
				}
			} else {
				// If typing a message, check for special characters

				if ( MSG.messageon && MSG.inmenu ) {
					if ( scancode == sc_Escape ) {
						MSG.messageon = false;
						MSG.directed = false;
						FinishModemMessage( MSG.textnum, false );
						KeyboardQueue[head] = 0;
						Keyboard[sc_Enter] = 0;
						Keyboard[sc_Escape] = 0;
						LastScan = 0;
					}
				} else if ( MSG.messageon && !MSG.inmenu ) {
					if (( scancode >= sc_F1 ) &&
						( scancode <= sc_F10 )) {
						MSG.remoteridicule = scancode - sc_F1;
						MSG.messageon = false;
						FinishModemMessage( MSG.textnum, true );
						KeyboardQueue[head] = 0;
						Keyboard[sc_Enter] = 0;
						Keyboard[sc_Escape] = 0;
						LastScan = 0;
					}

					switch ( scancode ) {
					case sc_BackSpace:KeyboardQueue[head] = 0;
						if ( MSG.length > 1 ) {
							ModemMessageDeleteChar( MSG.textnum );
						}
						Keystate[scancode] = 0;
						break;

					case sc_Enter:MSG.messageon = false;
						FinishModemMessage( MSG.textnum, true );
						KeyboardQueue[head] = 0;
						Keyboard[sc_Enter] = 0;
						Keyboard[sc_Escape] = 0;
						LastScan = 0;
						Keystate[scancode] = 0;
						break;

					case sc_Escape:MSG.messageon = false;
						MSG.directed = false;
						FinishModemMessage( MSG.textnum, false );
						KeyboardQueue[head] = 0;
						Keyboard[sc_Enter] = 0;
						Keyboard[sc_Escape] = 0;
						LastScan = 0;
						break;
					}
				}
			}
		}

		head = ( head + 1 ) & ( KEYQMAX - 1 );
	}        // while
}
