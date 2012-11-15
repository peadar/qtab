#include <stdio.h>
#include "x11helper.h"
#include "X11/Xlib.h"
#include "X11/keysym.h"

X11Helper::X11Helper()
{
}

void
X11Helper::doit(Display *dpy, Window w)
{
    KeyCode keycode = XKeysymToKeycode(dpy, XK_F9);
    int rc;
    
    rc = XGrabKey(dpy, keycode, ShiftMask, w, False, GrabModeAsync, GrabModeAsync);
    printf("Grab: %d\n", rc);
    rc = XFlush(dpy);
    printf("Flush: %d\n", rc);
}
