/* xsdl: the part of Xutil.h BoboBot's helpers use (see Xlib.h).
 * The AmiCradle Team. GPL 2, like BoboBot. */
#ifndef XSDL_XUTIL_H
#define XSDL_XUTIL_H

#include <X11/Xlib.h>

#define USPosition (1L << 0)
#define USSize (1L << 1)
#define PMinSize (1L << 4)
#define PBaseSize (1L << 8)

#define InputHint (1L << 0)
#define StateHint (1L << 1)

#define WithdrawnState 0
#define NormalState 1
#define IconicState 3

#define VisualScreenMask 0x2
#define VisualClassMask 0x8

#define BitmapSuccess 0
#define BitmapOpenFailed 1
#define BitmapFileInvalid 2
#define BitmapNoMemory 3

typedef struct {
    long flags;
    int x, y, width, height;
    int min_width, min_height;
    int base_width, base_height;
} XSizeHints;

typedef struct {
    long flags;
    Bool input;
    int initial_state;
} XWMHints;

typedef struct {
    char *res_name;
    char *res_class;
} XClassHint;

typedef struct {
    Visual *visual;
    int screen;
    unsigned int depth;
    int class;
} XVisualInfo;

int XSetClassHint(Display *d, Window w, XClassHint *h);
int XSetWMHints(Display *d, Window w, XWMHints *h);
void XSetWMNormalHints(Display *d, Window w, XSizeHints *h);
int XSetNormalHints(Display *d, Window w, XSizeHints *h);
XVisualInfo *XGetVisualInfo(Display *d, long mask, XVisualInfo *tmpl, int *n);
KeySym XLookupKeysym(XKeyEvent *e, int index);
/* X has unsigned int *w and *h; BoboBot passes ints, so ints here. */
int XReadBitmapFile(Display *d, Drawable dr, const char *file, int *w,
                    int *h, Pixmap *bitmap, int *xhot, int *yhot);

#endif
