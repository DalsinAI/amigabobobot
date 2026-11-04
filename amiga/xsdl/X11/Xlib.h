/* xsdl: the part of Xlib that BoboBot calls, drawn with acsdl12 (SDL 1.2,
 * 8-bit) on AmigaOS. One window (the SDL screen), pixmaps of bytes, one
 * shared 256-colour map, GCs with a foreground, background, line width,
 * clip mask and clip origin (GXcopy only), the "fixed" font, and key
 * events. Not X: names and values are X11's so the game builds unchanged.
 * The AmiCradle Team. GPL 2, like BoboBot. */
#ifndef XSDL_XLIB_H
#define XSDL_XLIB_H

#include <stddef.h>
#include <string.h> /* text.c calls strlen with only Xlib.h included */

typedef unsigned long XID;
typedef XID Window;
typedef XID Pixmap;
typedef XID Drawable;
typedef XID Colormap;
typedef XID Font;
typedef XID Cursor;
typedef unsigned long KeySym;
typedef unsigned long Atom;
typedef int Bool;
typedef int Status;

#define True 1
#define False 0
#define None 0L
#define CopyFromParent 0L
#define InputOutput 1

#define KeyPress 2
#define KeyRelease 3
#define KeyPressMask (1L << 0)
#define KeyReleaseMask (1L << 1)
#define ExposureMask (1L << 15)

#define StaticGray 0
#define GrayScale 1
#define StaticColor 2
#define PseudoColor 3
#define TrueColor 4
#define DirectColor 5

#define AllocNone 0
#define DoRed (1 << 0)
#define DoGreen (1 << 1)
#define DoBlue (1 << 2)

#define GXclear 0x0
#define GXand 0x1
#define GXcopy 0x3
#define GXandInverted 0x4
#define GXxor 0x6
#define GXor 0x7

#define FillSolid 0

#define GCFunction (1L << 0)
#define GCForeground (1L << 2)
#define GCBackground (1L << 3)
#define GCLineWidth (1L << 4)
#define GCFillStyle (1L << 8)

#define CWBackPixel (1L << 1)
#define CWBorderPixel (1L << 3)
#define CWOverrideRedirect (1L << 9)
#define CWEventMask (1L << 11)

#define Complex 0
#define Nonconvex 1
#define Convex 2
#define CoordModeOrigin 0

typedef struct {
    int class;
    int bits_per_rgb;
    int map_entries;
} Visual;

typedef struct {
    int depth;
    Visual *root_visual;
    Colormap cmap;
    unsigned long black_pixel, white_pixel;
    Window root;
} Screen;

typedef struct _XDisplay {
    Screen screen;
    int default_screen;
} Display;

#define DefaultScreen(d) ((d)->default_screen)
#define ScreenOfDisplay(d, s) (&(d)->screen)
#define DefaultScreenOfDisplay(d) (&(d)->screen)
#define RootWindow(d, s) ((d)->screen.root)
#define DefaultVisual(d, s) ((d)->screen.root_visual)
#define DefaultDepth(d, s) ((d)->screen.depth)
#define DefaultColormap(d, s) ((d)->screen.cmap)
#define BlackPixel(d, s) ((d)->screen.black_pixel)
#define WhitePixel(d, s) ((d)->screen.white_pixel)
#define DefaultDepthOfScreen(s) ((s)->depth)

typedef struct {
    int function;
    unsigned long foreground, background;
    int line_width;
    int fill_style;
} XGCValues;

typedef struct _XGC *GC;

typedef struct {
    unsigned long pixel;
    unsigned short red, green, blue;
    char flags, pad;
} XColor;

typedef struct {
    short x, y;
} XPoint;

typedef struct {
    unsigned long background_pixel, border_pixel;
    long event_mask;
    Bool override_redirect;
} XSetWindowAttributes;

typedef struct {
    int ascent, descent;
    int min_char_width, max_char_width;
} XFontStruct;

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    unsigned int state;
    unsigned int keycode;
    KeySym xsdl_keysym; /* what XLookupKeysym gives */
} XKeyEvent;

typedef union _XEvent {
    int type;
    XKeyEvent xkey;
    long pad[24];
} XEvent;

Display *XOpenDisplay(const char *name);
Window XCreateWindow(Display *d, Window parent, int x, int y, unsigned int w,
                     unsigned int h, unsigned int border, int depth, unsigned int cls,
                     Visual *v, unsigned long mask, XSetWindowAttributes *a);
int XMapWindow(Display *d, Window w);
int XMapRaised(Display *d, Window w);
int XStoreName(Display *d, Window w, const char *name);
Pixmap XCreatePixmap(Display *d, Drawable dr, unsigned int w, unsigned int h,
                     unsigned int depth);
GC XCreateGC(Display *d, Drawable dr, unsigned long mask, XGCValues *v);
int XChangeGC(Display *d, GC gc, unsigned long mask, XGCValues *v);
int XFreeGC(Display *d, GC gc);
int XSetClipMask(Display *d, GC gc, Pixmap mask);
int XSetClipOrigin(Display *d, GC gc, int x, int y);
Colormap XCreateColormap(Display *d, Window w, Visual *v, int alloc);
int XSetWindowColormap(Display *d, Window w, Colormap c);
Status XAllocColor(Display *d, Colormap c, XColor *col);
Status XAllocNamedColor(Display *d, Colormap c, const char *name, XColor *hw,
                        XColor *exact);
int XFillRectangle(Display *d, Drawable dr, GC gc, int x, int y, unsigned int w,
                   unsigned int h);
int XDrawRectangle(Display *d, Drawable dr, GC gc, int x, int y, unsigned int w,
                   unsigned int h);
int XDrawPoint(Display *d, Drawable dr, GC gc, int x, int y);
int XDrawLine(Display *d, Drawable dr, GC gc, int x1, int y1, int x2, int y2);
int XDrawArc(Display *d, Drawable dr, GC gc, int x, int y, unsigned int w,
             unsigned int h, int a1, int a2);
int XFillPolygon(Display *d, Drawable dr, GC gc, XPoint *p, int n, int shape,
                 int mode);
int XCopyArea(Display *d, Drawable src, Drawable dst, GC gc, int sx, int sy,
              unsigned int w, unsigned int h, int dx, int dy);
XFontStruct *XLoadQueryFont(Display *d, const char *name);
int XTextWidth(XFontStruct *f, const char *s, int n);
int XDrawImageString(Display *d, Drawable dr, GC gc, int x, int y, const char *s,
                     int n);
int XPending(Display *d);
int XNextEvent(Display *d, XEvent *e);
int XSync(Display *d, Bool discard);
int XFlush(Display *d);
int XFree(void *p);

#endif
