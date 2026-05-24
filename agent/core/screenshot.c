#include "screenshot.h"
#include <windows.h>
#include <stdio.h>

void capture_screenshot(const char *filename) {
    HWND  hwnd      = GetDesktopWindow();
    HDC   hdc_src   = GetDC(hwnd);
    HDC   hdc_mem   = CreateCompatibleDC(hdc_src);
    RECT  rect;

    GetWindowRect(hwnd, &rect);
    int width  = rect.right  - rect.left;
    int height = rect.bottom - rect.top;

    HBITMAP hbmp = CreateCompatibleBitmap(hdc_src, width, height);
    SelectObject(hdc_mem, hbmp);
    BitBlt(hdc_mem, 0, 0, width, height, hdc_src, 0, 0, SRCCOPY);

    BITMAPFILEHEADER file_hdr = { 0 };
    BITMAPINFOHEADER info_hdr = { 0 };

    info_hdr.biSize        = sizeof(BITMAPINFOHEADER);
    info_hdr.biWidth       = width;
    info_hdr.biHeight      = -height; /* top-down */
    info_hdr.biPlanes      = 1;
    info_hdr.biBitCount    = 32;
    info_hdr.biCompression = BI_RGB;

    DWORD row_size  = ((width * 32 + 31) / 32) * 4;
    DWORD bmp_size  = row_size * height;
    DWORD file_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmp_size;

    file_hdr.bfType    = 0x4D42;
    file_hdr.bfSize    = file_size;
    file_hdr.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    HANDLE buf = GlobalAlloc(GHND, bmp_size);
    char  *px  = (char *)GlobalLock(buf);
    GetDIBits(hdc_src, hbmp, 0, height, px, (BITMAPINFO *)&info_hdr, DIB_RGB_COLORS);

    DWORD written;
    HANDLE hfile = CreateFileA(filename, GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(hfile, &file_hdr, sizeof(file_hdr), &written, NULL);
    WriteFile(hfile, &info_hdr, sizeof(info_hdr), &written, NULL);
    WriteFile(hfile, px, bmp_size, &written, NULL);

    CloseHandle(hfile);
    GlobalUnlock(buf);
    GlobalFree(buf);
    DeleteObject(hbmp);
    DeleteDC(hdc_mem);
    ReleaseDC(hwnd, hdc_src);
}
