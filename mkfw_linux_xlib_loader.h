// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

#pragma once

#include <dlfcn.h>

typedef XClassHint *(*PFN_XAllocClassHint)(void);
typedef XSizeHints *(*PFN_XAllocSizeHints)(void);
typedef int (*PFN_XChangeProperty)(Display *, Window, Atom, Atom, int, int, const unsigned char *, int);
typedef Bool (*PFN_XCheckTypedWindowEvent)(Display *, Window, int, XEvent *);
typedef int (*PFN_XCloseDisplay)(Display *);
typedef Status (*PFN_XCloseIM)(XIM);
typedef int (*PFN_XConvertSelection)(Display *, Atom, Atom, Atom, Window, Time);
typedef Colormap (*PFN_XCreateColormap)(Display *, Window, Visual *, int);
typedef Cursor (*PFN_XCreateFontCursor)(Display *, unsigned int);
typedef XIC (*PFN_XCreateIC)(XIM, ...);
typedef Pixmap (*PFN_XCreatePixmap)(Display *, Drawable, unsigned int, unsigned int, unsigned int);
typedef Cursor (*PFN_XCreatePixmapCursor)(Display *, Pixmap, Pixmap, XColor *, XColor *, unsigned int, unsigned int);
typedef Window (*PFN_XCreateWindow)(Display *, Window, int, int, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual *, unsigned long, XSetWindowAttributes *);
typedef int (*PFN_XDefineCursor)(Display *, Window, Cursor);
typedef int (*PFN_XDeleteProperty)(Display *, Window, Atom);
typedef void (*PFN_XDestroyIC)(XIC);
typedef int (*PFN_XDestroyWindow)(Display *, Window);
typedef int (*PFN_XFlush)(Display *);
typedef int (*PFN_XFree)(void *);
typedef int (*PFN_XFreeColormap)(Display *, Colormap);
typedef int (*PFN_XFreeCursor)(Display *, Cursor);
typedef void (*PFN_XFreeEventData)(Display *, XGenericEventCookie *);
typedef int (*PFN_XFreePixmap)(Display *, Pixmap);
typedef Bool (*PFN_XGetEventData)(Display *, XGenericEventCookie *);
typedef Status (*PFN_XGetGeometry)(Display *, Drawable, Window *, int *, int *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
typedef Window (*PFN_XGetSelectionOwner)(Display *, Atom);
typedef Status (*PFN_XGetWindowAttributes)(Display *, Window, XWindowAttributes *);
typedef int (*PFN_XGetWindowProperty)(Display *, Window, Atom, long, long, Bool, Atom, Atom *, int *, unsigned long *, unsigned long *, unsigned char **);
typedef int (*PFN_XGrabPointer)(Display *, Window, Bool, unsigned int, int, int, Window, Cursor, Time);
typedef Status (*PFN_XIconifyWindow)(Display *, Window, int);
typedef Status (*PFN_XInitThreads)(void);
typedef Atom (*PFN_XInternAtom)(Display *, const char *, Bool);
typedef KeySym (*PFN_XLookupKeysym)(XKeyEvent *, int);
typedef int (*PFN_XLookupString)(XKeyEvent *, char *, int, KeySym *, XComposeStatus *);
typedef int (*PFN_XMapWindow)(Display *, Window);
typedef int (*PFN_XMoveResizeWindow)(Display *, Window, int, int, unsigned int, unsigned int);
typedef int (*PFN_XMoveWindow)(Display *, Window, int, int);
typedef int (*PFN_XNextEvent)(Display *, XEvent *);
typedef Display *(*PFN_XOpenDisplay)(const char *);
typedef XIM (*PFN_XOpenIM)(Display *, XrmDatabase, char *, char *);
typedef int (*PFN_XPending)(Display *);
typedef Bool (*PFN_XQueryExtension)(Display *, const char *, int *, int *, int *);
typedef Bool (*PFN_XQueryPointer)(Display *, Window, Window *, Window *, int *, int *, int *, int *, unsigned int *);
typedef int (*PFN_XResizeWindow)(Display *, Window, unsigned int, unsigned int);
typedef char *(*PFN_XResourceManagerString)(Display *);
typedef Status (*PFN_XSendEvent)(Display *, Window, Bool, long, XEvent *);
typedef int (*PFN_XSetClassHint)(Display *, Window, XClassHint *);
typedef void (*PFN_XSetICFocus)(XIC);
typedef int (*PFN_XSetSelectionOwner)(Display *, Atom, Window, Time);
typedef void (*PFN_XSetWMNormalHints)(Display *, Window, XSizeHints *);
typedef Status (*PFN_XSetWMProtocols)(Display *, Window, Atom *, int);
typedef int (*PFN_XStoreName)(Display *, Window, const char *);
typedef int (*PFN_XSync)(Display *, Bool);
typedef Bool (*PFN_XTranslateCoordinates)(Display *, Window, Window, int, int, int *, int *, Window *);
typedef int (*PFN_XUndefineCursor)(Display *, Window);
typedef int (*PFN_XUngrabPointer)(Display *, Time);
typedef int (*PFN_XUnmapWindow)(Display *, Window);
typedef void (*PFN_XUnsetICFocus)(XIC);
typedef int (*PFN_XWarpPointer)(Display *, Window, Window, int, int, unsigned int, unsigned int, int, int);
typedef void (*PFN_XrmInitialize)(void);
typedef XrmDatabase (*PFN_XrmGetStringDatabase)(const char *);
typedef Bool (*PFN_XrmGetResource)(XrmDatabase, const char *, const char *, char **, XrmValue *);
typedef void (*PFN_XrmDestroyDatabase)(XrmDatabase);
typedef int (*PFN_Xutf8LookupString)(XIC, XKeyPressedEvent *, char *, int, KeySym *, Status *);

static PFN_XAllocClassHint mkfw_XAllocClassHint;
static PFN_XAllocSizeHints mkfw_XAllocSizeHints;
static PFN_XChangeProperty mkfw_XChangeProperty;
static PFN_XCheckTypedWindowEvent mkfw_XCheckTypedWindowEvent;
static PFN_XCloseDisplay mkfw_XCloseDisplay;
static PFN_XCloseIM mkfw_XCloseIM;
static PFN_XConvertSelection mkfw_XConvertSelection;
static PFN_XCreateColormap mkfw_XCreateColormap;
static PFN_XCreateFontCursor mkfw_XCreateFontCursor;
static PFN_XCreateIC mkfw_XCreateIC;
static PFN_XCreatePixmap mkfw_XCreatePixmap;
static PFN_XCreatePixmapCursor mkfw_XCreatePixmapCursor;
static PFN_XCreateWindow mkfw_XCreateWindow;
static PFN_XDefineCursor mkfw_XDefineCursor;
static PFN_XDeleteProperty mkfw_XDeleteProperty;
static PFN_XDestroyIC mkfw_XDestroyIC;
static PFN_XDestroyWindow mkfw_XDestroyWindow;
static PFN_XFlush mkfw_XFlush;
static PFN_XFree mkfw_XFree;
static PFN_XFreeColormap mkfw_XFreeColormap;
static PFN_XFreeCursor mkfw_XFreeCursor;
static PFN_XFreeEventData mkfw_XFreeEventData;
static PFN_XFreePixmap mkfw_XFreePixmap;
static PFN_XGetEventData mkfw_XGetEventData;
static PFN_XGetGeometry mkfw_XGetGeometry;
static PFN_XGetSelectionOwner mkfw_XGetSelectionOwner;
static PFN_XGetWindowAttributes mkfw_XGetWindowAttributes;
static PFN_XGetWindowProperty mkfw_XGetWindowProperty;
static PFN_XGrabPointer mkfw_XGrabPointer;
static PFN_XIconifyWindow mkfw_XIconifyWindow;
static PFN_XInitThreads mkfw_XInitThreads;
static PFN_XInternAtom mkfw_XInternAtom;
static PFN_XLookupKeysym mkfw_XLookupKeysym;
static PFN_XLookupString mkfw_XLookupString;
static PFN_XMapWindow mkfw_XMapWindow;
static PFN_XMoveResizeWindow mkfw_XMoveResizeWindow;
static PFN_XMoveWindow mkfw_XMoveWindow;
static PFN_XNextEvent mkfw_XNextEvent;
static PFN_XOpenDisplay mkfw_XOpenDisplay;
static PFN_XOpenIM mkfw_XOpenIM;
static PFN_XPending mkfw_XPending;
static PFN_XQueryExtension mkfw_XQueryExtension;
static PFN_XQueryPointer mkfw_XQueryPointer;
static PFN_XResizeWindow mkfw_XResizeWindow;
static PFN_XResourceManagerString mkfw_XResourceManagerString;
static PFN_XSendEvent mkfw_XSendEvent;
static PFN_XSetClassHint mkfw_XSetClassHint;
static PFN_XSetICFocus mkfw_XSetICFocus;
static PFN_XSetSelectionOwner mkfw_XSetSelectionOwner;
static PFN_XSetWMNormalHints mkfw_XSetWMNormalHints;
static PFN_XSetWMProtocols mkfw_XSetWMProtocols;
static PFN_XStoreName mkfw_XStoreName;
static PFN_XSync mkfw_XSync;
static PFN_XTranslateCoordinates mkfw_XTranslateCoordinates;
static PFN_XUndefineCursor mkfw_XUndefineCursor;
static PFN_XUngrabPointer mkfw_XUngrabPointer;
static PFN_XUnmapWindow mkfw_XUnmapWindow;
static PFN_XUnsetICFocus mkfw_XUnsetICFocus;
static PFN_XWarpPointer mkfw_XWarpPointer;
static PFN_XrmInitialize mkfw_XrmInitialize;
static PFN_XrmGetStringDatabase mkfw_XrmGetStringDatabase;
static PFN_XrmGetResource mkfw_XrmGetResource;
static PFN_XrmDestroyDatabase mkfw_XrmDestroyDatabase;
static PFN_Xutf8LookupString mkfw_Xutf8LookupString;

#define XAllocClassHint mkfw_XAllocClassHint
#define XAllocSizeHints mkfw_XAllocSizeHints
#define XChangeProperty mkfw_XChangeProperty
#define XCheckTypedWindowEvent mkfw_XCheckTypedWindowEvent
#define XCloseDisplay mkfw_XCloseDisplay
#define XCloseIM mkfw_XCloseIM
#define XConvertSelection mkfw_XConvertSelection
#define XCreateColormap mkfw_XCreateColormap
#define XCreateFontCursor mkfw_XCreateFontCursor
#define XCreateIC mkfw_XCreateIC
#define XCreatePixmap mkfw_XCreatePixmap
#define XCreatePixmapCursor mkfw_XCreatePixmapCursor
#define XCreateWindow mkfw_XCreateWindow
#define XDefineCursor mkfw_XDefineCursor
#define XDeleteProperty mkfw_XDeleteProperty
#define XDestroyIC mkfw_XDestroyIC
#define XDestroyWindow mkfw_XDestroyWindow
#define XFlush mkfw_XFlush
#define XFree mkfw_XFree
#define XFreeColormap mkfw_XFreeColormap
#define XFreeCursor mkfw_XFreeCursor
#define XFreeEventData mkfw_XFreeEventData
#define XFreePixmap mkfw_XFreePixmap
#define XGetEventData mkfw_XGetEventData
#define XGetGeometry mkfw_XGetGeometry
#define XGetSelectionOwner mkfw_XGetSelectionOwner
#define XGetWindowAttributes mkfw_XGetWindowAttributes
#define XGetWindowProperty mkfw_XGetWindowProperty
#define XGrabPointer mkfw_XGrabPointer
#define XIconifyWindow mkfw_XIconifyWindow
#define XInitThreads mkfw_XInitThreads
#define XInternAtom mkfw_XInternAtom
#define XLookupKeysym mkfw_XLookupKeysym
#define XLookupString mkfw_XLookupString
#define XMapWindow mkfw_XMapWindow
#define XMoveResizeWindow mkfw_XMoveResizeWindow
#define XMoveWindow mkfw_XMoveWindow
#define XNextEvent mkfw_XNextEvent
#define XOpenDisplay mkfw_XOpenDisplay
#define XOpenIM mkfw_XOpenIM
#define XPending mkfw_XPending
#define XQueryExtension mkfw_XQueryExtension
#define XQueryPointer mkfw_XQueryPointer
#define XResizeWindow mkfw_XResizeWindow
#define XResourceManagerString mkfw_XResourceManagerString
#define XSendEvent mkfw_XSendEvent
#define XSetClassHint mkfw_XSetClassHint
#define XSetICFocus mkfw_XSetICFocus
#define XSetSelectionOwner mkfw_XSetSelectionOwner
#define XSetWMNormalHints mkfw_XSetWMNormalHints
#define XSetWMProtocols mkfw_XSetWMProtocols
#define XStoreName mkfw_XStoreName
#define XSync mkfw_XSync
#define XTranslateCoordinates mkfw_XTranslateCoordinates
#define XUndefineCursor mkfw_XUndefineCursor
#define XUngrabPointer mkfw_XUngrabPointer
#define XUnmapWindow mkfw_XUnmapWindow
#define XUnsetICFocus mkfw_XUnsetICFocus
#define XWarpPointer mkfw_XWarpPointer
#define XrmInitialize mkfw_XrmInitialize
#define XrmGetStringDatabase mkfw_XrmGetStringDatabase
#define XrmGetResource mkfw_XrmGetResource
#define XrmDestroyDatabase mkfw_XrmDestroyDatabase
#define Xutf8LookupString mkfw_Xutf8LookupString

static void load_x11_functions(void) {
	static uint8_t loaded = 0;
	if(loaded) {
		return;
	}
	loaded = 1;

	void *lib = dlopen("libX11.so.6", RTLD_LAZY | RTLD_GLOBAL);
	if(!lib) {
		mkfw_error("failed to load libX11.so.6");
		exit(EXIT_FAILURE);
	}

	#define LOAD(name) *(void **)&mkfw_##name = dlsym(lib, #name)
	LOAD(XAllocClassHint);
	LOAD(XAllocSizeHints);
	LOAD(XChangeProperty);
	LOAD(XCheckTypedWindowEvent);
	LOAD(XCloseDisplay);
	LOAD(XCloseIM);
	LOAD(XConvertSelection);
	LOAD(XCreateColormap);
	LOAD(XCreateFontCursor);
	LOAD(XCreateIC);
	LOAD(XCreatePixmap);
	LOAD(XCreatePixmapCursor);
	LOAD(XCreateWindow);
	LOAD(XDefineCursor);
	LOAD(XDeleteProperty);
	LOAD(XDestroyIC);
	LOAD(XDestroyWindow);
	LOAD(XFlush);
	LOAD(XFree);
	LOAD(XFreeColormap);
	LOAD(XFreeCursor);
	LOAD(XFreeEventData);
	LOAD(XFreePixmap);
	LOAD(XGetEventData);
	LOAD(XGetGeometry);
	LOAD(XGetSelectionOwner);
	LOAD(XGetWindowAttributes);
	LOAD(XGetWindowProperty);
	LOAD(XGrabPointer);
	LOAD(XIconifyWindow);
	LOAD(XInitThreads);
	LOAD(XInternAtom);
	LOAD(XLookupKeysym);
	LOAD(XLookupString);
	LOAD(XMapWindow);
	LOAD(XMoveResizeWindow);
	LOAD(XMoveWindow);
	LOAD(XNextEvent);
	LOAD(XOpenDisplay);
	LOAD(XOpenIM);
	LOAD(XPending);
	LOAD(XQueryExtension);
	LOAD(XQueryPointer);
	LOAD(XResizeWindow);
	LOAD(XResourceManagerString);
	LOAD(XSendEvent);
	LOAD(XSetClassHint);
	LOAD(XSetICFocus);
	LOAD(XSetSelectionOwner);
	LOAD(XSetWMNormalHints);
	LOAD(XSetWMProtocols);
	LOAD(XStoreName);
	LOAD(XSync);
	LOAD(XTranslateCoordinates);
	LOAD(XUndefineCursor);
	LOAD(XUngrabPointer);
	LOAD(XUnmapWindow);
	LOAD(XUnsetICFocus);
	LOAD(XWarpPointer);
	LOAD(XrmInitialize);
	LOAD(XrmGetStringDatabase);
	LOAD(XrmGetResource);
	LOAD(XrmDestroyDatabase);
	LOAD(Xutf8LookupString);
	#undef LOAD

	if(!mkfw_XOpenDisplay || !mkfw_XCreateWindow || !mkfw_XMapWindow || !mkfw_XInternAtom) {
		mkfw_error("failed to load required X11 functions");
		exit(EXIT_FAILURE);
	}
}
