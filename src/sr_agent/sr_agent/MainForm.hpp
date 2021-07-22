#pragma once

#include <Global.hpp>
#include <DesktopRecorder.hpp>
#include <StartOption.hpp>
#include <ryulib/debug_tools.hpp>
#include <ryulib/strg.hpp>
#include <ryulib/Worker.hpp>

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stc/stc.h>
#include <wx/sizer.h>
#include <wx/frame.h>

class MainForm : public wxFrame {
private:
	const static int TASK_START = 1;
	const static int TASK_STOP = 2;
	const static int TASK_PAUSE = 3;
	const static int TASK_RESUME = 4;

public:
	MainForm(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
	{
		this->SetSizeHints(wxDefaultSize, wxDefaultSize);

		wxBoxSizer* bSizer;
		bSizer = new wxBoxSizer(wxVERTICAL);

		m_staticText7 = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
		m_staticText7->Wrap(-1);
		bSizer->Add(m_staticText7, 0, wxALL, 5);

		txtFrames = new wxStaticText(this, wxID_ANY, wxT("Frames in buffer: %d"), wxDefaultPosition, wxDefaultSize, 0);
		txtFrames->Wrap(-1);
		bSizer->Add(txtFrames, 0, wxALL, 5);

		txtLength = new wxStaticText(this, wxID_ANY, wxT("build version: 17"), wxDefaultPosition, wxDefaultSize, 0);
		txtLength->Wrap(-1);
		bSizer->Add(txtLength, 0, wxALL, 5);

		moOptions = new wxStyledTextCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString);
		moOptions->SetUseTabs(true);
		moOptions->SetTabWidth(4);
		moOptions->SetIndent(4);
		moOptions->SetTabIndents(true);
		moOptions->SetBackSpaceUnIndents(true);
		moOptions->SetViewEOL(false);
		moOptions->SetViewWhiteSpace(false);
		moOptions->SetMarginWidth(2, 0);
		moOptions->SetIndentationGuides(true);
		moOptions->SetMarginType(1, wxSTC_MARGIN_SYMBOL);
		moOptions->SetMarginMask(1, wxSTC_MASK_FOLDERS);
		moOptions->SetMarginWidth(1, 16);
		moOptions->SetMarginSensitive(1, true);
		moOptions->SetProperty(wxT("fold"), wxT("1"));
		moOptions->SetFoldFlags(wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);
		moOptions->SetMarginType(0, wxSTC_MARGIN_NUMBER);
		moOptions->SetMarginWidth(0, moOptions->TextWidth(wxSTC_STYLE_LINENUMBER, wxT("_99999")));
		moOptions->MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS);
		moOptions->MarkerSetBackground(wxSTC_MARKNUM_FOLDER, wxColour(wxT("BLACK")));
		moOptions->MarkerSetForeground(wxSTC_MARKNUM_FOLDER, wxColour(wxT("WHITE")));
		moOptions->MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS);
		moOptions->MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPEN, wxColour(wxT("BLACK")));
		moOptions->MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPEN, wxColour(wxT("WHITE")));
		moOptions->MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY);
		moOptions->MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS);
		moOptions->MarkerSetBackground(wxSTC_MARKNUM_FOLDEREND, wxColour(wxT("BLACK")));
		moOptions->MarkerSetForeground(wxSTC_MARKNUM_FOLDEREND, wxColour(wxT("WHITE")));
		moOptions->MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS);
		moOptions->MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPENMID, wxColour(wxT("BLACK")));
		moOptions->MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPENMID, wxColour(wxT("WHITE")));
		moOptions->MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY);
		moOptions->MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY);
		moOptions->SetSelBackground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
		moOptions->SetSelForeground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
		bSizer->Add(moOptions, 1, wxEXPAND | wxALL, 5);

		this->SetSizer(bSizer);
		this->Layout();
		this->Centre(wxBOTH);

		worker_.setOnTask([](int task, const string text, const void* data, int size, int tag) {
			DesktopRecorder* recorder = (DesktopRecorder*) data;
			recorder->stop();
			recorder->terminateAndWait();
			delete recorder;
		});
	}

	~MainForm()
	{
	}

	void sendErrorMessage(int error_code)
	{
		PostMessage(target_handle_, WM_RECORD_AGENT_ERROR, error_code, 0);
	}

protected:
	wxStaticText* m_staticText7;
	wxStaticText* txtFrames;
	wxStaticText* txtLength;
	wxStyledTextCtrl* moOptions;

	WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) override
	{
		int result = 0;

		switch (nMsg) {
			case WM_COPYDATA: result = wm_start(wParam, lParam); break;
			case WM_RECORD_AGENT_STOP: result = wm_stop(wParam, lParam); break;
			case WM_RECORD_AGENT_TERMINATE: result = wm_terminate(wParam, lParam); break;
			case WM_RECORD_AGENT_PAUSE: result = wm_pause(wParam, lParam); break;
			case WM_RECORD_AGENT_PING: result = wm_ping(wParam, lParam);break;
			case WM_RECORD_AGENT_GET_STATUS: result = wm_get_status(wParam, lParam); break;
			default: return wxFrame::MSWWindowProc(nMsg, wParam, lParam);
		}

		return result;
	}

private:
	DesktopRecorder* recorder_ = nullptr;
	Worker worker_;

	bool is_terminated_ = false;
	HWND target_handle_ = 0;

	IntegerEvent on_packets_in_buffer_ = [&](const void* sender, int packets) {
		txtFrames->SetLabelText(format_string("Frames in buffer: %d", packets));
		PostMessage(target_handle_, WM_RECORD_AGENT_FRAMES_IN_BUFFER, packets, 0);
	};

	NotifyEvent on_completed_ = [&](const void* sender) {
		txtFrames->SetLabelText("Frames in buffer: 0");
		PostMessage(target_handle_, WM_RECORD_AGENT_FRAMES_IN_BUFFER, 0, 0);
		PostMessage(target_handle_, WM_RECORD_AGENT_COMPLETED, 0, 0);

		recorder_->setOnError(nullptr);
		recorder_->setOnPacketsInBuffer(nullptr);
		recorder_->setOnCompleted(nullptr);

		worker_.add(0, recorder_);
		recorder_ = nullptr;

		if (is_terminated_) Close(true);
	};

	IntegerEvent on_error_ = [&](const void* sneder, int error_code) {
		if (error_code == ERROR_PING_TIMEOUT) {
			// TODO: 메시지 한 번만 가도록
			sendErrorMessage(error_code);
			PostMessage(this->GetHandle(), WM_RECORD_AGENT_TERMINATE, 0, 0);
		} else {
			sendErrorMessage(error_code);
			PostMessage(this->GetHandle(), WM_RECORD_AGENT_STOP, 0, 0);
		}
	};

	int wm_start(WXWPARAM wParam, WXLPARAM lParam)
	{
		target_handle_ = (HWND) wParam;

		COPYDATASTRUCT* data = (COPYDATASTRUCT*) lParam;
		int data_size = data->cbData;
		if (data_size <= 0) return ERROR_CREATE_ENCODER;

		if (recorder_ != nullptr) return ERROR_RECODER_IS_STARTED;

		char* text = new char[data_size];
		memcpy(text, data->lpData, data_size);
		moOptions->ClearAll();
		moOptions->AddText(text);
		if (StartOption::getObj()->parseJson(text) == false) {
			sendErrorMessage(ERROR_PARSING_JSON);
		}
		delete[] text;

		recorder_ = new DesktopRecorder();

		int result = recorder_->start();
		if (result != 0) {
			worker_.add(0, recorder_);
			recorder_ = nullptr;
			return result;
		}

		recorder_->setOnPacketsInBuffer(on_packets_in_buffer_);
		recorder_->setOnCompleted(on_completed_);
		recorder_->setOnError(on_error_);

		return 0;
	}

	int wm_stop(WXWPARAM wParam, WXLPARAM lParam)
	{
		if (recorder_ == nullptr) return -1;
		recorder_->stop();
		return 0;
	}

	int wm_terminate(WXWPARAM wParam, WXLPARAM lParam)
	{
		if (is_terminated_) return 0;
		is_terminated_ = true;
		recorder_->stop();

		return 0;
	}

	int wm_pause(WXWPARAM wParam, WXLPARAM lParam)
	{
		if (recorder_ == nullptr) return -1;
		switch (wParam) {
			case 1:	recorder_->pause(); break;
			case 0: recorder_->resume(); break;
			default: return -1;
		}
		return 0;
	}

	int wm_ping(WXWPARAM wParam, WXLPARAM lParam)
	{
		if (recorder_ == nullptr) return -1;
		recorder_->keepAlive();
		return 0;
	}

	int wm_get_status(WXWPARAM wParam, WXLPARAM lParam)
	{
		if (recorder_ == nullptr) return DesktopRecorderState::Stoped;
		return recorder_->getStatus();
	}	
};

