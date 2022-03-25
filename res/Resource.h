/*
MIT License

Copyright (c) 2022 sbplat

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#define IDI_APPICON 103

#define VERSION_MAIN 1
#define VERSION_MAIN2 0
#define VERSION_SUB 0
#define VERSION_SUB2 0

#define VER_COPYRIGHT_STR "(C) 2022 sbplat"
#define VER_TRADEMARK_STR "Stats Overlay"
#define VER_COMPANY_STR "sbplat"

#define _TOSTRING(x) #x
#define TOSTRING(x) _TOSTRING(x)

#define VER_FILE_VERSION VERSION_MAIN, VERSION_MAIN2, VERSION_SUB, VERSION_SUB2
#define VER_FILE_VERSION_STR TOSTRING(VERSION_MAIN) "." TOSTRING(VERSION_MAIN2) "." TOSTRING(VERSION_SUB) "." TOSTRING(VERSION_SUB2)
#define VER_PRODUCT_VERSION VER_FILE_VERSION
#define VER_PRODUCT_VERSION_STR VER_FILE_VERSION_STR
#ifdef _DEBUG
    #define VER_VER_DEBUG VS_FF_DEBUG | VS_FF_PRERELEASE
#else
    #define VER_VER_DEBUG VS_FF_PRERELEASE
#endif
#define VER_FILEOS VOS__WINDOWS32
#define VER_FILEFLAGS VER_VER_DEBUG
#define VER_FILETYPE VFT_APP
#define VER_FILESUBTYPE VFT2_UNKNOWN
