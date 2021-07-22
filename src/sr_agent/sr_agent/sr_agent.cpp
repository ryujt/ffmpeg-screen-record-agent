#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <global.hpp>
#include "MainForm.hpp"
#include <windows.h>
#include <ryulib/debug_tools.hpp>

using namespace std;

HANDLE mutex_ = nullptr;

class MyApp : public wxApp {
public:
	virtual bool OnInit()
	{
		mutex_ = CreateMutex(nullptr, TRUE, L"Screen_Recording_Agent");
		if (ERROR_ALREADY_EXISTS == GetLastError()) {
			CloseHandle(mutex_);
			return false;
		}

		//main_form_ = new MainForm(
		//	NULL, 0, "Screen Recorder Agent", wxPoint(50, 50), wxSize(480, 320), 
		//	wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL | wxFRAME_NO_TASKBAR);
		main_form_ = new MainForm(
			NULL, 0, "Screen Recorder Agent", wxPoint(50, 50), wxSize(480, 320),
			wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);

		//frame->Show(false);
		main_form_->Show(true);

		return true;
	}

	virtual int OnExit()
	{
		CloseHandle(mutex_);
		return 0;
	}

	virtual bool OnExceptionInMainLoop() override
	{
		try { throw; } catch (std::exception& e) {
			DebugOutput::trace(e.what());
			main_form_->sendErrorMessage(ERROR_CREATE_UNKNOWN);
		}
		return true;
	}
private:
	MainForm* main_form_ = nullptr;
};

wxIMPLEMENT_APP(MyApp);