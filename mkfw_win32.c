// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT

#include <windows.h>
#include <windowsx.h>
#include <wingdi.h>
#include <shellapi.h>
#include <stdlib.h>

__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

// WGL constants for context creation
#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x00000002

// WGL function pointer for creating modern contexts
typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int *attribList);
static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

/* Platform casting macros */
#define PLATFORM(state) ((struct win32_mkfw_window *)(state)->platform)
#define CTX_PLATFORM(c) ((struct win32_mkfw_context *)(c)->platform)

/* Platform-specific raw input scaling factor to normalize across platforms */
#define WIN32_RAW_MOUSE_SCALE 3.0

// Process-global platform state
static LARGE_INTEGER mkfw_win32_perf_freq;
static UINT (WINAPI *mkfw_win32_GetDpiForWindow)(HWND);
static uint8_t mkfw_win32_class_registered;

struct win32_mkfw_context {
	HINSTANCE hinstance;
};

// Win32 per-window platform state
struct win32_mkfw_window {
	HINSTANCE hinstance;
	HWND hwnd;
	HDC hdc;
	HGLRC hglrc;
	float aspect_ratio;
	uint8_t should_close;

	RECT saved_rect;
	int32_t saved_style;
	int32_t saved_style_ex;

	uint8_t mouse_constrained;
	double last_mouse_dx;
	double last_mouse_dy;
	double accumulated_dx;
	double accumulated_dy;
	double mouse_sensitivity;
	int32_t min_width;
	int32_t min_height;
	HCURSOR hidden_cursor;
	uint8_t is_fullscreen;
	uint8_t cursor_hidden;
	uint8_t aspect_ratio_enabled;
	WINDOWPLACEMENT window_placement;

	HCURSOR cursors[MKFW_CURSOR_LAST];
	uint32_t current_cursor;
	uint16_t high_surrogate;
	uint8_t mouse_tracked;

	// Window state callback tracking
	uint8_t last_maximized;
	uint8_t last_minimized;
};

// [=]===^=[ map_vk_to_scancode ]=================================================================[=]
static uint32_t map_vk_to_scancode(struct mkfw_window *state, WPARAM wParam, LPARAM lParam, int key_down) {
	uint32_t keycode = 0;

	// Special handling for ambiguous keys (VK_SHIFT, VK_CONTROL, VK_MENU)
	if(wParam == VK_SHIFT) {
		uint32_t scancode = (lParam >> 16) & 0xFF;
		if(scancode == 0x2A) {
			wParam = VK_LSHIFT;

		} else if(scancode == 0x36) {
			wParam = VK_RSHIFT;
		}
	} else if(wParam == VK_CONTROL) {
		if(lParam & 0x01000000) {
			wParam = VK_RCONTROL;

		} else {
			wParam = VK_LCONTROL;
		}
	} else if(wParam == VK_MENU) {
		if(lParam & 0x01000000) {
			wParam = VK_RMENU;

		} else {
			wParam = VK_LMENU;
		}
	}

	// Track modifier state
	switch(wParam) {
		case VK_LSHIFT:
			state->keyboard_state[MKFW_KEY_LSHIFT] = key_down;
			break;
		case VK_RSHIFT:
			state->keyboard_state[MKFW_KEY_RSHIFT] = key_down;
			break;
		case VK_LCONTROL:
			state->keyboard_state[MKFW_KEY_LCTRL] = key_down;
			break;
		case VK_RCONTROL:
			state->keyboard_state[MKFW_KEY_RCTRL] = key_down;
			break;
		case VK_LMENU:
			state->keyboard_state[MKFW_KEY_LALT] = key_down;
			break;
		case VK_RMENU:
			state->keyboard_state[MKFW_KEY_RALT] = key_down;
			break;
		case VK_LWIN:
			state->keyboard_state[MKFW_KEY_LSUPER] = key_down;
			break;
		case VK_RWIN:
			state->keyboard_state[MKFW_KEY_RSUPER] = key_down;
			break;
	}

	// Update combined modifier states from keyboard_state
	state->keyboard_state[MKFW_KEY_SHIFT] = state->keyboard_state[MKFW_KEY_LSHIFT] || state->keyboard_state[MKFW_KEY_RSHIFT];
	state->keyboard_state[MKFW_KEY_CTRL]  = state->keyboard_state[MKFW_KEY_LCTRL] || state->keyboard_state[MKFW_KEY_RCTRL];
	state->keyboard_state[MKFW_KEY_ALT]   = state->keyboard_state[MKFW_KEY_LALT] || state->keyboard_state[MKFW_KEY_RALT];

	// Update modifier_state array for compatibility
	state->modifier_state[MKFW_MODIFIER_SHIFT] = state->keyboard_state[MKFW_KEY_SHIFT];
	state->modifier_state[MKFW_MODIFIER_CTRL] = state->keyboard_state[MKFW_KEY_CTRL];
	state->modifier_state[MKFW_MODIFIER_ALT] = state->keyboard_state[MKFW_KEY_ALT];

	// Handle number keys (VK_0 - VK_9)
	if(wParam >= 0x30 && wParam <= 0x39) {
		keycode = MKFW_KEY_0 + (wParam - 0x30);
	} else if(wParam >= 0x41 && wParam <= 0x5A) {  // A-Z range
		keycode = wParam + 32;  // Convert to lowercase (a-z)
	} else if(wParam >= 0x20 && wParam <= 0x7E) {
		keycode = wParam;
	}

	// Handle non-extended special keys
	switch(wParam) {
		case VK_ESCAPE:    keycode = MKFW_KEY_ESCAPE; break;
		case VK_BACK:      keycode = MKFW_KEY_BACKSPACE; break;
		case VK_TAB:       keycode = MKFW_KEY_TAB; break;
		case VK_RETURN:    keycode = MKFW_KEY_RETURN; break;
		case VK_CAPITAL:   keycode = MKFW_KEY_CAPSLOCK; break;
		case VK_F1:        keycode = MKFW_KEY_F1; break;
		case VK_F2:        keycode = MKFW_KEY_F2; break;
		case VK_F3:        keycode = MKFW_KEY_F3; break;
		case VK_F4:        keycode = MKFW_KEY_F4; break;
		case VK_F5:        keycode = MKFW_KEY_F5; break;
		case VK_F6:        keycode = MKFW_KEY_F6; break;
		case VK_F7:        keycode = MKFW_KEY_F7; break;
		case VK_F8:        keycode = MKFW_KEY_F8; break;
		case VK_F9:        keycode = MKFW_KEY_F9; break;
		case VK_F10:       keycode = MKFW_KEY_F10; break;
		case VK_F11:       keycode = MKFW_KEY_F11; break;
		case VK_F12:       keycode = MKFW_KEY_F12; break;
	}

	// Handle extended keys (lParam & 0x01000000)
	if(lParam & 0x01000000) {
		switch(wParam) {
			case VK_LEFT:      keycode = MKFW_KEY_LEFT; break;
			case VK_RIGHT:     keycode = MKFW_KEY_RIGHT; break;
			case VK_UP:        keycode = MKFW_KEY_UP; break;
			case VK_DOWN:      keycode = MKFW_KEY_DOWN; break;
			case VK_RETURN:    keycode = MKFW_KEY_NUMPAD_ENTER; break;
			case VK_INSERT:    keycode = MKFW_KEY_INSERT; break;
			case VK_DELETE:    keycode = MKFW_KEY_DELETE; break;
			case VK_HOME:      keycode = MKFW_KEY_HOME; break;
			case VK_END:       keycode = MKFW_KEY_END; break;
			case VK_PRIOR:     keycode = MKFW_KEY_PAGEUP; break;
			case VK_NEXT:      keycode = MKFW_KEY_PAGEDOWN; break;
			case VK_NUMLOCK:   keycode = MKFW_KEY_NUMLOCK; break;
			case VK_SCROLL:    keycode = MKFW_KEY_SCROLLLOCK; break;
			case VK_PRINT:     keycode = MKFW_KEY_PRINTSCREEN; break;
			case VK_PAUSE:     keycode = MKFW_KEY_PAUSE; break;
			case VK_APPS:      keycode = MKFW_KEY_MENU; break;
			case VK_DIVIDE:    keycode = MKFW_KEY_NUMPAD_DIVIDE; break;
		}
	} else {
		// Handle numpad keys
		switch(wParam) {
			case VK_NUMPAD0:   keycode = MKFW_KEY_NUMPAD_0; break;
			case VK_NUMPAD1:   keycode = MKFW_KEY_NUMPAD_1; break;
			case VK_NUMPAD2:   keycode = MKFW_KEY_NUMPAD_2; break;
			case VK_NUMPAD3:   keycode = MKFW_KEY_NUMPAD_3; break;
			case VK_NUMPAD4:   keycode = MKFW_KEY_NUMPAD_4; break;
			case VK_NUMPAD5:   keycode = MKFW_KEY_NUMPAD_5; break;
			case VK_NUMPAD6:   keycode = MKFW_KEY_NUMPAD_6; break;
			case VK_NUMPAD7:   keycode = MKFW_KEY_NUMPAD_7; break;
			case VK_NUMPAD8:   keycode = MKFW_KEY_NUMPAD_8; break;
			case VK_NUMPAD9:   keycode = MKFW_KEY_NUMPAD_9; break;
			case VK_DECIMAL:   keycode = MKFW_KEY_NUMPAD_DECIMAL; break;
			case VK_MULTIPLY:  keycode = MKFW_KEY_NUMPAD_MULTIPLY; break;
			case VK_SUBTRACT:  keycode = MKFW_KEY_NUMPAD_SUBTRACT; break;
			case VK_ADD:       keycode = MKFW_KEY_NUMPAD_ADD; break;
			case VK_SEPARATOR: keycode = MKFW_KEY_NUMPAD_ENTER; break;
		}
	}

	// Update key state
	if(keycode) {
		state->keyboard_state[keycode] = key_down;
	}

	// Call the key callback
	if(keycode && state->key_callback) {
		state->key_callback(state, keycode, key_down ? MKFW_PRESSED : MKFW_RELEASED,
			(state->keyboard_state[MKFW_KEY_SHIFT] ? MKFW_MOD_SHIFT : 0) |
			(state->keyboard_state[MKFW_KEY_CTRL] ? MKFW_MOD_CTRL : 0) |
			(state->keyboard_state[MKFW_KEY_ALT] ? MKFW_MOD_ALT : 0) |
			(state->keyboard_state[MKFW_KEY_LSUPER] ? MKFW_MOD_LSUPER : 0) |
			(state->keyboard_state[MKFW_KEY_RSUPER] ? MKFW_MOD_RSUPER : 0));
	}

	return keycode;
}

// [=]===^=[ Win32WindowProc ]====================================================================[=]
static LRESULT CALLBACK Win32WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	struct mkfw_window *state = (struct mkfw_window *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	// Handle WM_CREATE to set up the user data
	if(uMsg == WM_CREATE) {
		CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
		state = (struct mkfw_window *)cs->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
		return 0;
	}

	if(!state) {
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	switch(uMsg) {
		case WM_CLOSE:
			PLATFORM(state)->should_close = 1;
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_SIZING: {
			if(!PLATFORM(state)->aspect_ratio_enabled || PLATFORM(state)->is_fullscreen) {
				return 0;
			}

			RECT *rect = (RECT *)lParam;
			int width = rect->right - rect->left;
			int height = rect->bottom - rect->top;

			// Get window border size
			RECT client_rect = {0, 0, width, height};
			AdjustWindowRectEx(&client_rect, GetWindowLong(hwnd, GWL_STYLE), FALSE, GetWindowLong(hwnd, GWL_EXSTYLE));
			int border_width = (client_rect.right - client_rect.left) - width;
			int border_height = (client_rect.bottom - client_rect.top) - height;

			// Adjust width/height based on client area
			int new_client_width = width - border_width;
			int new_client_height = height - border_height;

			switch(wParam) {
				case WMSZ_LEFT:
				case WMSZ_RIGHT: {
					new_client_height = (int)(new_client_width / PLATFORM(state)->aspect_ratio);
				} break;

				case WMSZ_TOP:
				case WMSZ_BOTTOM: {
					new_client_width = (int)(new_client_height * PLATFORM(state)->aspect_ratio);
				} break;

				case WMSZ_TOPLEFT:
				case WMSZ_TOPRIGHT:
				case WMSZ_BOTTOMLEFT:
				case WMSZ_BOTTOMRIGHT: {
					if((double)new_client_width / new_client_height > PLATFORM(state)->aspect_ratio) {
						new_client_width = (int)(new_client_height * PLATFORM(state)->aspect_ratio);
					} else {
						new_client_height = (int)(new_client_width / PLATFORM(state)->aspect_ratio);
					}
				} break;
			}

			// Convert back to window size
			rect->right = rect->left + new_client_width + border_width;
			rect->bottom = rect->top + new_client_height + border_height;

			return TRUE;
		}

		case WM_SIZE: {
			int new_width  = LOWORD(lParam);
			int new_height = HIWORD(lParam);

			if(state->framebuffer_callback) {
				state->framebuffer_callback(state, new_width, new_height, PLATFORM(state)->aspect_ratio);
			}

			if(state->window_state_callback) {
				uint8_t maximized = (wParam == SIZE_MAXIMIZED) ? 1 : 0;
				uint8_t minimized = (wParam == SIZE_MINIMIZED) ? 1 : 0;
				if(maximized != PLATFORM(state)->last_maximized || minimized != PLATFORM(state)->last_minimized) {
					PLATFORM(state)->last_maximized = maximized;
					PLATFORM(state)->last_minimized = minimized;
					state->window_state_callback(state, maximized, minimized);
				}
			}
		} break;

		case WM_GETMINMAXINFO: {
			MINMAXINFO *mmi = (MINMAXINFO *)lParam;
			LONG style = GetWindowLong(hwnd, GWL_STYLE);
			LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

			// Check if the window is in fullscreen mode
			if(style & WS_POPUP) {
				return 0; // No min/max restrictions in fullscreen
			}

			// Get the monitor info
			HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = { 0 };
			mi.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(monitor, &mi);

			// Convert min client size to actual window size
			RECT adjusted = { 0, 0, PLATFORM(state)->min_width, (int)(PLATFORM(state)->min_width / PLATFORM(state)->aspect_ratio) };
			AdjustWindowRectEx(&adjusted, style, FALSE, exStyle);

			int min_w = adjusted.right - adjusted.left;
			int min_h = adjusted.bottom - adjusted.top;

			mmi->ptMaxTrackSize.x = mi.rcWork.right - mi.rcWork.left; // No max limit in windowed mode
			mmi->ptMaxTrackSize.y = mi.rcWork.bottom - mi.rcWork.top;

			mmi->ptMinTrackSize.x = min_w; // Apply min size based on adjusted window size
			mmi->ptMinTrackSize.y = min_h;

			return 0;
		} break;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN: {
			map_vk_to_scancode(state, wParam, lParam, 1);
		} break;

		case WM_SYSKEYUP:
		case WM_KEYUP: {
			map_vk_to_scancode(state, wParam, lParam, 0);
		} break;

		case WM_CHAR: {
			uint32_t ch = (uint32_t)wParam;
			if(ch >= 0xD800 && ch <= 0xDBFF) {
				PLATFORM(state)->high_surrogate = (uint16_t)ch;
				break;
			}
			if(ch >= 0xDC00 && ch <= 0xDFFF) {
				if(PLATFORM(state)->high_surrogate) {
					ch = 0x10000 + ((uint32_t)(PLATFORM(state)->high_surrogate - 0xD800) << 10) + (ch - 0xDC00);
					PLATFORM(state)->high_surrogate = 0;
				} else {
					break;
				}
			} else {
				PLATFORM(state)->high_surrogate = 0;
			}
			if(ch == 8 || ch >= 32) {
				if(state->char_callback) {
					state->char_callback(state, ch);
				}
			}
		} break;

		case WM_SETFOCUS: {
			state->has_focus = 1;
			if(state->focus_callback) {
				state->focus_callback(state, 1);
			}
		} break;

		case WM_KILLFOCUS: {
			state->has_focus = 0;
			if(state->focus_callback) {
				state->focus_callback(state, 0);
			}
		} break;

		case WM_MOUSEWHEEL: {
			short delta = GET_WHEEL_DELTA_WPARAM(wParam);
			if(state->scroll_callback) {
				state->scroll_callback(state, 0.0, (double)delta / (double)WHEEL_DELTA);
			}
		} break;

		case WM_MOUSEHWHEEL: {
			short delta = GET_WHEEL_DELTA_WPARAM(wParam);
			if(state->scroll_callback) {
				state->scroll_callback(state, (double)delta / (double)WHEEL_DELTA, 0.0);
			}
		} break;

		case WM_MOUSEMOVE: {
			state->mouse_x = GET_X_LPARAM(lParam);
			state->mouse_y = GET_Y_LPARAM(lParam);
			if(!PLATFORM(state)->mouse_tracked) {
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(tme);
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hwnd;
				tme.dwHoverTime = 0;
				TrackMouseEvent(&tme);
				PLATFORM(state)->mouse_tracked = 1;
				state->mouse_in_window = 1;
			}
		} break;

		case WM_MOUSELEAVE: {
			PLATFORM(state)->mouse_tracked = 0;
			state->mouse_in_window = 0;
		} break;

		case WM_LBUTTONDOWN: {
			SetCapture(hwnd);
			state->mouse_buttons[MKFW_MOUSE_LEFT] = 1;
			if(state->mouse_button_callback) {
				state->mouse_button_callback(state, MKFW_MOUSE_LEFT, MKFW_PRESSED);
			}
		} break;

		case WM_LBUTTONUP: {
			state->mouse_buttons[MKFW_MOUSE_LEFT] = 0;
			if(!state->mouse_buttons[MKFW_MOUSE_RIGHT] && !state->mouse_buttons[MKFW_MOUSE_MIDDLE]) {
				ReleaseCapture();
			}
			if(state->mouse_button_callback) {
				state->mouse_button_callback(state, MKFW_MOUSE_LEFT, MKFW_RELEASED);
			}
		} break;

		case WM_MBUTTONDOWN: {
			SetCapture(hwnd);
			state->mouse_buttons[MKFW_MOUSE_MIDDLE] = 1;
			if(state->mouse_button_callback) {
				state->mouse_button_callback(state, MKFW_MOUSE_MIDDLE, MKFW_PRESSED);
			}
		} break;

		case WM_MBUTTONUP: {
			state->mouse_buttons[MKFW_MOUSE_MIDDLE] = 0;
			if(!state->mouse_buttons[MKFW_MOUSE_LEFT] && !state->mouse_buttons[MKFW_MOUSE_RIGHT]) {
				ReleaseCapture();
			}
			if(state->mouse_button_callback) {
				state->mouse_button_callback(state, MKFW_MOUSE_MIDDLE, MKFW_RELEASED);
			}
		} break;

		case WM_RBUTTONDOWN: {
			SetCapture(hwnd);
			state->mouse_buttons[MKFW_MOUSE_RIGHT] = 1;
			if(state->mouse_button_callback) {
				state->mouse_button_callback(state, MKFW_MOUSE_RIGHT, MKFW_PRESSED);
			}
		} break;

		case WM_RBUTTONUP: {
			state->mouse_buttons[MKFW_MOUSE_RIGHT] = 0;
			if(!state->mouse_buttons[MKFW_MOUSE_LEFT] && !state->mouse_buttons[MKFW_MOUSE_MIDDLE]) {
				ReleaseCapture();
			}
			if(state->mouse_button_callback) {
				state->mouse_button_callback(state, MKFW_MOUSE_RIGHT, MKFW_RELEASED);
			}
		} break;

		case WM_XBUTTONDOWN: {
			SetCapture(hwnd);
			uint8_t mapped = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? MKFW_MOUSE_EXTRA1 : MKFW_MOUSE_EXTRA2;
			state->mouse_buttons[mapped] = 1;
			if(state->mouse_button_callback) {
				state->mouse_button_callback(state, mapped, MKFW_PRESSED);
			}
			return TRUE;
		}

		case WM_XBUTTONUP: {
			uint8_t mapped = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? MKFW_MOUSE_EXTRA1 : MKFW_MOUSE_EXTRA2;
			state->mouse_buttons[mapped] = 0;
			if(!state->mouse_buttons[MKFW_MOUSE_LEFT] && !state->mouse_buttons[MKFW_MOUSE_RIGHT] && !state->mouse_buttons[MKFW_MOUSE_MIDDLE]) {
				ReleaseCapture();
			}
			if(state->mouse_button_callback) {
				state->mouse_button_callback(state, mapped, MKFW_RELEASED);
			}
			return TRUE;
		}

		case WM_INPUT: {
			RAWINPUT raw;
			UINT size = sizeof(RAWINPUT);

			if(GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) > 0) {
				if(raw.header.dwType == RIM_TYPEMOUSE) {
					double dx = (double)raw.data.mouse.lLastX * WIN32_RAW_MOUSE_SCALE;
					double dy = (double)raw.data.mouse.lLastY * WIN32_RAW_MOUSE_SCALE;

					if(PLATFORM(state)->mouse_constrained) {
						if(dx * dx + dy * dy < 0.1) {
							dx = PLATFORM(state)->last_mouse_dx;
							dy = PLATFORM(state)->last_mouse_dy;
						}
					}

					PLATFORM(state)->last_mouse_dx = dx;
					PLATFORM(state)->last_mouse_dy = dy;

					// Accumulate for read-and-clear API with sensitivity scaling
					PLATFORM(state)->accumulated_dx += dx * PLATFORM(state)->mouse_sensitivity;
					PLATFORM(state)->accumulated_dy += dy * PLATFORM(state)->mouse_sensitivity;

					if(state->mouse_move_delta_callback) {
						state->mouse_move_delta_callback(state, (int)dx, (int)dy);
					}
				}
			}
			return 0;
		} break;

		case WM_DROPFILES: {
			HDROP hdrop = (HDROP)wParam;
			uint32_t count = DragQueryFileW(hdrop, 0xFFFFFFFF, 0, 0);
			if(count > 0 && state->drop_callback) {
				char **paths = (char **)malloc(count * sizeof(char *));
				for(uint32_t i = 0; i < count; ++i) {
					uint32_t wlen = DragQueryFileW(hdrop, i, 0, 0) + 1;
					wchar_t *wpath = (wchar_t *)malloc(wlen * sizeof(wchar_t));
					DragQueryFileW(hdrop, i, wpath, wlen);
					int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, 0, 0, 0, 0);
					paths[i] = (char *)malloc(utf8_len);
					WideCharToMultiByte(CP_UTF8, 0, wpath, -1, paths[i], utf8_len, 0, 0);
					free(wpath);
				}
				state->drop_callback(count, (const char **)paths);
				for(uint32_t i = 0; i < count; ++i) {
					free(paths[i]);
				}
				free(paths);
			}
			DragFinish(hdrop);
			return 0;
		}

		case WM_SETCURSOR: {
			if(LOWORD(lParam) == HTCLIENT) {
				if(PLATFORM(state)->cursor_hidden) {
					SetCursor(0);
					return TRUE;
				}
				SetCursor(PLATFORM(state)->cursors[PLATFORM(state)->current_cursor]);
				return TRUE;
			}
			break;
		}

		default:
			break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


// [=]===^=[ mkfw_window_detach_context ]================================================================[=]
static void mkfw_window_detach_context(struct mkfw_window *state) {
	(void)state;
	wglMakeCurrent(0, 0);
}

// [=]===^=[ mkfw_window_attach_context ]================================================================[=]
static void mkfw_window_attach_context(struct mkfw_window *state) {
	wglMakeCurrent(PLATFORM(state)->hdc, PLATFORM(state)->hglrc);
}

// [=]===^=[ mkfw_enable_raw_mouse ]==============================================================[=]
static void mkfw_enable_raw_mouse(struct mkfw_window *state, int32_t enable) {
	(void)enable;
	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;	// Generic desktop
	rid.usUsage     = 0x02;	// Mouse
	rid.dwFlags     = 0;		// Default flags for raw input
	rid.hwndTarget  = PLATFORM(state)->hwnd;
	if(!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
		mkfw_error("failed to register raw mouse input");
	}
}

// [=]===^=[ mkfw_window_show ]===================================================================[=]
static void mkfw_window_show(struct mkfw_window *state) {
	ShowWindow(PLATFORM(state)->hwnd, SW_SHOW);
	UpdateWindow(PLATFORM(state)->hwnd);
}

// [=]===^=[ mkfw_window_hide ]===================================================================[=]
static void mkfw_window_hide(struct mkfw_window *state) {
	ShowWindow(PLATFORM(state)->hwnd, SW_HIDE);
}

// [=]===^=[ mkfw_query_max_gl_version ]==========================================================[=]
static uint32_t mkfw_query_max_gl_version(int32_t *major, int32_t *minor) {
	WNDCLASS wc = {0};
	wc.lpfnWndProc = DefWindowProc;
	wc.hInstance = GetModuleHandle(0);
	wc.lpszClassName = "mkfw_gl_query";
	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(0, wc.lpszClassName, "", 0, 0, 0, 1, 1, 0, 0, wc.hInstance, 0);
	HDC hdc = GetDC(hwnd);

	PIXELFORMATDESCRIPTOR pfd = {0};
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;

	int pf = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, pf, &pfd);

	HGLRC ctx = wglCreateContext(hdc);
	wglMakeCurrent(hdc, ctx);

	// glGetString is a GL 1.1 function exported from opengl32.dll
	typedef const unsigned char *(__stdcall *PFNGLGETSTRINGPROC)(unsigned int);
	HMODULE gl_module = GetModuleHandleA("opengl32.dll");
	PFNGLGETSTRINGPROC pglGetString = gl_module ? (PFNGLGETSTRINGPROC)(void *)GetProcAddress(gl_module, "glGetString") : 0;

	uint32_t result = 0;
	if(pglGetString) {
		const char *version = (const char *)pglGetString(0x1F02); // GL_VERSION
		if(version) {
			int32_t tmp_major, tmp_minor;
			result = mkfw_parse_version(version, &tmp_major, &tmp_minor);
			if(result) {
				if(major) {
					*major = tmp_major;
				}
				if(minor) {
					*minor = tmp_minor;
				}
			}
		}
	}

	wglMakeCurrent(0, 0);
	wglDeleteContext(ctx);
	ReleaseDC(hwnd, hdc);
	DestroyWindow(hwnd);
	UnregisterClass("mkfw_gl_query", wc.hInstance);

	return result;
}

// Forward declaration so mkfw_init can prime the monitor cache
static int32_t mkfw_query_monitors_into(struct mkfw_monitor *out, int32_t max);

// [=]===^=[ mkfw_init ]==========================================================================[=]
static struct mkfw_context *mkfw_init(struct mkfw_options *opts) {
	(void)opts;

	struct mkfw_context *ctx = (struct mkfw_context *)calloc(1, sizeof(struct mkfw_context));
	if(!ctx) {
		return 0;
	}
	ctx->platform = calloc(1, sizeof(struct win32_mkfw_context));
	if(!ctx->platform) {
		free(ctx);
		return 0;
	}

	SetProcessDPIAware();

	HMODULE user32 = GetModuleHandle("user32.dll");
	if(user32 && !mkfw_win32_GetDpiForWindow) {
		mkfw_win32_GetDpiForWindow = (UINT (WINAPI *)(HWND))(void *)GetProcAddress(user32, "GetDpiForWindow");
	}

	if(mkfw_win32_perf_freq.QuadPart == 0) {
		QueryPerformanceFrequency(&mkfw_win32_perf_freq);
	}

	CTX_PLATFORM(ctx)->hinstance = GetModuleHandle(0);

	if(!mkfw_win32_class_registered) {
		WNDCLASS wc = {0};
		wc.lpfnWndProc   = Win32WindowProc;
		wc.hInstance     = CTX_PLATFORM(ctx)->hinstance;
		wc.hCursor       = LoadCursor(0, IDC_ARROW);
		wc.lpszClassName = "OpenGLWindowClass";
		RegisterClass(&wc);
		mkfw_win32_class_registered = 1;
	}

	ctx->monitor_count = (uint32_t)mkfw_query_monitors_into(ctx->monitors, MKFW_MAX_MONITORS);

	return ctx;
}

// [=]===^=[ mkfw_window_create ]=================================================================[=]
static struct mkfw_window *mkfw_window_create(struct mkfw_context *ctx, struct mkfw_window_options *opts) {
	if(!ctx || ctx->window_count >= MKFW_MAX_WINDOWS) {
		return 0;
	}

	struct mkfw_window_options defaults = {0};
	if(!opts) {
		opts = &defaults;
	}
	int32_t width    = opts->width    > 0 ? opts->width    : 1280;
	int32_t height   = opts->height   > 0 ? opts->height   : 720;
	int32_t gl_major = opts->gl_major > 0 ? opts->gl_major : 3;
	int32_t gl_minor = opts->gl_minor > 0 ? opts->gl_minor : 1;
	const char *title = opts->title ? opts->title : "mkfw";
	int32_t transparent = (opts->flags & MKFW_WIN_TRANSPARENT) ? 1 : 0;

	struct mkfw_window *state = (struct mkfw_window *)calloc(1, sizeof(struct mkfw_window));
	if(!state) {
		return 0;
	}
	state->context = ctx;

	state->platform = calloc(1, sizeof(struct win32_mkfw_window));
	if(!state->platform) {
		free(state);
		return 0;
	}

	PLATFORM(state)->mouse_sensitivity = 1.0;
	PLATFORM(state)->hinstance = CTX_PLATFORM(ctx)->hinstance;

	DWORD style = WS_OVERLAPPEDWINDOW;
	RECT rect = { 0, 0, width, height };
	AdjustWindowRect(&rect, style, FALSE);

	PLATFORM(state)->hwnd = CreateWindowEx(0, "OpenGLWindowClass", title, style, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, 0, 0, PLATFORM(state)->hinstance, state);

	SetWindowPos(PLATFORM(state)->hwnd, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

	PLATFORM(state)->hdc = GetDC(PLATFORM(state)->hwnd);

	PIXELFORMATDESCRIPTOR pfd = {0};
	pfd.nSize      = sizeof(pfd);
	pfd.nVersion   = 1;
	pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int pf = ChoosePixelFormat(PLATFORM(state)->hdc, &pfd);
	SetPixelFormat(PLATFORM(state)->hdc, pf, &pfd);

	HGLRC temp_ctx = wglCreateContext(PLATFORM(state)->hdc);
	wglMakeCurrent(PLATFORM(state)->hdc, temp_ctx);

	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)(void *)wglGetProcAddress("wglCreateContextAttribsARB");
	if(!wglCreateContextAttribsARB) {
		PLATFORM(state)->hglrc = temp_ctx;
	} else {
		int ctx_attribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, gl_major,
			WGL_CONTEXT_MINOR_VERSION_ARB, gl_minor,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
			0
		};

		HGLRC modern_ctx = wglCreateContextAttribsARB(PLATFORM(state)->hdc, 0, ctx_attribs);
		if(modern_ctx) {
			wglMakeCurrent(0, 0);
			wglDeleteContext(temp_ctx);
			wglMakeCurrent(PLATFORM(state)->hdc, modern_ctx);
			PLATFORM(state)->hglrc = modern_ctx;
		} else {
			typedef const unsigned char *(__stdcall *PFNGLGETSTRINGPROC)(unsigned int);
			HMODULE gl_module = GetModuleHandleA("opengl32.dll");
			PFNGLGETSTRINGPROC pglGetString = gl_module ? (PFNGLGETSTRINGPROC)(void *)GetProcAddress(gl_module, "glGetString") : 0;
			int32_t max_major = 0, max_minor = 0;
			if(pglGetString) {
				const char *ver = (const char *)pglGetString(0x1F02);
				if(ver) {
					mkfw_parse_version(ver, &max_major, &max_minor);
				}
			}
			wglMakeCurrent(0, 0);
			wglDeleteContext(temp_ctx);
			mkfw_error("OpenGL %d.%d Compatibility Profile not available (driver supports up to %d.%d)",
				gl_major, gl_minor, max_major, max_minor);
			ReleaseDC(PLATFORM(state)->hwnd, PLATFORM(state)->hdc);
			DestroyWindow(PLATFORM(state)->hwnd);
			free(state->platform);
			free(state);
			return 0;
		}
	}

	PLATFORM(state)->saved_style = style;
	GetWindowRect(PLATFORM(state)->hwnd, &PLATFORM(state)->saved_rect);
	mkfw_enable_raw_mouse(state, 1);

	PLATFORM(state)->cursors[MKFW_CURSOR_ARROW]       = LoadCursor(0, IDC_ARROW);
	PLATFORM(state)->cursors[MKFW_CURSOR_TEXT_INPUT]  = LoadCursor(0, IDC_IBEAM);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_ALL]  = LoadCursor(0, IDC_SIZEALL);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_NS]   = LoadCursor(0, IDC_SIZENS);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_EW]   = LoadCursor(0, IDC_SIZEWE);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_NESW] = LoadCursor(0, IDC_SIZENESW);
	PLATFORM(state)->cursors[MKFW_CURSOR_RESIZE_NWSE] = LoadCursor(0, IDC_SIZENWSE);
	PLATFORM(state)->cursors[MKFW_CURSOR_HAND]        = LoadCursor(0, IDC_HAND);
	PLATFORM(state)->cursors[MKFW_CURSOR_NOT_ALLOWED] = LoadCursor(0, IDC_NO);

	if(transparent) {
		typedef struct { int left; int right; int top; int bottom; } MKFW_DWM_MARGINS;
		typedef HRESULT (WINAPI *PFN_DwmExtendFrameIntoClientArea)(HWND, const MKFW_DWM_MARGINS *);
		HMODULE dwm = LoadLibraryA("dwmapi.dll");
		if(dwm) {
			PFN_DwmExtendFrameIntoClientArea pExtend = (PFN_DwmExtendFrameIntoClientArea)(void *)GetProcAddress(dwm, "DwmExtendFrameIntoClientArea");
			if(pExtend) {
				MKFW_DWM_MARGINS m = {-1, -1, -1, -1};
				pExtend(PLATFORM(state)->hwnd, &m);
			}
			FreeLibrary(dwm);
		}
	}

	state->has_focus = 1;

	ctx->windows[ctx->window_count++] = state;

	if(!(opts->flags & MKFW_WIN_HIDDEN)) {
		ShowWindow(PLATFORM(state)->hwnd, SW_SHOW);
	}

	return state;
}

// [=]===^=[ mkfw_window_set_fullscreen ]====================================================================[=]
static void mkfw_window_set_fullscreen(struct mkfw_window *state, int32_t enable) {
	PLATFORM(state)->window_placement.length = sizeof(PLATFORM(state)->window_placement);
	DWORD dwStyle = GetWindowLong(PLATFORM(state)->hwnd, GWL_STYLE);

	if(!PLATFORM(state)->is_fullscreen && enable) {
		MONITORINFO mi;
		memset(&mi, 0, sizeof(mi));
		mi.cbSize = sizeof(mi);
		if(GetWindowPlacement(PLATFORM(state)->hwnd, &PLATFORM(state)->window_placement) && GetMonitorInfo(MonitorFromWindow(PLATFORM(state)->hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
			PLATFORM(state)->saved_style = dwStyle;
			SetWindowLong(PLATFORM(state)->hwnd, GWL_STYLE, (dwStyle & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
			SetWindowPos(PLATFORM(state)->hwnd, HWND_TOPMOST, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		}
		PLATFORM(state)->is_fullscreen = 1;
		state->is_fullscreen = 1;

	} else if(PLATFORM(state)->is_fullscreen && !enable) {
		SetWindowLong(PLATFORM(state)->hwnd, GWL_STYLE, PLATFORM(state)->saved_style);
		SetWindowPlacement(PLATFORM(state)->hwnd, &PLATFORM(state)->window_placement);
		SetWindowPos(PLATFORM(state)->hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		PLATFORM(state)->is_fullscreen = 0;
		state->is_fullscreen = 0;
	}
}

// [=]===^=[ mkfw_poll_events ]===================================================================[=]
static void mkfw_poll_events(struct mkfw_context *ctx) {
	if(!ctx) {
		return;
	}
	MSG msg;
	while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		if(msg.message == WM_QUIT) {
			for(uint32_t i = 0; i < ctx->window_count; ++i) {
				PLATFORM(ctx->windows[i])->should_close = 1;
			}
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

// [=]===^=[ mkfw_window_constrain_mouse ]===============================================================[=]
static void mkfw_window_constrain_mouse(struct mkfw_window *state, int32_t constrain) {
	PLATFORM(state)->mouse_constrained = constrain;
	if(constrain) {
		RECT rect;
		GetClientRect(PLATFORM(state)->hwnd, &rect);
		POINT ul = { rect.left, rect.top };
		POINT lr = { rect.right, rect.bottom };
		ClientToScreen(PLATFORM(state)->hwnd, &ul);
		ClientToScreen(PLATFORM(state)->hwnd, &lr);
		rect.left   = ul.x;
		rect.top    = ul.y;
		rect.right  = lr.x;
		rect.bottom = lr.y;
		ClipCursor(&rect);
	} else {
		ClipCursor(0);
	}
}

// [=]===^=[ mkfw_window_set_mouse_cursor ]==============================================================[=]
static void mkfw_window_set_mouse_cursor(struct mkfw_window *state, int32_t visible) {
	if(visible) {
		PLATFORM(state)->cursor_hidden = 0;
		mkfw_window_constrain_mouse(state, 0);
		SetCursor(LoadCursor(0, IDC_ARROW));
	} else {
		PLATFORM(state)->cursor_hidden = 1;
		mkfw_window_constrain_mouse(state, 1);
		SetCursor(0);
	}
}

// [=]===^=[ mkfw_window_set_swap_interval ]==============================================================[=]
static void mkfw_window_set_swap_interval(struct mkfw_window *state, uint32_t interval) {
	(void)state;
	typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXT)(int);
	PFNWGLSWAPINTERVALEXT wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXT)(void*)wglGetProcAddress("wglSwapIntervalEXT");

	if(wglSwapIntervalEXT) {
		wglSwapIntervalEXT(interval);
	}
}

// [=]===^=[ mkfw_window_get_swap_interval ]==============================================================[=]
static int32_t mkfw_window_get_swap_interval(struct mkfw_window *state) {
	(void)state;
	typedef int (WINAPI *PFNWGLGETSWAPINTERVALEXT)(void);
	PFNWGLGETSWAPINTERVALEXT wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXT)(void *)wglGetProcAddress("wglGetSwapIntervalEXT");
	if(wglGetSwapIntervalEXT) {
		return wglGetSwapIntervalEXT();
	}
	return 0;
}

// [=]===^=[ mkfw_window_should_close ]==================================================================[=]
static uint32_t mkfw_window_should_close(struct mkfw_window *state) {
	return PLATFORM(state)->should_close;
}

// [=]===^=[ mkfw_window_set_should_close ]==============================================================[=]
static void mkfw_window_set_should_close(struct mkfw_window *state, int32_t value) {
	PLATFORM(state)->should_close = value;
}

// [=]===^=[ mkfw_window_swap_buffers ]==================================================================[=]
static void mkfw_window_swap_buffers(struct mkfw_window *state) {
	SwapBuffers(PLATFORM(state)->hdc);
}

// [=]===^=[ mkfw_window_set_min_size_and_aspect ]================================================[=]
static void mkfw_window_set_min_size_and_aspect(struct mkfw_window *state, int32_t min_width, int32_t min_height, float aspect_width, float aspect_height) {
	PLATFORM(state)->aspect_ratio_enabled = 1;

	PLATFORM(state)->aspect_ratio = aspect_width / aspect_height;
	PLATFORM(state)->min_width = min_width;
	PLATFORM(state)->min_height = min_height;

	RECT window_rect, client_rect;
	GetWindowRect(PLATFORM(state)->hwnd, &window_rect);
	GetClientRect(PLATFORM(state)->hwnd, &client_rect);

	int window_width = window_rect.right - window_rect.left;
	int window_height = window_rect.bottom - window_rect.top;

	int client_width = client_rect.right - client_rect.left;
	int client_height = client_rect.bottom - client_rect.top;

	// Calculate the exact border size dynamically
	int border_width = window_width - client_width;
	int border_height = window_height - client_height;

	// Apply aspect ratio correction based on client area
	if((float)client_width / client_height > PLATFORM(state)->aspect_ratio) {
		client_width = (int)(client_height * PLATFORM(state)->aspect_ratio);
	} else {
		client_height = (int)(client_width / PLATFORM(state)->aspect_ratio);
	}

	// Convert back to window size
	int new_window_width = client_width + border_width;
	int new_window_height = client_height + border_height;

	// Get current position to prevent accidental movement
	int x = window_rect.left;
	int y = window_rect.top;

	// Resize without moving the window
	SetWindowPos(PLATFORM(state)->hwnd, 0, x, y, new_window_width, new_window_height, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
}

// [=]===^=[ mkfw_window_set_size ]===============================================================[=]
static void mkfw_window_set_size(struct mkfw_window *state, int32_t width, int32_t height) {
	DWORD style = GetWindowLong(PLATFORM(state)->hwnd, GWL_STYLE);
	DWORD ex_style = GetWindowLong(PLATFORM(state)->hwnd, GWL_EXSTYLE);
	RECT rect = {0, 0, width, height};
	AdjustWindowRectEx(&rect, style, FALSE, ex_style);
	SetWindowPos(PLATFORM(state)->hwnd, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

// [=]===^=[ mkfw_window_get_framebuffer_size ]==========================================================[=]
static void mkfw_window_get_framebuffer_size(struct mkfw_window *state, int32_t *width, int32_t *height) {
	RECT rect;
	GetClientRect(PLATFORM(state)->hwnd, &rect);
	if(width) {
		*width = rect.right - rect.left;
	}
	if(height) {
		*height = rect.bottom - rect.top;
	}
}

// [=]===^=[ mkfw_window_set_title ]==============================================================[=]
static void mkfw_window_set_title(struct mkfw_window *state, const char *title) {
	if(!title) {
		return;
	}
	SetWindowTextA(PLATFORM(state)->hwnd, title);
}

// [=]===^=[ mkfw_window_set_resizable ]==========================================================[=]
static void mkfw_window_set_resizable(struct mkfw_window *state, int32_t resizable) {
	DWORD style = GetWindowLong(PLATFORM(state)->hwnd, GWL_STYLE);
	DWORD exStyle = GetWindowLong(PLATFORM(state)->hwnd, GWL_EXSTYLE);
	RECT window_rect, client_rect;
	GetWindowRect(PLATFORM(state)->hwnd, &window_rect);
	GetClientRect(PLATFORM(state)->hwnd, &client_rect);

	// Save the current client area size
	int client_width = client_rect.right - client_rect.left;
	int client_height = client_rect.bottom - client_rect.top;

	if(resizable) {	// Add back resize border and maximize box
		style |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
	} else {				// Remove resize border and maximize box
		style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
	}

	SetWindowLong(PLATFORM(state)->hwnd, GWL_STYLE, style);

	// Calculate new window size to maintain the same client area
	RECT adjusted_rect = {0, 0, client_width, client_height};
	AdjustWindowRectEx(&adjusted_rect, style, FALSE, exStyle);

	int new_width = adjusted_rect.right - adjusted_rect.left;
	int new_height = adjusted_rect.bottom - adjusted_rect.top;

	// Update window size and force frame to redraw
	SetWindowPos(PLATFORM(state)->hwnd, 0, window_rect.left, window_rect.top, new_width, new_height, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

// [=]===^=[ mkfw_window_set_decorated ]==========================================================[=]
static void mkfw_window_set_decorated(struct mkfw_window *state, int32_t decorated) {
	DWORD style = GetWindowLong(PLATFORM(state)->hwnd, GWL_STYLE);
	DWORD ex_style = GetWindowLong(PLATFORM(state)->hwnd, GWL_EXSTYLE);
	RECT client_rect;
	GetClientRect(PLATFORM(state)->hwnd, &client_rect);
	int32_t client_width = client_rect.right - client_rect.left;
	int32_t client_height = client_rect.bottom - client_rect.top;

	if(decorated) {
		style |= WS_OVERLAPPEDWINDOW;
		style &= ~WS_POPUP;
	} else {
		style &= ~WS_OVERLAPPEDWINDOW;
		style |= WS_POPUP;
	}

	SetWindowLong(PLATFORM(state)->hwnd, GWL_STYLE, style);

	RECT adjusted = {0, 0, client_width, client_height};
	AdjustWindowRectEx(&adjusted, style, FALSE, ex_style);
	RECT wr;
	GetWindowRect(PLATFORM(state)->hwnd, &wr);
	SetWindowPos(PLATFORM(state)->hwnd, 0, wr.left, wr.top, adjusted.right - adjusted.left, adjusted.bottom - adjusted.top, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

// [=]===^=[ mkfw_window_set_opacity ]============================================================[=]
static void mkfw_window_set_opacity(struct mkfw_window *state, float opacity) {
	DWORD ex_style = GetWindowLong(PLATFORM(state)->hwnd, GWL_EXSTYLE);
	if(opacity >= 1.0f) {
		SetWindowLong(PLATFORM(state)->hwnd, GWL_EXSTYLE, ex_style & ~WS_EX_LAYERED);
	} else {
		SetWindowLong(PLATFORM(state)->hwnd, GWL_EXSTYLE, ex_style | WS_EX_LAYERED);
		BYTE alpha = (BYTE)(opacity * 255.0f);
		SetLayeredWindowAttributes(PLATFORM(state)->hwnd, 0, alpha, LWA_ALPHA);
	}
}

// [=]===^=[ mkfw_window_set_icon ]=============================================================[=]
static void mkfw_window_set_icon(struct mkfw_window *state, int32_t width, int32_t height, const uint8_t *rgba) {
	if(!rgba) {
		return;
	}
	// Create a DIB section with BGRA pixel data
	BITMAPV5HEADER bi = {0};
	bi.bV5Size = sizeof(bi);
	bi.bV5Width = width;
	bi.bV5Height = -height; // top-down
	bi.bV5Planes = 1;
	bi.bV5BitCount = 32;
	bi.bV5Compression = BI_BITFIELDS;
	bi.bV5RedMask   = 0x00FF0000;
	bi.bV5GreenMask = 0x0000FF00;
	bi.bV5BlueMask  = 0x000000FF;
	bi.bV5AlphaMask = 0xFF000000;

	HDC dc = GetDC(0);
	uint8_t *target = 0;
	HBITMAP color = CreateDIBSection(dc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, (void **)&target, 0, 0);
	ReleaseDC(0, dc);
	if(!color) {
		return;
	}

	// Convert RGBA to BGRA
	int32_t pixel_count = width * height;
	for(int32_t i = 0; i < pixel_count; ++i) {
		target[i * 4 + 0] = rgba[i * 4 + 2]; // B
		target[i * 4 + 1] = rgba[i * 4 + 1]; // G
		target[i * 4 + 2] = rgba[i * 4 + 0]; // R
		target[i * 4 + 3] = rgba[i * 4 + 3]; // A
	}

	HBITMAP mask = CreateBitmap(width, height, 1, 1, 0);

	ICONINFO ii = {0};
	ii.fIcon = TRUE;
	ii.hbmMask = mask;
	ii.hbmColor = color;

	HICON icon = CreateIconIndirect(&ii);
	DeleteObject(color);
	DeleteObject(mask);

	if(icon) {
		SendMessage(PLATFORM(state)->hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
		SendMessage(PLATFORM(state)->hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
	}
}

struct mkfw_monitor_enum_data {
	struct mkfw_monitor *out;
	int32_t max;
	int32_t count;
};

// [=]===^=[ mkfw_monitor_enum_proc ]===========================================================[=]
static BOOL CALLBACK mkfw_monitor_enum_proc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	(void)hdcMonitor;
	(void)lprcMonitor;
	struct mkfw_monitor_enum_data *d = (struct mkfw_monitor_enum_data *)dwData;
	if(d->count >= d->max) {
		return FALSE;
	}

	MONITORINFOEXA mi = {0};
	mi.cbSize = sizeof(mi);
	GetMonitorInfoA(hMonitor, (MONITORINFO *)&mi);

	DEVMODEA dm = {0};
	dm.dmSize = sizeof(dm);
	EnumDisplaySettingsA(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm);

	struct mkfw_monitor *m = &d->out[d->count++];
	memset(m, 0, sizeof(*m));
	snprintf(m->name, sizeof(m->name), "%s", mi.szDevice);
	m->x = mi.rcMonitor.left;
	m->y = mi.rcMonitor.top;
	m->width = mi.rcMonitor.right - mi.rcMonitor.left;
	m->height = mi.rcMonitor.bottom - mi.rcMonitor.top;
	m->refresh_rate = dm.dmDisplayFrequency;
	m->primary = (mi.dwFlags & MONITORINFOF_PRIMARY) ? 1 : 0;

	return TRUE;
}

// [=]===^=[ mkfw_query_monitors_into ]==========================================================[=]
static int32_t mkfw_query_monitors_into(struct mkfw_monitor *out, int32_t max) {
	if(!out || max <= 0) {
		return 0;
	}
	struct mkfw_monitor_enum_data data = {out, max, 0};
	EnumDisplayMonitors(0, 0, mkfw_monitor_enum_proc, (LPARAM)&data);

	for(int32_t i = 1; i < data.count; ++i) {
		if(out[i].primary) {
			struct mkfw_monitor tmp = out[0];
			out[0] = out[i];
			out[i] = tmp;
			break;
		}
	}

	return data.count;
}

// [=]===^=[ mkfw_get_monitors ]================================================================[=]
static int32_t mkfw_get_monitors(struct mkfw_context *ctx, struct mkfw_monitor *out, int32_t max) {
	if(!ctx || !out || max <= 0) {
		return 0;
	}
	int32_t n = (int32_t)ctx->monitor_count;
	if(n > max) {
		n = max;
	}
	for(int32_t i = 0; i < n; ++i) {
		out[i] = ctx->monitors[i];
	}
	return n;
}

// [=]===^=[ mkfw_window_set_position ]=========================================================[=]
static void mkfw_window_set_position(struct mkfw_window *state, int32_t x, int32_t y) {
	SetWindowPos(PLATFORM(state)->hwnd, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

// [=]===^=[ mkfw_window_get_position ]=========================================================[=]
static void mkfw_window_get_position(struct mkfw_window *state, int32_t *x, int32_t *y) {
	RECT rect;
	GetWindowRect(PLATFORM(state)->hwnd, &rect);
	if(x) {
		*x = rect.left;
	}
	if(y) {
		*y = rect.top;
	}
}

// [=]===^=[ mkfw_window_maximize ]=============================================================[=]
static void mkfw_window_maximize(struct mkfw_window *state) {
	ShowWindow(PLATFORM(state)->hwnd, SW_MAXIMIZE);
}

// [=]===^=[ mkfw_window_minimize ]=============================================================[=]
static void mkfw_window_minimize(struct mkfw_window *state) {
	ShowWindow(PLATFORM(state)->hwnd, SW_MINIMIZE);
}

// [=]===^=[ mkfw_window_restore ]==============================================================[=]
static void mkfw_window_restore(struct mkfw_window *state) {
	ShowWindow(PLATFORM(state)->hwnd, SW_RESTORE);
}

// [=]===^=[ mkfw_window_is_minimized ]=================================================================[=]
static uint32_t mkfw_window_is_minimized(struct mkfw_window *state) {
	return IsIconic(PLATFORM(state)->hwnd) ? 1 : 0;
}

// [=]===^=[ mkfw_window_is_maximized ]=================================================================[=]
static uint32_t mkfw_window_is_maximized(struct mkfw_window *state) {
	return IsZoomed(PLATFORM(state)->hwnd) ? 1 : 0;
}

// [=]===^=[ mkfw_window_get_content_scale ]=============================================================[=]
static float mkfw_window_get_content_scale(struct mkfw_window *state) {
	if(mkfw_win32_GetDpiForWindow) {
		UINT dpi = mkfw_win32_GetDpiForWindow(PLATFORM(state)->hwnd);
		return (float)dpi / 96.0f;
	}
	int dpi = GetDeviceCaps(PLATFORM(state)->hdc, LOGPIXELSX);
	return (float)dpi / 96.0f;
}

// [=]===^=[ mkfw_window_request_attention ]=============================================================[=]
static void mkfw_window_request_attention(struct mkfw_window *state) {
	FLASHWINFO fi;
	fi.cbSize = sizeof(fi);
	fi.hwnd = PLATFORM(state)->hwnd;
	fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
	fi.uCount = 3;
	fi.dwTimeout = 0;
	FlashWindowEx(&fi);
}

// [=]===^=[ mkfw_wait_events ]==================================================================[=]
static void mkfw_wait_events(struct mkfw_context *ctx) {
	WaitMessage();
	mkfw_poll_events(ctx);
}

// [=]===^=[ mkfw_wait_events_timeout ]===========================================================[=]
static void mkfw_wait_events_timeout(struct mkfw_context *ctx, uint64_t nanoseconds) {
	MsgWaitForMultipleObjectsEx(0, 0, (DWORD)(nanoseconds / 1000000), QS_ALLINPUT, MWMO_INPUTAVAILABLE);
	mkfw_poll_events(ctx);
}

// [=]===^=[ mkfw_window_destroy ]================================================================[=]
static void mkfw_window_destroy(struct mkfw_window *state) {
	if(!state) {
		return;
	}

	mkfw_window_set_mouse_cursor(state, 1);
	mkfw_window_constrain_mouse(state, 0);

	if(PLATFORM(state)->hglrc) {
		wglMakeCurrent(0, 0);
		wglDeleteContext(PLATFORM(state)->hglrc);
	}
	if(PLATFORM(state)->hdc) {
		ReleaseDC(PLATFORM(state)->hwnd, PLATFORM(state)->hdc);
	}
	if(PLATFORM(state)->hwnd) {
		DestroyWindow(PLATFORM(state)->hwnd);
	}

	MSG msg;
	while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Unlink from its context's window list
	struct mkfw_context *ctx = state->context;
	if(ctx) {
		for(uint32_t i = 0; i < ctx->window_count; ++i) {
			if(ctx->windows[i] == state) {
				for(uint32_t j = i + 1; j < ctx->window_count; ++j) {
					ctx->windows[j - 1] = ctx->windows[j];
				}
				ctx->windows[ctx->window_count - 1] = 0;
				--ctx->window_count;
				break;
			}
		}
	}

	free(state->platform);
	free(state);
}

// [=]===^=[ mkfw_shutdown ]======================================================================[=]
static void mkfw_shutdown(struct mkfw_context *ctx) {
	if(!ctx) {
		return;
	}
	while(ctx->window_count > 0) {
		mkfw_window_destroy(ctx->windows[ctx->window_count - 1]);
	}
	free(ctx->platform);
	free(ctx);
}

// [=]===^=[ mkfw_get_time ]=======================================================================[=]
static uint64_t mkfw_get_time(void) {
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return (now.QuadPart * 1000000000ULL) / mkfw_win32_perf_freq.QuadPart;
}

// [=]===^=[ mkfw_sleep ]=========================================================================[=]
static void mkfw_sleep(uint64_t nanoseconds) {
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(LONGLONG)(nanoseconds / 100);

	timer = CreateWaitableTimerA(0, TRUE, 0);
	if(timer == 0) {
		return;
	}

	SetWaitableTimer(timer, &ft, 0, 0, 0, FALSE);
	WaitForSingleObject(timer, INFINITE);

	CloseHandle(timer);
}

// [=]===^=[ mkfw_window_set_mouse_sensitivity ]=========================================================[=]
static void mkfw_window_set_mouse_sensitivity(struct mkfw_window *state, double sensitivity) {
	PLATFORM(state)->mouse_sensitivity = sensitivity;
}

// [=]===^=[ mkfw_window_get_and_clear_mouse_delta ]=====================================================[=]
static void mkfw_window_get_and_clear_mouse_delta(struct mkfw_window *state, int32_t *dx, int32_t *dy) {
	int32_t tdx = (int32_t)PLATFORM(state)->accumulated_dx;
	int32_t tdy = (int32_t)PLATFORM(state)->accumulated_dy;
	PLATFORM(state)->accumulated_dx -= (double)tdx;
	PLATFORM(state)->accumulated_dy -= (double)tdy;
	if(dx) {
		*dx = tdx;
	}
	if(dy) {
		*dy = tdy;
	}
}

// [=]===^=[ mkfw_window_set_cursor_shape ]=============================================================[=]
static void mkfw_window_set_cursor_shape(struct mkfw_window *state, uint32_t cursor) {
	if(cursor >= MKFW_CURSOR_LAST) {
		cursor = MKFW_CURSOR_ARROW;
	}
	PLATFORM(state)->current_cursor = cursor;
	if(!PLATFORM(state)->cursor_hidden) {
		SetCursor(PLATFORM(state)->cursors[cursor]);
	}
}

// [=]===^=[ mkfw_window_get_cursor_position ]===========================================================[=]
static void mkfw_window_get_cursor_position(struct mkfw_window *state, int32_t *x, int32_t *y) {
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(PLATFORM(state)->hwnd, &pt);
	if(x) {
		*x = pt.x;
	}
	if(y) {
		*y = pt.y;
	}
}

// [=]===^=[ mkfw_window_set_cursor_position ]===========================================================[=]
static void mkfw_window_set_cursor_position(struct mkfw_window *state, int32_t x, int32_t y) {
	POINT pt = {x, y};
	ClientToScreen(PLATFORM(state)->hwnd, &pt);
	SetCursorPos(pt.x, pt.y);
}

// [=]===^=[ mkfw_window_set_clipboard_text ]===========================================================[=]
static void mkfw_window_set_clipboard_text(struct mkfw_window *state, const char *text) {
	if(!OpenClipboard(PLATFORM(state)->hwnd)) {
		return;
	}
	EmptyClipboard();
	if(text) {
		int wlen = MultiByteToWideChar(CP_UTF8, 0, text, -1, 0, 0);
		HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wlen * sizeof(wchar_t));
		if(hg) {
			wchar_t *dst = (wchar_t *)GlobalLock(hg);
			MultiByteToWideChar(CP_UTF8, 0, text, -1, dst, wlen);
			GlobalUnlock(hg);
			SetClipboardData(CF_UNICODETEXT, hg);
		}
	}
	CloseClipboard();
}

// [=]===^=[ mkfw_window_get_clipboard_text ]===========================================================[=]
static const char *mkfw_window_get_clipboard_text(struct mkfw_window *state) {
	static char *buf = 0;
	if(!OpenClipboard(PLATFORM(state)->hwnd)) {
		return "";
	}
	HANDLE hg = GetClipboardData(CF_UNICODETEXT);
	if(!hg) {
		CloseClipboard();
		return "";
	}
	const wchar_t *src = (const wchar_t *)GlobalLock(hg);
	if(!src) {
		CloseClipboard();
		return "";
	}
	int len = WideCharToMultiByte(CP_UTF8, 0, src, -1, 0, 0, 0, 0);
	free(buf);
	buf = (char *)malloc(len);
	WideCharToMultiByte(CP_UTF8, 0, src, -1, buf, len, 0, 0);
	GlobalUnlock(hg);
	CloseClipboard();
	return buf;
}

// [=]===^=[ mkfw_window_enable_drop ]===================================================================[=]
static void mkfw_window_enable_drop(struct mkfw_window *state, uint8_t enable) {
	DragAcceptFiles(PLATFORM(state)->hwnd, enable ? TRUE : FALSE);
}

