extern rw::EngineOpenParams engineOpenParams;

namespace sk {

using namespace rw;

// same as RW skeleton
enum Key
{
	// ascii...

	KEY_ESC   = 128,

	KEY_F1    = 129,
	KEY_F2    = 130,
	KEY_F3    = 131,
	KEY_F4    = 132,
	KEY_F5    = 133,
	KEY_F6    = 134,
	KEY_F7    = 135,
	KEY_F8    = 136,
	KEY_F9    = 137,
	KEY_F10   = 138,
	KEY_F11   = 139,
	KEY_F12   = 140,

	KEY_INS   = 141,
	KEY_DEL   = 142,
	KEY_HOME  = 143,
	KEY_END   = 144,
	KEY_PGUP  = 145,
	KEY_PGDN  = 146,

	KEY_UP    = 147,
	KEY_DOWN  = 148,
	KEY_LEFT  = 149,
	KEY_RIGHT = 150,

	// some stuff ommitted

	KEY_BACKSP = 168,
	KEY_TAB    = 169,
	KEY_CAPSLK = 170,
	KEY_ENTER  = 171,
	KEY_LSHIFT = 172,
	KEY_RSHIFT = 173,
	KEY_LCTRL  = 174,
	KEY_RCTRL  = 175,
	KEY_LALT   = 176,
	KEY_RALT   = 177,

	KEY_NULL,	// unused
	KEY_NUMKEYS,
};

enum EventStatus
{
	EVENTERROR,
	EVENTPROCESSED,
	EVENTNOTPROCESSED
};

enum Event
{
	INITIALIZE,
	RWINITIALIZE,
	RWTERMINATE,
	SELECTDEVICE,
	PLUGINATTACH,
	KEYDOWN,
	KEYUP,
	CHARINPUT,
	MOUSEMOVE,
	MOUSEBTN,
	MOUSEWHEEL,
	RESIZE,
	IDLE,
	QUIT
};

struct Globals
{
	const char *windowtitle;
	int32 width;
	int32 height;
	bool32 quit;
};
extern Globals globals;

// Argument to mouse events
struct MouseState
{
	int posx, posy;
	int buttons;	// bits 0-2 are left, middle, right button down
	float wheelDelta;
};

struct Args
{
	int argc;
	char **argv;
};
extern Args args;

// Pad buttons, same bit order as the PS2 pad
enum PadButton
{
	PADL2       = 0x0001,
	PADR2       = 0x0002,
	PADL1       = 0x0004,
	PADR1       = 0x0008,
	PADTRIANGLE = 0x0010,
	PADCIRCLE   = 0x0020,
	PADCROSS    = 0x0040,
	PADSQUARE   = 0x0080,
	PADSELECT   = 0x0100,
	PADL3       = 0x0200,
	PADR3       = 0x0400,
	PADSTART    = 0x0800,
	PADUP       = 0x1000,
	PADRIGHT    = 0x2000,
	PADDOWN     = 0x4000,
	PADLEFT     = 0x8000,
};

struct PadState
{
	// -1..1, down/right positive, deadzone already applied
	float leftx, lefty;
	float rightx, righty;
	uint32 buttons;
	bool32 connected;
};

// Input state, tracked by EventHandler so apps don't have to.
extern bool32 keys[KEY_NUMKEYS];
extern MouseState mouse;	// live
extern MouseState prevmouse;	// as of the previous frame
extern PadState pad;

// File paths.
// On PS2 a CDROM build goes through cdrom0:, otherwise paths are
// prefixed with hostPrefix ("host:" for pcsx2, "host0:" for the SCE
// TOOL) and relative paths get a "./" so the host fs resolves them.
// On PC the path is passed through unchanged.
#define SK_MAXPATH 256
char *GetFilePath(char *dst, const char *path);
#ifdef RW_PS2
extern const char *hostPrefix;
#endif

bool InitRW(void);
void TerminateRW(void);
rw::Camera *CameraCreate(int32 width, int32 height, bool32 z);
void CameraDestroy(rw::Camera *cam);
void CameraSize(rw::Camera *cam, Rect *r, float viewWindow = 0.0f, float aspectRatio = 0.0f);
void CameraMove(rw::Camera *cam, V3d *delta);
void CameraPan(rw::Camera *cam, V3d *pos, float angle);
void CameraTilt(rw::Camera *cam, V3d *pos, float angle);
void CameraRotate(rw::Camera *cam, V3d *pos, float angle);
void SetMousePosition(int x, int y);
EventStatus EventHandler(Event e, void *param);

}

#include "camera.h"

sk::EventStatus AppEventHandler(sk::Event e, void *param);

#ifndef RW_PS2
#include "imgui/imgui.h"
#include "imgui/imgui_impl_rw.h"
#endif
