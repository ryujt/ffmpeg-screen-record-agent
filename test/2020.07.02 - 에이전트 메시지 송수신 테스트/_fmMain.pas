unit _fmMain;

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.StdCtrls, Vcl.ExtCtrls,
  SynEditHighlighter, SynHighlighterJSON, SynEdit, SynMemo;

const
  WM_RECORD_AGENT_STOP = WM_USER + 1;
  WM_RECORD_AGENT_PING = WM_USER + 2;
  WM_RECORD_AGENT_ERROR = WM_USER + 3;
  WM_RECORD_AGENT_PAUSE = WM_USER + 4;
  WM_RECORD_AGENT_FRAMES_IN_BUFFER = WM_USER + 5;
  WM_RECORD_AGENT_COMPLETED = WM_USER + 6;
  WM_RECORD_AGENT_TERMINATE = WM_USER + 7;
  WM_RECORD_AGENT_GET_STATUS = WM_USER + 8;

type
  TfmMain = class(TForm)
    Panel1: TPanel;
    btStart: TButton;
    btStop: TButton;
    moJson: TSynMemo;
    SynJSONSyn: TSynJSONSyn;
    lbFrames: TLabel;
    lbState: TLabel;
    btTerminate: TButton;
    btPause: TButton;
    btResume: TButton;
    btRandom: TButton;
    TimerRandom: TTimer;
    lbErrorCode: TLabel;
    tmPing: TTimer;
    btStatus: TButton;
    moMsg: TMemo;
    procedure btStartClick(Sender: TObject);
    procedure btStopClick(Sender: TObject);
    procedure btTerminateClick(Sender: TObject);
    procedure btPauseClick(Sender: TObject);
    procedure btResumeClick(Sender: TObject);
    procedure btRandomClick(Sender: TObject);
    procedure TimerRandomTimer(Sender: TObject);
    procedure tmPingTimer(Sender: TObject);
    procedure btStatusClick(Sender: TObject);
  private
    target_handle : integer;
    procedure get_tartget_handle;
    function send_string(ATargetHandle:integer; const AText:AnsiString):integer;
  protected
    procedure wm_error(var msg:TMessage); message WM_RECORD_AGENT_ERROR;
    procedure wm_frames(var msg:TMessage); message WM_RECORD_AGENT_FRAMES_IN_BUFFER;
    procedure wm_completed(var msg:TMessage); message WM_RECORD_AGENT_COMPLETED;
  public
  end;

var
  fmMain: TfmMain;

implementation

{$R *.dfm}

procedure TfmMain.btRandomClick(Sender: TObject);
begin
  if Assigned(TimerRandom.OnTimer) then TimerRandom.OnTimer := nil
  else TimerRandom.OnTimer := TimerRandomTimer;

  if Assigned(TimerRandom.OnTimer) then begin
    btRandom.Caption := 'Random - on';
  end else begin
    btRandom.Caption := 'Random - off';
  end;
end;

procedure TfmMain.btResumeClick(Sender: TObject);
begin
  get_tartget_handle;
  PostMessage(target_handle, WM_RECORD_AGENT_PAUSE, 0, 0);
  lbState.Caption := 'State: resumed';
end;

procedure TfmMain.btStartClick(Sender: TObject);
var
  result : integer;
begin
  get_tartget_handle;

  result := send_string(target_handle, moJson.Text);
  if (result = 0) then begin
    lbState.Caption := 'State: start ok';
  end else begin
    lbState.Caption := Format('State: start error (%d)', [result]);
  end;
end;

procedure TfmMain.btStatusClick(Sender: TObject);
begin
  get_tartget_handle;
  case SendMessage(target_handle, WM_RECORD_AGENT_GET_STATUS, 0, 0) of
    0: lbState.Caption := 'State: 인코딩 중';
    1: lbState.Caption := 'State: 정지';
    2: lbState.Caption := 'State: 일시 정지';
  end;
end;

procedure TfmMain.btStopClick(Sender: TObject);
begin
  get_tartget_handle;
  PostMessage(target_handle, WM_RECORD_AGENT_STOP, 0, 0);
  lbState.Caption := 'State: stoped';
end;

procedure TfmMain.btTerminateClick(Sender: TObject);
begin
  TimerRandom.Enabled := false;

  get_tartget_handle;
  PostMessage(target_handle, WM_RECORD_AGENT_TERMINATE, 0, 0);
  lbState.Caption := 'State: terminated';
end;

procedure TfmMain.get_tartget_handle;
begin
  target_handle := FindWindow('wxWindowNR', 'Screen Recorder Agent');

  if target_handle = 0 then begin
    TimerRandom.Enabled := false;
    btRandom.Caption := 'Random - off';
  end;
end;

procedure TfmMain.btPauseClick(Sender: TObject);
begin
  get_tartget_handle;
  PostMessage(target_handle, WM_RECORD_AGENT_PAUSE, 1, 0);
  lbState.Caption := 'State: paused';
end;

function TfmMain.send_string(ATargetHandle:integer; const AText: AnsiString):integer;
var
  copyDataStruct : TCopyDataStruct;
begin
  copyDataStruct.dwData := 0;
  copyDataStruct.cbData := 1 + Length(AText);
  copyDataStruct.lpData := PAnsiChar(AText);
  Result := SendMessage(ATargetHandle, WM_COPYDATA, integer(Handle), Integer(@copyDataStruct));
end;

procedure TfmMain.TimerRandomTimer(Sender: TObject);
begin
  TimerRandom.Enabled := false;
  try
    case Random(1024) mod 4 of
         0: btStart.Click;
         1: btStop.Click;
         2: btPause.Click;
         3: btResume.Click;
    end;
  finally
    TimerRandom.Enabled := true;
  end;
end;

procedure TfmMain.tmPingTimer(Sender: TObject);
begin
  get_tartget_handle;
  PostMessage(target_handle, WM_RECORD_AGENT_PING, 0, 0);
end;

procedure TfmMain.wm_completed(var msg: TMessage);
begin
  lbState.Caption := 'State: completed';
end;

procedure TfmMain.wm_error(var msg: TMessage);
var
  error_code : integer;
begin
  error_code := Integer(msg.WParam);
  moMsg.Lines.Add( Format('Last Error Code: %d', [error_code]) );
end;

procedure TfmMain.wm_frames(var msg: TMessage);
begin
  lbFrames.Caption := Format('Frames in buffer: %d', [msg.WParam]);
end;

end.

