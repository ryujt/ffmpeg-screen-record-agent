object fmMain: TfmMain
  Left = 0
  Top = 0
  Caption = 'Protocol tester'
  ClientHeight = 411
  ClientWidth = 784
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 784
    Height = 89
    Align = alTop
    BevelOuter = bvNone
    TabOrder = 0
    DesignSize = (
      784
      89)
    object lbFrames: TLabel
      Left = 12
      Top = 44
      Width = 103
      Height = 13
      Caption = 'Frames in buffer: %d'
    end
    object lbState: TLabel
      Left = 12
      Top = 63
      Width = 33
      Height = 13
      Caption = 'State: '
    end
    object lbErrorCode: TLabel
      Left = 384
      Top = 63
      Width = 60
      Height = 13
      Caption = 'Error Codes '
    end
    object btStart: TButton
      Left = 12
      Top = 9
      Width = 75
      Height = 25
      Caption = 'Start'
      TabOrder = 0
      OnClick = btStartClick
    end
    object btStop: TButton
      Left = 93
      Top = 9
      Width = 75
      Height = 25
      Caption = 'Stop'
      TabOrder = 1
      OnClick = btStopClick
    end
    object btTerminate: TButton
      Left = 700
      Top = 9
      Width = 75
      Height = 25
      Anchors = [akTop, akRight]
      Caption = 'Terminate'
      TabOrder = 2
      OnClick = btTerminateClick
    end
    object btPause: TButton
      Left = 188
      Top = 9
      Width = 75
      Height = 25
      Caption = 'btPause'
      TabOrder = 3
      OnClick = btPauseClick
    end
    object btResume: TButton
      Left = 269
      Top = 9
      Width = 75
      Height = 25
      Caption = 'Resume'
      TabOrder = 4
      OnClick = btResumeClick
    end
    object btRandom: TButton
      Left = 364
      Top = 9
      Width = 75
      Height = 25
      Caption = 'Random - off'
      TabOrder = 5
      OnClick = btRandomClick
    end
    object btStatus: TButton
      Left = 456
      Top = 9
      Width = 75
      Height = 25
      Caption = 'Get Status'
      TabOrder = 6
      OnClick = btStatusClick
    end
  end
  object moJson: TSynMemo
    Left = 0
    Top = 89
    Width = 384
    Height = 322
    Align = alClient
    Color = clWhite
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlack
    Font.Height = -13
    Font.Name = 'Courier New'
    Font.Pitch = fpFixed
    Font.Style = []
    TabOrder = 1
    CodeFolding.CollapsedLineColor = clGrayText
    CodeFolding.FolderBarLinesColor = clGrayText
    CodeFolding.ShowCollapsedLine = False
    CodeFolding.IndentGuidesColor = clGray
    CodeFolding.IndentGuides = True
    UseCodeFolding = False
    Gutter.Font.Charset = DEFAULT_CHARSET
    Gutter.Font.Color = clWindowText
    Gutter.Font.Height = -11
    Gutter.Font.Name = 'Courier New'
    Gutter.Font.Style = []
    Gutter.ShowLineNumbers = True
    Highlighter = SynJSONSyn
    Lines.Strings = (
      '{'
      '    "code": "start",'
      '    "filename": "D:/Work/test01.mp4",'
      '    "mic": -1,'
      '    "system-audio": true,'
      '    "volume-mic": 1.0,'
      '    "volume-system": 1.0,'
      '    "screen-type": "region",'
      '    "fps": 24,'
      '    "speed": "veryfast",'
      '    "left": 0,'
      '    "top": 0,'
      '    "width": 1920,'
      '    "height": 1080,'
      '    "with-cursor": true,'
      '    "timeout": -1'
      '}'
      '')
    Options = [eoAutoIndent, eoDragDropEditing, eoEnhanceEndKey, eoGroupUndo, eoScrollPastEol, eoShowScrollHint, eoTabIndent]
    ScrollBars = ssVertical
    TabWidth = 4
    WantTabs = True
    WordWrap = True
    FontSmoothing = fsmNone
  end
  object moMsg: TMemo
    Left = 384
    Top = 89
    Width = 400
    Height = 322
    Align = alRight
    ScrollBars = ssBoth
    TabOrder = 2
  end
  object SynJSONSyn: TSynJSONSyn
    Options.AutoDetectEnabled = False
    Options.AutoDetectLineLimit = 0
    Options.Visible = False
    Left = 88
    Top = 168
  end
  object TimerRandom: TTimer
    Interval = 500
    Left = 288
    Top = 212
  end
  object tmPing: TTimer
    OnTimer = tmPingTimer
    Left = 404
    Top = 216
  end
end
