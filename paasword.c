#include <stdio.h>
#include <stdlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <math.h>

#include "config.h"

Display* display;
Drawable window;
int screen;
cairo_surface_t* surface;
cairo_t* cairo;
unsigned int width = 800, height = 600;
unsigned int winx = 0, winy = 0;

char* text;
float* circles;
unsigned int head, buffer;

typedef struct color_t {
	float r, g, b;
} color_t;

color_t bg_color, fg_color;

static inline color_t
parse_color(const char* name) {
	int r, g, b;
	
	sscanf(name, "#%2x%2x%2x", &r, &g, &b);
	
	return (color_t){(float)r/255, (float)g/255, (float)b/255};
}

void
xinit() {
	XColor border_color, bruh;
	unsigned int i;
	Atom stop_atom;
	XineramaScreenInfo * info;
	unsigned int n;
	int di;
	int curx, cury;
	union {Window w; int x;} junk;
	Window w;
	XWindowAttributes wa;
	XSetWindowAttributes swa = {
		.bit_gravity = NorthWestGravity,
		.override_redirect = True,
		.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask
	};

	if (!(display = XOpenDisplay(NULL))) {
		fprintf(stderr, "Unable to open display\n");
		exit(1);
	}
	screen = DefaultScreen(display);

	if (info = XineramaQueryScreens(display, &n)) {
		XGetInputFocus(display, &w, &di);
		XGetWindowAttributes(display, w, &wa);
		for (i = 0; i < n; i++)
			if (info[i].x_org <= wa.x + wa.width/2 && info[i].y_org <= wa.y + wa.height/2 && info[i].x_org + info[i].width >= wa.x + wa.width/2 && info[i].y_org + info[i].height >= wa.y + wa.height/2)
				break;
		if (i < n) {
			winx = info[i].x_org;
			winy = info[i].y_org;
			width = info[i].width;
			height = info[i].height;
		} else {
			XQueryPointer(display, DefaultRootWindow(display), (Window*)&junk, (Window*)&junk, &curx, &cury, (int*)&junk, (int*)&junk, (unsigned int*)&junk);
			for (i = 0; i < n; i++)
				if (info[i].x_org <= curx && info[i].y_org <= cury && info[i].x_org + info[i].width >= curx && info[i].y_org + info[i].height >= cury)
					break;
			winx = info[i].x_org;
			winy = info[i].y_org;
			width = info[i].width;
			height = info[i].height;
		}
	}


	window = XCreateWindow(display, DefaultRootWindow(display), winx, winy,
		width, height, 0, CopyFromParent, CopyFromParent, CopyFromParent,
		CWOverrideRedirect | CWEventMask | CWBitGravity, &swa);

	XMapRaised(display, window);

	while (XGrabKeyboard(display, DefaultRootWindow(display), True, GrabModeAsync, GrabModeAsync, CurrentTime));

	surface = cairo_xlib_surface_create(display, window, DefaultVisual(display, screen), width, height);
	cairo_xlib_surface_set_size(surface, width, height);
	cairo = cairo_create(surface);

	stop_atom = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, window, &stop_atom, 1);
}

void
redraw() {
	unsigned int i;
	float s;

	cairo_push_group(cairo);
		cairo_set_source_rgb(cairo, bg_color.r, bg_color.g, bg_color.b);
		cairo_paint(cairo);


		if (head) {
			s = sin(M_PI/head) * big_circle_size - 2;
			if (s > little_circle_size || head == 1)
				s = little_circle_size;

			circles[head-1] += ((float)0.5/(float)head - circles[head-1])*animation_speed;
			cairo_set_source_rgba(cairo, fg_color.r, fg_color.g, fg_color.b, circles[head-1]*(float)head*2.0);
			cairo_arc(cairo, width/2 + sin(M_PI*2*circles[head-1]) * big_circle_size, height/2 - cos(M_PI*2*circles[head-1]) * big_circle_size, s, 0, 2*M_PI);
			cairo_fill(cairo);

			cairo_set_source_rgba(cairo, fg_color.r, fg_color.g, fg_color.b, 1);
			for (i = 0; i < head - 1; i++) {
				circles[i] += ((float)(head-i-0.5)/(float)head - circles[i])*animation_speed;
				cairo_arc(cairo, width/2 + sin(M_PI*2*circles[i]) * big_circle_size, height/2 - cos(M_PI*2*circles[i]) * big_circle_size, s, 0, 2*M_PI);
				cairo_fill(cairo);
			}
		}
	cairo_pop_group_to_source(cairo);
	cairo_paint(cairo);
}

void
keypress(XKeyEvent e) {
	char buf[64];
	KeySym k;

	XLookupString(&e, buf, 64, &k, NULL);

	switch (k) {
		case XK_Escape:
			exit(0);
			break;
		case XK_Return:
			puts(text);
			exit(0);
			break;
		case XK_BackSpace:
			if (e.state & ControlMask)
				text[head = 0] = 0;
			else if (head)
				text[--head] = 0;
			break;
		default:
			if (*buf && !buf[1]) {
				if (head + 2 > buffer) {
					text = realloc(text, buffer = buffer<<1);
					circles = realloc(circles, buffer * sizeof(float));
				}
				text[head] = *buf;
				circles[head] = 0;
				text[++head] = 0;
			}
			break;
	}
	redraw();
}

int
main(int argc, char** argv) {
	XEvent e;

	xinit();

	text = malloc(buffer = 16);
	circles = malloc(buffer * sizeof(float));
	*text = 0;
	head = 0;

	bg_color = parse_color(color_bg);
	fg_color = parse_color(color_fg);

	for (;;) {
		while (XPending(display)) {
			XNextEvent(display, &e);
			if (XFilterEvent(&e, None)) continue;
			switch (e.type) {
				case ConfigureNotify:
					width = e.xconfigure.width;
					height = e.xconfigure.height;
					cairo_xlib_surface_set_size(surface, width, height);
					break;
				case Expose:
					redraw();
					break;
				case KeyPress:
					keypress(e.xkey);
					break;
			}
		}
		usleep(33333);
		redraw();
	}
}
