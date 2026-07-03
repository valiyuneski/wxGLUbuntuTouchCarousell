// main.cpp - demo host for CarouselCanvas
#include <wx/wx.h>
#include "Carousel.h"

class CarouselFrame : public wxFrame
{
public:
    CarouselFrame()
        : wxFrame(nullptr, wxID_ANY, "Ubuntu Touch Style Carousel",
                  wxDefaultPosition, wxSize(1000, 620))
    {
        SetBackgroundColour(wxColour(15, 17, 20));

        auto* canvas = new CarouselCanvas(this);
        canvas->AddItem("Music",    wxColour(79, 209, 197));
        canvas->AddItem("Videos",   wxColour(255, 159, 67));
        canvas->AddItem("Photos",   wxColour(255, 99, 132));
        canvas->AddItem("Files",    wxColour(154, 205, 50));
        canvas->AddItem("Weather",  wxColour(100, 181, 246));
        canvas->AddItem("Settings", wxColour(186, 156, 255));
        canvas->AddItem("Store",    wxColour(255, 205, 86));

        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(canvas, 1, wxEXPAND);
        SetSizer(sizer);

        canvas->SetFocus();
    }
};

class CarouselApp : public wxApp
{
public:
    bool OnInit() override
    {
        auto* frame = new CarouselFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(CarouselApp);
