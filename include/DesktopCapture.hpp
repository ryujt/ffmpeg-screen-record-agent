#pragma once

#include <dwmapi.h>
#include <shellscalingapi.h>
#include <boost/scope_exit.hpp>
#include <Global.hpp>
#include <ryulib/base.hpp>
#include <ryulib/Scheduler.hpp>
#include <ryulib/debug_tools.hpp>

#pragma comment(lib, "dwmapi.lib")

typedef struct DesktopCaptureOption {
	int target_handle = -1;
	int fps = 25;
	int left = 0;
	int top = 0;
	int width = 0;
	int height = 0;
	bool with_cursor = true;

	int getBitmapWidth()
	{
		int result = width;
		if ((result % BITMAP_CELL_SIZE) != 0) result = result + (BITMAP_CELL_SIZE - (result % BITMAP_CELL_SIZE));
		return result;
	}

	int getBitmapHeight()
	{
		int result = height;
		if ((result % 2) != 0) result = result + (2 - (result % 2));
		return result;
	}

	int getLeftMargin()
	{
		if (target_handle == -1) return 0;

		tagRECT rect_dwm;
		DwmGetWindowAttribute((HWND) target_handle, DWMWA_EXTENDED_FRAME_BOUNDS, &rect_dwm, sizeof(rect_dwm));

		tagRECT rect;
		GetWindowRect((HWND) target_handle, &rect);

		return rect_dwm.left - rect.left;
	};

	int getTopMargin()
	{
		if (target_handle == -1) return 0;

		tagRECT rect_dwm;
		DwmGetWindowAttribute((HWND) target_handle, DWMWA_EXTENDED_FRAME_BOUNDS, &rect_dwm, sizeof(rect_dwm));

		tagRECT rect;
		GetWindowRect((HWND) target_handle, &rect);

		return rect_dwm.top - rect.top;
	};

	int getWindowLeft()
	{
		if (target_handle == -1) return 0;

		tagRECT rect;
		GetWindowRect((HWND) target_handle, &rect);

		return rect.left;
	};

	int getWindowTop()
	{
		if (target_handle == -1) return 0;

		tagRECT rect;
		GetWindowRect((HWND) target_handle, &rect);

		return rect.top;
	};

	int getBitmapSize() { return getBitmapWidth() * getBitmapHeight() * BITMAP_PIXEL_SIZE; }

	bool isIconic()
	{
		if (target_handle == -1) return false;

		tagRECT rect;
		GetWindowRect((HWND) target_handle, &rect);
		// -32000이면 최소화 된 상태 안전하게 30000으로 확인
		return rect.left <= -30000;
	}
};

class DesktopCapture {
private:
	const static int TASK_START = 1;
	const static int TASK_STOP = 2;

public:
	DesktopCapture()
	{
		scheduler_.setOnTask([&](int task, const string text, const void* data, int size, int tag) {
			switch (task) {
				case TASK_START: do_start(data); break;
				case TASK_STOP: do_stop(); break;
			}
		});

		scheduler_.setOnRepeat([&]() {
			do_capture();
			if (option_.fps < 1) scheduler_.sleep(50);
			else scheduler_.sleep(1000 / option_.fps);
		});
	}

	~DesktopCapture()
	{
		if (bitmap_backup_ != nullptr) delete bitmap_backup_;
	}

	void start(DesktopCaptureOption option)
	{
		DesktopCaptureOption* opt = new DesktopCaptureOption();
		*opt = option;
		scheduler_.add(TASK_START, (void*) opt);
	}

	void stop()
	{
		scheduler_.add(TASK_STOP);
	}

	void terminate()
	{
		scheduler_.terminate();
	}

	void terminateNow()
	{
		scheduler_.terminateNow();
	}

	void terminateAndWait()
	{
		scheduler_.terminateAndWait();
	}

	void setOnData(MemoryEvent event) { on_data_ = event; }

private:
	Scheduler scheduler_;
	DesktopCaptureOption option_;

	Memory* bitmap_backup_ = nullptr; 

	MemoryEvent on_data_ = nullptr;

	void do_start(const void* data)
	{
		option_ = *((DesktopCaptureOption*) data);

		if (bitmap_backup_ != nullptr) delete bitmap_backup_;
		bitmap_backup_ = new Memory(option_.getBitmapSize());

		scheduler_.start();
	}

	void do_stop()
	{
		scheduler_.stop();
		if (bitmap_backup_ != nullptr) {
			delete bitmap_backup_;
			bitmap_backup_ = nullptr;
		}
	}

	void do_capture()
	{
		BITMAPINFO bmpInfo;
		HDC hMemDC = NULL;
		HDC hScrDC = NULL;
		HDC hDesktopDC = GetDC(NULL);
		HBITMAP hBit = NULL;
		HBITMAP hOldBitmap = NULL;

		BOOST_SCOPE_EXIT(&hMemDC, &hScrDC, &hDesktopDC, &hBit, &hOldBitmap)
		{
			if (hMemDC != NULL) {
				if (hOldBitmap != NULL) SelectObject(hMemDC, hOldBitmap);
				DeleteDC(hMemDC);
			}

			if (hBit != NULL) DeleteObject(hBit);
			if (hScrDC != NULL) DeleteDC(hScrDC);
			if (hDesktopDC != NULL) DeleteDC(hDesktopDC);
		}
		BOOST_SCOPE_EXIT_END;

		if (option_.target_handle != -1) hScrDC = GetWindowDC((HWND)option_.target_handle);
		else hScrDC = GetDC(0);
		if (hScrDC == NULL) {
			DebugOutput::trace("GetDC() Error %d", GetLastError());
			return;
		}

		hMemDC = CreateCompatibleDC(hScrDC);
		if (hMemDC == NULL) {
			DebugOutput::trace("CreateCompatibleDC() Error %d", GetLastError());
			return;
		}

		hBit = CreateCompatibleBitmap(hScrDC, option_.getBitmapWidth(), option_.getBitmapHeight());
		if (hBit == NULL) {
			DebugOutput::trace("CreateCompatibleBitmap() Error %d", GetLastError());
			return;
		}

		hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBit);

		BOOL result = BitBlt(hMemDC, 0, 0, option_.width, option_.height, hScrDC, option_.left, option_.top, SRCCOPY);
		if (result == FALSE) {
			DebugOutput::trace("BitBlt() Error %d", GetLastError());
			return;
		}

		if (option_.with_cursor) {
			POINT curPoint;
			GetCursorPos(&curPoint);

			curPoint.x -= GetSystemMetrics(SM_XVIRTUALSCREEN);
			curPoint.y -= GetSystemMetrics(SM_YVIRTUALSCREEN);

			// 화면 비율 적용하기
			int x = GetSystemMetrics(SM_CXSCREEN);
			int y = GetSystemMetrics(SM_CYSCREEN);
			int h = GetDeviceCaps(hDesktopDC, DESKTOPHORZRES);
			int v = GetDeviceCaps(hDesktopDC, DESKTOPVERTRES);
			float dx = h;
			float dy = v;
			dx = dx / x;
			dy = dy / y;
			curPoint.x = ceil(curPoint.x * dx);
			curPoint.y = ceil(curPoint.y * dy);

			{
				CURSORINFO cursorInfo;
				cursorInfo.cbSize = sizeof(CURSORINFO);

				if (GetCursorInfo(&cursorInfo)) {
					ICONINFO iconInfo;
					if (GetIconInfo(cursorInfo.hCursor, &iconInfo)) {
						curPoint.x = curPoint.x - iconInfo.xHotspot;
						curPoint.y = curPoint.y - iconInfo.yHotspot;
						if (iconInfo.hbmMask != NULL) DeleteObject(iconInfo.hbmMask);
						if (iconInfo.hbmColor != NULL) DeleteObject(iconInfo.hbmColor);
					}
					DrawIconEx(hMemDC, curPoint.x - option_.left - option_.getWindowLeft(), curPoint.y - option_.top - option_.getWindowTop(), cursorInfo.hCursor, 0, 0, 0, NULL, DI_NORMAL);
				}
			}
		}

		bmpInfo.bmiHeader.biBitCount = 0;
		bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmpInfo.bmiHeader.biWidth = option_.getBitmapWidth();
		bmpInfo.bmiHeader.biHeight = -option_.getBitmapHeight();
		bmpInfo.bmiHeader.biPlanes = 1;
		bmpInfo.bmiHeader.biBitCount = BITMAP_PIXEL_SIZE * 8;
		bmpInfo.bmiHeader.biCompression = BI_RGB;
		bmpInfo.bmiHeader.biSizeImage = 0;
		bmpInfo.bmiHeader.biXPelsPerMeter = 0;
		bmpInfo.bmiHeader.biYPelsPerMeter = 0;
		bmpInfo.bmiHeader.biClrUsed = 0;
		bmpInfo.bmiHeader.biClrImportant = 0;

		if (on_data_ != nullptr) {
			Memory* data = new Memory(option_.getBitmapSize());
			if (option_.isIconic() == false) {
				GetDIBits(hMemDC, hBit, 0, option_.getBitmapHeight(), (PBYTE) data->getData(), &bmpInfo, DIB_RGB_COLORS);
				bitmap_backup_->loadMemory(data->getData(), data->getSize());
			} else {
				data->loadMemory(bitmap_backup_->getData(), bitmap_backup_->getSize());
			}
			on_data_(this, data);
		}
	}
};
