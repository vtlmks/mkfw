// Copyright (c) 2025-2026 Peter Fors
// SPDX-License-Identifier: MIT
//
// Gamepad mapping database - curated subset of SDL GameController DB
// https://github.com/gabomdq/SDL_GameControllerDB (MIT License)
//
// To use the full community database, replace mkfw_gamecontrollerdb_data
// with the contents of gamecontrollerdb.txt converted to a C string.


#pragma once

/* Standardized gamepad button constants */
enum {
	MKFW_GAMEPAD_A = 0,
	MKFW_GAMEPAD_B,
	MKFW_GAMEPAD_X,
	MKFW_GAMEPAD_Y,
	MKFW_GAMEPAD_LEFT_BUMPER,
	MKFW_GAMEPAD_RIGHT_BUMPER,
	MKFW_GAMEPAD_BACK,
	MKFW_GAMEPAD_START,
	MKFW_GAMEPAD_GUIDE,
	MKFW_GAMEPAD_LEFT_THUMB,
	MKFW_GAMEPAD_RIGHT_THUMB,
	MKFW_GAMEPAD_DPAD_UP,
	MKFW_GAMEPAD_DPAD_DOWN,
	MKFW_GAMEPAD_DPAD_LEFT,
	MKFW_GAMEPAD_DPAD_RIGHT,
	MKFW_GAMEPAD_BUTTON_LAST
};

/* Standardized gamepad axis constants */
enum {
	MKFW_GAMEPAD_AXIS_LEFT_X = 0,
	MKFW_GAMEPAD_AXIS_LEFT_Y,
	MKFW_GAMEPAD_AXIS_RIGHT_X,
	MKFW_GAMEPAD_AXIS_RIGHT_Y,
	MKFW_GAMEPAD_AXIS_LEFT_TRIGGER,
	MKFW_GAMEPAD_AXIS_RIGHT_TRIGGER,
	MKFW_GAMEPAD_AXIS_LAST
};

/* Curated controller mapping database (SDL GameController DB format)
   Each line: GUID,Name,mapping1:source1,mapping2:source2,...,platform:Platform,

   Mapping targets: a,b,x,y,back,start,guide,leftshoulder,rightshoulder,
                    leftstick,rightstick,dpup,dpdown,dpleft,dpright,
                    leftx,lefty,rightx,righty,lefttrigger,righttrigger

   Source types: b0 = button 0, a0 = axis 0, h0.1 = hat 0 bit 1
   Hat bits: 1=up, 2=right, 4=down, 8=left */

static const char mkfw_gamecontrollerdb_data[] =
	/* Xbox 360 Controller */
	"030000005e0400008e02000014010000,Xbox 360 Controller,a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,guide:b8,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5,platform:Linux,\n"
	"030000005e0400008e02000000000000,Xbox 360 Controller,a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,guide:b10,leftshoulder:b4,rightshoulder:b5,leftstick:b8,rightstick:b9,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5,platform:Linux,\n"
	/* Xbox 360 Wireless */
	"030000005e0400001907000000010000,Xbox 360 Wireless Controller,a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,guide:b8,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5,platform:Linux,\n"
	/* Xbox 360 Wireless Receiver */
	"030000005e040000a102000000010000,Xbox 360 Wireless Receiver,a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,guide:b8,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5,platform:Linux,\n"
	/* Xbox One Controller */
	"030000005e040000d102000001010000,Xbox One Controller,a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,guide:b8,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5,platform:Linux,\n"
	"030000005e040000ea02000001030000,Xbox One Controller,a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,guide:b8,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5,platform:Linux,\n"
	/* Xbox Series X|S Controller */
	"030000005e040000130b000005050000,Xbox Series X Controller,a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,guide:b8,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5,platform:Linux,\n"
	"030000005e040000e002000003090000,Xbox One S Controller,a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,guide:b8,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5,platform:Linux,\n"
	/* PS4 DualShock 4 (USB) */
	"030000004c050000c405000011010000,PS4 Controller,a:b1,b:b2,x:b0,y:b3,back:b8,start:b9,guide:b12,leftshoulder:b4,rightshoulder:b5,leftstick:b10,rightstick:b11,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a5,lefttrigger:a3,righttrigger:a4,platform:Linux,\n"
	/* PS4 DualShock 4 (Bluetooth) */
	"030000004c050000cc09000011010000,PS4 Controller,a:b1,b:b2,x:b0,y:b3,back:b8,start:b9,guide:b12,leftshoulder:b4,rightshoulder:b5,leftstick:b10,rightstick:b11,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a5,lefttrigger:a3,righttrigger:a4,platform:Linux,\n"
	/* PS4 DualShock 4 v2 */
	"030000004c050000a00b000011010000,PS4 Controller v2,a:b1,b:b2,x:b0,y:b3,back:b8,start:b9,guide:b12,leftshoulder:b4,rightshoulder:b5,leftstick:b10,rightstick:b11,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a5,lefttrigger:a3,righttrigger:a4,platform:Linux,\n"
	/* PS5 DualSense */
	"030000004c050000e60c000011010000,PS5 Controller,a:b1,b:b2,x:b0,y:b3,back:b8,start:b9,guide:b12,leftshoulder:b4,rightshoulder:b5,leftstick:b10,rightstick:b11,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a5,lefttrigger:a3,righttrigger:a4,platform:Linux,\n"
	/* PS5 DualSense Edge */
	"030000004c050000f20d000011010000,PS5 Controller Edge,a:b1,b:b2,x:b0,y:b3,back:b8,start:b9,guide:b12,leftshoulder:b4,rightshoulder:b5,leftstick:b10,rightstick:b11,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a5,lefttrigger:a3,righttrigger:a4,platform:Linux,\n"
	/* PS3 DualShock 3 / Sixaxis */
	"030000004c0500006802000011010000,PS3 Controller,a:b14,b:b13,x:b15,y:b12,back:b0,start:b3,guide:b16,leftshoulder:b10,rightshoulder:b11,leftstick:b1,rightstick:b2,dpup:b4,dpdown:b6,dpleft:b7,dpright:b5,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a12,righttrigger:a13,platform:Linux,\n"
	/* Nintendo Switch Pro Controller (USB) */
	"030000007e0500000920000011010000,Nintendo Switch Pro Controller,a:b0,b:b1,x:b2,y:b3,back:b8,start:b9,guide:b12,leftshoulder:b4,rightshoulder:b5,leftstick:b10,rightstick:b11,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,platform:Linux,\n"
	/* Logitech F310 */
	"030000006d0400001dc2000014400000,Logitech F310,a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,guide:b8,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5,platform:Linux,\n"
	/* Logitech F710 */
	"030000006d0400001fc2000005030000,Logitech F710,a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,guide:b8,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5,platform:Linux,\n"
	/* 8BitDo Pro 2 */
	"03000000c82d00000631000014010000,8BitDo Pro 2,a:b1,b:b0,x:b4,y:b3,back:b10,start:b11,guide:b12,leftshoulder:b6,rightshoulder:b7,leftstick:b13,rightstick:b14,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a5,righttrigger:a4,platform:Linux,\n"
	/* 8BitDo SN30 Pro (USB) */
	"03000000c82d00000160000011010000,8BitDo SN30 Pro,a:b1,b:b0,x:b4,y:b3,back:b10,start:b11,guide:b12,leftshoulder:b6,rightshoulder:b7,leftstick:b13,rightstick:b14,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:b8,righttrigger:b9,platform:Linux,\n"
	/* 8BitDo SN30 Pro+ */
	"03000000c82d00000260000011010000,8BitDo SN30 Pro+,a:b1,b:b0,x:b4,y:b3,back:b10,start:b11,guide:b12,leftshoulder:b6,rightshoulder:b7,leftstick:b13,rightstick:b14,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a5,righttrigger:a4,platform:Linux,\n"
	/* Steam Controller */
	"03000000de280000ff11000001000000,Steam Virtual Gamepad,a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,guide:b8,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5,platform:Linux,\n"
	/* Google Stadia Controller */
	"03000000d11800000094000011010000,Stadia Controller,a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,guide:b8,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a5,righttrigger:a4,platform:Linux,\n"
	/* PowerA Xbox Controller */
	"030000005e040000ea02000001030000,PowerA Xbox Controller,a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,guide:b8,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5,platform:Linux,\n"
	/* Generic XInput on Linux via xpad/xone/xpadneo */
	"030000005e0400008e02000010010000,Xbox 360 Controller (xpad),a:b0,b:b1,x:b2,y:b3,back:b6,start:b7,guide:b8,leftshoulder:b4,rightshoulder:b5,leftstick:b9,rightstick:b10,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,leftx:a0,lefty:a1,rightx:a3,righty:a4,lefttrigger:a2,righttrigger:a5,platform:Linux,\n"
	"\n";
