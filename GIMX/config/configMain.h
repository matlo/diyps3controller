/*
 Copyright (c) 2010 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#ifndef CONFIGMAIN_H
#define CONFIGMAIN_H

//(*Headers(configFrame)
#include <wx/notebook.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/menu.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/panel.h>
#include <wx/grid.h>
#include <wx/filedlg.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/frame.h>
#include <wx/combobox.h>
#include <wx/statusbr.h>
//*)

#include <event_catcher.h>
#include <ConfigurationFile.h>

class configFrame: public wxFrame
{
    public:

        configFrame(wxString file,wxWindow* parent,wxWindowID id = -1);
        virtual ~configFrame();

        void save_current();
        void load_current();
        void refresh_gui();

    private:

        //(*Handlers(configFrame)
        void OnQuit(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);
        void OnMenuItemNew(wxCommandEvent& event);
        void OnButtonTabModifyClick(wxCommandEvent& event);
        void OnButtonAddPanelButton(wxCommandEvent& event);
        void OnEventTypeSelectPanelAxis(wxCommandEvent& event);
        void OnAxisTabBufferSizeText(wxCommandEvent& event);
        void OnAxisTabAxisIdSelect(wxCommandEvent& event);
        void OnButtonAddPanelAxis(wxCommandEvent& event);
        void OnButtonRemovePanelButton(wxCommandEvent& event);
        void OnButtonRemovePanelAxis(wxCommandEvent& event);
        void OnButtonTabEventTypeSelect(wxCommandEvent& event);
        void OnChoice6Select(wxCommandEvent& event);
        void OnChoice6Select1(wxCommandEvent& event);
        void OnButtonAutoDetectClick(wxCommandEvent& event);
        void OnAxisTabShape5Select(wxCommandEvent& event);
        void OnAxisTabShape7Select(wxCommandEvent& event);
        void OnAxisTabShape8Select(wxCommandEvent& event);
        void OnCheckBox2Click(wxCommandEvent& event);
        void OnCheckBoxSwitchBackClick(wxCommandEvent& event);
        void OnButtonTabAutoDetectClick(wxCommandEvent& event);
        void OnAxisTabAutoDetectClick(wxCommandEvent& event);
        void OnButtonDeleteClick(wxCommandEvent& event);
        void OnMenuOpen(wxCommandEvent& event);
        void OnMenuItemExpert(wxCommandEvent& event);
        void OnMenuSave(wxCommandEvent& event);
        void OnMenuSaveAs(wxCommandEvent& event);
        void OnButtonModifyButton(wxCommandEvent& event);
        void OnButtonModifyAxis(wxCommandEvent& event);
        void OnAxisTabShapeSelect(wxCommandEvent& event);
        void OnAxisTabAxisIdSelect1(wxCommandEvent& event);
        void OnMenuItemCopyConfiguration(wxCommandEvent& event);
        void OnMenuItemPasteConfiguration(wxCommandEvent& event);
        void OnMenuItemCopyController(wxCommandEvent& event);
        void OnMenuItemPasteController(wxCommandEvent& event);
        void OnButtonDeleteTrigger(wxCommandEvent& event);
        void OnMenuReplaceMouse(wxCommandEvent& event);
        void OnMenuReplaceKeyboard(wxCommandEvent& event);
        void OnMenuItemReplaceMouseDPI(wxCommandEvent& event);
        void OnMenuReplaceMouseDPI(wxCommandEvent& event);
        void OnMenuSetMouseDPI(wxCommandEvent& event);
        void OnMenuMultipleMK(wxCommandEvent& event);
        void OnTextCtrl(wxCommandEvent& event);
        void OnMenuUpdate(wxCommandEvent& event);
        void OnMenuAutoBindControls(wxCommandEvent& event);
        void OnMenuItemController(wxCommandEvent& event);
        void OnMenuItemConfiguration(wxCommandEvent& event);
        void OnChoice1Select(wxCommandEvent& event);
        void OnIntensityAutoDetectClick(wxCommandEvent& event);
        void OnIntensityDeadZoneChange(wxSpinEvent& event);
        void OnIntensityModifyClick(wxCommandEvent& event);
        void OnIntensityAddClick(wxCommandEvent& event);
        void OnIntensityRemoveClick(wxCommandEvent& event);
        void OnIntensityAxisSelect(wxCommandEvent& event);
        //*)
        void DeleteSelectedRows(wxGrid* grid);
        void DeleteLinkedRows(wxGrid* grid, int row);
        void replaceDevice(wxString wx_device_type);
        void auto_detect(wxStaticText* device_type, wxStaticText* device_name, wxStaticText* device_id, wxString event_type, wxStaticText* event_id);
        void updateButtonConfigurations();
        void updateAxisConfigurations();
        void updateIntensityConfigurations(Intensity* oldI, Intensity* newI);
        wxString isAlreadyUsed(wxString device_type, wxString device_name, wxString device_id, wxString event_type, wxString event_id, int gridIndex1, int gridIndex2, int gridIndex3);
        void readLabels();

        //(*Identifiers(configFrame)
        static const long ID_STATICTEXT35;
        static const long ID_STATICTEXT27;
        static const long ID_STATICTEXT36;
        static const long ID_STATICTEXT37;
        static const long ID_BUTTON1;
        static const long ID_CHECKBOX1;
        static const long ID_STATICTEXT28;
        static const long ID_SPINCTRL5;
        static const long ID_BUTTON10;
        static const long ID_STATICTEXT45;
        static const long ID_STATICTEXT46;
        static const long ID_STATICTEXT47;
        static const long ID_STATICTEXT52;
        static const long ID_STATICTEXT73;
        static const long ID_BUTTON19;
        static const long ID_STATICTEXT74;
        static const long ID_STATICTEXT53;
        static const long ID_STATICTEXT1;
        static const long ID_STATICTEXT54;
        static const long ID_STATICTEXT55;
        static const long ID_CHOICE9;
        static const long ID_SPINCTRL6;
        static const long ID_CHOICE2;
        static const long ID_SPINCTRL7;
        static const long ID_CHOICE6;
        static const long ID_GRID3;
        static const long ID_BUTTON21;
        static const long ID_BUTTON22;
        static const long ID_BUTTON23;
        static const long ID_PANEL1;
        static const long ID_STATICTEXT38;
        static const long ID_STATICTEXT30;
        static const long ID_STATICTEXT39;
        static const long ID_CHOICE4;
        static const long ID_STATICTEXT40;
        static const long ID_STATICTEXT2;
        static const long ID_BUTTON8;
        static const long ID_STATICTEXT8;
        static const long ID_STATICTEXT7;
        static const long ID_STATICTEXT34;
        static const long ID_TEXTCTRL3;
        static const long ID_CHOICE5;
        static const long ID_COMBOBOX1;
        static const long ID_GRID1;
        static const long ID_BUTTON4;
        static const long ID_BUTTON6;
        static const long ID_BUTTON2;
        static const long ID_PANEL2;
        static const long ID_STATICTEXT41;
        static const long ID_STATICTEXT32;
        static const long ID_STATICTEXT42;
        static const long ID_CHOICE7;
        static const long ID_STATICTEXT43;
        static const long ID_STATICTEXT21;
        static const long ID_BUTTON9;
        static const long ID_STATICTEXT16;
        static const long ID_STATICTEXT15;
        static const long ID_STATICTEXT17;
        static const long ID_STATICTEXT18;
        static const long ID_STATICTEXT22;
        static const long ID_STATICTEXT33;
        static const long ID_STATICTEXT44;
        static const long ID_CHOICE8;
        static const long ID_TEXTCTRL8;
        static const long ID_TEXTCTRL9;
        static const long ID_TEXTCTRL10;
        static const long ID_CHOICE1;
        static const long ID_TEXTCTRL1;
        static const long ID_TEXTCTRL2;
        static const long ID_COMBOBOX2;
        static const long ID_GRID2;
        static const long ID_BUTTON3;
        static const long ID_BUTTON7;
        static const long ID_BUTTON5;
        static const long ID_PANEL3;
        static const long ID_NOTEBOOK1;
        static const long idMenuNew;
        static const long idMenuOpen;
        static const long idMenuSave;
        static const long idMenuSaveAs;
        static const long idMenuQuit;
        static const long ID_MENUITEM12;
        static const long ID_MENUITEM18;
        static const long ID_MENUITEM17;
        static const long ID_MENUITEM19;
        static const long ID_MENUITEM23;
        static const long ID_MENUITEM20;
        static const long ID_MENUITEM22;
        static const long ID_MENUITEM21;
        static const long ID_MENUITEM1;
        static const long ID_MENUITEM2;
        static const long ID_MENUITEM3;
        static const long ID_MENUITEM4;
        static const long ID_MENUITEM5;
        static const long ID_MENUITEM6;
        static const long ID_MENUITEM7;
        static const long ID_MENUITEM8;
        static const long ID_MENUITEM9;
        static const long ID_MENUITEM10;
        static const long ID_MENUITEM11;
        static const long ID_MENUITEM13;
        static const long ID_MENUITEM14;
        static const long ID_MENUITEM15;
        static const long ID_MENUITEM16;
        static const long ID_MENUITEM24;
        static const long ID_MENUITEM25;
        static const long ID_MENUITEM27;
        static const long ID_MENUITEM26;
        static const long idMenuAbout;
        static const long ID_STATUSBAR1;
        //*)

        //(*Declarations(configFrame)
        wxStaticText* StaticTextLabel;
        wxMenuItem* MenuConfiguration6;
        wxStaticText* StaticTextAccelPanelAxis;
        wxButton* IntensityModify;
        wxMenuItem* MenuController4;
        wxComboBox* ButtonTabLabel;
        wxTextCtrl* AxisTabAcceleration;
        wxMenuItem* MenuItemSetMouseDPI;
        wxMenuItem* MenuItemSave;
        wxStaticText* StaticTextSmoothingPanelAxis;
        wxSpinCtrl* IntensityDeadZone;
        wxMenuItem* MenuItemPasteController;
        wxStaticText* ButtonTabDeviceId;
        wxNotebook* Notebook1;
        wxMenuItem* MenuItemSaveAs;
        wxButton* ButtonTabAutoDetect;
        wxStaticText* ButtonTabDeviceType;
        wxStaticText* StaticText2;
        wxMenuItem* MenuController1;
        wxMenuItem* MenuController5;
        wxMenuItem* MenuConfiguration1;
        wxChoice* ButtonTabButtonId;
        wxStaticText* StaticText6;
        wxMenuItem* MenuController7;
        wxStaticText* StaticTextSensPanelAxis;
        wxButton* AxisTabAutoDetect;
        wxStaticText* ButtonTabEventId;
        wxStaticText* ProfileTriggerDeviceType;
        wxStaticText* AxisTabEventId;
        wxMenuItem* MenuItemPasteProfile;
        wxMenuItem* MenuController2;
        wxStaticText* StaticText8;
        wxSpinCtrl* ProfileTriggerDelay;
        wxStaticText* StaticText18;
        wxMenu* MenuController;
        wxChoice* IntensityDirection;
        wxStaticText* StaticTextShapePanelAxis;
        wxChoice* IntensityShape;
        wxStaticText* StaticText1;
        wxButton* ButtonAutoDetect;
        wxStaticText* AxisTabDeviceId;
        wxFileDialog* FileDialog1;
        wxStaticText* StaticTextDelayPanelOverall;
        wxMenuItem* MenuConfiguration3;
        wxStaticText* AxisTabDeviceName;
        wxStaticText* StaticText3;
        wxMenuItem* MenuItemReplaceMouse;
        wxMenu* MenuAdvanced;
        wxTextCtrl* AxisTabSensitivity;
        wxStaticText* StaticTextEmptyPanelAxis;
        wxStaticText* ProfileTriggerButtonId;
        wxMenuItem* MenuConfiguration2;
        wxChoice* AxisTabEventType;
        wxButton* Button5;
        wxTextCtrl* AxisTabBufferSize;
        wxStaticText* StaticTextThresholdPanelButton;
        wxStaticText* AxisTabDeviceType;
        wxButton* Button3;
        wxSpinCtrl* IntensitySteps;
        wxButton* Button7;
        wxStaticText* StaticText7;
        wxStaticText* StaticTextButtonPanelButton;
        wxTextCtrl* AxisTabDeadZone;
        wxStatusBar* StatusBar1;
        wxMenu* MenuFile;
        wxTextCtrl* AxisTabFilter;
        wxCheckBox* CheckBoxSwitchBack;
        wxStaticText* IntensityDeviceId;
        wxStaticText* IntensityButtonId;
        wxMenuItem* MenuItemCopyProfile;
        wxButton* ButtonTabModify;
        wxMenu* MenuEdit;
        wxMenuItem* MenuItemMultipleMiceAndKeyboards;
        wxChoice* AxisTabShape;
        wxPanel* PanelAxis;
        wxMenuItem* MenuItemReplaceKeyboard;
        wxMenuItem* MenuItemNew;
        wxGrid* GridIntensity;
        wxComboBox* AxisTabLabel;
        wxMenuItem* MenuConfiguration8;
        wxMenuItem* MenuConfiguration4;
        wxButton* ButtonTabRemove;
        wxStaticText* IntensityDeviceType;
        wxStaticText* StaticTextDZPanelAxis;
        wxPanel* PanelButton;
        wxButton* IntensityAdd;
        wxMenuItem* MenuController6;
        wxChoice* IntensityAxis;
        wxButton* IntensityAutoDetect;
        wxMenu* MenuConfiguration;
        wxMenuItem* MenuItemCopyController;
        wxGrid* GridPanelButton;
        wxMenuItem* MenuConfiguration7;
        wxMenuItem* MenuConfiguration5;
        wxStaticText* ProfileTriggerDeviceId;
        wxStaticText* StaticText17;
        wxStaticText* StaticTextAxisPanelAxis;
        wxButton* ButtonTabAdd;
        wxButton* IntensityRemove;
        wxMenuItem* MenuItemLinkControls;
        wxMenuItem* MenuController3;
        wxChoice* ButtonTabEventType;
        wxTextCtrl* ButtonTabThreshold;
        wxButton* ButtonDelete;
        wxMenuItem* MenuItemReplaceMouseDPI;
        wxGrid* GridPanelAxis;
        wxStaticText* IntensityDeviceName;
        wxPanel* PanelOverall;
        wxStaticText* ProfileTriggerDeviceName;
        wxChoice* AxisTabAxisId;
        wxMenuItem* MenuUpdate;
        wxMenuItem* MenuAutoBindControls;
        wxStaticText* ButtonTabDeviceName;
        //*)

        event_catcher evcatch;
        ConfigurationFile configFile;
        unsigned int currentController;
        unsigned int currentConfiguration;

        Configuration tempConfiguration;
        Controller tempController;

        unsigned int grid1mod;
        unsigned int grid2mod;
        unsigned int grid3mod;
        
        wxString default_directory;

        DECLARE_EVENT_TABLE()
};

#endif // CONFIGMAIN_H
