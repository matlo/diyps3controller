/***************************************************************
 * Name:      sixemuguiMain.cpp
 * Purpose:   Code for Application Frame
 * Author:    Matlo (mat.lau@laposte.net)
 * Created:   2011-01-12
 * Copyright: Matlo (http://diy-machine.blogspot.com/)
 * License:
 **************************************************************/

#include "wx_pch.h"
#include "sixemuguiMain.h"
#include <wx/msgdlg.h>

//(*InternalHeaders(sixemuguiFrame)
#include <wx/intl.h>
#include <wx/string.h>
//*)

#include <glib.h>

#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <pwd.h>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

#define CONFIG_DIR ".emuclient/config/"

char* homedir;

//helper functions
enum wxbuildinfoformat
{
    short_f, long_f
};

wxString wxbuildinfo(wxbuildinfoformat format)
{
    wxString wxbuild(wxVERSION_STRING);

    if (format == long_f )
    {
#if defined(__WXMSW__)
        wxbuild << _T("-Windows");
#elif defined(__UNIX__)
        wxbuild << _T("-Linux");
#endif

#if wxUSE_UNICODE
        wxbuild << _T("-Unicode build");
#else
        wxbuild << _T("-ANSI build");
#endif // wxUSE_UNICODE
    }

    return wxbuild;
}

//(*IdInit(sixemuguiFrame)
const long sixemuguiFrame::ID_STATICTEXT1 = wxNewId();
const long sixemuguiFrame::ID_CHOICE1 = wxNewId();
const long sixemuguiFrame::ID_STATICTEXT2 = wxNewId();
const long sixemuguiFrame::ID_CHOICE2 = wxNewId();
const long sixemuguiFrame::ID_STATICTEXT3 = wxNewId();
const long sixemuguiFrame::ID_CHOICE3 = wxNewId();
const long sixemuguiFrame::ID_STATICTEXT4 = wxNewId();
const long sixemuguiFrame::ID_CHOICE5 = wxNewId();
const long sixemuguiFrame::ID_STATICTEXT6 = wxNewId();
const long sixemuguiFrame::ID_CHOICE6 = wxNewId();
const long sixemuguiFrame::ID_STATICTEXT7 = wxNewId();
const long sixemuguiFrame::ID_CHOICE7 = wxNewId();
const long sixemuguiFrame::ID_STATICTEXT8 = wxNewId();
const long sixemuguiFrame::ID_BUTTON2 = wxNewId();
const long sixemuguiFrame::ID_CHECKBOX5 = wxNewId();
const long sixemuguiFrame::ID_CHOICE8 = wxNewId();
const long sixemuguiFrame::ID_BUTTON1 = wxNewId();
const long sixemuguiFrame::ID_STATICTEXT5 = wxNewId();
const long sixemuguiFrame::ID_STATICTEXT9 = wxNewId();
const long sixemuguiFrame::ID_STATICTEXT10 = wxNewId();
const long sixemuguiFrame::ID_STATICTEXT11 = wxNewId();
const long sixemuguiFrame::ID_STATICTEXT12 = wxNewId();
const long sixemuguiFrame::ID_STATICTEXT13 = wxNewId();
const long sixemuguiFrame::ID_STATICTEXT14 = wxNewId();
const long sixemuguiFrame::ID_CHECKBOX1 = wxNewId();
const long sixemuguiFrame::ID_CHECKBOX4 = wxNewId();
const long sixemuguiFrame::ID_CHECKBOX2 = wxNewId();
const long sixemuguiFrame::ID_CHECKBOX3 = wxNewId();
const long sixemuguiFrame::ID_CHECKBOX6 = wxNewId();
const long sixemuguiFrame::ID_CHOICE4 = wxNewId();
const long sixemuguiFrame::ID_BUTTON3 = wxNewId();
const long sixemuguiFrame::ID_PANEL1 = wxNewId();
const long sixemuguiFrame::ID_MENUITEM1 = wxNewId();
const long sixemuguiFrame::ID_MENUITEM2 = wxNewId();
const long sixemuguiFrame::idMenuQuit = wxNewId();
const long sixemuguiFrame::idMenuAbout = wxNewId();
const long sixemuguiFrame::ID_STATUSBAR1 = wxNewId();
//*)

BEGIN_EVENT_TABLE(sixemuguiFrame,wxFrame)
    //(*EventTable(sixemuguiFrame)
    //*)
END_EVENT_TABLE()

static int readCommandResults(const char * argv[], int nb_params, wxString params[], int nb_repeat, wxString results[])
{
    int exit_status = 0;
    char* out = NULL;
    GError *error = NULL;
    gboolean test;
    char* line;
    char* end;
    int ret = 0;
    char c_param[256];

    test = g_spawn_sync(NULL, (gchar**)argv, NULL, (GSpawnFlags)G_SPAWN_SEARCH_PATH, NULL, NULL, &out, NULL, &exit_status, &error);

    if(test)
    {
        line = out;

        printf("%s\n", out);

        for(int i=0; i<nb_params*nb_repeat; ++i)
        {
            strncpy(c_param, params[i%nb_params].mb_str(), sizeof(c_param));

            if((line = strstr(line, c_param)))
            {
                if((end = strstr(line, "\n")))
                {
                    *end = '\0';
                }
                results[i] = wxString(line+strlen(c_param), wxConvUTF8);
                line=end+1;
            }
            else
            {
                if(nb_repeat == 1)
                {
                    ret = -1;
                }
                break;
            }
        }
    }
    else
    {
        ret = -1;
    }

    if(out) g_free(out);
    if(error) g_free(error);

    return ret;
}

static char hci[7];
static char bad[18];

static const char *sixaxis_device[] = { "sixaddr", NULL };

static const char *hciconfig_all[] = { "hciconfig", "-a", hci, NULL };
static const char *hciconfig_revision[] = { "hcirevision", hci, NULL };

static const char *bdaddr[] = { "bdaddr", "-i", hci, NULL };
static const char *bdaddr_modify[] = { "bdaddr", "-r", "-i", hci, bad, NULL };

void sixemuguiFrame::readSixaxis()
{
    wxString params[2] = {_("Current Bluetooth master: "), _("Current Bluetooth Device Address: ")};
    wxString results[14];
    unsigned int j;

    int res = readCommandResults(sixaxis_device, 2, params, 7, results);

    if(res != -1)
    {
        for(int i=0; i<13; i+=2)
        {
            if(results[i].IsEmpty())
            {
                break;
            }
            for(j=0; j<Choice1->GetCount(); ++j)
            {
              if(Choice1->GetString(j) == results[i+1].MakeUpper())
              {
                break;
              }
            }
            if(j == Choice1->GetCount())
            {
              Choice2->Append(results[i].MakeUpper());
              Choice1->Append(results[i+1].MakeUpper());
            }
        }
    }
}

void sixemuguiFrame::readDongles()
{
    wxString params1[3] = {_("BD Address: "), _("Name: "), _("Manufacturer: ")};
    wxString results1[3];
    wxString params2[1] = {_("Chip version: ")};
    wxString results2[1];
    int res;

    for(int i=0; i<256; ++i)
    {
        snprintf(hci, sizeof(hci), "hci%d", i);
        res = readCommandResults(hciconfig_all, 3, params1, 1, results1);

        if(res != -1)
        {
            Choice3->Append(results1[0].Left(17));

            Choice5->Append(results1[1]);

            Choice6->Append(results1[2]);

            res = readCommandResults(hciconfig_revision, 1, params2, 1, results2);

            if(res != -1)
            {
                Choice7->Append(results2[0]);
            }
            else
            {
                Choice7->Append(_(""));
            }
        }
        else
        {
            break;
        }
    }
}

int sixemuguiFrame::setDongleAddress()
{
    int i;
    int j = 0;
    unsigned int k;
    wxString params1[1] = {_("Address changed - ")};
    wxString results1[1];
    wxString params2[1] = {_("Device address: ")};
    wxString results2[1];
    int res;

    if(Choice1->GetStringSelection().IsEmpty())
    {
        wxMessageBox( wxT("Please select a Sixaxis Address!"), wxT("Error"), wxICON_ERROR);
        return -1;
    }
    else if(Choice3->GetStringSelection().IsEmpty())
    {
        wxMessageBox( wxT("Please select a Bluetooth Dongle!"), wxT("Error"), wxICON_ERROR);
        return -1;
    }

    for(k=0; k<Choice3->GetCount(); ++k)
    {
        if(Choice3->GetString(k) == Choice1->GetStringSelection())
        {
            wxMessageBox( wxT("Address already used!"), wxT("Error"), wxICON_ERROR);
            return -1;
        }
    }

    i = Choice3->GetSelection();
    snprintf(hci, sizeof(hci), "hci%d", i);
    strncpy( bad, Choice1->GetStringSelection().mb_str(), 18 );

    res = readCommandResults(bdaddr_modify, 1, params1, 1, results1);

    if(res != -1)
    {
        wxMessageBox( results1[0], wxT("Success"), wxICON_INFORMATION);
    }

    //wait up to 5s for the device to come back
    while(readCommandResults(bdaddr, 1, params2, 1, results2) == -1 && j<50)
    {
        usleep(100000);
        j++;
    }

    if(results2[0] !=  Choice1->GetStringSelection())
    {
        wxMessageBox( wxT("Read address after set: ko!"), wxT("Error"), wxICON_ERROR);
    }
    else
    {
        Choice3->SetString(Choice3->GetSelection(), Choice1->GetStringSelection());
        wxMessageBox( wxT("Read address after set: seems ok!"), wxT("Success"), wxICON_INFORMATION);
    }

    return 0;
}

static void read_filenames(const char* dir, wxChoice* choice)
{
    DIR *dirp;
    char dir_path[PATH_MAX];
    struct dirent *d;
    string filename = "";
    string line = "";

    filename.append(homedir);
    filename.append("/.sixemugui/default");
    ifstream infile (filename.c_str());
    if ( infile.is_open() )
    {
        if( infile.good() )
        {
            getline (infile,line);
        }
        infile.close();
    }

    choice->Clear();

    snprintf(dir_path, sizeof(dir_path), "%s/%s", homedir, dir);
    dirp = opendir(dir_path);
    if (dirp == NULL)
    {
      printf("Warning: can't open config directory %s\n", dir_path);
      return;
    }

    while ((d = readdir(dirp)))
    {
      if (d->d_type == DT_REG)
      {
        if(!line.empty() && line == d->d_name)
        {
          choice->SetSelection(choice->Append(wxString(d->d_name, wxConvUTF8)));
        }
        else
        {
          choice->Append(wxString(d->d_name, wxConvUTF8));
        }
      }
    }

    closedir(dirp);
}

static int read_sixaxis_config(wxChoice* cdevice, wxChoice* cmaster)
{
    string filename;
    string line;
    string device;
    string master;
    int ret = -1;

    filename.append(homedir);
    filename.append("/.sixemugui/config");

    ifstream myfile(filename.c_str());
    if(myfile.is_open())
    {
        while ( myfile.good() )
        {
            getline (myfile,line);
            if(!line.empty())
            {
                stringstream parser(line);
                parser >> device;
                parser >> master;
                cdevice->Append(wxString(device.c_str(), wxConvUTF8));
                cmaster->Append(wxString(master.c_str(), wxConvUTF8));
                ret = 0;
            }
        }
        myfile.close();
    }
    else
    {
        wxMessageBox( wxT("Cannot open config file."), wxT("Info"), wxICON_INFORMATION);
    }
    return ret;
}

void sixemuguiFrame::refresh()
{
    Choice3->Clear();
    Choice5->Clear();
    Choice6->Clear();
    Choice7->Clear();
    readSixaxis();
    readDongles();
    read_filenames(CONFIG_DIR, Choice4);
    if(Choice1->GetCount() == 0)
    {
        wxMessageBox( wxT("No Sixaxis Detected!\nSixaxis usb wire plugged?"), wxT("Error"), wxICON_ERROR);
    }
    else
    {
        if(Choice1->GetSelection() < 0)
        {
            Choice1->SetSelection(0);
        }
        if(Choice2->GetSelection() < 0)
        {
            Choice2->SetSelection(Choice1->GetSelection());
        }
    }
    if(Choice3->GetCount() == 0)
    {
        wxMessageBox( wxT("No Bluetooth Dongle Detected!"), wxT("Error"), wxICON_ERROR);
    }
    else
    {
        if(Choice3->GetSelection() < 0)
        {
            Choice3->SetSelection(0);
            Choice5->SetSelection(0);
            Choice6->SetSelection(0);
            Choice7->SetSelection(0);
        }
    }
    if(Choice4->GetCount() == 0)
    {
        wxMessageBox( wxT("No Config Files Detected!"), wxT("Error"), wxICON_ERROR);
    }
    else
    {
        if(Choice4->GetSelection() < 0)
        {
            Choice4->SetSelection(0);
        }
    }
}

sixemuguiFrame::sixemuguiFrame(wxWindow* parent,wxWindowID id)
{
    unsigned int i;

    //(*Initialize(sixemuguiFrame)
    wxStaticBoxSizer* StaticBoxSizer2;
    wxFlexGridSizer* FlexGridSizer4;
    wxMenuItem* MenuItem2;
    wxStaticBoxSizer* StaticBoxSizer4;
    wxFlexGridSizer* FlexGridSizer10;
    wxFlexGridSizer* FlexGridSizer3;
    wxMenuItem* MenuItem1;
    wxFlexGridSizer* FlexGridSizer5;
    wxFlexGridSizer* FlexGridSizer9;
    wxFlexGridSizer* FlexGridSizer2;
    wxMenu* Menu1;
    wxFlexGridSizer* FlexGridSizer7;
    wxStaticBoxSizer* StaticBoxSizer8;
    wxStaticBoxSizer* StaticBoxSizer3;
    wxStaticBoxSizer* StaticBoxSizer6;
    wxFlexGridSizer* FlexGridSizer8;
    wxFlexGridSizer* FlexGridSizer13;
    wxFlexGridSizer* FlexGridSizer12;
    wxMenuBar* MenuBar1;
    wxFlexGridSizer* FlexGridSizer6;
    wxStaticBoxSizer* StaticBoxSizer1;
    wxFlexGridSizer* FlexGridSizer1;
    wxFlexGridSizer* FlexGridSizer11;
    wxMenu* Menu2;
    wxStaticBoxSizer* StaticBoxSizer5;

    Create(parent, wxID_ANY, _("Sixemugui"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE, _T("wxID_ANY"));
    SetClientSize(wxSize(675,525));
    Panel1 = new wxPanel(this, ID_PANEL1, wxDefaultPosition, wxSize(0,0), wxTAB_TRAVERSAL, _T("ID_PANEL1"));
    FlexGridSizer1 = new wxFlexGridSizer(2, 1, 0, 0);
    FlexGridSizer7 = new wxFlexGridSizer(1, 2, 0, 0);
    StaticBoxSizer1 = new wxStaticBoxSizer(wxHORIZONTAL, Panel1, _("Sixaxis"));
    FlexGridSizer2 = new wxFlexGridSizer(2, 2, 0, 0);
    StaticText1 = new wxStaticText(Panel1, ID_STATICTEXT1, _("Address"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT1"));
    FlexGridSizer2->Add(StaticText1, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Choice1 = new wxChoice(Panel1, ID_CHOICE1, wxDefaultPosition, wxDefaultSize, 0, 0, 0, wxDefaultValidator, _T("ID_CHOICE1"));
    FlexGridSizer2->Add(Choice1, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticText2 = new wxStaticText(Panel1, ID_STATICTEXT2, _("PS3 address"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT2"));
    FlexGridSizer2->Add(StaticText2, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Choice2 = new wxChoice(Panel1, ID_CHOICE2, wxDefaultPosition, wxDefaultSize, 0, 0, 0, wxDefaultValidator, _T("ID_CHOICE2"));
    FlexGridSizer2->Add(Choice2, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticBoxSizer1->Add(FlexGridSizer2, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer7->Add(StaticBoxSizer1, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticBoxSizer2 = new wxStaticBoxSizer(wxHORIZONTAL, Panel1, _("Dongle"));
    FlexGridSizer3 = new wxFlexGridSizer(5, 2, 0, 0);
    StaticText3 = new wxStaticText(Panel1, ID_STATICTEXT3, _("Address"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT3"));
    FlexGridSizer3->Add(StaticText3, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Choice3 = new wxChoice(Panel1, ID_CHOICE3, wxDefaultPosition, wxDefaultSize, 0, 0, 0, wxDefaultValidator, _T("ID_CHOICE3"));
    FlexGridSizer3->Add(Choice3, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticText4 = new wxStaticText(Panel1, ID_STATICTEXT4, _("Name"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT4"));
    FlexGridSizer3->Add(StaticText4, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Choice5 = new wxChoice(Panel1, ID_CHOICE5, wxDefaultPosition, wxDefaultSize, 0, 0, 0, wxDefaultValidator, _T("ID_CHOICE5"));
    FlexGridSizer3->Add(Choice5, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticText6 = new wxStaticText(Panel1, ID_STATICTEXT6, _("Manufacturer"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT6"));
    FlexGridSizer3->Add(StaticText6, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Choice6 = new wxChoice(Panel1, ID_CHOICE6, wxDefaultPosition, wxDefaultSize, 0, 0, 0, wxDefaultValidator, _T("ID_CHOICE6"));
    FlexGridSizer3->Add(Choice6, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticText7 = new wxStaticText(Panel1, ID_STATICTEXT7, _("Chip version"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT7"));
    FlexGridSizer3->Add(StaticText7, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Choice7 = new wxChoice(Panel1, ID_CHOICE7, wxDefaultPosition, wxDefaultSize, 0, 0, 0, wxDefaultValidator, _T("ID_CHOICE7"));
    FlexGridSizer3->Add(Choice7, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticText8 = new wxStaticText(Panel1, ID_STATICTEXT8, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT8"));
    FlexGridSizer3->Add(StaticText8, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Button2 = new wxButton(Panel1, ID_BUTTON2, _("Set Dongle Address"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_BUTTON2"));
    FlexGridSizer3->Add(Button2, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticBoxSizer2->Add(FlexGridSizer3, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer7->Add(StaticBoxSizer2, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer1->Add(FlexGridSizer7, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer6 = new wxFlexGridSizer(1, 2, 0, 0);
    StaticBoxSizer3 = new wxStaticBoxSizer(wxVERTICAL, Panel1, _("emu"));
    CheckBox5 = new wxCheckBox(Panel1, ID_CHECKBOX5, _("Autostart\nemuclient"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_CHECKBOX5"));
    CheckBox5->SetValue(true);
    StaticBoxSizer3->Add(CheckBox5, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer12 = new wxFlexGridSizer(0, 3, 0, 0);
    Choice8 = new wxChoice(Panel1, ID_CHOICE8, wxDefaultPosition, wxDefaultSize, 0, 0, 0, wxDefaultValidator, _T("ID_CHOICE8"));
    Choice8->SetSelection( Choice8->Append(_("0")) );
    Choice8->Append(_("1"));
    Choice8->Append(_("2"));
    Choice8->Append(_("3"));
    Choice8->Append(_("4"));
    Choice8->Append(_("5"));
    Choice8->Append(_("6"));
    FlexGridSizer12->Add(Choice8, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Button1 = new wxButton(Panel1, ID_BUTTON1, _("Start"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_BUTTON1"));
    FlexGridSizer12->Add(Button1, 1, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
    StaticBoxSizer3->Add(FlexGridSizer12, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer4 = new wxFlexGridSizer(1, 7, 0, 0);
    StaticText5 = new wxStaticText(Panel1, ID_STATICTEXT5, _("0"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT5"));
    StaticText5->Disable();
    FlexGridSizer4->Add(StaticText5, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticText9 = new wxStaticText(Panel1, ID_STATICTEXT9, _("1"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT9"));
    StaticText9->Disable();
    FlexGridSizer4->Add(StaticText9, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticText10 = new wxStaticText(Panel1, ID_STATICTEXT10, _("2"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT10"));
    StaticText10->Disable();
    FlexGridSizer4->Add(StaticText10, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticText11 = new wxStaticText(Panel1, ID_STATICTEXT11, _("3"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT11"));
    StaticText11->Disable();
    FlexGridSizer4->Add(StaticText11, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticText12 = new wxStaticText(Panel1, ID_STATICTEXT12, _("4"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT12"));
    StaticText12->Disable();
    FlexGridSizer4->Add(StaticText12, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticText13 = new wxStaticText(Panel1, ID_STATICTEXT13, _("5"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT13"));
    StaticText13->Disable();
    FlexGridSizer4->Add(StaticText13, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticText14 = new wxStaticText(Panel1, ID_STATICTEXT14, _("6"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT14"));
    StaticText14->Disable();
    StaticText14->SetForegroundColour(wxColour(0,0,0));
    FlexGridSizer4->Add(StaticText14, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticBoxSizer3->Add(FlexGridSizer4, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer6->Add(StaticBoxSizer3, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticBoxSizer4 = new wxStaticBoxSizer(wxHORIZONTAL, Panel1, _("emuclient"));
    FlexGridSizer5 = new wxFlexGridSizer(3, 1, 0, 0);
    FlexGridSizer8 = new wxFlexGridSizer(2, 2, 0, 0);
    StaticBoxSizer5 = new wxStaticBoxSizer(wxHORIZONTAL, Panel1, _("Mouse"));
    FlexGridSizer10 = new wxFlexGridSizer(1, 2, 0, 0);
    CheckBox1 = new wxCheckBox(Panel1, ID_CHECKBOX1, _("grab"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_CHECKBOX1"));
    CheckBox1->SetValue(true);
    FlexGridSizer10->Add(CheckBox1, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    CheckBox4 = new wxCheckBox(Panel1, ID_CHECKBOX4, _("calibrate"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_CHECKBOX4"));
    CheckBox4->SetValue(false);
    FlexGridSizer10->Add(CheckBox4, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticBoxSizer5->Add(FlexGridSizer10, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer8->Add(StaticBoxSizer5, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticBoxSizer6 = new wxStaticBoxSizer(wxHORIZONTAL, Panel1, _("Output"));
    FlexGridSizer11 = new wxFlexGridSizer(0, 3, 0, 0);
    CheckBox2 = new wxCheckBox(Panel1, ID_CHECKBOX2, _("gui"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_CHECKBOX2"));
    CheckBox2->SetValue(false);
    FlexGridSizer11->Add(CheckBox2, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    CheckBox3 = new wxCheckBox(Panel1, ID_CHECKBOX3, _("terminal"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_CHECKBOX3"));
    CheckBox3->SetValue(false);
    FlexGridSizer11->Add(CheckBox3, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticBoxSizer6->Add(FlexGridSizer11, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer8->Add(StaticBoxSizer6, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer5->Add(FlexGridSizer8, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer13 = new wxFlexGridSizer(0, 3, 0, 0);
    CheckBox6 = new wxCheckBox(Panel1, ID_CHECKBOX6, _("Force updates"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_CHECKBOX6"));
    CheckBox6->SetValue(true);
    FlexGridSizer13->Add(CheckBox6, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer5->Add(FlexGridSizer13, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer9 = new wxFlexGridSizer(1, 2, 0, 0);
    StaticBoxSizer8 = new wxStaticBoxSizer(wxHORIZONTAL, Panel1, _("Config"));
    Choice4 = new wxChoice(Panel1, ID_CHOICE4, wxDefaultPosition, wxDefaultSize, 0, 0, 0, wxDefaultValidator, _T("ID_CHOICE4"));
    StaticBoxSizer8->Add(Choice4, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer9->Add(StaticBoxSizer8, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Button3 = new wxButton(Panel1, ID_BUTTON3, _("Start"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_BUTTON3"));
    Button3->Disable();
    FlexGridSizer9->Add(Button3, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer5->Add(FlexGridSizer9, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    StaticBoxSizer4->Add(FlexGridSizer5, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer6->Add(StaticBoxSizer4, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    FlexGridSizer1->Add(FlexGridSizer6, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    Panel1->SetSizer(FlexGridSizer1);
    FlexGridSizer1->SetSizeHints(Panel1);
    MenuBar1 = new wxMenuBar();
    Menu1 = new wxMenu();
    MenuItem3 = new wxMenuItem(Menu1, ID_MENUITEM1, _("Refresh\tF5"), wxEmptyString, wxITEM_NORMAL);
    Menu1->Append(MenuItem3);
    MenuItem4 = new wxMenuItem(Menu1, ID_MENUITEM2, _("Save"), wxEmptyString, wxITEM_NORMAL);
    Menu1->Append(MenuItem4);
    MenuItem1 = new wxMenuItem(Menu1, idMenuQuit, _("Quit\tAlt-F4"), _("Quit the application"), wxITEM_NORMAL);
    Menu1->Append(MenuItem1);
    MenuBar1->Append(Menu1, _("&File"));
    Menu2 = new wxMenu();
    MenuItem2 = new wxMenuItem(Menu2, idMenuAbout, _("About\tF1"), _("Show info about this application"), wxITEM_NORMAL);
    Menu2->Append(MenuItem2);
    MenuBar1->Append(Menu2, _("Help"));
    SetMenuBar(MenuBar1);
    StatusBar1 = new wxStatusBar(this, ID_STATUSBAR1, 0, _T("ID_STATUSBAR1"));
    int __wxStatusBarWidths_1[2] = { -1, 20 };
    int __wxStatusBarStyles_1[2] = { wxSB_NORMAL, wxSB_NORMAL };
    StatusBar1->SetFieldsCount(2,__wxStatusBarWidths_1);
    StatusBar1->SetStatusStyles(2,__wxStatusBarStyles_1);
    SetStatusBar(StatusBar1);
    SingleInstanceChecker1.Create(_T("Sixemugui_") + wxGetUserId() + _T("_Guard"));

    Connect(ID_CHOICE1,wxEVT_COMMAND_CHOICE_SELECTED,(wxObjectEventFunction)&sixemuguiFrame::OnSelectSixaxisBdaddr);
    Connect(ID_CHOICE2,wxEVT_COMMAND_CHOICE_SELECTED,(wxObjectEventFunction)&sixemuguiFrame::OnSelectPS3Bdaddr);
    Connect(ID_CHOICE3,wxEVT_COMMAND_CHOICE_SELECTED,(wxObjectEventFunction)&sixemuguiFrame::OnSelectBtDongle);
    Connect(ID_CHOICE5,wxEVT_COMMAND_CHOICE_SELECTED,(wxObjectEventFunction)&sixemuguiFrame::OnChoice5Select);
    Connect(ID_CHOICE6,wxEVT_COMMAND_CHOICE_SELECTED,(wxObjectEventFunction)&sixemuguiFrame::OnChoice6Select);
    Connect(ID_CHOICE7,wxEVT_COMMAND_CHOICE_SELECTED,(wxObjectEventFunction)&sixemuguiFrame::OnChoice7Select);
    Connect(ID_BUTTON2,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&sixemuguiFrame::OnButton2Click);
    Connect(ID_CHOICE8,wxEVT_COMMAND_CHOICE_SELECTED,(wxObjectEventFunction)&sixemuguiFrame::OnChoice8Select);
    Connect(ID_BUTTON1,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&sixemuguiFrame::OnButton1Click);
    Connect(ID_CHECKBOX4,wxEVT_COMMAND_CHECKBOX_CLICKED,(wxObjectEventFunction)&sixemuguiFrame::OnCheckBoxCalibrate);
    Connect(ID_CHECKBOX2,wxEVT_COMMAND_CHECKBOX_CLICKED,(wxObjectEventFunction)&sixemuguiFrame::OnCheckBox2Click);
    Connect(ID_CHECKBOX3,wxEVT_COMMAND_CHECKBOX_CLICKED,(wxObjectEventFunction)&sixemuguiFrame::OnCheckBox3Click);
    Connect(ID_BUTTON3,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&sixemuguiFrame::OnButton3Click);
    Connect(ID_MENUITEM1,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&sixemuguiFrame::OnSelectRefresh);
    Connect(ID_MENUITEM2,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&sixemuguiFrame::OnSave);
    Connect(idMenuQuit,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&sixemuguiFrame::OnQuit);
    Connect(idMenuAbout,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&sixemuguiFrame::OnAbout);
    //*)

    if(SingleInstanceChecker1.IsAnotherRunning())
    {
        wxMessageBox( wxT("Sixemugui is already running!"), wxT("Error"), wxICON_ERROR);
        exit(-1);
    }

    for(i=0; i<7; ++i)
    {
        dongle[i] = -1;
        pid[i] = -1;
    }

    for(i=0; i<256; ++i)
    {
        dongleInUse[i] = false;
    }

    if(!getuid())
    {
    	int answer = wxMessageBox(_("It's not recommended to run as root user. Continue?"), _("Confirm"), wxYES_NO);
      if (answer == wxNO)
      {
        exit(0);
      }
    }

    homedir = getpwuid(getuid())->pw_dir;

    string cmd;
    cmd.append("mkdir -p ");
    cmd.append(homedir);
    cmd.append("/.sixemugui");
    if(system(cmd.c_str()) < 0)
    {
        wxMessageBox( wxT("Cannot open sixemugui config directory!"), wxT("Error"), wxICON_ERROR);
    }
    cmd.erase();
    cmd.append("test -d ");
	cmd.append(homedir);
	cmd.append("/.emuclient || cp -r /etc/emuclient ");
	cmd.append(homedir);
	cmd.append("/.emuclient");
    if(system(cmd.c_str()) < 0)
    {
        wxMessageBox( wxT("Cannot open emuclient config directory!"), wxT("Error"), wxICON_ERROR);
    }

    read_sixaxis_config(Choice1, Choice2);

    refresh();
}

sixemuguiFrame::~sixemuguiFrame()
{
    //(*Destroy(sixemuguiFrame)
    //*)
}

void sixemuguiFrame::OnQuit(wxCommandEvent& event)
{
    int answer;
    int i;

    for(i=0; i<7; ++i)
    {
        if(dongle[i] >= 0) break;
    }

    if(i < 7)
    {
        answer = wxMessageBox(_("Emu processes will be killed!"), _("Confirm"), wxNO | wxCANCEL);
        if (answer == wxYES)
        {
            g_spawn_command_line_sync ("killall emu", NULL, NULL, NULL, NULL);
        }
        else
        {
            return;
        }
    }
    Close();
}

void sixemuguiFrame::OnAbout(wxCommandEvent& event)
{
    wxString msg = wxbuildinfo(long_f);
    wxMessageBox(msg, _("Welcome to..."));
}

void sixemuguiFrame::OnSelectSixaxisBdaddr(wxCommandEvent& event)
{
    Choice2->SetSelection(Choice1->GetSelection());
}

void sixemuguiFrame::OnSelectPS3Bdaddr(wxCommandEvent& event)
{
    Choice1->SetSelection(Choice2->GetSelection());
}

void sixemuguiFrame::OnSelectBtDongle(wxCommandEvent& event)
{
    Choice5->SetSelection(Choice3->GetSelection());
    Choice6->SetSelection(Choice3->GetSelection());
    Choice7->SetSelection(Choice3->GetSelection());
}

void sixemuguiFrame::OnSelectRefresh(wxCommandEvent& event)
{
    refresh();
}

void sixemuguiFrame::OnButton2Click(wxCommandEvent& event)
{
    int answer;

    if(dongleInUse[Choice3->GetSelection()])
    {
        wxMessageBox( wxT("This dongle is in use!"), wxT("Error"), wxICON_ERROR);
        return;
    }

    answer = wxMessageBox(_("Did you saved your dongle address?"), _("Confirm"), wxYES_NO | wxCANCEL);
    if (answer == wxYES)
    {
        setDongleAddress();
    }
    else if (answer == wxNO)
    {
        wxMessageBox(_("Please save it!"), _("Info"));
    }
}

void sixemuguiFrame::OnChoice5Select(wxCommandEvent& event)
{
    Choice3->SetSelection(Choice5->GetSelection());
    Choice6->SetSelection(Choice5->GetSelection());
    Choice7->SetSelection(Choice5->GetSelection());
}

void sixemuguiFrame::OnChoice6Select(wxCommandEvent& event)
{
    Choice3->SetSelection(Choice6->GetSelection());
    Choice5->SetSelection(Choice6->GetSelection());
    Choice7->SetSelection(Choice6->GetSelection());
}

void sixemuguiFrame::OnChoice7Select(wxCommandEvent& event)
{
    Choice3->SetSelection(Choice7->GetSelection());
    Choice5->SetSelection(Choice7->GetSelection());
    Choice6->SetSelection(Choice7->GetSelection());
}

static char emu_bdaddr[18];
static char bt_device[4];
static char controller[2];

static const char *emu_command[] = { "emu", emu_bdaddr, bt_device, controller, NULL };

typedef enum
{
    E_CONNECTING,
    E_CONNECTED,
    E_DISCONNECTED,
    E_ERROR
} e_emu_state;

e_emu_state emu_state = E_DISCONNECTED;

void* emu_thread(void* arg)
{
    GError *error = NULL;
    gboolean test;
    GPid child_pid;
    int out = -1;
    char tmp[1024];
    int ret;

    test = g_spawn_async_with_pipes(NULL, (gchar**)emu_command, NULL, (GSpawnFlags)(G_SPAWN_SEARCH_PATH), NULL, NULL, &child_pid, NULL, &out, NULL,  &error);

    if(error)
    {
        printf("%s\n", error->message);
    }

    if(test)
    {
        *(int*)arg = child_pid;
        while(1)
        {
            ret = read(out, tmp, 1024);
            if(ret == -1 && errno != EAGAIN)
            {
                emu_state = E_ERROR;
                break;
            }
            if(ret > 0)
            {
                if(ret == 1024)
                {
                    tmp[1023] = '\0';
                }
                else
                {
                    tmp[ret] = '\0';
                }
                cout << tmp << endl;
                if(strstr(tmp, "connected"))
                {
                    emu_state = E_CONNECTED;
                    break;
                }
                else if(strstr(tmp, "can't connect to"))
                {
                    emu_state = E_ERROR;
                    break;
                }
            }
            sleep(1);
        }
    }

    if(error) g_free(error);
    //if(out > -1) close(out);
    printf("emu_thread: end\n");
    pthread_exit (0);
}

void sixemuguiFrame::OnButton1Click(wxCommandEvent& event)
{
    pthread_t thread;
    pthread_attr_t thread_attr;
    string command;
    unsigned char c_id = Choice8->GetSelection();
    unsigned char d_id = Choice3->GetSelection();
    char kill_command[32];
    int i;

    if(dongle[c_id] < 0)
    {
        if(!dongleInUse[d_id])
        {
            /*
             * This allows not to launch the emu process as root.
             */
//            command.append("gksudo --description Sixemugui hciconfig hci");
//            stringstream stream;
//            stream << Choice3->GetSelection();
//            command.append(stream.str());
//            command.append(" class 0x508");
//            g_spawn_command_line_sync (command.c_str(), NULL, NULL, NULL, NULL);
            /*
             * Update variables to be read by the thread.
             */
            strncpy(emu_bdaddr, Choice2->GetStringSelection().mb_str(), 18 );
            snprintf(bt_device, 4, "%d", d_id);
            snprintf(controller, 2, "%d", c_id);
            /*
             * Launches the emu process.
             */
            emu_state = E_CONNECTING;
            pthread_attr_init(&thread_attr);
            pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
            pthread_create( &thread, &thread_attr, emu_thread, pid+c_id);

            while(emu_state == E_CONNECTING)
            {
                usleep(100000);
            }
            if(emu_state == E_CONNECTED)
            {
                /*
                 * Update the GUI.
                 */
                Button1->SetLabel(_("Stop"));
                dongleInUse[d_id] = true;
                dongle[c_id] = d_id;
                switch(c_id)
                {
                    case 0:
                    StaticText5->Enable();
                    StaticText5->SetForegroundColour( wxColour(255, 0, 0) );
                    break;
                    case 1:
                    StaticText9->Enable();
                    StaticText9->SetForegroundColour( wxColour(255, 0, 0) );
                    break;
                    case 2:
                    StaticText10->Enable();
                    StaticText10->SetForegroundColour( wxColour(255, 0, 0) );
                    break;
                    case 3:
                    StaticText11->Enable();
                    StaticText11->SetForegroundColour( wxColour(255, 0, 0) );
                    break;
                    case 4:
                    StaticText12->Enable();
                    StaticText12->SetForegroundColour( wxColour(255, 0, 0) );
                    break;
                    case 5:
                    StaticText13->Enable();
                    StaticText13->SetForegroundColour( wxColour(255, 0, 0) );
                    break;
                    case 6:
                    StaticText14->Enable();
                    StaticText14->SetForegroundColour( wxColour(255, 0, 0) );
                    break;
                    default:
                    break;
                }
                Button3->Enable();
                if(CheckBox5->IsChecked())
                {
                    OnButton3Click(event);
                }
                else
                {
                    wxMessageBox(_("Connected!\nStart emuclient now."), _("Info"));
                }
            }
            else if(emu_state == E_ERROR)
            {
                pid[c_id] = -1;
                wxMessageBox( wxT("Connection error!\nDid you set the dongle address?\nIf yes, try another dongle!"), wxT("Error"), wxICON_ERROR);
            }
        }
        else
        {
            wxMessageBox( wxT("This dongle is already used!"), wxT("Error"), wxICON_ERROR);
        }
    }
    else
    {
        /*
         * Kills the emu process.
         */
        sprintf(kill_command, "kill %d", pid[c_id]);
        cout << kill_command << endl;
        g_spawn_command_line_sync (kill_command, NULL, NULL, NULL, NULL);
        Button1->SetLabel(_("Start"));
        dongleInUse[dongle[c_id]] = false;
        dongle[c_id] = -1;
        switch(c_id)
        {
            case 0:
            StaticText5->Disable();
            break;
            case 1:
            StaticText9->Disable();
            break;
            case 2:
            StaticText10->Disable();
            break;
            case 3:
            StaticText11->Disable();
            break;
            case 4:
            StaticText12->Disable();
            break;
            case 5:
            StaticText13->Disable();
            break;
            case 6:
            StaticText14->Disable();
            break;
            default:
            break;
        }
        for(i=0; i<7; ++i)
        {
            if(dongle[i] >= 0)
            {
                break;
            }
        }
        if(i == 7)
        {
            Button3->Disable();
        }
    }
}

void sixemuguiFrame::OnButton3Click(wxCommandEvent& event)
{
    string command = "";
    string filename = "";

    if(CheckBox3->IsChecked())
    {
        command.append("gnome-terminal -e \"");
    }
    else
    {
        command.append("/bin/bash -c \"");
    }
    command.append(" emuclient");
    if(!CheckBox1->IsChecked())
    {
        command.append(" --nograb");
    }
    if(CheckBox6->IsChecked())
    {
        command.append(" --force-updates");
    }
    command.append(" --config ");
    command.append(Choice4->GetStringSelection().mb_str());
    if(CheckBox2->IsChecked())
    {
        command.append(" --status | sixstatus");
    }
    else if(CheckBox3->IsChecked())
    {
        command.append(" --status");
    }
    command.append("\"");

    //cout << command << endl;

    Button3->Disable();
    filename.append(homedir);
    filename.append("/.sixemugui/default");
    ofstream outfile (filename.c_str(), ios_base::trunc);
    if(outfile.is_open())
    {
        outfile << Choice4->GetStringSelection().mb_str() << endl;
        outfile.close();
    }
    g_spawn_command_line_sync (command.c_str(), NULL, NULL, NULL, NULL);
    Button3->Enable();
}


void sixemuguiFrame::OnSave(wxCommandEvent& event)
{
    string filename;
    string line;
    string device;
    string master;
    unsigned int i;

    filename.append(homedir);
    filename.append("/.sixemugui/config");

    ofstream outfile (filename.c_str(), ios_base::trunc);

    if(outfile.is_open())
    {
        for(i=0; i<Choice1->GetCount(); ++i)
        {
            device = string(Choice1->GetString(i).mb_str());
            master = string(Choice2->GetString(i).mb_str());
            outfile << device << " " << master << endl;
        }
        outfile.close();
    }
    else
    {
        wxMessageBox( wxT("Cannot open config directory!"), wxT("Error"), wxICON_ERROR);
    }
}

void sixemuguiFrame::OnCheckBox2Click(wxCommandEvent& event)
{
    CheckBox3->SetValue(false);
}

void sixemuguiFrame::OnCheckBox3Click(wxCommandEvent& event)
{
    CheckBox2->SetValue(false);
}

void sixemuguiFrame::OnChoice8Select(wxCommandEvent& event)
{
    if(dongle[Choice8->GetSelection()] >= 0)
    {
        Button1->SetLabel(_("Stop"));
    }
    else
    {
        Button1->SetLabel(_("Start"));
    }
}

void sixemuguiFrame::OnCheckBoxCalibrate(wxCommandEvent& event)
{
    if(CheckBox4->IsChecked())
    {
        CheckBox2->SetValue(true);
        CheckBox3->SetValue(false);
    }
    else
    {
        CheckBox2->SetValue(false);
    }
}
