unit _fmMain;

interface

uses
  AudioDevice,
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.StdCtrls;

type
  TfmMain = class(TForm)
    moResult: TMemo;
    procedure FormCreate(Sender: TObject);
  private
  public
  end;

var
  fmMain: TfmMain;

implementation

{$R *.dfm}

procedure TfmMain.FormCreate(Sender: TObject);
var
  i: Integer;
begin
  LoadAudioDeviceList;
  for i := 0 to GetAudioDeviceCount-1 do begin
    moResult.Lines.Add(
      Format('%d: %s, %d, %d',
      [i, GetAudioDeviceName(i), GetAudioDeviceInputChannels(i), GetAudioDeviceOutputChannels(i)])
    );
  end;
end;

end.
