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

        auto* choice = new wxChoice(this, wxID_ANY);
        choice->Append("Coverflow");
        choice->Append("Flat");
        choice->Append("Stack");
        choice->Append("Wheel");
        choice->Append("Fan");
        choice->Append("Tilted");
        choice->Append("Spiral");
        choice->Append("Wave");
        choice->Append("Carousel3D");
        choice->Append("Cylinder");
        choice->Append("Helix");
        choice->Append("Flip");
        choice->Append("Pendulum");
        choice->Append("Staircase");
        choice->Append("Accordion");
        choice->Append("Vortex");
        choice->Append("Ripple");
        choice->Append("Bounce");
        choice->Append("Cascade");
        choice->Append("Tunnel");
        choice->Append("Blinds");
        choice->Append("Fade");
        choice->Append("Slide");
        choice->Append("Swing");
        choice->Append("Twirl");
        choice->Append("Ribbon");
        choice->Append("Pyramid");
        choice->Append("Diagonal");
        choice->Append("Spring");
        choice->Append("Blast");
        choice->Append("Orbit");
        choice->Append("Grid");
        choice->Append("Rain");
        choice->Append("Fountain");
        choice->Append("Ring");
        choice->Append("Cross");
        choice->Append("Diamond");
        choice->Append("Curtain");
        choice->Append("Circle");
        choice->Append("Boomerang");
        choice->Append("Infinity");
        choice->Append("Corkscrew");
        choice->Append("Galaxy");
        choice->Append("Pulse");
        choice->Append("Target");
        choice->Append("Pinwheel");
        choice->Append("Scatter");
        choice->Append("Converge");
        choice->Append("Diverge");
        choice->Append("Wave2D");
        canvas->SyncDisplayModeChoice(choice);

        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(choice, 0, wxALL | wxALIGN_CENTER, 8);
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
