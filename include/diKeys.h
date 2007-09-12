/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no
  
  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef _diKeys_h
#define _diKeys_h

/**
   \brief Keycodes for internal use
*/

enum KeyType {
  key_unknown= 0,
  
  key_Escape = 0x1000,            // misc keys
  key_Tab = 0x1001,
  key_Backtab = 0x1002, key_BackTab = key_Backtab,
  key_Backspace = 0x1003, key_BackSpace = key_Backspace,
  key_Return = 0x1004,
  key_Enter = 0x1005,
  key_Insert = 0x1006,
  key_Delete = 0x1007,
  key_Pause = 0x1008,
  key_Print = 0x1009,
  key_SysReq = 0x100a,
  key_Home = 0x1010,              // cursor movement
  key_End = 0x1011,
  key_Left = 0x1012,
  key_Up = 0x1013,
  key_Right = 0x1014,
  key_Down = 0x1015,
  key_Prior = 0x1016, key_PageUp = key_Prior,
  key_Next = 0x1017, key_PageDown = key_Next,
  key_Shift = 0x1020,             // modifiers
  key_Control = 0x1021,
  key_Meta = 0x1022,
  key_Alt = 0x1023,
  key_CapsLock = 0x1024,
  key_NumLock = 0x1025,
  key_ScrollLock = 0x1026,
  key_F1 = 0x1030,                // function keys
  key_F2 = 0x1031,
  key_F3 = 0x1032,
  key_F4 = 0x1033,
  key_F5 = 0x1034,
  key_F6 = 0x1035,
  key_F7 = 0x1036,
  key_F8 = 0x1037,
  key_F9 = 0x1038,
  key_F10 = 0x1039,
  key_F11 = 0x103a,
  key_F12 = 0x103b,
  key_F13 = 0x103c,
  key_F14 = 0x103d,
  key_F15 = 0x103e,
  key_F16 = 0x103f,
  key_F17 = 0x1040,
  key_F18 = 0x1041,
  key_F19 = 0x1042,
  key_F20 = 0x1043,
  key_F21 = 0x1044,
  key_F22 = 0x1045,
  key_F23 = 0x1046,
  key_F24 = 0x1047,
  key_F25 = 0x1048,               // F25 .. F35 only on X11
  key_F26 = 0x1049,
  key_F27 = 0x104a,
  key_F28 = 0x104b,
  key_F29 = 0x104c,
  key_F30 = 0x104d,
  key_F31 = 0x104e,
  key_F32 = 0x104f,
  key_F33 = 0x1050,
  key_F34 = 0x1051,
  key_F35 = 0x1052,
  key_Super_L = 0x1053,           // extra keys
  key_Super_R = 0x1054,
  key_Menu = 0x1055,
  key_Hyper_L = 0x1056,
  key_Hyper_R = 0x1057,
  key_Space = 0x20,               // 7 bit printable ASCII
  key_Any = key_Space,
  key_Exclam = 0x21,
  key_QuoteDbl = 0x22,
  key_NumberSign = 0x23,
  key_Dollar = 0x24,
  key_Percent = 0x25,
  key_Ampersand = 0x26,
  key_Apostrophe = 0x27,
  key_ParenLeft = 0x28,
  key_ParenRight = 0x29,
  key_Asterisk = 0x2a,
  key_Plus = 0x2b,
  key_Comma = 0x2c,
  key_Minus = 0x2d,
  key_Period = 0x2e,
  key_Slash = 0x2f,
  key_0 = 0x30,
  key_1 = 0x31,
  key_2 = 0x32,
  key_3 = 0x33,
  key_4 = 0x34,
  key_5 = 0x35,
  key_6 = 0x36,
  key_7 = 0x37,
  key_8 = 0x38,
  key_9 = 0x39,
  key_Colon = 0x3a,
  key_Semicolon = 0x3b,
  key_Less = 0x3c,
  key_Equal = 0x3d,
  key_Greater = 0x3e,
  key_Question = 0x3f,
  key_At = 0x40,
  key_A = 0x41,
  key_B = 0x42,
  key_C = 0x43,
  key_D = 0x44,
  key_E = 0x45,
  key_F = 0x46,
  key_G = 0x47,
  key_H = 0x48,
  key_I = 0x49,
  key_J = 0x4a,
  key_K = 0x4b,
  key_L = 0x4c,
  key_M = 0x4d,
  key_N = 0x4e,
  key_O = 0x4f,
  key_P = 0x50,
  key_Q = 0x51,
  key_R = 0x52,
  key_S = 0x53,
  key_T = 0x54,
  key_U = 0x55,
  key_V = 0x56,
  key_W = 0x57,
  key_X = 0x58,
  key_Y = 0x59,
  key_Z = 0x5a,
  key_BracketLeft = 0x5b,
  key_Backslash = 0x5c,
  key_BracketRight = 0x5d,
  key_AsciiCircum = 0x5e,
  key_Underscore = 0x5f,
  key_QuoteLeft = 0x60,
  key_BraceLeft = 0x7b,
  key_Bar = 0x7c,
  key_BraceRight = 0x7d,
  key_AsciiTilde = 0x7e,

  // Latin 1 codes adapted from X: keysymdef.h,v 1.21 94/08/28 16:17:06
  
  key_nobreakspace = 0x0a0,
  key_exclamdown = 0x0a1,
  key_cent = 0x0a2,
  key_sterling = 0x0a3,
  key_currency = 0x0a4,
  key_yen = 0x0a5,
  key_brokenbar = 0x0a6,
  key_section = 0x0a7,
  key_diaeresis = 0x0a8,
  key_copyright = 0x0a9,
  key_ordfeminine = 0x0aa,
  key_guillemotleft = 0x0ab,      // left angle quotation mark
  key_notsign = 0x0ac,
  key_hyphen = 0x0ad,
  key_registered = 0x0ae,
  key_macron = 0x0af,
  key_degree = 0x0b0,
  key_plusminus = 0x0b1,
  key_twosuperior = 0x0b2,
  key_threesuperior = 0x0b3,
  key_acute = 0x0b4,
  key_mu = 0x0b5,
  key_paragraph = 0x0b6,
  key_periodcentered = 0x0b7,
  key_cedilla = 0x0b8,
  key_onesuperior = 0x0b9,
  key_masculine = 0x0ba,
  key_guillemotright = 0x0bb,     // right angle quotation mark
  key_onequarter = 0x0bc,
  key_onehalf = 0x0bd,
  key_threequarters = 0x0be,
  key_questiondown = 0x0bf,
  key_Agrave = 0x0c0,
  key_Aacute = 0x0c1,
  key_Acircumflex = 0x0c2,
  key_Atilde = 0x0c3,
  key_Adiaeresis = 0x0c4,
  key_Aring = 0x0c5,
  key_AE = 0x0c6,
  key_Ccedilla = 0x0c7,
  key_Egrave = 0x0c8,
  key_Eacute = 0x0c9,
  key_Ecircumflex = 0x0ca,
  key_Ediaeresis = 0x0cb,
  key_Igrave = 0x0cc,
  key_Iacute = 0x0cd,
  key_Icircumflex = 0x0ce,
  key_Idiaeresis = 0x0cf,
  key_ETH = 0x0d0,
  key_Ntilde = 0x0d1,
  key_Ograve = 0x0d2,
  key_Oacute = 0x0d3,
  key_Ocircumflex = 0x0d4,
  key_Otilde = 0x0d5,
  key_Odiaeresis = 0x0d6,
  key_multiply = 0x0d7,
  key_Ooblique = 0x0d8,
  key_Ugrave = 0x0d9,
  key_Uacute = 0x0da,
  key_Ucircumflex = 0x0db,
  key_Udiaeresis = 0x0dc,
  key_Yacute = 0x0dd,
  key_THORN = 0x0de,
  key_ssharp = 0x0df,
  key_agrave = 0x0e0,
  key_aacute = 0x0e1,
  key_acircumflex = 0x0e2,
  key_atilde = 0x0e3,
  key_adiaeresis = 0x0e4,
  key_aring = 0x0e5,
  key_ae = 0x0e6,
  key_ccedilla = 0x0e7,
  key_egrave = 0x0e8,
  key_eacute = 0x0e9,
  key_ecircumflex = 0x0ea,
  key_ediaeresis = 0x0eb,
  key_igrave = 0x0ec,
  key_iacute = 0x0ed,
  key_icircumflex = 0x0ee,
  key_idiaeresis = 0x0ef,
  key_eth = 0x0f0,
  key_ntilde = 0x0f1,
  key_ograve = 0x0f2,
  key_oacute = 0x0f3,
  key_ocircumflex = 0x0f4,
  key_otilde = 0x0f5,
  key_odiaeresis = 0x0f6,
  key_division = 0x0f7,
  key_oslash = 0x0f8,
  key_ugrave = 0x0f9,
  key_uacute = 0x0fa,
  key_ucircumflex = 0x0fb,
  key_udiaeresis = 0x0fc,
  key_yacute = 0x0fd,
  key_thorn = 0x0fe,
  key_ydiaeresis = 0x0ff
};


#endif
