/* xsdl: the part of Xlib that BoboBot calls, on acsdl12 (SDL 1.2, 8-bit).
 *
 * BoboBot draws with Xlib and uses SDL only for its sound. On the Amiga
 * its one window is acsdl12's 8-bit screen, and every pixmap is a block
 * of bytes in our own memory: one colour index a pixel, or 0 and 1 for
 * the 1-bit masks. Colours come from one shared 256-entry map: black and
 * white first, then each colour the game asks for, exactly while there
 * is room (amiga/convert_gfx.py keeps the pictures to 240), the nearest
 * one after. GCs are GXcopy only (the game uses no other function) with
 * a line width and a clip mask at a clip origin, as the sprites and the
 * dark rooms need. The font is X's own "fixed", 6x13.
 *
 * What is drawn in the window shows when the game next looks for events
 * (XPending), syncs or flushes, or waits out its frame (xsdl_usleep), or
 * writes text straight to it (the loading screen).
 * Keys come as KeyPress and KeyRelease with X keysyms; the joystick (or
 * CD32 pad) as the cursor keys, its fire button as Z (fire) and Return
 * (start, on the title and menus), its second button as Space (jump),
 * any other as Tab (weapons); the Help key is Tab too.
 *
 * The AmiCradle Team. GPL 2, like BoboBot.
 */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <SDL.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "font6x13.h"
#define XSDL_NO_TIME_MACROS
#include "xsdl_time.h"

/* ------------------------------------------------------------ drawables */

#define MAX_DRAWABLES 1024
#define ROOT_ID 1
#define WINDOW_ID 2

typedef struct {
    int w, h, pitch, depth;
    unsigned char *pix;
} Draw;

static Draw g_draw[MAX_DRAWABLES];
static int g_ndraw = WINDOW_ID + 1;

static Display g_display;
static Visual g_visual;
static SDL_Surface *g_screen;
static SDL_Joystick *g_joy;
static int g_dirty;      /* the window has been drawn in since it last showed */

static SDL_Color g_pal[256];
static int g_ncolours;
static int g_pal_dirty;

struct _XGC {
    unsigned long fg, bg;
    int function, line_width;
    Pixmap clip_mask;
    int clip_x, clip_y;
};

static Draw *drawable(Drawable d)
{
    if (d < ROOT_ID + 1 || d >= (Drawable)g_ndraw || !g_draw[d].pix)
        return NULL;
    if (d == WINDOW_ID)
        g_dirty = 1;
    return &g_draw[d];
}

static unsigned char ink(Draw *dr, unsigned long pixel)
{
    return dr->depth == 1 ? (unsigned char)(pixel & 1) : (unsigned char)pixel;
}

static void present(void)
{
    if (!g_screen)
        return;
    if (g_pal_dirty) {
        SDL_SetColors(g_screen, g_pal, 0, 256);
        g_pal_dirty = 0;
        g_dirty = 1;
    }
    if (g_dirty) {
        SDL_Flip(g_screen);
        g_dirty = 0;
    }
}

/* ------------------------------------------------------------- display */

Display *XOpenDisplay(const char *name)
{
    (void)name;
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
        return NULL;
    atexit(SDL_Quit);
    if (SDL_NumJoysticks() > 0)
        g_joy = SDL_JoystickOpen(0);

    g_visual.class = PseudoColor;
    g_visual.bits_per_rgb = 8;
    g_visual.map_entries = 256;
    g_display.default_screen = 0;
    g_display.screen.depth = 8;
    g_display.screen.root_visual = &g_visual;
    g_display.screen.cmap = 1;
    g_display.screen.black_pixel = 0;
    g_display.screen.white_pixel = 1;
    g_display.screen.root = ROOT_ID;

    memset(g_pal, 0, sizeof(g_pal));
    g_pal[1].r = g_pal[1].g = g_pal[1].b = 255;
    g_ncolours = 2;
    g_pal_dirty = 1;
    return &g_display;
}

Window XCreateWindow(Display *d, Window parent, int x, int y, unsigned int w,
                     unsigned int h, unsigned int border, int depth, unsigned int cls,
                     Visual *v, unsigned long mask, XSetWindowAttributes *a)
{
    (void)d; (void)parent; (void)x; (void)y; (void)border; (void)depth;
    (void)cls; (void)v; (void)mask; (void)a;
    if (g_screen)
        return None;   /* BoboBot opens one window */
    g_screen = SDL_SetVideoMode((int)w, (int)h, 8, SDL_SWSURFACE | SDL_FULLSCREEN);
    if (!g_screen) {
        fprintf(stderr, "%s\n", SDL_GetError());
        return None;
    }
    SDL_ShowCursor(SDL_DISABLE);
    SDL_WM_SetCaption("BoboBot", "BoboBot");
    g_draw[WINDOW_ID].w = g_screen->w;
    g_draw[WINDOW_ID].h = g_screen->h;
    g_draw[WINDOW_ID].pitch = g_screen->pitch;
    g_draw[WINDOW_ID].depth = 8;
    g_draw[WINDOW_ID].pix = (unsigned char *)g_screen->pixels;
    SDL_FillRect(g_screen, NULL, 0);
    g_pal_dirty = 1;
    return WINDOW_ID;
}

int XMapWindow(Display *d, Window w) { (void)d; (void)w; return 1; }
int XMapRaised(Display *d, Window w) { (void)d; (void)w; return 1; }
int XStoreName(Display *d, Window w, const char *name)
{
    (void)d; (void)w;
    SDL_WM_SetCaption(name, name);
    return 1;
}
int XSetClassHint(Display *d, Window w, XClassHint *h) { (void)d; (void)w; (void)h; return 1; }
int XSetWMHints(Display *d, Window w, XWMHints *h) { (void)d; (void)w; (void)h; return 1; }
void XSetWMNormalHints(Display *d, Window w, XSizeHints *h) { (void)d; (void)w; (void)h; }
int XSetNormalHints(Display *d, Window w, XSizeHints *h) { (void)d; (void)w; (void)h; return 1; }
int XFree(void *p) { free(p); return 1; }

XVisualInfo *XGetVisualInfo(Display *d, long mask, XVisualInfo *tmpl, int *n)
{
    XVisualInfo *vi = (XVisualInfo *)calloc(1, sizeof(*vi));

    (void)mask; (void)tmpl;
    if (!vi) {
        *n = 0;
        return NULL;
    }
    vi->visual = d->screen.root_visual;
    vi->depth = 8;
    vi->class = PseudoColor;
    *n = 1;
    return vi;
}

Pixmap XCreatePixmap(Display *d, Drawable dr, unsigned int w, unsigned int h,
                     unsigned int depth)
{
    Draw *p;

    (void)d; (void)dr;
    if (g_ndraw >= MAX_DRAWABLES) {
        fprintf(stderr, "xsdl: too many pixmaps\n");
        exit(1);
    }
    p = &g_draw[g_ndraw];
    p->w = (int)w;
    p->h = (int)h;
    p->pitch = (int)w;
    p->depth = depth == 1 ? 1 : 8;
    p->pix = (unsigned char *)calloc((w && h) ? (size_t)w * h : 1, 1);
    if (!p->pix) {
        fprintf(stderr, "xsdl: out of memory for a %ux%u pixmap\n", w, h);
        exit(1);
    }
    return (Pixmap)g_ndraw++;
}

/* ------------------------------------------------------------------ GCs */

static void gc_values(GC gc, unsigned long mask, XGCValues *v)
{
    if (!v)
        return;
    if (mask & GCFunction)
        gc->function = v->function;
    if (mask & GCForeground)
        gc->fg = v->foreground;
    if (mask & GCBackground)
        gc->bg = v->background;
    if (mask & GCLineWidth)
        gc->line_width = v->line_width;
}

GC XCreateGC(Display *d, Drawable dr, unsigned long mask, XGCValues *v)
{
    GC gc = (GC)calloc(1, sizeof(*gc));

    (void)d; (void)dr;
    if (!gc) {
        fprintf(stderr, "xsdl: out of memory for a GC\n");
        exit(1);
    }
    gc->fg = 0;
    gc->bg = 1;
    gc->function = GXcopy;
    gc_values(gc, mask, v);
    return gc;
}

int XChangeGC(Display *d, GC gc, unsigned long mask, XGCValues *v)
{
    (void)d;
    gc_values(gc, mask, v);
    return 1;
}

int XFreeGC(Display *d, GC gc) { (void)d; free(gc); return 1; }
int XSetClipMask(Display *d, GC gc, Pixmap mask) { (void)d; gc->clip_mask = mask; return 1; }
int XSetClipOrigin(Display *d, GC gc, int x, int y)
{
    (void)d;
    gc->clip_x = x;
    gc->clip_y = y;
    return 1;
}

/* Is (x, y) of a drawable inside gc's clip mask? */
static int clipped_in(GC gc, int x, int y)
{
    Draw *m;
    int mx, my;

    if (!gc->clip_mask)
        return 1;
    m = &g_draw[gc->clip_mask];
    mx = x - gc->clip_x;
    my = y - gc->clip_y;
    return mx >= 0 && my >= 0 && mx < m->w && my < m->h && m->pix[my * m->pitch + mx];
}

/* -------------------------------------------------------------- colours */

Colormap XCreateColormap(Display *d, Window w, Visual *v, int alloc)
{
    (void)w; (void)v; (void)alloc;
    return d->screen.cmap;
}

int XSetWindowColormap(Display *d, Window w, Colormap c) { (void)d; (void)w; (void)c; return 1; }

Status XAllocColor(Display *d, Colormap c, XColor *col)
{
    int r = col->red >> 8, g = col->green >> 8, b = col->blue >> 8;
    int i, best = 0;
    long bestd = 0x7fffffffL;

    (void)d; (void)c;
    for (i = 0; i < g_ncolours; i++)
        if (g_pal[i].r == r && g_pal[i].g == g && g_pal[i].b == b)
            break;
    if (i == g_ncolours) {
        if (g_ncolours < 256) {
            g_pal[i].r = (Uint8)r;
            g_pal[i].g = (Uint8)g;
            g_pal[i].b = (Uint8)b;
            g_ncolours++;
            g_pal_dirty = 1;
        } else {
            for (i = 0; i < 256; i++) {
                long dr = g_pal[i].r - r, dg = g_pal[i].g - g, db = g_pal[i].b - b;
                long dd = 2 * dr * dr + 4 * dg * dg + 3 * db * db;
                if (dd < bestd) {
                    bestd = dd;
                    best = i;
                }
            }
            i = best;
        }
    }
    col->pixel = (unsigned long)i;
    col->red = (unsigned short)(g_pal[i].r * 257);
    col->green = (unsigned short)(g_pal[i].g * 257);
    col->blue = (unsigned short)(g_pal[i].b * 257);
    return 1;
}

/* The colours BoboBot asks for by name, from X's rgb.txt. */
static const struct { const char *name; unsigned char r, g, b; } g_named[] = {
    { "black", 0, 0, 0 },          { "white", 255, 255, 255 },
    { "dark green", 0, 100, 0 },   { "green3", 0, 205, 0 },
    { "green", 0, 255, 0 },        { "dark red", 139, 0, 0 },
    { "red3", 205, 0, 0 },         { "red", 255, 0, 0 },
    { "yellow4", 139, 139, 0 },    { "yellow3", 205, 205, 0 },
    { "yellow", 255, 255, 0 },
};

Status XAllocNamedColor(Display *d, Colormap c, const char *name, XColor *hw,
                        XColor *exact)
{
    unsigned i;

    for (i = 0; i < sizeof(g_named) / sizeof(g_named[0]); i++) {
        const char *a = g_named[i].name, *b = name;
        while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b))
            a++, b++;
        if (!*a && !*b) {
            hw->red = (unsigned short)(g_named[i].r * 257);
            hw->green = (unsigned short)(g_named[i].g * 257);
            hw->blue = (unsigned short)(g_named[i].b * 257);
            hw->flags = DoRed | DoGreen | DoBlue;
            if (exact)
                *exact = *hw;
            return XAllocColor(d, c, hw);
        }
    }
    return 0;
}

/* -------------------------------------------------------------- drawing */

static void fill(Draw *dr, int x, int y, int w, int h, unsigned char c)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > dr->w) w = dr->w - x;
    if (y + h > dr->h) h = dr->h - y;
    if (w <= 0 || h <= 0)
        return;
    for (; h > 0; h--, y++)
        memset(dr->pix + y * dr->pitch + x, c, (size_t)w);
}

static void plot(Draw *dr, GC gc, int x, int y)
{
    if (x >= 0 && y >= 0 && x < dr->w && y < dr->h && clipped_in(gc, x, y))
        dr->pix[y * dr->pitch + x] = ink(dr, gc->fg);
}

/* A point of a line or arc: a pixel, or a square of the line's width. */
static void pen(Draw *dr, GC gc, int x, int y)
{
    int lw = gc->line_width, i, j;

    if (lw <= 1) {
        plot(dr, gc, x, y);
        return;
    }
    for (j = 0; j < lw; j++)
        for (i = 0; i < lw; i++)
            plot(dr, gc, x - lw / 2 + i, y - lw / 2 + j);
}

int XFillRectangle(Display *d, Drawable drw, GC gc, int x, int y, unsigned int w,
                   unsigned int h)
{
    Draw *dr = drawable(drw);
    int i, j;

    (void)d;
    if (!dr || (int)w <= 0 || (int)h <= 0)
        return 0;
    if (!gc->clip_mask) {
        fill(dr, x, y, (int)w, (int)h, ink(dr, gc->fg));
        return 1;
    }
    for (j = 0; j < (int)h; j++)
        for (i = 0; i < (int)w; i++)
            plot(dr, gc, x + i, y + j);
    return 1;
}

int XDrawPoint(Display *d, Drawable drw, GC gc, int x, int y)
{
    Draw *dr = drawable(drw);

    (void)d;
    if (dr)
        plot(dr, gc, x, y);
    return 1;
}

static void line(Draw *dr, GC gc, int x0, int y0, int x1, int y1)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        pen(dr, gc, x0, y0);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

int XDrawLine(Display *d, Drawable drw, GC gc, int x1, int y1, int x2, int y2)
{
    Draw *dr = drawable(drw);

    (void)d;
    if (dr)
        line(dr, gc, x1, y1, x2, y2);
    return 1;
}

int XDrawRectangle(Display *d, Drawable drw, GC gc, int x, int y, unsigned int w,
                   unsigned int h)
{
    Draw *dr = drawable(drw);
    int iw = (int)w, ih = (int)h;

    (void)d;
    if (!dr || iw < 0 || ih < 0)
        return 0;
    line(dr, gc, x, y, x + iw, y);
    line(dr, gc, x + iw, y, x + iw, y + ih);
    line(dr, gc, x + iw, y + ih, x, y + ih);
    line(dr, gc, x, y + ih, x, y);
    return 1;
}

/* Angles in 64ths of a degree, anticlockwise from three o'clock. */
int XDrawArc(Display *d, Drawable drw, GC gc, int x, int y, unsigned int w,
             unsigned int h, int a1, int a2)
{
    Draw *dr = drawable(drw);
    double cx = x + w / 2.0, cy = y + h / 2.0, rx = w / 2.0, ry = h / 2.0;
    double t0 = a1 / 64.0 * M_PI / 180.0, span = a2 / 64.0 * M_PI / 180.0;
    int steps, i, lx = 0, ly = 0;

    (void)d;
    if (!dr)
        return 0;
    steps = (int)(fabs(span) * (rx > ry ? rx : ry)) + 2;
    for (i = 0; i <= steps; i++) {
        double t = t0 + span * i / steps;
        int px = (int)floor(cx + rx * cos(t) + 0.5);
        int py = (int)floor(cy - ry * sin(t) + 0.5);
        if (i > 0)
            line(dr, gc, lx, ly, px, py);
        lx = px;
        ly = py;
    }
    return 1;
}

/* Even-odd fill of the pixels whose centres are inside. */
int XFillPolygon(Display *d, Drawable drw, GC gc, XPoint *p, int n, int shape,
                 int mode)
{
    Draw *dr = drawable(drw);
    int miny, maxy, y, i, j, k, nx;
    double xs[32];

    (void)d; (void)shape; (void)mode;
    if (!dr || n < 3 || n > 32)
        return 0;
    miny = maxy = p[0].y;
    for (i = 1; i < n; i++) {
        if (p[i].y < miny) miny = p[i].y;
        if (p[i].y > maxy) maxy = p[i].y;
    }
    if (miny < 0) miny = 0;
    if (maxy > dr->h - 1) maxy = dr->h - 1;
    for (y = miny; y <= maxy; y++) {
        double sy = y + 0.5;
        nx = 0;
        for (i = 0, j = n - 1; i < n; j = i++) {
            double ya = p[j].y, yb = p[i].y;
            if ((ya <= sy && yb > sy) || (yb <= sy && ya > sy))
                xs[nx++] = p[j].x + (sy - ya) * (p[i].x - p[j].x) / (yb - ya);
        }
        for (i = 1; i < nx; i++)          /* a handful: insertion sort */
            for (k = i; k > 0 && xs[k - 1] > xs[k]; k--) {
                double t = xs[k]; xs[k] = xs[k - 1]; xs[k - 1] = t;
            }
        for (i = 0; i + 1 < nx; i += 2) {
            int xa = (int)ceil(xs[i] - 0.5), xb = (int)ceil(xs[i + 1] - 0.5);
            if (!gc->clip_mask)
                fill(dr, xa, y, xb - xa, 1, ink(dr, gc->fg));
            else
                for (k = xa; k < xb; k++)
                    plot(dr, gc, k, y);
        }
    }
    return 1;
}

int XCopyArea(Display *d, Drawable src, Drawable dst, GC gc, int sx, int sy,
              unsigned int uw, unsigned int uh, int dx, int dy)
{
    Draw *s = drawable(src), *t = drawable(dst);
    int w = (int)uw, h = (int)uh, j;

    (void)d;
    if (!s || !t || w <= 0 || h <= 0)
        return 0;
    /* inside the source, then inside the destination */
    if (sx < 0) { w += sx; dx -= sx; sx = 0; }
    if (sy < 0) { h += sy; dy -= sy; sy = 0; }
    if (sx + w > s->w) w = s->w - sx;
    if (sy + h > s->h) h = s->h - sy;
    if (dx < 0) { w += dx; sx -= dx; dx = 0; }
    if (dy < 0) { h += dy; sy -= dy; dy = 0; }
    if (dx + w > t->w) w = t->w - dx;
    if (dy + h > t->h) h = t->h - dy;
    if (w <= 0 || h <= 0)
        return 1;

    if (!gc->clip_mask) {
        if (s == t && sy < dy)
            for (j = h - 1; j >= 0; j--)
                memmove(t->pix + (dy + j) * t->pitch + dx,
                        s->pix + (sy + j) * s->pitch + sx, (size_t)w);
        else
            for (j = 0; j < h; j++)
                memmove(t->pix + (dy + j) * t->pitch + dx,
                        s->pix + (sy + j) * s->pitch + sx, (size_t)w);
        return 1;
    }

    {
        Draw *m = &g_draw[gc->clip_mask];
        int i;

        for (j = 0; j < h; j++) {
            int my = dy + j - gc->clip_y;
            const unsigned char *mrow, *srow;
            unsigned char *trow;

            if (my < 0 || my >= m->h)
                continue;
            mrow = m->pix + my * m->pitch;
            srow = s->pix + (sy + j) * s->pitch + sx;
            trow = t->pix + (dy + j) * t->pitch + dx;
            for (i = 0; i < w; i++) {
                int mx = dx + i - gc->clip_x;
                if (mx >= 0 && mx < m->w && mrow[mx])
                    trow[i] = srow[i];
            }
        }
    }
    return 1;
}

/* ----------------------------------------------------------------- text */

static XFontStruct g_font = { XSDL_FONT_ASCENT, XSDL_FONT_DESCENT, XSDL_FONT_W, XSDL_FONT_W };

XFontStruct *XLoadQueryFont(Display *d, const char *name)
{
    (void)d; (void)name;
    return &g_font;
}

int XTextWidth(XFontStruct *f, const char *s, int n)
{
    (void)f; (void)s;
    return n * XSDL_FONT_W;
}

int XDrawImageString(Display *d, Drawable drw, GC gc, int x, int y, const char *s,
                     int n)
{
    Draw *dr = drawable(drw);
    int c, row, col;
    unsigned char fg, bg;

    (void)d;
    if (!dr || n <= 0)
        return 0;
    fg = ink(dr, gc->fg);
    bg = ink(dr, gc->bg);
    y -= XSDL_FONT_ASCENT;
    fill(dr, x, y, n * XSDL_FONT_W, XSDL_FONT_H, bg);
    for (c = 0; c < n; c++, x += XSDL_FONT_W) {
        int ch = (unsigned char)s[c];
        const unsigned char *g;

        if (ch < 32 || ch > 126)
            ch = '?';
        g = xsdl_font[ch - 32];
        for (row = 0; row < XSDL_FONT_H; row++) {
            int py = y + row;
            if (py < 0 || py >= dr->h || !g[row])
                continue;
            for (col = 0; col < XSDL_FONT_W; col++) {
                int px = x + col;
                if ((g[row] & (0x80 >> col)) && px >= 0 && px < dr->w)
                    dr->pix[py * dr->pitch + px] = fg;
            }
        }
    }
    /* Only "Loading..." is written straight to the window, while the
       pictures load and no events are looked for: show it (with the
       bar as it stands) now. */
    if (drw == WINDOW_ID)
        present();
    return 1;
}

/* ------------------------------------------------------------- bitmaps */

/* An XBM: "#define NAME_width W", "#define NAME_height H", then the bytes,
 * each row a whole number of bytes, the leftmost pixel in bit 0. */
int XReadBitmapFile(Display *d, Drawable drw, const char *file, int *w,
                    int *h, Pixmap *bitmap, int *xhot, int *yhot)
{
    FILE *f = fopen(file, "r");
    char *text, *p;
    long size, bw = -1, bh = -1;
    int x, y, rowbytes;
    Pixmap pm;
    Draw *dr;

    (void)drw;
    if (!f)
        return BitmapOpenFailed;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0 || !(text = (char *)malloc((size_t)size + 1))) {
        fclose(f);
        return BitmapNoMemory;
    }
    size = (long)fread(text, 1, (size_t)size, f);
    fclose(f);
    text[size] = '\0';

    if ((p = strstr(text, "_width")))
        bw = strtol(p + 6, NULL, 10);
    if ((p = strstr(text, "_height")))
        bh = strtol(p + 7, NULL, 10);
    p = strchr(text, '{');
    if (bw <= 0 || bh <= 0 || bw > 4096 || bh > 4096 || !p) {
        free(text);
        return BitmapFileInvalid;
    }
    pm = XCreatePixmap(d, drw, (unsigned)bw, (unsigned)bh, 1);
    dr = &g_draw[pm];
    rowbytes = (int)((bw + 7) / 8);
    p++;
    for (y = 0; y < bh; y++)
        for (x = 0; x < rowbytes; x++) {
            char *end;
            long v;
            int bit;

            while (*p && *p != '0' && *p != '}')
                p++;
            if (*p != '0') {
                free(text);
                return BitmapFileInvalid;
            }
            v = strtol(p, &end, 0);
            p = end;
            for (bit = 0; bit < 8 && x * 8 + bit < bw; bit++)
                dr->pix[y * dr->pitch + x * 8 + bit] = (unsigned char)((v >> bit) & 1);
        }
    free(text);
    *w = (int)bw;
    *h = (int)bh;
    *bitmap = pm;
    if (xhot) *xhot = -1;
    if (yhot) *yhot = -1;
    return BitmapSuccess;
}

/* --------------------------------------------------------------- events */

#define QUEUE 64
static XEvent g_queue[QUEUE];
static int g_qhead, g_qcount;
static int g_stick[4];     /* left, right, up, down held on the joystick */

static void post(int type, KeySym sym)
{
    XEvent *e;

    if (g_qcount == QUEUE)
        return;
    e = &g_queue[(g_qhead + g_qcount++) % QUEUE];
    memset(e, 0, sizeof(*e));
    e->type = type;
    e->xkey.type = type;
    e->xkey.display = &g_display;
    e->xkey.window = WINDOW_ID;
    e->xkey.xsdl_keysym = sym;
}

static KeySym keysym(SDLKey k)
{
    if (k >= SDLK_a && k <= SDLK_z)
        return XK_a + (k - SDLK_a);
    if (k >= SDLK_0 && k <= SDLK_9)
        return XK_0 + (k - SDLK_0);
    if (k >= SDLK_KP0 && k <= SDLK_KP9)
        return XK_KP_0 + (k - SDLK_KP0);
    switch (k) {
    case SDLK_SPACE: return XK_space;
    case SDLK_RETURN: return XK_Return;
    case SDLK_KP_ENTER: return XK_KP_Enter;
    case SDLK_TAB: return XK_Tab;
    case SDLK_HELP: return XK_Tab;
    case SDLK_ESCAPE: return XK_Escape;
    case SDLK_LEFT: return XK_Left;
    case SDLK_RIGHT: return XK_Right;
    case SDLK_UP: return XK_Up;
    case SDLK_DOWN: return XK_Down;
    default: return 0;
    }
}

static void stick(int which, int held)
{
    static const KeySym syms[4] = { XK_Left, XK_Right, XK_Up, XK_Down };

    if (g_stick[which] != held) {
        g_stick[which] = held;
        post(held ? KeyPress : KeyRelease, syms[which]);
    }
}

static void pump(void)
{
    SDL_Event ev;
    KeySym k;

    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            if ((k = keysym(ev.key.keysym.sym)) != 0)
                post(ev.type == SDL_KEYDOWN ? KeyPress : KeyRelease, k);
            break;
        case SDL_JOYAXISMOTION:
            if (ev.jaxis.axis == 0) {
                stick(0, ev.jaxis.value < -16000);
                stick(1, ev.jaxis.value > 16000);
            } else if (ev.jaxis.axis == 1) {
                stick(2, ev.jaxis.value < -16000);
                stick(3, ev.jaxis.value > 16000);
            }
            break;
        case SDL_JOYHATMOTION:
            if (ev.jhat.hat == 0) {
                stick(0, (ev.jhat.value & SDL_HAT_LEFT) != 0);
                stick(1, (ev.jhat.value & SDL_HAT_RIGHT) != 0);
                stick(2, (ev.jhat.value & SDL_HAT_UP) != 0);
                stick(3, (ev.jhat.value & SDL_HAT_DOWN) != 0);
            }
            break;
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP: {
            int type = ev.type == SDL_JOYBUTTONDOWN ? KeyPress : KeyRelease;
            if (ev.jbutton.button == 0) {
                post(type, XK_z);
                if (type == KeyPress) {        /* start, on the title */
                    post(KeyPress, XK_Return);
                    post(KeyRelease, XK_Return);
                }
            } else if (ev.jbutton.button == 1)
                post(type, XK_space);
            else
                post(type, XK_Tab);
            break;
        }
        case SDL_QUIT:
            post(KeyPress, XK_q);
            post(KeyRelease, XK_q);
            break;
        default:
            break;
        }
    }
}

int XPending(Display *d)
{
    (void)d;
    present();
    pump();
    return g_qcount;
}

int XNextEvent(Display *d, XEvent *e)
{
    (void)d;
    while (!g_qcount) {
        present();
        pump();
        if (!g_qcount)
            SDL_Delay(10);
    }
    *e = g_queue[g_qhead];
    g_qhead = (g_qhead + 1) % QUEUE;
    g_qcount--;
    return 0;
}

KeySym XLookupKeysym(XKeyEvent *e, int index)
{
    (void)index;
    return e->xsdl_keysym;
}

int XSync(Display *d, Bool discard)
{
    (void)d; (void)discard;
    present();
    return 1;
}

int XFlush(Display *d)
{
    (void)d;
    present();
    return 1;
}

/* --------------------------------------------------------------- timing */

/* BoboBot paces its frames with gettimeofday and usleep. libnix's clock
 * moves in 20 ms steps, too coarse for a 22.5 ms frame: these count
 * acsdl12's milliseconds and sleep on timer.device. The frame is shown
 * before the wait, and the time that takes comes out of the wait. */
int xsdl_gettimeofday(struct timeval *tv, void *tz)
{
    Uint32 t = SDL_GetTicks();

    (void)tz;
    tv->tv_sec = (long)(t / 1000);
    tv->tv_usec = (long)(t % 1000) * 1000;
    return 0;
}

int xsdl_usleep(unsigned long us)
{
    Uint32 t0 = SDL_GetTicks(), ms = (Uint32)((us + 500) / 1000), spent;

    present();
    spent = SDL_GetTicks() - t0;
    if (ms > spent)
        SDL_Delay(ms - spent);
    return 0;
}
