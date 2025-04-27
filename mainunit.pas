unit MainUnit;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, Menus, ComCtrls,
  StdCtrls, Buttons;

type

  { TMainForm }

  TMainForm = class(TForm)
    AddressBox: TComboBox;
    BrowserList: TListView;
    ToolbarImages: TImageList;
    MainMenu: TMainMenu;
    FileMenu: TMenuItem;
    FileSaveMenuItem: TMenuItem;
    FileExitMenu: TMenuItem;
    Separator1: TMenuItem;
    StatusBar: TStatusBar;
    ToolBar: TToolBar;
    BackButton: TToolButton;
    ForwardButton: TToolButton;
    RefreshButton: TToolButton;
    GoButton: TToolButton;
    ToolButtonDivider1: TToolButton;
    UpButton: TToolButton;
  private

  public

  end;

var
  MainForm: TMainForm;

implementation

{$R *.lfm}

end.

