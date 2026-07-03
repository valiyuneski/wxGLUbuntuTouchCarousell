// Carousel.h
// Header-only Ubuntu-Touch-style 3D carousel widget.
// wxWidgets + immediate-mode OpenGL + wxTimer animation loop.
//
// Ubuntu Touch's classic scope/dash carousel: a row of cards, the centered
// one faces the camera at full size, cards to the left/right shrink, push
// back in depth and rotate inward (coverflow-like), with inertial drag and
// snap-to-item settling.
//
// Usage:
//   CarouselCanvas* car = new CarouselCanvas(parent);
//   car->AddItem("Music",  wxColour(79, 209, 197));
//   car->AddItem("Videos", wxColour(255, 159, 67));
//   ...
//
#pragma once

#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <wx/timer.h>
#include <wx/graphics.h>
#include <wx/dcmemory.h>

#include <vector>
#include <cmath>
#include <algorithm>

#ifdef __APPLE__
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

// ---------------------------------------------------------------------------
// CarouselItem
// ---------------------------------------------------------------------------
struct CarouselItem
{
    wxString label;
    wxColour accent;
    GLuint   texture   = 0;
    int      texWidth  = 0;
    int      texHeight = 0;

    CarouselItem(const wxString& lbl, const wxColour& col)
        : label(lbl), accent(col) {}
};

// ---------------------------------------------------------------------------
// CarouselCanvas
// ---------------------------------------------------------------------------
class CarouselCanvas : public wxGLCanvas
{
public:
    explicit CarouselCanvas(wxWindow* parent)
        : wxGLCanvas(parent, GetGLAttribs(), wxID_ANY,
                     wxDefaultPosition, wxDefaultSize,
                     wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS)
        , m_timer(this)
    {
        m_context = new wxGLContext(this);

        Bind(wxEVT_PAINT,       &CarouselCanvas::OnPaint,      this);
        Bind(wxEVT_SIZE,        &CarouselCanvas::OnSize,       this);
        Bind(wxEVT_TIMER,       &CarouselCanvas::OnTimer,      this);
        Bind(wxEVT_LEFT_DOWN,   &CarouselCanvas::OnMouseDown,  this);
        Bind(wxEVT_LEFT_UP,     &CarouselCanvas::OnMouseUp,    this);
        Bind(wxEVT_MOTION,      &CarouselCanvas::OnMouseMove,  this);
        Bind(wxEVT_MOUSEWHEEL,  &CarouselCanvas::OnMouseWheel, this);
        Bind(wxEVT_MOUSE_CAPTURE_LOST, &CarouselCanvas::OnCaptureLost, this);
        Bind(wxEVT_LEAVE_WINDOW, &CarouselCanvas::OnMouseLeave, this);
        Bind(wxEVT_KEY_DOWN,    &CarouselCanvas::OnKeyDown,    this);

        SetBackgroundStyle(wxBG_STYLE_PAINT);
        m_timer.Start(16); // ~60 FPS
    }

    ~CarouselCanvas() override
    {
        if (m_glInitialized)
        {
            SetCurrent(*m_context);
            for (auto& item : m_items)
                if (item.texture) glDeleteTextures(1, &item.texture);
        }
        delete m_context;
    }

    void AddItem(const wxString& label, const wxColour& accent)
    {
        m_items.emplace_back(label, accent);
    }

    // Jump (animated) to a given item index.
    void GoTo(int index)
    {
        index = std::clamp(index, 0, (int)m_items.size() - 1);
        m_target      = (double)index;
        m_snapping    = true;
        m_velocity    = 0.0;
    }

    int CurrentIndex() const { return (int)std::lround(m_offset); }
    int ItemCount()    const { return (int)m_items.size(); }
    wxString ItemLabel(int i) const { return m_items[i].label; }

    // Display mode
    enum DisplayMode {
        Coverflow, Flat, Stack, Wheel, Fan, Tilted, Spiral, Wave, Carousel3D, Cylinder,
        Helix, Flip, Pendulum, Staircase, Accordion, Vortex, Ripple, Bounce, Cascade, Tunnel,
        Blinds, Fade, Slide, Swing, Twirl, Ribbon, Pyramid, Diagonal, Spring, Blast,
        Orbit, Grid, Rain, Fountain, Ring, Cross, Diamond, Curtain, Circle, Boomerang,
        Infinity, Corkscrew, Galaxy, Pulse, Target, Pinwheel, Scatter, Converge, Diverge, Wave2D,
        Domino, Flag, Hinge, Lattice, Tornado, Mobius, Propeller, Slinky, Paddle, Portal,
        Prism, Shuffle, Tide, Umbrella, Wing, Wobble, YoYo, Oscillate, Drift, Hover,
        Avalanche, Bridge, Cone, Deck, Drum, Feather, Flower, Glide, Gravity, Hop,
        Orbit3D, Petal, Rocket, Sail, Snow, Spider, Sway, Tumble, Vibrate, Zigzag,
        Butterfly, Caterpillar, Firework, Jelly, Kaleido, Ribbon3D, Ripple2D, Spectrum, Splash, Twister,
        Aurora, Cascade3D, Crown, Cyclone, Dandelion, Echo, Funnel, Heart, Honeycomb, Lantern,
        Bubble, Comet, Garland, Globe, Leaf, Meteor, Mountain, Nebula, Origami, Peacock,
        Arch, Arrow, Balloon, Blizzard, Books, Cage, Candle, Chain, Checker, Clock,
        Cloud, Coil, Compass, Cube, Dice, Dome, Door, Fence, Fog, Gear,
        Hammer, Horn, Hurricane, Kite, Ladder, Lens, Lock, Magnet, Mask, Net,
        Nut, Orb, Pillar, Puzzle, Rack, Ramp, Reel, Rope, Saw, Scale,
        Screw, Shield, Spike, Star, Sun, Swan, Sword, Temple, Tree, Turbine,
        Alpha, Bravo, Charlie, Delta, Foxtrot, Golf, Hotel, India, Juliet, Kilo,
        Lima, Mike, November, Oscar, Papa, Quebec, Romeo, Sierra, Tango, Uniform,
        Victor, Whiskey, Xray, Yankee, Zulu,
        Oval, Hexagon, Octagon, Crescent, Trapezoid, Parabola, Teardrop, Lozenge, Chevron, Arc2D,
        Bolt2D, Drop2D, Ring3D, Diamond3D, Wiggle2D,
        Blossom, Breeze, Burst, Canyon, Cave, Cliff, Cosmos2D, Crater, Dune2D, Geode,
        Glacier, Grove, Iceberg, Jungle, Lava2D, Meadow, Ocean2D, Prairie, Rapids, Reef,
        Ridge, River, Sand, Shore, Valley, Vine, Volcano, Water2D, Woods, Abyss,
        Anchor2D, Barrel, Basket, Bell, Blade, Block, Board, Boat, Brick, Castle,
        Chair, Chest, Crown2D, Disc, Dome2D, Drum2D, Flag2D, Frame, Gate, Gem,
        Globe2D, Goblet, Hat, Helmet, Lamp, Shield2D, Throne, Trophy, Wheel2D, Beacon,
        Canvas, Chasm, Core, Depth, Edge, Field, Haze, Mist, Smoke, Echo2D
    };

    void SetDisplayMode(DisplayMode mode)
    {
        m_displayMode = mode;
        if (m_displayModeChoice)
            m_displayModeChoice->SetSelection((int)mode);
        Refresh(false);
    }

    DisplayMode GetDisplayMode() const { return m_displayMode; }

    // Attach a wxChoice dropdown that stays in sync with the carousel.
    void SyncChoice(wxChoice* choice) { m_choice = choice; }

    // Attach a wxChoice for display mode selection.
    void SyncDisplayModeChoice(wxChoice* choice)
    {
        m_displayModeChoice = choice;
        if (choice)
        {
            choice->SetSelection((int)m_displayMode);
            choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent& evt) {
                SetDisplayMode((DisplayMode)evt.GetInt());
            });
        }
    }

private:
    // -- OpenGL / layout constants -----------------------------------------
    static constexpr double kSpacing     = 1.35;  // horizontal gap between items, in "card widths"
    static constexpr double kMaxAngleDeg = 55.0;  // side-card inward rotation
    static constexpr double kZStep       = 0.55;  // how far back side cards sink
    static constexpr double kScaleFalloff= 0.32;  // how quickly side cards shrink
    static constexpr double kVisibleSpan = 4.0;   // cull cards further than this
    static constexpr double kFriction    = 0.90;  // per-frame velocity decay
    static constexpr double kSnapSpeed   = 0.18;  // ease factor towards snapped target
    static constexpr double kDragPxPerItem = 220.0;

    wxGLContext* m_context;
    wxTimer      m_timer;
    bool         m_glInitialized = false;

    std::vector<CarouselItem> m_items;

    double m_offset   = 0.0;   // continuous "center index" position
    double m_velocity = 0.0;
    double m_target   = 0.0;
    bool   m_snapping = false;

    wxChoice* m_choice           = nullptr;
    wxChoice* m_displayModeChoice = nullptr;
    DisplayMode m_displayMode     = Coverflow;

    bool    m_dragging = false;
    wxPoint m_dragStart;
    double  m_dragStartOffset = 0.0;
    wxLongLong m_lastDragTimeMs;
    double  m_lastDragOffset = 0.0;

    static wxGLAttributes GetGLAttribs()
    {
        wxGLAttributes attrs;
        attrs.PlatformDefaults().RGBA().DoubleBuffer().Depth(16).EndList();
        return attrs;
    }

    // -----------------------------------------------------------------
    void InitGL()
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_LINE_SMOOTH);
        for (auto& item : m_items)
            BuildTexture(item);
        m_glInitialized = true;
    }

    // Render a card's label onto a wxBitmap, upload as an RGBA texture.
    void BuildTexture(CarouselItem& item)
    {
        const int w = 512, h = 320;
        wxBitmap bmp(w, h, 32);
        {
            wxMemoryDC dc(bmp);
            wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
            gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

            // Transparent background, draw only foreground glyphs; alpha
            // channel built from an offscreen mask below.
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawRectangle(0, 0, w, h);

            // Large index/icon circle
            wxColour accent = item.accent;
            gc->SetPen(wxPen(accent, 6));
            gc->SetBrush(wxBrush(wxColour(accent.Red(), accent.Green(), accent.Blue(), 40)));
            gc->DrawEllipse(w/2.0 - 70, 30, 140, 140);

            gc->SetFont(wxFontInfo(56).Bold(), accent);
            wxDouble tw, th;
            wxString idx = item.label.Left(1).Upper();
            gc->GetTextExtent(idx, &tw, &th);
            gc->DrawText(idx, w/2.0 - tw/2.0, 30 + 70 - th/2.0);

            gc->SetFont(wxFontInfo(34).Bold(), wxColour(230, 235, 238));
            gc->GetTextExtent(item.label, &tw, &th);
            gc->DrawText(item.label, w/2.0 - tw/2.0, h - 90);

            delete gc;
        }

        wxImage img = bmp.ConvertToImage();
        if (!img.HasAlpha())
            img.InitAlpha();

        // Build alpha from luminance so untouched (black) areas are fully
        // transparent and drawn glyph/shape areas are opaque.
        unsigned char* data  = img.GetData();
        unsigned char* alpha = img.GetAlpha();
        for (int i = 0; i < w * h; ++i)
        {
            unsigned char r = data[i*3+0], g = data[i*3+1], b = data[i*3+2];
            int lum = (r + g + b) / 3;
            alpha[i] = (unsigned char)std::clamp(lum * 2, 0, 255);
        }

        std::vector<unsigned char> rgba(w * h * 4);
        for (int i = 0; i < w * h; ++i)
        {
            rgba[i*4+0] = data[i*3+0];
            rgba[i*4+1] = data[i*3+1];
            rgba[i*4+2] = data[i*3+2];
            rgba[i*4+3] = alpha[i];
        }

        glGenTextures(1, &item.texture);
        glBindTexture(GL_TEXTURE_2D, item.texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

        item.texWidth  = w;
        item.texHeight = h;
    }

    // -----------------------------------------------------------------
    void OnPaint(wxPaintEvent&)
    {
        if (!IsShownOnScreen()) return;
        wxPaintDC dc(this);
        SetCurrent(*m_context);

        if (!m_glInitialized)
            InitGL();

        Render();
        SwapBuffers();
    }

    void OnSize(wxSizeEvent& evt)
    {
        if (IsShownOnScreen())
        {
            SetCurrent(*m_context);
            wxSize sz = evt.GetSize() * GetContentScaleFactor();
            glViewport(0, 0, sz.x, sz.y);
        }
        evt.Skip();
    }

    void OnTimer(wxTimerEvent&)
    {
        bool needsRefresh = false;

        if (m_dragging)
        {
            // nothing to integrate while actively dragging; offset is set
            // directly in OnMouseMove.
            needsRefresh = true;
        }
        else if (std::abs(m_velocity) > 0.0005 && !m_snapping)
        {
            m_offset   += m_velocity;
            m_velocity *= kFriction;
            ClampOffset();
            if (std::abs(m_velocity) <= 0.0005)
            {
                m_velocity = 0.0;
                m_snapping = true;
                m_target   = std::round(m_offset);
            }
            needsRefresh = true;
        }
        else if (m_snapping)
        {
            double diff = m_target - m_offset;
            if (std::abs(diff) < 0.0015)
            {
                m_offset = m_target;
                m_snapping = false;
            }
            else
            {
                m_offset += diff * kSnapSpeed;
            }
            needsRefresh = true;
        }

        if (needsRefresh)
        {
            if (m_choice)
            {
                int idx = CurrentIndex();
                if (idx != m_choice->GetSelection())
                    m_choice->SetSelection(idx);
            }
            Refresh(false);
        }
    }

    void ClampOffset()
    {
        double maxIdx = (double)std::max((int)m_items.size() - 1, 0);
        if (m_offset < 0.0)   { m_offset = 0.0;   m_velocity = 0.0; }
        if (m_offset > maxIdx){ m_offset = maxIdx; m_velocity = 0.0; }
    }

    // -- Input ---------------------------------------------------------
    void OnMouseDown(wxMouseEvent& evt)
    {
        m_dragging        = true;
        m_snapping        = false;
        m_velocity        = 0.0;
        m_dragStart       = evt.GetPosition();
        m_dragStartOffset = m_offset;
        m_lastDragTimeMs  = wxGetLocalTimeMillis();
        m_lastDragOffset  = m_offset;
        CaptureMouse();
    }

    void OnMouseMove(wxMouseEvent& evt)
    {
        if (!m_dragging || !evt.Dragging() || !evt.LeftIsDown()) return;

        int dx = evt.GetPosition().x - m_dragStart.x;
        m_offset = m_dragStartOffset - dx / kDragPxPerItem;
        ClampOffset();

        wxLongLong now = wxGetLocalTimeMillis();
        wxLongLong dt  = now - m_lastDragTimeMs;
        if (dt.GetValue() > 0)
        {
            m_velocity = (m_offset - m_lastDragOffset) / (dt.GetValue() / 16.0);
            m_lastDragTimeMs = now;
            m_lastDragOffset = m_offset;
        }
        Refresh(false);
    }

    void OnMouseUp(wxMouseEvent&)
    {
        FinishDrag();
    }

    void OnMouseLeave(wxMouseEvent&)
    {
        if (m_dragging) FinishDrag();
    }

    void OnCaptureLost(wxMouseCaptureLostEvent&)
    {
        m_dragging = false;
    }

    void FinishDrag()
    {
        if (!m_dragging) return;
        m_dragging = false;
        if (HasCapture()) ReleaseMouse();

        if (std::abs(m_velocity) < 0.01)
        {
            m_snapping = true;
            m_target   = std::round(m_offset);
            m_velocity = 0.0;
        }
        // otherwise let inertia carry it; OnTimer will trigger snapping
        // once velocity decays.
    }

    void OnMouseWheel(wxMouseEvent& evt)
    {
        int step = evt.GetWheelRotation() > 0 ? -1 : 1;
        GoTo(CurrentIndex() + step);
    }

    void OnKeyDown(wxKeyEvent& evt)
    {
        switch (evt.GetKeyCode())
        {
            case WXK_LEFT:  GoTo(CurrentIndex() - 1); break;
            case WXK_RIGHT: GoTo(CurrentIndex() + 1); break;
            default: evt.Skip();
        }
    }

    // -- Rendering -------------------------------------------------------
    void Render()
    {
        wxSize sz = GetClientSize() * GetContentScaleFactor();
        if (sz.x <= 0 || sz.y <= 0) return;

        glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        double aspect = (double)sz.x / (double)sz.y;

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        ApplyPerspective(45.0, aspect, 0.1, 50.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -4.2f);

        DrawBackdrop();

        if (m_items.empty()) return;

        // Draw back-to-front for correct alpha blending / depth ordering.
        std::vector<int> order;
        for (int i = 0; i < (int)m_items.size(); ++i)
        {
            double d = i - m_offset;
            if (std::abs(d) <= kVisibleSpan) order.push_back(i);
        }
        std::sort(order.begin(), order.end(), [&](int a, int b) {
            return std::abs(a - m_offset) > std::abs(b - m_offset);
        });

        for (int i : order)
            DrawCard(i);
    }

    // Manual replacement for gluPerspective (no GLU dependency required).
    static void ApplyPerspective(double fovYDeg, double aspect, double zNear, double zFar)
    {
        double fH = std::tan(fovYDeg * M_PI / 360.0) * zNear;
        double fW = fH * aspect;
        glFrustum(-fW, fW, -fH, fH, zNear, zFar);
    }

    void DrawBackdrop()
    {
        // Subtle horizon glow behind the carousel, matching a dark HMI look.
        glPushMatrix();
        glTranslatef(0.0f, -1.2f, -3.0f);
        glBegin(GL_TRIANGLE_FAN);
            glColor4f(0.10f, 0.16f, 0.17f, 0.6f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glColor4f(0.06f, 0.07f, 0.09f, 0.0f);
            const int segs = 32;
            for (int s = 0; s <= segs; ++s)
            {
                double a = M_PI * s / segs;
                glVertex3f((float)std::cos(a) * 6.0f, (float)std::sin(a) * 2.5f, 0.0f);
            }
        glEnd();
        glPopMatrix();
    }

    void DrawCard(int i)
    {
        const CarouselItem& item = m_items[i];
        double d = i - m_offset;
        double absd = std::abs(d);

        double x, y, z, angle, angleX, scale, focus, alpha;
        y = 0.0;
        angleX = 0.0;
        alpha = 1.0;
        const double cardW = 1.6, cardH = 1.0;

        switch (m_displayMode)
        {
        case Flat:
        {
            x     = d * kSpacing;
            angle = 0.0;
            z     = 0.0;
            scale = (absd < 0.5) ? 1.0 : 0.85;
            focus = std::clamp(1.0 - absd * 0.35, 0.15, 1.0);
            break;
        }
        case Stack:
        {
            const double stackSide = (d >= 0) ? -1.0 : 1.0;
            x     = stackSide * 0.06;
            z     = -absd * 0.12;
            angle = stackSide * 4.0;
            scale = 1.0 / (1.0 + absd * 0.25);
            focus = (absd < 0.5) ? 1.0 : 0.6;
            break;
        }
        case Wheel:
        {
            double rad = d * 0.35;
            x     = std::sin(rad) * 2.2;
            z     = (1.0 - std::cos(rad)) * 2.2 - 0.4;
            angle = d * 20.0;
            scale = 1.0;
            focus = std::clamp(1.0 - absd * 0.12, 0.3, 1.0);
            break;
        }
        case Fan:
        {
            const double sign = (d >= 0) ? 1.0 : -1.0;
            double t = absd * 0.18;
            x     = (t + 0.3) * sign;
            z     = -absd * 0.08;
            angle = sign * 40.0;
            scale = 1.0 / (1.0 + absd * 0.15);
            focus = (absd < 0.5) ? 1.0 : 0.5;
            break;
        }
        case Tilted:
        {
            x     = d * kSpacing;
            angle = 15.0;
            z     = -absd * kZStep;
            scale = 1.0 / (1.0 + absd * kScaleFalloff);
            focus = std::clamp(1.0 - absd / kVisibleSpan, 0.0, 1.0);
            break;
        }
        case Spiral:
        {
            double rad = d * 0.5;
            x     = std::cos(rad) * (1.2 + absd * 0.3);
            z     = std::sin(rad) * (1.2 + absd * 0.3) - absd * 0.15;
            angle = d * 45.0;
            scale = 1.0 / (1.0 + absd * 0.35);
            focus = std::clamp(1.0 - absd * 0.2, 0.0, 1.0);
            break;
        }
        case Wave:
        {
            x     = d * kSpacing;
            angle = 0.0;
            z     = std::sin(d * 1.2) * 0.35;
            scale = 1.0 / (1.0 + absd * 0.1);
            focus = std::clamp(1.0 - absd * 0.15, 0.2, 1.0);
            break;
        }
        case Carousel3D:
        {
            double rad = d * 0.45;
            x     = std::sin(rad) * 2.5;
            z     = std::cos(rad) * 2.5 - 2.5;
            angle = -d * 25.0;
            scale = 1.0;
            focus = std::clamp(1.0 - absd * 0.08, 0.4, 1.0);
            break;
        }
        case Cylinder:
        {
            double rad = d * 0.3;
            x     = std::sin(rad) * 2.0;
            z     = (1.0 - std::cos(rad)) * 2.0 - 0.3;
            angle = -d * 18.0;
            scale = 1.0;
            focus = std::clamp(1.0 - absd * 0.1, 0.35, 1.0);
            break;
        }
        case Helix:
        {
            double rad = d * 0.6;
            x     = std::cos(rad) * 1.8;
            z     = std::sin(rad) * 1.8 - absd * 0.1;
            angle = d * 30.0;
            scale = 1.0 / (1.0 + absd * 0.2);
            focus = std::clamp(1.0 - absd * 0.12, 0.3, 1.0);
            y     = std::sin(d * 0.5) * 0.4;
            break;
        }
        case Flip:
        {
            x     = d * kSpacing;
            z     = -absd * 0.15;
            scale = 1.0 / (1.0 + absd * 0.2);
            focus = std::clamp(1.0 - absd * 0.2, 0.3, 1.0);
            {
                const double sign = (d >= 0) ? -1.0 : 1.0;
                double raw = 180.0;
                angle = (absd < 0.5) ? raw * (absd / 0.5) : raw;
                angle *= sign;
            }
            break;
        }
        case Pendulum:
        {
            double swing = std::sin(d * 0.8) * 0.5;
            x     = d * kSpacing;
            z     = -absd * 0.12;
            angle = 0.0;
            scale = 1.0 / (1.0 + absd * 0.15);
            focus = std::clamp(1.0 - absd * 0.15, 0.2, 1.0);
            y     = std::abs(swing);
            break;
        }
        case Staircase:
        {
            x     = d * kSpacing;
            angle = 0.0;
            z     = -absd * 0.25;
            scale = 1.0;
            focus = std::clamp(1.0 - absd * 0.12, 0.3, 1.0);
            y     = d * 0.25;
            break;
        }
        case Accordion:
        {
            x     = d * kSpacing;
            z     = -(absd * 0.08 + std::abs(std::sin(d * M_PI)) * 0.3);
            angle = 0.0;
            scale = 1.0 / (1.0 + absd * 0.1);
            focus = std::clamp(1.0 - absd * 0.1, 0.4, 1.0);
            break;
        }
        case Vortex:
        {
            double t = d * 0.7;
            double r = 0.3 + absd * 0.25;
            x     = std::cos(t) * r * 2.5;
            z     = std::sin(t) * r * 2.5 - absd * 0.2;
            angle = d * 40.0;
            scale = 1.0 / (1.0 + absd * 0.3);
            focus = std::clamp(1.0 - absd * 0.18, 0.0, 1.0);
            break;
        }
        case Ripple:
        {
            double r = absd * 0.4;
            x     = d * kSpacing;
            z     = -std::sin(r * M_PI * 2.0) * 0.3 * std::max(0.0, 1.0 - absd * 0.2);
            angle = 0.0;
            scale = 1.0 / (1.0 + absd * 0.08);
            focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0);
            break;
        }
        case Bounce:
        {
            x     = d * kSpacing;
            z     = -absd * 0.1;
            angle = 0.0;
            scale = 1.0 / (1.0 + absd * 0.12);
            focus = std::clamp(1.0 - absd * 0.12, 0.3, 1.0);
            y     = std::abs(std::sin(d * M_PI * 0.8)) * 0.4 * std::max(0.0, 1.0 - absd * 0.15);
            break;
        }
        case Cascade:
        {
            x     = d * kSpacing;
            z     = -absd * 0.2;
            angle = d * 10.0;
            scale = 1.0 / (1.0 + absd * 0.15);
            focus = std::clamp(1.0 - absd * 0.15, 0.2, 1.0);
            y     = -absd * 0.2;
            break;
        }
        case Tunnel:
        {
            double t = d * 0.25;
            x     = std::sin(t) * 1.5;
            z     = std::cos(t) * 1.5 - 1.5 - absd * 0.35;
            angle = -d * 15.0;
            scale = 1.0 / (1.0 + absd * 0.35);
            focus = std::clamp(1.0 - absd / kVisibleSpan, 0.0, 1.0);
            break;
        }
        case Blinds:
        {
            x     = d * kSpacing;
            z     = -absd * 0.08;
            angle = 0.0;
            double raw = 90.0;
            angleX = (absd < 0.5) ? raw * (absd / 0.5) : raw;
            scale = 1.0 / (1.0 + absd * 0.1);
            focus = std::clamp(1.0 - absd * 0.12, 0.3, 1.0);
            break;
        }
        case Fade:
        {
            x     = d * kSpacing;
            z     = -absd * 0.15;
            angle = 0.0;
            scale = 1.0 / (1.0 + absd * 0.08);
            focus = std::clamp(1.0 - absd * 0.1, 0.2, 1.0);
            alpha = std::clamp(1.0 - absd * 0.35, 0.0, 1.0);
            break;
        }
        case Slide:
        {
            double sign = (i % 2 == 0) ? -1.0 : 1.0;
            x     = d * kSpacing + sign * 0.6 * std::max(0.0, 1.0 - absd);
            z     = -absd * 0.12;
            angle = sign * 8.0 * std::max(0.0, 1.0 - absd);
            scale = 1.0 / (1.0 + absd * 0.12);
            focus = std::clamp(1.0 - absd * 0.12, 0.3, 1.0);
            break;
        }
        case Swing:
        {
            double swingAngle = std::sin(d * 0.6) * 25.0;
            x     = d * kSpacing;
            z     = -absd * 0.1;
            angle = swingAngle;
            scale = 1.0 / (1.0 + absd * 0.12);
            focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0);
            y     = std::abs(std::sin(d * 0.6)) * 0.3;
            break;
        }
        case Twirl:
        {
            x     = d * kSpacing;
            z     = -absd * 0.08;
            angle = d * 360.0;
            scale = 1.0 / (1.0 + absd * 0.2);
            focus = std::clamp(1.0 - absd * 0.15, 0.2, 1.0);
            break;
        }
        case Ribbon:
        {
            double rad = d * 0.4;
            double twist = d * 0.6;
            x     = std::sin(rad) * 1.5;
            z     = std::cos(rad) * 1.5 - 1.5 - absd * 0.1;
            angle = d * 20.0 + std::sin(twist) * 15.0;
            scale = 1.0 / (1.0 + absd * 0.1);
            focus = std::clamp(1.0 - absd * 0.08, 0.4, 1.0);
            break;
        }
        case Pyramid:
        {
            x     = d * kSpacing;
            z     = -absd * 0.3;
            angle = 0.0;
            scale = 1.0 / (1.0 + absd * 0.25);
            focus = std::clamp(1.0 - absd * 0.18, 0.2, 1.0);
            y     = kSpacing - 1.0 / (1.0 + absd * 0.15);
            break;
        }
        case Diagonal:
        {
            x     = d * kSpacing * 1.2;
            z     = -absd * 0.2;
            angle = 0.0;
            scale = 1.0 / (1.0 + absd * 0.1);
            focus = std::clamp(1.0 - absd * 0.12, 0.3, 1.0);
            y     = d * 0.15;
            break;
        }
        case Spring:
        {
            double coil = d * 0.8;
            x     = std::sin(coil) * 0.5;
            z     = std::cos(coil) * 0.5 - absd * 0.2;
            angle = d * 30.0;
            scale = 1.0 / (1.0 + absd * 0.2);
            focus = std::clamp(1.0 - absd * 0.12, 0.3, 1.0);
            y     = d * 0.15;
            break;
        }
        case Blast:
        {
            double spread = 1.0 + absd * 0.6;
            double sign = (d >= 0) ? 1.0 : -1.0;
            x     = sign * absd * kSpacing * spread;
            z     = -absd * 0.08 * spread;
            angle = d * 30.0;
            scale = 1.0 / (1.0 + absd * 0.45);
            focus = std::clamp(1.0 - absd / kVisibleSpan, 0.0, 1.0);
            break;
        }
        case Orbit:
        {
            double rad = d * 0.5;
            double rx = 2.0 + absd * 0.3;
            double rz = 1.2 + absd * 0.2;
            x     = std::sin(rad) * rx;
            z     = std::cos(rad) * rz - rz;
            angle = -d * 20.0;
            scale = 1.0 / (1.0 + absd * 0.08);
            focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0);
            y     = std::cos(rad * 1.3) * 0.25;
            break;
        }
        case Grid:
        {
            int row = i / 3;
            int col = i % 3;
            double gridOffset = (double)row * 3 + col;
            double gd = gridOffset - m_offset;
            double gabsd = std::abs(gd);
            x     = (double)(col - 1) * 1.8;
            z     = -gabsd * 0.25 - row * 0.3;
            angle = 0.0;
            scale = 1.0 / (1.0 + gabsd * 0.2);
            focus = std::clamp(1.0 - gabsd * 0.12, 0.2, 1.0);
            y     = (double)(1 - row) * 1.1;
            break;
        }
        case Rain:
        {
            double fall = absd * 0.5;
            x     = d * kSpacing;
            z     = -fall - 0.2;
            angle = 0.0;
            scale = 1.0 - absd * 0.06;
            focus = std::clamp(1.0 - absd * 0.08, 0.2, 1.0);
            y     = fall;
            break;
        }
        case Fountain:
        {
            double t = absd * 0.6;
            double arc = t * (1.0 - t * 0.25) * 2.0;
            x     = d * kSpacing;
            z     = -t * 0.2;
            angle = 0.0;
            scale = 1.0 - t * 0.1;
            focus = std::clamp(1.0 - t * 0.15, 0.2, 1.0);
            y     = std::max(0.0, arc);
            break;
        }
        case Ring:
        {
            double rad = d * 0.5;
            x     = std::sin(rad) * 2.0;
            z     = std::cos(rad) * 2.0 - 2.0;
            angle = -d * 20.0;
            scale = 1.0;
            focus = std::clamp(1.0 - absd * 0.08, 0.4, 1.0);
            y     = std::cos(rad * 2.0) * 0.3;
            break;
        }
        case Cross:
        {
            double arm = (i % 4);
            double armOff = (double)(i / 4);
            double cd = armOff - m_offset;
            double cabsd = std::abs(cd);
            if (arm < 2) {
                x = (double)(arm == 0 ? -1 : 1) * (1.0 + cd * 0.1);
                z = -cabsd * 0.2;
                y = 0.0;
            } else {
                x = 0.0;
                z = -cabsd * 0.2;
                y = (double)(arm == 2 ? -1 : 1) * (1.0 + cd * 0.1);
            }
            angle = 0.0;
            scale = 1.0 / (1.0 + cabsd * 0.15);
            focus = std::clamp(1.0 - cabsd * 0.1, 0.3, 1.0);
            break;
        }
        case Diamond:
        {
            double stride = (i % 5) - 2;
            double dd = (double)(i / 5) - m_offset;
            double dabsd = std::abs(dd);
            x     = stride * 0.9;
            z     = -dabsd * 0.2;
            angle = 0.0;
            scale = 1.0 / (1.0 + dabsd * 0.15);
            focus = std::clamp(1.0 - dabsd * 0.1, 0.3, 1.0);
            y     = -std::abs(stride) * 0.5;
            break;
        }
        case Curtain:
        {
            double fold = std::sin(d * 1.5) * 0.4;
            x     = d * kSpacing + fold;
            z     = -absd * 0.12 + std::abs(fold) * 0.1;
            angle = 0.0;
            scale = 1.0 / (1.0 + absd * 0.1);
            focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0);
            y     = -std::abs(fold) * 0.3;
            break;
        }
        case Circle:
        {
            double rad = d * 0.4;
            x     = std::sin(rad) * 2.0;
            z     = std::cos(rad) * 2.0 - 2.0;
            angle = -d * 20.0;
            scale = 1.0;
            focus = std::clamp(1.0 - absd * 0.08, 0.4, 1.0);
            break;
        }
        case Boomerang:
        {
            double t = d * 0.3;
            double curve = std::sin(t * M_PI) * 0.8;
            x     = d * kSpacing + curve;
            z     = -absd * 0.2 + std::abs(curve) * 0.3;
            angle = d * 15.0;
            scale = 1.0 / (1.0 + absd * 0.12);
            focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0);
            break;
        }
        case Infinity:
        {
            double t = d * 0.4;
            double infX = std::sin(t) / (1.0 + std::cos(t) * std::cos(t));
            double infZ = std::sin(t) * std::cos(t) / (1.0 + std::cos(t) * std::cos(t));
            x     = infX * 1.8;
            z     = infZ * 1.8 - absd * 0.05;
            angle = d * 25.0;
            scale = 1.0 / (1.0 + absd * 0.08);
            focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0);
            break;
        }
        case Corkscrew:
        {
            double rad = d * 0.7;
            x     = std::sin(rad) * 1.2;
            z     = std::cos(rad) * 1.2 - 1.2 - absd * 0.08;
            angle = d * 35.0;
            scale = 1.0 / (1.0 + absd * 0.12);
            focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0);
            y     = d * 0.2;
            break;
        }
        case Galaxy:
        {
            double rad = d * 0.4;
            double armSpread = 1.0 + absd * 0.1;
            x     = std::sin(rad) * armSpread * 1.5;
            z     = std::cos(rad) * armSpread * 1.5 - 1.5 - absd * 0.06;
            angle = d * 15.0 + rad * 30.0;
            scale = 1.0 / (1.0 + absd * 0.08);
            focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0);
            break;
        }
        case Pulse:
        {
            double ringR = 0.3 + absd * 0.5;
            double pulseY = std::sin(d * M_PI * 2.0) * 0.15;
            x     = std::cos(d * 0.5) * ringR;
            z     = std::sin(d * 0.5) * ringR - ringR;
            angle = d * 10.0;
            scale = 1.0 / (1.0 + absd * 0.25);
            focus = std::clamp(1.0 - absd * 0.15, 0.2, 1.0);
            y     = pulseY;
            break;
        }
        case Target:
        {
            double ring = absd * 0.5;
            x     = d * kSpacing;
            z     = -ring;
            angle = 0.0;
            scale = 1.0 / (1.0 + ring * 0.15);
            focus = std::clamp(1.0 - ring * 0.12, 0.3, 1.0);
            break;
        }
        case Pinwheel:
        {
            double spin = d * 1.5;
            x     = std::cos(spin) * absd * 0.5;
            z     = std::sin(spin) * absd * 0.5 - absd * 0.08;
            angle = d * 180.0;
            scale = 1.0 / (1.0 + absd * 0.18);
            focus = std::clamp(1.0 - absd * 0.15, 0.2, 1.0);
            break;
        }
        case Scatter:
        {
            double sx = ((i * 7 + 3) % 11) / 5.5 - 1.0;
            double sy = ((i * 13 + 5) % 9) / 4.5 - 1.0;
            double sd = ((i * 17 + 11) % 7) / 7.0 * 2.0 - 1.0;
            double sdoff = sd - m_offset * 0.5;
            double sdabs = std::abs(sdoff);
            x     = sx * 1.5;
            z     = -sdabs * 0.3;
            angle = 0.0;
            scale = 1.0 / (1.0 + sdabs * 0.2);
            focus = std::clamp(1.0 - sdabs * 0.12, 0.2, 1.0);
            y     = sy * 0.8;
            break;
        }
        case Converge:
        {
            double converge = std::max(0.0, 1.0 - absd * 0.3);
            x     = d * kSpacing * converge;
            z     = -absd * 0.15 * converge;
            angle = d * 20.0 * converge;
            scale = 0.5 + 0.5 * converge;
            focus = std::clamp(converge + 0.2, 0.2, 1.0);
            break;
        }
        case Diverge:
        {
            double diverge = std::max(0.0, absd * 0.4 - 0.1);
            x     = d * kSpacing * (1.0 + diverge);
            z     = -absd * 0.1 - diverge * 0.2;
            angle = d * 15.0 + diverge * 30.0;
            scale = 1.0 / (1.0 + absd * 0.1 + diverge * 0.3);
            focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0);
            break;
        }
        case Wave2D:
        {
            double wx = ((i % 5) - 2) * 0.9;
            double wz = -(double)(i / 5) * 0.35;
            double wd = (double)(i / 5) - m_offset;
            double wabsd = std::abs(wd);
            double phase = wd * 1.2 + (i % 5) * 0.5;
            x     = wx;
            z     = wz + std::sin(phase) * 0.2;
            angle = 0.0;
            scale = 1.0 / (1.0 + wabsd * 0.15);
            focus = std::clamp(1.0 - wabsd * 0.1, 0.3, 1.0);
            y     = std::cos(phase) * 0.25;
            break;
        }
        case Domino:
        {
            x     = d * kSpacing;
            z     = -absd * 0.1;
            angle = 0.0;
            double raw = 90.0;
            angleX = (absd < 0.5) ? raw * (absd / 0.5) : raw;
            scale = 1.0 / (1.0 + absd * 0.12);
            focus = std::clamp(1.0 - absd * 0.12, 0.3, 1.0);
            break;
        }
        case Flag:
        {
            x     = d * kSpacing;
            z     = -absd * 0.08;
            angle = 0.0;
            angleX = std::sin(d * 2.0) * 20.0;
            scale = 1.0 / (1.0 + absd * 0.1);
            focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0);
            y     = std::abs(std::sin(d * 1.5)) * 0.15;
            break;
        }
        case Hinge:
        {
            double sign = (d >= 0) ? -1.0 : 1.0;
            x     = d * kSpacing;
            z     = -absd * 0.1;
            double raw = 90.0;
            angle = (absd < 0.5) ? raw * (absd / 0.5) : raw;
            angle *= sign;
            scale = 1.0 / (1.0 + absd * 0.15);
            focus = std::clamp(1.0 - absd * 0.12, 0.3, 1.0);
            break;
        }
        case Lattice:
        {
            int lr = i / 4;
            int lc = i % 4;
            double ld = (double)lr - m_offset;
            double labs = std::abs(ld);
            x     = (double)(lc - 1) * 1.4 + std::sin(labs * 0.5) * 0.2;
            z     = -labs * 0.15 - (lc % 2) * 0.1;
            angle = (lc % 2 == 0) ? 5.0 : -5.0;
            scale = 1.0 / (1.0 + labs * 0.12);
            focus = std::clamp(1.0 - labs * 0.1, 0.3, 1.0);
            y     = (1.0 - (double)(lr % 3)) * 0.8 + std::cos(ld * 0.5) * 0.1;
            break;
        }
        case Tornado:
        {
            double rad = d * 0.6;
            double r = 0.1 + absd * 0.3;
            x     = std::cos(rad) * r * 2.5;
            z     = std::sin(rad) * r * 2.5 - absd * 0.15;
            angle = d * 50.0;
            scale = 1.0 / (1.0 + absd * 0.25);
            focus = std::clamp(1.0 - absd * 0.15, 0.2, 1.0);
            y     = d * 0.15 - absd * 0.05;
            break;
        }
        case Mobius:
        {
            double t = d * 0.5;
            double s = t * 0.5;
            double mx = (1.0 + s * 0.3) * std::cos(t);
            double mz = (1.0 + s * 0.3) * std::sin(t);
            x     = mx * 1.2;
            z     = mz * 1.2 - 1.2 - absd * 0.04;
            angle = d * 30.0 + std::atan2(mz, mx) * 20.0;
            scale = 1.0 / (1.0 + absd * 0.08);
            focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0);
            y     = s * 0.5;
            break;
        }
        case Propeller:
        {
            double rad = d * 0.8;
            x     = std::cos(rad) * absd * 0.6;
            z     = std::sin(rad) * absd * 0.6 - absd * 0.06;
            angle = d * 360.0;
            scale = 1.0 / (1.0 + absd * 0.15);
            focus = std::clamp(1.0 - absd * 0.12, 0.3, 1.0);
            break;
        }
        case Slinky:
        {
            double coil = d * 1.2;
            x     = std::sin(coil) * 0.8;
            z     = std::cos(coil) * 0.8 - 0.8 - absd * 0.06;
            angle = d * 25.0;
            scale = 1.0 / (1.0 + absd * 0.12);
            focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0);
            y     = d * 0.3;
            break;
        }
        case Paddle:
        {
            double rad = d * 0.3;
            x     = std::sin(rad) * 2.0;
            z     = std::cos(rad) * 2.0 - 2.0;
            angle = d * 15.0 + 90.0;
            scale = 1.0;
            focus = std::clamp(1.0 - absd * 0.08, 0.4, 1.0);
            break;
        }
        case Portal:
        {
            double rad = d * 0.5;
            double shrink = 1.0 / (1.0 + absd * 0.3);
            x     = std::sin(rad) * 2.0 * shrink;
            z     = std::cos(rad) * 2.0 * shrink - 2.0 - absd * 0.2;
            angle = d * 30.0;
            scale = shrink;
            focus = std::clamp(shrink, 0.2, 1.0);
            break;
        }
        case Prism:
        {
            int side = i % 3;
            double sd = (double)(i / 3) - m_offset;
            double sabs = std::abs(sd);
            double angleOff = (double)side * 2.094 - 1.047;
            x     = std::sin(angleOff) * 1.2;
            z     = std::cos(angleOff) * 1.2 - 1.2 - sabs * 0.15;
            angle = -angleOff * 40.0;
            scale = 1.0 / (1.0 + sabs * 0.15);
            focus = std::clamp(1.0 - sabs * 0.1, 0.3, 1.0);
            break;
        }
        case Shuffle:
        {
            int slot = (i % 2 == 0) ? 1 : -1;
            int pair = i / 2;
            double pd = (double)pair - m_offset;
            double pabs = std::abs(pd);
            double jitter = std::sin(pd * 3.0) * 0.1;
            x     = (double)slot * 0.5 + jitter;
            z     = -pabs * 0.12 + (slot > 0 ? 0.03 : -0.03);
            angle = (double)slot * 12.0;
            scale = 1.0 / (1.0 + pabs * 0.12);
            focus = std::clamp(1.0 - pabs * 0.1, 0.3, 1.0);
            y     = (slot > 0 ? 0.1 : -0.1);
            break;
        }
        case Tide:
        {
            double phase = d * 0.8;
            double wave = std::sin(phase) * 0.4;
            x     = d * kSpacing + wave;
            z     = -absd * 0.12 + std::abs(wave) * 0.1;
            angle = wave * 20.0;
            scale = 1.0 / (1.0 + absd * 0.1);
            focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0);
            y     = std::cos(phase) * 0.15;
            break;
        }
        case Umbrella:
        {
            double ribs = (double)(i % 6) * 1.047;
            double rd = (double)(i / 6) - m_offset;
            double rabs = std::abs(rd);
            x     = std::sin(ribs) * 1.5;
            z     = std::cos(ribs) * 1.5 - 1.5 - rabs * 0.1;
            angle = -ribs * 30.0;
            scale = 1.0 / (1.0 + rabs * 0.1);
            focus = std::clamp(1.0 - rabs * 0.08, 0.3, 1.0);
            y     = -0.3 + rabs * 0.05;
            break;
        }
        case Wing:
        {
            double sign = (i % 2 == 0) ? -1.0 : 1.0;
            double wd = (double)(i / 2) - m_offset;
            double wabs = std::abs(wd);
            x     = sign * (0.8 + wabs * 0.1);
            z     = -wabs * 0.12;
            angle = sign * 30.0;
            scale = 1.0 / (1.0 + wabs * 0.12);
            focus = std::clamp(1.0 - wabs * 0.1, 0.3, 1.0);
            y     = std::abs(sign) * 0.1 - wabs * 0.05;
            break;
        }
        case Wobble:
        {
            x     = d * kSpacing + std::sin(d * 4.0) * 0.08;
            z     = -absd * 0.08 + std::cos(d * 3.0) * 0.06;
            angle = std::sin(d * 2.5) * 15.0;
            scale = 1.0 / (1.0 + absd * 0.1);
            focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0);
            y     = std::sin(d * 3.5) * 0.1;
            break;
        }
        case YoYo:
        {
            double bob = std::sin(d * M_PI * 0.6) * 0.5;
            x     = d * kSpacing;
            z     = -absd * 0.1;
            angle = 0.0;
            scale = 1.0 - absd * 0.05;
            focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0);
            y     = bob - absd * 0.05;
            break;
        }
        case Oscillate:
        {
            double osc = std::sin(d * M_PI * 2.0);
            x     = d * kSpacing;
            z     = -absd * 0.08 + osc * 0.15;
            angle = osc * 25.0;
            scale = 1.0 / (1.0 + absd * 0.1);
            focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0);
            break;
        }
        case Drift:
        {
            double driftOff = d * 0.3;
            x     = d * kSpacing + std::sin(driftOff) * 0.3;
            z     = -absd * 0.1 + std::cos(driftOff * 1.5) * 0.1;
            angle = std::sin(driftOff) * 15.0;
            scale = 1.0 / (1.0 + absd * 0.08);
            focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0);
            y     = std::sin(driftOff * 1.3) * 0.15;
            break;
        }
        case Hover:
        {
            double h = std::sin(d * 1.5) * 0.2;
            x     = d * kSpacing;
            z     = -absd * 0.1;
            angle = std::sin(d * 1.2) * 8.0;
            scale = 1.0 / (1.0 + absd * 0.08);
            focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0);
            y     = 0.2 + h;
            break;
        }
        case Avalanche:
        {
            double slope = absd * 0.5;
            x     = d * kSpacing;
            z     = -slope * 0.15;
            angle = d * 15.0 + slope * 20.0;
            scale = 1.0 / (1.0 + slope * 0.2);
            focus = std::clamp(1.0 - absd * 0.12, 0.2, 1.0);
            y     = -slope;
            angleX = slope * 30.0;
            break;
        }
        case Bridge:
        {
            double t = absd * 0.4;
            double arch = std::sin(t * M_PI) * 0.5;
            x     = d * kSpacing;
            z     = -t * 0.2 + arch * 0.1;
            angle = 0.0;
            scale = 1.0 / (1.0 + t * 0.15);
            focus = std::clamp(1.0 - t * 0.12, 0.2, 1.0);
            y     = arch;
            break;
        }
        case Cone:
        {
            double coneR = absd * 0.5;
            x     = d * kSpacing;
            z     = -coneR * 0.8;
            angle = d * 15.0;
            scale = 1.0 - std::min(absd * 0.12, 0.7);
            focus = std::clamp(1.0 - absd * 0.12, 0.2, 1.0);
            y     = -coneR * 0.5;
            break;
        }
        case Deck:
        {
            double splay = (i % 2 == 0) ? -0.3 : 0.3;
            double dd = (double)(i / 2) - m_offset;
            double dabs = std::abs(dd);
            x     = splay + dd * 0.15;
            z     = -dabs * 0.08 + std::abs(splay) * 0.05;
            angle = splay * 30.0;
            scale = 1.0 - dabs * 0.06;
            focus = std::clamp(1.0 - dabs * 0.08, 0.3, 1.0);
            y     = std::abs(splay) * 0.2 - dabs * 0.03;
            break;
        }
        case Drum:
        {
            double rad = d * 0.35;
            x     = std::sin(rad) * 1.8;
            z     = std::cos(rad) * 1.8 - 1.8;
            angle = -d * 15.0;
            scale = 1.0;
            focus = std::clamp(1.0 - absd * 0.08, 0.4, 1.0);
            y     = std::cos(rad * 2.0) * 0.25;
            break;
        }
        case Feather:
        {
            double drift = std::sin(d * 0.8) * 0.5;
            x     = d * kSpacing + drift;
            z     = -absd * 0.08 + std::abs(drift) * 0.05;
            angle = std::cos(d * 1.2) * 20.0;
            scale = 1.0 - absd * 0.04;
            focus = std::clamp(1.0 - absd * 0.06, 0.3, 1.0);
            y     = -absd * 0.3 + std::abs(drift) * 0.1;
            break;
        }
        case Flower:
        {
            double petal = (double)(i % 6) * 1.047;
            double fd = (double)(i / 6) - m_offset;
            double fabs = std::abs(fd);
            x     = std::sin(petal) * 1.5;
            z     = std::cos(petal) * 1.5 - 1.5 - fabs * 0.1;
            angle = -petal * 30.0;
            scale = 1.0 / (1.0 + fabs * 0.12);
            focus = std::clamp(1.0 - fabs * 0.08, 0.3, 1.0);
            y     = std::cos(petal * 2.0) * 0.15;
            break;
        }
        case Glide:
        {
            double g = std::sin(d * 0.6) * 0.2;
            x     = d * kSpacing + g;
            z     = -absd * 0.1 + std::abs(g) * 0.1;
            angle = g * 20.0;
            scale = 1.0 / (1.0 + absd * 0.08);
            focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0);
            y     = g;
            break;
        }
        case Gravity:
        {
            double fall = absd * 0.4;
            x     = d * kSpacing;
            z     = -fall * 0.15;
            angle = 0.0;
            scale = 1.0 - fall * 0.08;
            focus = std::clamp(1.0 - fall * 0.1, 0.2, 1.0);
            y     = -fall * fall;
            break;
        }
        case Hop:
        {
            double hop = std::max(0.0, std::sin(d * M_PI * 1.5)) * 0.4;
            x     = d * kSpacing;
            z     = -absd * 0.1;
            angle = 0.0;
            scale = 1.0 - absd * 0.06 + hop * 0.1;
            focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0);
            y     = hop - absd * 0.05;
            break;
        }
        case Orbit3D:
        {
            double rad = d * 0.5;
            double tilt = std::cos(d * 0.3) * 0.3;
            x     = std::sin(rad) * 2.0;
            z     = std::cos(rad) * 2.0 - 2.0;
            angle = -d * 20.0;
            scale = 1.0;
            focus = std::clamp(1.0 - absd * 0.08, 0.4, 1.0);
            y     = tilt;
            angleX = tilt * 20.0;
            break;
        }
        case Petal:
        {
            double pAngle = (double)(i % 5) * 1.256;
            double pd = (double)(i / 5) - m_offset;
            double pabs = std::abs(pd);
            double r = 0.5 + pabs * 0.15;
            x     = std::sin(pAngle) * r * 1.5;
            z     = std::cos(pAngle) * r * 1.5 - 1.5 - pabs * 0.08;
            angle = -pAngle * 25.0;
            scale = 1.0 / (1.0 + pabs * 0.1);
            focus = std::clamp(1.0 - pabs * 0.08, 0.3, 1.0);
            y     = std::cos(pAngle * 2.0) * 0.2;
            break;
        }
        case Rocket:
        {
            double t = absd * 0.6;
            x     = d * kSpacing;
            z     = -t * 0.1;
            angle = 0.0;
            scale = 1.0 - t * 0.05;
            focus = std::clamp(1.0 - t * 0.1, 0.2, 1.0);
            y     = t;
            break;
        }
        case Sail:
        {
            double sign = (d >= 0) ? 1.0 : -1.0;
            double belly = std::sin(d * 0.8) * 0.3;
            x     = d * kSpacing;
            z     = -absd * 0.1 + belly * 0.15;
            angle = sign * 20.0 + belly * 15.0;
            scale = 1.0 / (1.0 + absd * 0.1);
            focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0);
            y     = std::abs(belly) * 0.15;
            break;
        }
        case Snow:
        {
            double sway = std::sin(d * 1.5) * 0.4;
            x     = d * kSpacing + sway;
            z     = -absd * 0.06 + std::abs(sway) * 0.05;
            angle = sway * 15.0;
            scale = 1.0 - absd * 0.03;
            focus = std::clamp(1.0 - absd * 0.05, 0.3, 1.0);
            y     = -absd * 0.4 + sway * 0.2;
            break;
        }
        case Spider:
        {
            int leg = i % 8;
            double legAngle = (double)leg * 0.785;
            double sd = (double)(i / 8) - m_offset;
            double sabs = std::abs(sd);
            x     = std::sin(legAngle) * (0.8 + sabs * 0.1);
            z     = std::cos(legAngle) * (0.8 + sabs * 0.1) - 0.8 - sabs * 0.08;
            angle = -legAngle * 30.0;
            scale = 1.0 / (1.0 + sabs * 0.1);
            focus = std::clamp(1.0 - sabs * 0.08, 0.3, 1.0);
            y     = (leg % 2 == 0 ? 0.1 : -0.1);
            break;
        }
        case Sway:
        {
            double swayAmp = std::sin(d * 0.8) * 0.4;
            x     = d * kSpacing + swayAmp;
            z     = -absd * 0.1;
            angle = swayAmp * 25.0;
            scale = 1.0 / (1.0 + absd * 0.1);
            focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0);
            y     = std::abs(swayAmp) * 0.15;
            break;
        }
        case Tumble:
        {
            double t = d;
            x     = d * kSpacing;
            z     = -absd * 0.08;
            angle = t * 180.0;
            scale = 1.0 / (1.0 + absd * 0.15);
            focus = std::clamp(1.0 - absd * 0.12, 0.3, 1.0);
            angleX = std::cos(t * 1.5) * 180.0;
            break;
        }
        case Vibrate:
        {
            double v = d;
            x     = d * kSpacing + std::sin(v * 8.0) * 0.06;
            z     = -absd * 0.06 + std::cos(v * 7.0) * 0.04;
            angle = std::sin(v * 9.0) * 10.0;
            scale = 1.0 - absd * 0.05;
            focus = std::clamp(1.0 - absd * 0.06, 0.3, 1.0);
            y     = std::sin(v * 6.0) * 0.06;
            break;
        }
        case Zigzag:
        {
            int zag = (i % 2 == 0) ? -1 : 1;
            double zd = (double)(i / 2) - m_offset;
            double zabs = std::abs(zd);
            x     = (double)zag * 0.5;
            z     = -zabs * 0.15 + (zag > 0 ? 0.05 : -0.05);
            angle = (double)zag * 15.0;
            scale = 1.0 / (1.0 + zabs * 0.15);
            focus = std::clamp(1.0 - zabs * 0.1, 0.3, 1.0);
            y     = (double)zag * 0.15;
            break;
        }
        case Butterfly:
        {
            int side = (i % 2 == 0) ? -1 : 1;
            double bd = (double)(i / 2) - m_offset;
            double babs = std::abs(bd);
            double flap = std::sin(bd * 2.5) * 45.0;
            x     = (double)side * 0.6;
            z     = -babs * 0.1;
            angle = 0.0;
            angleX = flap;
            scale = 1.0 / (1.0 + babs * 0.12);
            focus = std::clamp(1.0 - babs * 0.1, 0.3, 1.0);
            y     = std::cos(bd * 2.0) * 0.1;
            break;
        }
        case Caterpillar:
        {
            double arch = std::sin(d * 1.5) * 0.3;
            x     = d * kSpacing;
            z     = -absd * 0.08 + std::abs(arch) * 0.1;
            angle = arch * 30.0;
            scale = 1.0 - absd * 0.05;
            focus = std::clamp(1.0 - absd * 0.06, 0.3, 1.0);
            y     = 0.2 + arch;
            break;
        }
        case Firework:
        {
            double burst = 0.5 + absd * 0.4;
            double fAngle = (double)(i % 8) * 0.785 + m_offset * 0.1;
            double fd = (double)(i / 8) - m_offset;
            double fabs = std::abs(fd);
            x     = std::cos(fAngle) * burst * 2.0;
            z     = std::sin(fAngle) * burst * 2.0 - 2.0 * burst - fabs * 0.1;
            angle = -fAngle * 40.0;
            scale = 1.0 / (1.0 + fabs * 0.2);
            focus = std::clamp(1.0 - fabs * 0.12, 0.2, 1.0);
            y     = std::sin(fAngle * 2.0) * burst * 0.3;
            break;
        }
        case Jelly:
        {
            double wobble = std::sin(d * 3.0) * 0.08;
            x     = d * kSpacing + wobble;
            z     = -absd * 0.06 + std::sin(d * 2.5) * 0.06;
            angle = std::sin(d * 2.0) * 12.0;
            scale = 1.0 + wobble * 0.3;
            focus = std::clamp(1.0 - absd * 0.06, 0.4, 1.0);
            y     = std::sin(d * 3.5) * 0.08;
            break;
        }
        case Kaleido:
        {
            int mirror = i % 4;
            double ka = (double)mirror * 1.571;
            double kd = (double)(i / 4) - m_offset;
            double kabs = std::abs(kd);
            x     = std::sin(ka) * 1.2;
            z     = std::cos(ka) * 1.2 - 1.2 - kabs * 0.1;
            angle = -ka * 40.0;
            scale = 1.0 / (1.0 + kabs * 0.12);
            focus = std::clamp(1.0 - kabs * 0.08, 0.3, 1.0);
            y     = (mirror % 2 == 0 ? 0.15 : -0.15);
            break;
        }
        case Ribbon3D:
        {
            double rd = d * 0.5;
            double twist = d * 0.4;
            x     = std::sin(rd) * 1.2;
            z     = std::cos(rd) * 1.2 - 1.2 - absd * 0.06;
            angle = d * 20.0 + twist * 30.0;
            scale = 1.0 / (1.0 + absd * 0.1);
            focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0);
            y     = std::sin(twist * 2.0) * 0.2;
            alpha = std::clamp(1.0 - absd * 0.12, 0.2, 1.0);
            break;
        }
        case Ripple2D:
        {
            int rx = i % 5 - 2;
            int ry = i / 5;
            double rd2 = (double)ry - m_offset;
            double r2abs = std::abs(rd2);
            double dist = std::sqrt((double)(rx * rx) + (double)(ry * ry));
            double phase = dist * 1.5 - m_offset * 0.5;
            x     = (double)rx * 0.9;
            z     = -(double)ry * 0.25 + std::sin(phase) * 0.2;
            angle = 0.0;
            scale = 1.0 / (1.0 + r2abs * 0.12);
            focus = std::clamp(1.0 - r2abs * 0.08, 0.3, 1.0);
            y     = std::cos(phase) * 0.2;
            break;
        }
        case Spectrum:
        {
            double sp = d * 0.3;
            x     = d * kSpacing * 0.8;
            z     = -absd * 0.08 + std::sin(sp) * 0.1;
            angle = sp * 20.0;
            scale = 1.0 / (1.0 + absd * 0.06);
            focus = std::clamp(1.0 - absd * 0.06, 0.3, 1.0);
            y     = std::cos(sp) * 0.15;
            break;
        }
        case Splash:
        {
            double sd2 = (double)(i % 6) * 1.047;
            double sr = 0.3 + absd * 0.5;
            double sd3 = (double)(i / 6) - m_offset;
            double s3abs = std::abs(sd3);
            x     = std::cos(sd2) * sr * 2.0;
            z     = std::sin(sd2) * sr * 2.0 - sr * 2.0 - s3abs * 0.08;
            angle = -sd2 * 30.0;
            scale = 1.0 / (1.0 + s3abs * 0.15);
            focus = std::clamp(1.0 - s3abs * 0.1, 0.3, 1.0);
            y     = std::sin(sd2 * 2.0) * sr * 0.3;
            break;
        }
        case Twister:
        {
            double trad = d * 0.8;
            double tr = 0.1 + absd * 0.2;
            x     = std::cos(trad) * tr * 3.0;
            z     = std::sin(trad) * tr * 3.0 - absd * 0.1;
            angle = d * 60.0;
            scale = 1.0 / (1.0 + absd * 0.2);
            focus = std::clamp(1.0 - absd * 0.12, 0.3, 1.0);
            y     = d * 0.2;
            break;
        }
        case Aurora:
        {
            double aPhase = d * 0.4;
            x     = d * kSpacing * 1.3;
            z     = -absd * 0.06 + std::sin(aPhase) * 0.3;
            angle = std::sin(aPhase * 1.3) * 25.0;
            scale = 1.0 / (1.0 + absd * 0.05);
            focus = std::clamp(1.0 - absd * 0.05, 0.3, 1.0);
            y     = std::cos(aPhase * 0.7) * 0.4;
            alpha = std::clamp(0.4 + (1.0 - absd * 0.08) * 0.6, 0.0, 1.0);
            break;
        }
        case Cascade3D:
        {
            int c3col = i % 3 - 1;
            double c3d = (double)(i / 3) - m_offset;
            double c3abs = std::abs(c3d);
            x     = (double)c3col * 1.2;
            z     = -c3abs * 0.2 + c3col * 0.05;
            angle = (double)c3col * 10.0;
            scale = 1.0 / (1.0 + c3abs * 0.12);
            focus = std::clamp(1.0 - c3abs * 0.1, 0.3, 1.0);
            y     = (double)(1 - c3col) * 0.3 - c3abs * 0.1;
            break;
        }
        case Crown:
        {
            int crownPt = i % 5;
            double ca = (double)crownPt * 1.256 - 1.256;
            double cd = (double)(i / 5) - m_offset;
            double cabs = std::abs(cd);
            double cr = 1.0 + (crownPt % 2 == 0 ? 0.5 : 0.0);
            x     = std::sin(ca) * cr;
            z     = std::cos(ca) * cr - cr - cabs * 0.1;
            angle = -ca * 35.0;
            scale = 1.0 / (1.0 + cabs * 0.12);
            focus = std::clamp(1.0 - cabs * 0.08, 0.3, 1.0);
            y     = (crownPt % 2 == 0 ? 0.4 : -0.1) - cabs * 0.03;
            break;
        }
        case Cyclone:
        {
            double cyRad = d * 0.5;
            double cyR = 0.2 + absd * 0.25;
            x     = std::cos(cyRad) * cyR * 3.5;
            z     = std::sin(cyRad) * cyR * 3.5 - absd * 0.08;
            angle = d * 40.0;
            scale = 1.0 / (1.0 + absd * 0.15);
            focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0);
            break;
        }
        case Dandelion:
        {
            double dn = (double)(i % 12) * 0.523;
            double dd = (double)(i / 12) - m_offset;
            double dabs = std::abs(dd);
            double dr = 0.1 + dabs * 0.2;
            x     = std::sin(dn) * dr * 3.0;
            z     = std::cos(dn) * dr * 3.0 - 3.0 * dr - dabs * 0.06;
            angle = -dn * 25.0;
            scale = 1.0 / (1.0 + dabs * 0.1);
            focus = std::clamp(1.0 - dabs * 0.06, 0.3, 1.0);
            y     = std::cos(dn * 2.0) * dr * 0.3;
            break;
        }
        case Echo:
        {
            int echo = i % 3;
            double ed = (double)(i / 3) - m_offset;
            double eabs = std::abs(ed);
            double eOff = (double)echo * 0.15;
            x     = d * kSpacing;
            z     = -eabs * 0.1 - eOff;
            angle = 0.0;
            scale = 1.0 / (1.0 + eabs * 0.1 + eOff * 0.5);
            focus = std::clamp(1.0 - eabs * 0.08, 0.2, 1.0);
            y     = -eOff * 0.5;
            alpha = std::clamp(1.0 - (double)echo * 0.3 - eabs * 0.06, 0.0, 1.0);
            break;
        }
        case Funnel:
        {
            double fn = absd * 0.3;
            x     = d * kSpacing * (1.0 - fn);
            z     = -fn * 0.8;
            angle = d * 20.0 * (1.0 - fn);
            scale = 1.0 - fn * 0.5;
            focus = std::clamp(1.0 - fn * 0.3, 0.2, 1.0);
            break;
        }
        case Heart:
        {
            double ht = d * 0.4;
            double hx = 16.0 * std::pow(std::sin(ht), 3.0);
            double hy = 13.0 * std::cos(ht) - 5.0 * std::cos(2.0 * ht) - 2.0 * std::cos(3.0 * ht) - std::cos(4.0 * ht);
            x     = hx * 0.08;
            z     = -absd * 0.08;
            angle = d * 15.0;
            scale = 1.0 / (1.0 + absd * 0.1);
            focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0);
            y     = hy * 0.06;
            break;
        }
        case Honeycomb:
        {
            int hcol = i % 4;
            int hrow = i / 4;
            double hxOff = (hrow % 2 == 0) ? 0.0 : 0.7;
            double hd = (double)hrow - m_offset;
            double habs = std::abs(hd);
            x     = (double)(hcol - 1) * 1.4 + hxOff;
            z     = -habs * 0.12;
            angle = 0.0;
            scale = 1.0 / (1.0 + habs * 0.1);
            focus = std::clamp(1.0 - habs * 0.08, 0.3, 1.0);
            y     = (double)(1 - hrow % 3) * 0.7;
            break;
        }
        case Lantern:
        {
            double lStr = (double)(i % 4) * 0.8 - 1.2;
            double ld = (double)(i / 4) - m_offset;
            double labs = std::abs(ld);
            x     = lStr;
            z     = -labs * 0.08;
            angle = std::sin(ld * 1.5) * 10.0;
            scale = 1.0 - labs * 0.04;
            focus = std::clamp(1.0 - labs * 0.06, 0.3, 1.0);
            y     = std::cos(ld * 1.2) * 0.15 - 0.5;
            break;
        }
        case Bubble:
        {
            double bl = std::sin(d * 2.0) * 0.15;
            x     = d * kSpacing + bl;
            z     = -absd * 0.06;
            angle = bl * 30.0;
            scale = 0.8 + std::sin(d * 1.5) * 0.2;
            focus = std::clamp(1.0 - absd * 0.06, 0.3, 1.0);
            y     = 0.3 + std::abs(bl) + std::cos(d * 1.8) * 0.1;
            alpha = std::clamp(0.6 + (1.0 - absd * 0.06) * 0.4, 0.0, 1.0);
            break;
        }
        case Comet:
        {
            double tail = (double)(i % 4) * 0.2;
            double cd = (double)(i / 4) - m_offset;
            double cabs = std::abs(cd);
            double sign = (cd >= 0) ? 1.0 : -1.0;
            x     = d * kSpacing - sign * tail * 0.5;
            z     = -cabs * 0.08 - tail * 0.1;
            angle = 0.0;
            scale = 1.0 / (1.0 + cabs * 0.1) * (1.0 - tail * 0.15);
            focus = std::clamp(1.0 - cabs * 0.06, 0.2, 1.0);
            y     = -tail * 0.1;
            alpha = std::clamp(1.0 - tail * 0.25 - cabs * 0.04, 0.0, 1.0);
            break;
        }
        case Garland:
        {
            int gSeg = i % 6;
            double ga = (double)gSeg * 1.047 - 2.618;
            double gd = (double)(i / 6) - m_offset;
            double gabs = std::abs(gd);
            double droop = std::cos(ga * 0.8) * 0.3;
            x     = std::sin(ga) * 1.8;
            z     = std::cos(ga) * 1.8 - 1.8 - gabs * 0.08;
            angle = -ga * 25.0;
            scale = 1.0 / (1.0 + gabs * 0.1);
            focus = std::clamp(1.0 - gabs * 0.08, 0.3, 1.0);
            y     = droop - 0.2;
            break;
        }
        case Globe:
        {
            double glRad = d * 0.4;
            double glLat = (double)(i % 3) * 1.047 - 1.047;
            double gd = (double)(i / 3) - m_offset;
            double gabs = std::abs(gd);
            double gr = std::cos(glLat) * 1.5;
            x     = std::sin(glRad) * gr;
            z     = std::cos(glRad) * gr - gr - gabs * 0.06;
            angle = -glRad * 30.0;
            scale = 1.0 / (1.0 + gabs * 0.08);
            focus = std::clamp(1.0 - gabs * 0.06, 0.3, 1.0);
            y     = std::sin(glLat) * 1.5;
            break;
        }
        case Leaf:
        {
            double fl = d * 0.6;
            x     = d * kSpacing;
            z     = -absd * 0.08;
            angle = std::sin(fl * 2.0) * 30.0;
            scale = 1.0 - absd * 0.05;
            focus = std::clamp(1.0 - absd * 0.06, 0.3, 1.0);
            y     = -absd * 0.3 + std::abs(std::sin(fl)) * 0.2;
            alpha = std::clamp(1.0 - absd * 0.08, 0.1, 1.0);
            break;
        }
        case Meteor:
        {
            double mPhase = d * 0.4;
            x     = d * kSpacing + std::sin(mPhase) * 0.5;
            z     = -absd * 0.12;
            angle = d * 20.0;
            scale = 1.0 / (1.0 + absd * 0.15);
            focus = std::clamp(1.0 - absd * 0.12, 0.2, 1.0);
            y     = -absd * 0.15 + std::cos(mPhase) * 0.2;
            break;
        }
        case Mountain:
        {
            int mPeak = i % 5 - 2;
            double md = (double)(i / 5) - m_offset;
            double mabs = std::abs(md);
            double peakH = 0.8 - std::abs((double)mPeak) * 0.25;
            x     = (double)mPeak * 0.7;
            z     = -mabs * 0.12;
            angle = 0.0;
            scale = 1.0 / (1.0 + mabs * 0.1);
            focus = std::clamp(1.0 - mabs * 0.08, 0.3, 1.0);
            y     = peakH - 0.3 - mabs * 0.02;
            break;
        }
        case Nebula:
        {
            double nPhase = d * 0.5;
            double nR = 0.3 + absd * 0.15;
            x     = std::cos(nPhase) * nR * 2.5;
            z     = std::sin(nPhase) * nR * 2.5 - nR * 2.5 - absd * 0.05;
            angle = d * 20.0;
            scale = 1.0 / (1.0 + absd * 0.08);
            focus = std::clamp(1.0 - absd * 0.06, 0.3, 1.0);
            y     = std::sin(nPhase * 1.5) * nR * 0.4;
            alpha = std::clamp(0.5 + (1.0 - absd * 0.06) * 0.5, 0.0, 1.0);
            break;
        }
        case Origami:
        {
            double oFold = (absd < 0.5) ? absd * 180.0 : 90.0;
            double sign = (d >= 0) ? -1.0 : 1.0;
            x     = d * kSpacing;
            z     = -absd * 0.1;
            angle = sign * oFold * 0.5;
            angleX = oFold * 0.5;
            scale = 1.0 / (1.0 + absd * 0.15);
            focus = std::clamp(1.0 - absd * 0.12, 0.3, 1.0);
            break;
        }
        case Peacock:
        {
            int pFeather = i % 7 - 3;
            double pd = (double)(i / 7) - m_offset;
            double pabs = std::abs(pd);
            double fSpread = (double)pFeather * 0.35;
            double fLen = 1.0 - std::abs((double)pFeather) * 0.12;
            x     = fSpread;
            z     = -pabs * 0.08 - (1.0 - fLen) * 0.2;
            angle = fSpread * 20.0;
            scale = fLen / (1.0 + pabs * 0.1);
            focus = std::clamp(fLen - pabs * 0.05, 0.2, 1.0);
            y     = fLen * 0.4 - 0.2;
            break;
        }
        case Arch: { double a = absd * 0.5; double ah = std::sin(a * M_PI) * 0.6; x = d * kSpacing; z = -a * 0.15; angle = 0.0; scale = 1.0 / (1.0 + a * 0.12); focus = std::clamp(1.0 - a * 0.1, 0.3, 1.0); y = ah; break; }
        case Arrow: { int aDir = i % 3 - 1; double ad = (double)(i / 3) - m_offset; double aabs = std::abs(ad); x = (double)aDir * 0.8; z = -aabs * 0.12; angle = (double)aDir * 15.0; scale = 1.0 / (1.0 + aabs * 0.1); focus = std::clamp(1.0 - aabs * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Balloon: { double blSway = std::sin(d * 1.2) * 0.3; x = d * kSpacing + blSway; z = -absd * 0.06; angle = blSway * 20.0; scale = 1.0 - absd * 0.04; focus = std::clamp(1.0 - absd * 0.05, 0.3, 1.0); y = 0.4 + std::abs(blSway) * 0.2; break; }
        case Blizzard: { double bz = d * 1.5; x = d * kSpacing + std::sin(bz) * 0.5; z = -absd * 0.06 + std::cos(bz * 1.3) * 0.1; angle = std::sin(bz * 2.0) * 30.0; scale = 1.0 - absd * 0.04 + std::sin(bz) * 0.05; focus = std::clamp(1.0 - absd * 0.05, 0.2, 1.0); y = std::sin(bz * 1.7) * 0.2; break; }
        case Books: { int bSlot = i % 3 - 1; double bd = (double)(i / 3) - m_offset; double babs = std::abs(bd); x = (double)bSlot * 0.5; z = -babs * 0.1 - 0.05; angle = 0.0; scale = 1.0 - babs * 0.06; focus = std::clamp(1.0 - babs * 0.06, 0.3, 1.0); y = 0.0; break; }
        case Cage: { int cBar = i % 6; double ca2 = (double)cBar * 1.047; double cd2 = (double)(i / 6) - m_offset; double c2abs = std::abs(cd2); x = std::sin(ca2) * 1.5; z = std::cos(ca2) * 1.5 - 1.5 - c2abs * 0.12; angle = -ca2 * 30.0; scale = 1.0 / (1.0 + c2abs * 0.12); focus = std::clamp(1.0 - c2abs * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Candle: { double cFlick = std::sin(d * 3.0) * 0.05; x = d * kSpacing + cFlick; z = -absd * 0.08; angle = cFlick * 40.0; scale = 1.0 + cFlick * 0.2; focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0); y = 0.2 + std::abs(cFlick) * 0.3; alpha = std::clamp(0.7 + (1.0 - absd * 0.05) * 0.3, 0.0, 1.0); break; }
        case Chain: { int cLink = i % 2; double cd3 = (double)(i / 2) - m_offset; double c3abs = std::abs(cd3); x = (cLink == 0 ? -0.2 : 0.2); z = -c3abs * 0.1; angle = (cLink == 0 ? 10.0 : -10.0); scale = 1.0 / (1.0 + c3abs * 0.1); focus = std::clamp(1.0 - c3abs * 0.08, 0.3, 1.0); y = (cLink == 0 ? 0.1 : -0.1); break; }
        case Checker: { int cRow = i / 4; int cCol = i % 4; double cOff = (cRow % 2 == 0) ? 0.0 : 0.5; double cRd = (double)cRow - m_offset; double cRabs = std::abs(cRd); x = (double)(cCol - 1) * 1.0 + cOff; z = -cRabs * 0.15; angle = 0.0; scale = 1.0 / (1.0 + cRabs * 0.12); focus = std::clamp(1.0 - cRabs * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Clock: { int cHr = i % 12; double cHrA = (double)cHr * 0.523; double cHd = (double)(i / 12) - m_offset; double cHabs = std::abs(cHd); x = std::sin(cHrA) * 1.8; z = std::cos(cHrA) * 1.8 - 1.8 - cHabs * 0.08; angle = -cHrA * 30.0; scale = 1.0 / (1.0 + cHabs * 0.1); focus = std::clamp(1.0 - cHabs * 0.08, 0.3, 1.0); y = std::cos(cHrA) * 0.15; break; }
        case Cloud: { double cPhase = d * 0.6; x = d * kSpacing; z = -absd * 0.06 + std::sin(cPhase) * 0.15; angle = 0.0; scale = 1.0 - absd * 0.04 + std::sin(cPhase * 1.5) * 0.05; focus = std::clamp(1.0 - absd * 0.05, 0.3, 1.0); y = 0.3 + std::cos(cPhase) * 0.1; alpha = std::clamp(0.8 + (1.0 - absd * 0.04) * 0.2, 0.0, 1.0); break; }
        case Coil: { double coR = d * 0.5; x = std::sin(coR) * (0.5 + absd * 0.1); z = std::cos(coR) * (0.5 + absd * 0.1) - (0.5 + absd * 0.1); angle = d * 30.0; scale = 1.0 / (1.0 + absd * 0.15); focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0); y = 0.0; break; }
        case Compass: { int cDir = i % 8; double cDA = (double)cDir * 0.785; double cDd = (double)(i / 8) - m_offset; double cDabs = std::abs(cDd); x = std::sin(cDA) * 1.5; z = std::cos(cDA) * 1.5 - 1.5 - cDabs * 0.1; angle = -cDA * 40.0; scale = 1.0 / (1.0 + cDabs * 0.1); focus = std::clamp(1.0 - cDabs * 0.08, 0.3, 1.0); y = (cDir % 2 == 0 ? 0.15 : -0.15); break; }
        case Cube: { int cFace = i % 6; double cFd = (double)(i / 6) - m_offset; double cFabs = std::abs(cFd); double cfA = (double)cFace * 1.047; x = std::sin(cfA) * 1.2; z = std::cos(cfA) * 1.2 - 1.2 - cFabs * 0.1; angle = -cfA * 40.0; scale = 1.0 / (1.0 + cFabs * 0.12); focus = std::clamp(1.0 - cFabs * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Dice: { double rOff = ((i * 7 + 3) % 5) / 5.0 - 0.5; double rd2 = ((i * 13 + 7) % 5) / 5.0 - 0.5; double rdd = ((i * 17 + 11) % 7) / 7.0 * 2.0 - 1.0; double rdOff = rdd - m_offset * 0.3; double rdAbs = std::abs(rdOff); x = rOff * 1.5; z = -rdAbs * 0.2; angle = rOff * 30.0; scale = 1.0 / (1.0 + rdAbs * 0.15); focus = std::clamp(1.0 - rdAbs * 0.1, 0.2, 1.0); y = rd2 * 0.8; break; }
        case Dome: { double dmR = absd * 0.4; double dmH = std::cos(dmR * 1.5) * 0.5; x = d * kSpacing; z = -dmR * 0.3; angle = 0.0; scale = 1.0 - dmR * 0.15; focus = std::clamp(1.0 - dmR * 0.12, 0.2, 1.0); y = dmH; break; }
        case Door: { double sign = (d >= 0) ? -1.0 : 1.0; double doorA = (absd < 0.5) ? absd * 90.0 : 45.0; x = d * kSpacing; z = -absd * 0.08 + std::abs(std::sin(doorA * 0.01745)) * 0.1; angle = sign * doorA; scale = 1.0 / (1.0 + absd * 0.1); focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Fence: { int fPicket = i % 5 - 2; double fd2 = (double)(i / 5) - m_offset; double f2abs = std::abs(fd2); x = (double)fPicket * 0.5; z = -f2abs * 0.1; angle = 0.0; scale = 1.0 - f2abs * 0.06; focus = std::clamp(1.0 - f2abs * 0.06, 0.3, 1.0); y = 0.0; break; }
        case Fog: { double fg = d * 0.3; x = d * kSpacing; z = -absd * 0.04 + std::sin(fg) * 0.08; angle = std::sin(fg * 1.5) * 10.0; scale = 1.0 - absd * 0.03; focus = std::clamp(1.0 - absd * 0.04, 0.2, 1.0); alpha = std::clamp(0.3 + (1.0 - absd * 0.05) * 0.7, 0.0, 1.0); y = 0.0; break; }
        case Gear: { int gTooth = i % 8; double gTA = (double)gTooth * 0.785; double gTd = (double)(i / 8) - m_offset; double gTabs = std::abs(gTd); double gR = 1.0 + (gTooth % 2 == 0 ? 0.3 : 0.0); x = std::sin(gTA) * gR; z = std::cos(gTA) * gR - gR - gTabs * 0.08; angle = -gTA * 35.0; scale = 1.0 / (1.0 + gTabs * 0.1); focus = std::clamp(1.0 - gTabs * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Hammer: { double hSwing = std::sin(d * 1.5) * 30.0; x = d * kSpacing; z = -absd * 0.08; angle = hSwing; scale = 1.0 / (1.0 + absd * 0.1); focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0); y = std::abs(std::sin(d * 1.5)) * 0.2; break; }
        case Horn: { double hSpread = 0.5 + absd * 0.2; x = d * kSpacing * hSpread; z = -absd * 0.1; angle = d * 10.0 * hSpread; scale = 1.0 / (1.0 + absd * 0.08); focus = std::clamp(1.0 - absd * 0.06, 0.3, 1.0); y = absd * 0.1; break; }
        case Hurricane: { double hRad = d * 0.6; double hR = 0.1 + absd * 0.2; x = std::cos(hRad) * hR * 4.0; z = std::sin(hRad) * hR * 4.0 - absd * 0.06; angle = d * 50.0; scale = 1.0 / (1.0 + absd * 0.15); focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0); y = std::sin(hRad * 1.5) * hR * 0.3; break; }
        case Kite: { double kSway = std::sin(d * 1.2) * 0.4; double kTilt = std::sin(d * 1.8) * 15.0; x = d * kSpacing + kSway; z = -absd * 0.08; angle = kTilt; scale = 1.0 / (1.0 + absd * 0.08); focus = std::clamp(1.0 - absd * 0.06, 0.3, 1.0); y = 0.3 + std::abs(kSway) * 0.3; break; }
        case Ladder: { int lRung = i % 2; double ld2 = (double)(i / 2) - m_offset; double l2abs = std::abs(ld2); x = (lRung == 0 ? -0.5 : 0.5); z = -l2abs * 0.1; angle = 0.0; scale = 1.0 - l2abs * 0.05; focus = std::clamp(1.0 - l2abs * 0.06, 0.3, 1.0); y = (double)(i / 2) * 0.25; break; }
        case Lens: { double lMag = 1.0 - absd * 0.08; x = d * kSpacing; z = -absd * 0.1; angle = 0.0; scale = 0.5 + lMag * 0.5; focus = lMag; break; }
        case Lock: { int lKey = i % 2; double ld3 = (double)(i / 2) - m_offset; double l3abs = std::abs(ld3); x = (lKey == 0 ? -0.4 : 0.4); z = -l3abs * 0.1; angle = 0.0; scale = 1.0 / (1.0 + l3abs * 0.1); focus = std::clamp(1.0 - l3abs * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Magnet: { double mForce = 1.0 / (1.0 + absd * 0.4); x = d * kSpacing * mForce; z = -absd * 0.06 * mForce; angle = d * 15.0 * mForce; scale = 0.3 + 0.7 * mForce; focus = mForce; break; }
        case Mask: { int mMouth = i % 3 - 1; double md2 = (double)(i / 3) - m_offset; double m2abs = std::abs(md2); x = (double)mMouth * 0.6; z = -m2abs * 0.1; angle = 0.0; scale = 1.0 / (1.0 + m2abs * 0.1); focus = std::clamp(1.0 - m2abs * 0.08, 0.3, 1.0); y = (mMouth == 0 ? 0.2 : -0.1); break; }
        case Net: { int nRow = i / 5; int nCol = i % 5; double nRd = (double)nRow - m_offset; double nRabs = std::abs(nRd); x = (double)(nCol - 2) * 0.8; z = -nRabs * 0.12; angle = 0.0; scale = 1.0 / (1.0 + nRabs * 0.1); focus = std::clamp(1.0 - nRabs * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Nut: { int nSide = i % 6; double nSA = (double)nSide * 1.047; double nSd = (double)(i / 6) - m_offset; double nSabs = std::abs(nSd); x = std::sin(nSA) * 1.2; z = std::cos(nSA) * 1.2 - 1.2 - nSabs * 0.1; angle = -nSA * 35.0; scale = 1.0 / (1.0 + nSabs * 0.1); focus = std::clamp(1.0 - nSabs * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Orb: { double oRad = d * 0.35; double oLat = (double)(i % 4) * 0.785; double oLon = d * 0.3; double od = (double)(i / 4) - m_offset; double oabs = std::abs(od); double oR = std::cos(oLat) * 1.5; x = std::sin(oLon) * oR; z = std::cos(oLon) * oR - oR - oabs * 0.06; angle = -oLon * 25.0; scale = 1.0 / (1.0 + oabs * 0.08); focus = std::clamp(1.0 - oabs * 0.06, 0.3, 1.0); y = std::sin(oLat) * 1.5; break; }
        case Pillar: { int pCol = i % 3 - 1; double pd2 = (double)(i / 3) - m_offset; double p2abs = std::abs(pd2); x = (double)pCol * 0.8; z = -p2abs * 0.08; angle = 0.0; scale = 1.0 - p2abs * 0.04; focus = std::clamp(1.0 - p2abs * 0.05, 0.3, 1.0); y = (double)(i / 3) * 0.2; break; }
        case Puzzle: { int pCol2 = i % 3 - 1; int pRow2 = i / 3; double pRd = (double)pRow2 - m_offset; double pRabs = std::abs(pRd); x = (double)pCol2 * 0.8 + ((pRow2 % 2 == 0) ? 0.0 : 0.1); z = -pRabs * 0.12 + (pCol2 % 2 == 0 ? 0.02 : -0.02); angle = (pCol2 % 2 == 0 ? 5.0 : -5.0); scale = 1.0 / (1.0 + pRabs * 0.1); focus = std::clamp(1.0 - pRabs * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Rack: { int rPos = i % 4 - 1; double rd3 = (double)(i / 4) - m_offset; double r3abs = std::abs(rd3); x = (double)rPos * 0.6; z = -r3abs * 0.08; angle = 0.0; scale = 1.0 - r3abs * 0.05; focus = std::clamp(1.0 - r3abs * 0.06, 0.3, 1.0); y = 0.0; break; }
        case Ramp: { double rSlope = absd * 0.3; x = d * kSpacing; z = -rSlope * 0.15; angle = 10.0; scale = 1.0 - rSlope * 0.08; focus = std::clamp(1.0 - rSlope * 0.1, 0.3, 1.0); y = -rSlope * 0.2; break; }
        case Reel: { double rRad = d * 0.5; double rR = 0.3 + absd * 0.1; x = std::sin(rRad) * rR * 2.0; z = std::cos(rRad) * rR * 2.0 - rR * 2.0 - absd * 0.06; angle = d * 25.0; scale = 1.0 / (1.0 + absd * 0.1); focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Rope: { double rTwist = d * 0.4; x = std::sin(rTwist * 3.0) * 0.3; z = std::cos(rTwist * 3.0) * 0.3 - absd * 0.1; angle = d * 20.0; scale = 1.0 / (1.0 + absd * 0.12); focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Saw: { int sTooth = i % 2; double sd3 = (double)(i / 2) - m_offset; double s3abs = std::abs(sd3); x = (sTooth == 0 ? -0.5 : 0.5); z = -s3abs * 0.1; angle = (sTooth == 0 ? -20.0 : 20.0); scale = 1.0 / (1.0 + s3abs * 0.1); focus = std::clamp(1.0 - s3abs * 0.08, 0.3, 1.0); y = (sTooth == 0 ? 0.15 : -0.15); break; }
        case Scale: { double sBal = std::sin(d * 0.8) * 0.2; x = d * kSpacing; z = -absd * 0.1; angle = sBal * 30.0; scale = 1.0 / (1.0 + absd * 0.1); focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0); y = std::abs(sBal) * 0.2; break; }
        case Screw: { double scRad = d * 1.0; double scR = 0.2 + absd * 0.05; x = std::cos(scRad) * scR * 2.0; z = std::sin(scRad) * scR * 2.0 - absd * 0.08; angle = d * 180.0; scale = 1.0 / (1.0 + absd * 0.15); focus = std::clamp(1.0 - absd * 0.1, 0.3, 1.0); y = d * 0.15; break; }
        case Shield: { int sSeg = i % 5 - 2; double sd4 = (double)(i / 5) - m_offset; double s4abs = std::abs(sd4); double sCurve = 1.0 - std::abs((double)sSeg) * 0.2; x = (double)sSeg * 0.5; z = -s4abs * 0.08 - (1.0 - sCurve) * 0.1; angle = (double)sSeg * 15.0; scale = sCurve / (1.0 + s4abs * 0.08); focus = std::clamp(sCurve - s4abs * 0.05, 0.2, 1.0); y = sCurve * 0.3 - 0.2; break; }
        case Spike: { double sp = d * 0.4; double spR = 0.2 + absd * 0.12; x = std::cos(sp) * spR * 3.0; z = std::sin(sp) * spR * 3.0 - spR * 3.0 - absd * 0.06; angle = d * 25.0; scale = 1.0 / (1.0 + absd * 0.1); focus = std::clamp(1.0 - absd * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Star: { int sPt = i % 5; double sPA = (double)sPt * 1.256 - 1.57; double sPd = (double)(i / 5) - m_offset; double sPabs = std::abs(sPd); double sR = (sPt % 2 == 0) ? 1.8 : 0.7; x = std::sin(sPA) * sR; z = std::cos(sPA) * sR - sR - sPabs * 0.08; angle = -sPA * 35.0; scale = 1.0 / (1.0 + sPabs * 0.1); focus = std::clamp(1.0 - sPabs * 0.08, 0.3, 1.0); y = (sPt % 2 == 0 ? 0.2 : -0.1); break; }
        case Sun: { int sRay = i % 8; double sRA = (double)sRay * 0.785; double sRd = (double)(i / 8) - m_offset; double sRabs = std::abs(sRd); double sRlen = 1.5 + (sRay % 2 == 0 ? 0.5 : 0.0); x = std::sin(sRA) * sRlen; z = std::cos(sRA) * sRlen - sRlen - sRabs * 0.06; angle = -sRA * 30.0; scale = 1.0 / (1.0 + sRabs * 0.08); focus = std::clamp(1.0 - sRabs * 0.06, 0.3, 1.0); y = (sRay % 2 == 0 ? 0.2 : -0.1); break; }
        case Swan: { double sw = d * 0.3; double swCurve = std::sin(sw * M_PI) * 0.6; x = d * kSpacing + swCurve; z = -absd * 0.08 + swCurve * 0.1; angle = d * 15.0 + swCurve * 20.0; scale = 1.0 / (1.0 + absd * 0.08); focus = std::clamp(1.0 - absd * 0.06, 0.3, 1.0); y = swCurve * 0.3; break; }
        case Sword: { int sCross = i % 2; double sd5 = (double)(i / 2) - m_offset; double s5abs = std::abs(sd5); x = (sCross == 0 ? -1.2 : 1.2); z = -s5abs * 0.1; angle = (sCross == 0 ? -20.0 : 20.0); scale = 1.0 / (1.0 + s5abs * 0.1); focus = std::clamp(1.0 - s5abs * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Temple: { int tCol = i % 5 - 2; double td = (double)(i / 5) - m_offset; double tabs = std::abs(td); double tH = 0.8 - std::abs((double)tCol) * 0.15; x = (double)tCol * 0.7; z = -tabs * 0.08; angle = 0.0; scale = 1.0 - tabs * 0.04; focus = std::clamp(1.0 - tabs * 0.05, 0.3, 1.0); y = tH - 0.3; break; }
        case Tree: { int tBran = i % 4; double tBd = (double)(i / 4) - m_offset; double tBabs = std::abs(tBd); double tBw = (double)(tBran - 1) * (0.5 + tBabs * 0.05); double tBh = 0.5 - std::abs((double)(tBran - 1)) * 0.12; x = tBw; z = -tBabs * 0.1; angle = (double)(tBran - 1) * 15.0; scale = tBh / (1.0 + tBabs * 0.08); focus = std::clamp(tBh - tBabs * 0.05, 0.2, 1.0); y = tBh; break; }
        case Turbine: { int tBlade = i % 6; double tBA = (double)tBlade * 1.047; double tBd = (double)(i / 6) - m_offset; double tBabs = std::abs(tBd); double tBl = 1.5; x = std::sin(tBA) * tBl; z = std::cos(tBA) * tBl - tBl - tBabs * 0.08; angle = -tBA * 35.0; scale = 1.0 / (1.0 + tBabs * 0.1); focus = std::clamp(1.0 - tBabs * 0.08, 0.3, 1.0); y = 0.0; break; }
        case Alpha: { x = d * kSpacing; z = -absd*0.1; angle = 0.0; scale = 1.0/(1.0+absd*0.1); focus = std::clamp(1.0-absd*0.08,0.3,1.0); break; }
        case Bravo: { x = d*kSpacing; z = -absd*0.12; angle = d*10.0; scale = 1.0/(1.0+absd*0.12); focus = std::clamp(1.0-absd*0.1,0.3,1.0); y = absd*0.05; break; }
        case Charlie: { double c = d*0.5; x = d*kSpacing; z = -absd*0.08+std::sin(c)*0.1; angle = std::sin(c*1.5)*15.0; scale = 1.0/(1.0+absd*0.08); focus = std::clamp(1.0-absd*0.06,0.3,1.0); y = std::cos(c)*0.1; break; }
        case Delta: { double s = (d>=0)?1.0:-1.0; x = d*kSpacing; z = -absd*0.1; angle = s*25.0; scale = 1.0/(1.0+absd*0.15); focus = std::clamp(1.0-absd*0.12,0.3,1.0); y = -absd*0.05; break; }
        case Foxtrot: { double ft = d*0.8; x = d*kSpacing; z = -absd*0.08; angle = std::sin(ft)*20.0; scale = 1.0/(1.0+absd*0.08); focus = std::clamp(1.0-absd*0.06,0.3,1.0); y = std::abs(std::sin(ft))*0.15; angleX = std::cos(ft)*15.0; break; }
        case Golf: { int gCl = i%3-1; double gd = (double)(i/3)-m_offset; double ga = std::abs(gd); x = (double)gCl*0.5; z = -ga*0.08; angle = (double)gCl*10.0; scale = 1.0/(1.0+ga*0.08); focus = std::clamp(1.0-ga*0.06,0.3,1.0); y = 0.0; break; }
        case Hotel: { int hFl = i%4-1; double hd = (double)(i/4)-m_offset; double ha = std::abs(hd); x = (double)hFl*0.6; z = -ha*0.08+0.05; angle = 0.0; scale = 1.0-ha*0.04; focus = std::clamp(1.0-ha*0.05,0.3,1.0); y = (double)(i/4)*0.15; break; }
        case India: { double in = d*0.4; x = d*kSpacing+std::sin(in)*0.2; z = -absd*0.08+std::cos(in*1.3)*0.06; angle = std::sin(in)*15.0; scale = 1.0/(1.0+absd*0.06); focus = std::clamp(1.0-absd*0.05,0.3,1.0); y = std::sin(in*1.7)*0.1; break; }
        case Juliet: { int jP = i%2; double jd = (double)(i/2)-m_offset; double ja = std::abs(jd); x = (jP==0?-0.3:0.3); z = -ja*0.08; angle = (jP==0?-15.0:15.0); scale = 1.0/(1.0+ja*0.08); focus = std::clamp(1.0-ja*0.06,0.3,1.0); y = (jP==0?0.1:-0.1); break; }
        case Kilo: { double kr = d*0.3; x = d*kSpacing; z = -absd*0.08; angle = d*15.0; scale = 1.0/(1.0+absd*0.08); focus = std::clamp(1.0-absd*0.06,0.3,1.0); y = std::sin(kr)*0.2; break; }
        case Lima: { int lPt = i%4-1; double ld = (double)(i/4)-m_offset; double la = std::abs(ld); x = (double)lPt*0.6; z = -la*0.1; angle = (double)lPt*15.0; scale = 1.0/(1.0+la*0.1); focus = std::clamp(1.0-la*0.08,0.3,1.0); y = 0.0; break; }
        case Mike: { double mk = d*0.6; x = d*kSpacing; z = -absd*0.08; angle = 0.0; scale = 1.0/(1.0+absd*0.06); focus = std::clamp(1.0-absd*0.05,0.3,1.0); y = std::cos(mk)*0.15; break; }
        case November: { double nv = d*0.4; x = d*kSpacing+std::sin(nv)*0.15; z = -absd*0.08; angle = std::sin(nv)*12.0; scale = 1.0/(1.0+absd*0.06); focus = std::clamp(1.0-absd*0.05,0.3,1.0); y = std::cos(nv*1.5)*0.1; break; }
        case Oscar: { double os = d*0.5; x = d*kSpacing; z = -absd*0.08; angle = d*20.0; scale = 1.0/(1.0+absd*0.1); focus = std::clamp(1.0-absd*0.08,0.3,1.0); y = std::sin(os)*0.15; break; }
        case Papa: { int pS = i%2; double pd = (double)(i/2)-m_offset; double pa = std::abs(pd); x = (pS==0?-0.4:0.4); z = -pa*0.1; angle = (pS==0?-20.0:20.0); scale = 1.0/(1.0+pa*0.1); focus = std::clamp(1.0-pa*0.08,0.3,1.0); y = 0.0; break; }
        case Quebec: { double qb = d*0.3; x = d*kSpacing+std::sin(qb)*0.15; z = -absd*0.1; angle = std::sin(qb)*20.0; scale = 1.0/(1.0+absd*0.08); focus = std::clamp(1.0-absd*0.06,0.3,1.0); y = std::cos(qb*2.0)*0.1; break; }
        case Romeo: { double rm = d*0.7; x = d*kSpacing; z = -absd*0.08+std::sin(rm)*0.06; angle = std::sin(rm)*25.0; scale = 1.0/(1.0+absd*0.08); focus = std::clamp(1.0-absd*0.06,0.3,1.0); y = std::abs(std::sin(rm))*0.1; break; }
        case Sierra: { int sR = i%3-1; double sd = (double)(i/3)-m_offset; double sa = std::abs(sd); x = (double)sR*0.7; z = -sa*0.1; angle = (double)sR*20.0; scale = 1.0/(1.0+sa*0.1); focus = std::clamp(1.0-sa*0.08,0.3,1.0); y = (double)sR*0.1; break; }
        case Tango: { double tg = d*0.5; x = d*kSpacing; z = -absd*0.1+std::sin(tg)*0.05; angle = std::sin(tg*2.0)*15.0; scale = 1.0/(1.0+absd*0.08); focus = std::clamp(1.0-absd*0.06,0.3,1.0); y = std::cos(tg)*0.15; break; }
        case Uniform: { int uP = i%2; double ud = (double)(i/2)-m_offset; double ua = std::abs(ud); x = (uP==0?-0.5:0.5); z = -ua*0.1; angle = (uP==0?-10.0:10.0); scale = 1.0-ua*0.05; focus = std::clamp(1.0-ua*0.06,0.3,1.0); y = 0.0; break; }
        case Victor: { double vt = d*0.6; x = d*kSpacing; z = -absd*0.08; angle = d*15.0; scale = 1.0/(1.0+absd*0.08); focus = std::clamp(1.0-absd*0.06,0.3,1.0); y = std::sin(vt)*0.2; break; }
        case Whiskey: { double wh = d*0.4; x = d*kSpacing+std::sin(wh)*0.25; z = -absd*0.08+std::cos(wh*1.5)*0.05; angle = std::sin(wh)*20.0; scale = 1.0/(1.0+absd*0.06); focus = std::clamp(1.0-absd*0.05,0.3,1.0); y = std::sin(wh*2.0)*0.1; break; }
        case Xray: { double xr = d*0.5; x = d*kSpacing; z = -absd*0.1; angle = d*25.0; scale = 1.0/(1.0+absd*0.12); focus = std::clamp(1.0-absd*0.1,0.3,1.0); alpha = std::clamp(0.5+(1.0-absd*0.06)*0.5,0.0,1.0); break; }
        case Yankee: { int yk = i%2; double yd = (double)(i/2)-m_offset; double ya = std::abs(yd); x = (yk==0?-0.6:0.6); z = -ya*0.1; angle = (yk==0?-25.0:25.0); scale = 1.0/(1.0+ya*0.1); focus = std::clamp(1.0-ya*0.08,0.3,1.0); y = (yk==0?0.15:-0.15); break; }
        case Zulu: { double zu = d*0.3; x = d*kSpacing; z = -absd*0.08; angle = 0.0; scale = 1.0/(1.0+absd*0.06); focus = std::clamp(1.0-absd*0.05,0.3,1.0); y = std::sin(zu)*0.2; break; }
        case Oval: { double ov = d*0.3; x = std::sin(ov)*2.5; z = std::cos(ov)*2.5-2.5-absd*0.04; angle = -d*15.0; scale = 1.0; focus = std::clamp(1.0-absd*0.06,0.4,1.0); y = std::sin(ov*2.0)*0.15; break; }
        case Hexagon: { int hx = i%6; double hxA = (double)hx*1.047; double hxd = (double)(i/6)-m_offset; double hxa = std::abs(hxd); x = std::sin(hxA)*1.3; z = std::cos(hxA)*1.3-1.3-hxa*0.08; angle = -hxA*35.0; scale = 1.0/(1.0+hxa*0.1); focus = std::clamp(1.0-hxa*0.08,0.3,1.0); y = 0.0; break; }
        case Octagon: { int oc = i%8; double ocA = (double)oc*0.785; double ocd = (double)(i/8)-m_offset; double oca = std::abs(ocd); x = std::sin(ocA)*1.4; z = std::cos(ocA)*1.4-1.4-oca*0.08; angle = -ocA*30.0; scale = 1.0/(1.0+oca*0.08); focus = std::clamp(1.0-oca*0.06,0.3,1.0); y = 0.0; break; }
        case Crescent: { double cr = d*0.3; double crR = 1.5-absd*0.03; x = std::sin(cr)*crR; z = std::cos(cr)*crR-crR-absd*0.05; angle = -d*20.0; scale = 1.0/(1.0+absd*0.06); focus = std::clamp(1.0-absd*0.05,0.3,1.0); y = std::cos(cr*2.0)*0.2; break; }
        case Trapezoid: { int tz = i%3-1; double tzd = (double)(i/3)-m_offset; double tza = std::abs(tzd); double tzS = 1.0-std::abs((double)tz)*0.2; x = (double)tz*tzS*0.8; z = -tza*0.1; angle = (double)tz*10.0; scale = tzS/(1.0+tza*0.08); focus = std::clamp(tzS-tza*0.05,0.3,1.0); y = std::abs((double)tz)*0.1; break; }
        case Parabola: { double pb = absd*0.5; double pbY = pb*pb*0.5; x = d*kSpacing; z = -pb*0.12; angle = d*10.0*pb; scale = 1.0/(1.0+pb*0.1); focus = std::clamp(1.0-pb*0.08,0.3,1.0); y = -pbY*0.5; break; }
        case Teardrop: { double td = d*0.4; double tdR = 0.5+absd*0.1; x = std::sin(td)*tdR*2.0; z = std::cos(td)*tdR*2.0-tdR*2.0-absd*0.06; angle = -d*20.0; scale = 1.0/(1.0+absd*0.08); focus = std::clamp(1.0-absd*0.06,0.3,1.0); y = -tdR*0.2+0.2; break; }
        case Lozenge: { int lz = i%3-1; double lzd = (double)(i/3)-m_offset; double lza = std::abs(lzd); x = (double)lz*0.8; z = -lza*0.08; angle = (double)lz*15.0; scale = 1.0/(1.0+lza*0.08); focus = std::clamp(1.0-lza*0.06,0.3,1.0); y = (double)lz*0.1; break; }
        case Chevron: { int cv = i%2; double cvd = (double)(i/2)-m_offset; double cva = std::abs(cvd); x = (cv==0?-0.7:0.7); z = -cva*0.1; angle = (cv==0?-30.0:30.0); scale = 1.0/(1.0+cva*0.1); focus = std::clamp(1.0-cva*0.08,0.3,1.0); y = (cv==0?0.2:-0.2); break; }
        case Arc2D: { double a2 = absd*0.4; double a2H = std::sin(a2*M_PI)*0.4; x = d*kSpacing; z = -a2*0.12; angle = 0.0; scale = 1.0/(1.0+a2*0.08); focus = std::clamp(1.0-a2*0.06,0.3,1.0); y = a2H; break; }
        case Bolt2D: { int bl = i%2; double bld = (double)(i/2)-m_offset; double bla = std::abs(bld); x = (bl==0?-0.3:0.3); z = -bla*0.1; angle = (bl==0?-45.0:45.0); scale = 1.0/(1.0+bla*0.12); focus = std::clamp(1.0-bla*0.08,0.3,1.0); y = (bl==0?0.15:-0.15); break; }
        case Drop2D: { double dp = d*0.5; x = d*kSpacing; z = -absd*0.08; angle = 0.0; scale = 1.0-absd*0.04; focus = std::clamp(1.0-absd*0.05,0.3,1.0); y = -absd*0.2+std::cos(dp)*0.1; break; }
        case Ring3D: { double r3 = d*0.4; x = std::sin(r3)*1.8; z = std::cos(r3)*1.8-1.8; angle = -d*20.0; scale = 1.0; focus = std::clamp(1.0-absd*0.06,0.4,1.0); y = 0.0; break; }
        case Diamond3D: { int dm = i%4; double dmA = (double)dm*1.571; double dmd = (double)(i/4)-m_offset; double dma = std::abs(dmd); x = std::sin(dmA)*1.4; z = std::cos(dmA)*1.4-1.4-dma*0.08; angle = -dmA*35.0; scale = 1.0/(1.0+dma*0.1); focus = std::clamp(1.0-dma*0.08,0.3,1.0); y = (dm<2?0.2:-0.2); break; }
        case Wiggle2D: { double wg = d*1.5; x = d*kSpacing+std::sin(wg)*0.15; z = -absd*0.06+std::cos(wg*1.3)*0.05; angle = std::sin(wg)*15.0; scale = 1.0-absd*0.04; focus = std::clamp(1.0-absd*0.05,0.3,1.0); y = std::sin(wg*1.7)*0.08; break; }
        case Blossom: { double bs = d*0.3; double bsR = 0.3+absd*0.12; x = std::sin(bs)*bsR*2.5; z = std::cos(bs)*bsR*2.5-bsR*2.5-absd*0.05; angle = -d*20.0; scale = 1.0/(1.0+absd*0.08); focus = std::clamp(1.0-absd*0.06,0.3,1.0); y = std::sin(bs*2.0)*bsR*0.2; break; }
        case Breeze: { double br = d*0.8; x = d*kSpacing+std::sin(br)*0.2; z = -absd*0.06+std::cos(br)*0.08; angle = std::sin(br)*20.0; scale = 1.0/(1.0+absd*0.05); focus = std::clamp(1.0-absd*0.04,0.3,1.0); y = std::sin(br*1.5)*0.1; break; }
        case Burst: { double bu = d*0.4; double buR = 0.2+absd*0.15; x = std::cos(bu)*buR*3.5; z = std::sin(bu)*buR*3.5-buR*3.5-absd*0.05; angle = d*30.0; scale = 1.0/(1.0+absd*0.12); focus = std::clamp(1.0-absd*0.08,0.3,1.0); y = std::sin(bu*2.0)*buR*0.2; break; }
        case Canyon: { int cn = i%3-1; double cnd = (double)(i/3)-m_offset; double cna = std::abs(cnd); x = (double)cn*1.2; z = -cna*0.15+0.1; angle = (double)cn*5.0; scale = 1.0/(1.0+cna*0.12); focus = std::clamp(1.0-cna*0.1,0.3,1.0); y = -cna*0.1; break; }
        case Cave: { double cv2 = d*0.3; double cvR = 0.5+absd*0.15; x = std::sin(cv2)*cvR*2.0; z = std::cos(cv2)*cvR*2.0-cvR*2.0-absd*0.12; angle = -d*15.0; scale = 1.0/(1.0+absd*0.15); focus = std::clamp(1.0-absd*0.12,0.3,1.0); y = std::cos(cv2*2.0)*0.15; alpha = std::clamp(0.7+(1.0-absd*0.08)*0.3,0.0,1.0); break; }
        case Cliff: { int cl = i%3-1; double cld = (double)(i/3)-m_offset; double cla = std::abs(cld); x = (double)cl*1.0; z = -cla*0.12; angle = (double)cl*10.0; scale = 1.0/(1.0+cla*0.1); focus = std::clamp(1.0-cla*0.08,0.3,1.0); y = (double)cl*0.15; break; }
        case Cosmos2D: { double cs = d*0.5; double csR = 0.2+absd*0.08; x = std::cos(cs)*csR*3.0; z = std::sin(cs)*csR*3.0-3.0*csR-absd*0.04; angle = d*25.0; scale = 1.0/(1.0+absd*0.06); focus = std::clamp(1.0-absd*0.05,0.3,1.0); y = std::sin(cs*2.0)*csR*0.3; alpha = std::clamp(0.6+(1.0-absd*0.04)*0.4,0.0,1.0); break; }
        case Crater: { double ct = absd*0.3; double ctR = 0.3+ct*0.2; x = d*kSpacing; z = -ctR*0.8; angle = 0.0; scale = 1.0-ct*0.1; focus = std::clamp(1.0-ct*0.08,0.3,1.0); y = -std::cos(ct*M_PI)*0.15; break; }
        case Dune2D: { double dn2 = d*0.4; double dnH = std::sin(dn2)*0.2; x = d*kSpacing; z = -absd*0.06; angle = dnH*30.0; scale = 1.0/(1.0+absd*0.04); focus = std::clamp(1.0-absd*0.04,0.3,1.0); y = dnH; break; }
        case Geode: { int gd2 = i%6; double gdA = (double)gd2*1.047; double gdd = (double)(i/6)-m_offset; double gda = std::abs(gdd); x = std::sin(gdA)*(0.8+gda*0.05); z = std::cos(gdA)*(0.8+gda*0.05)-0.8-gda*0.06; angle = -gdA*30.0; scale = 1.0/(1.0+gda*0.06); focus = std::clamp(1.0-gda*0.05,0.3,1.0); y = (gd2%2==0?0.1:-0.1); break; }
        case Glacier: { double gl = d*0.3; x = d*kSpacing; z = -absd*0.06; angle = d*5.0; scale = 1.0-absd*0.03; focus = std::clamp(1.0-absd*0.04,0.3,1.0); y = -absd*0.08; break; }
        case Grove: { int gv = i%3-1; double gvd = (double)(i/3)-m_offset; double gva = std::abs(gvd); x = (double)gv*0.9; z = -gva*0.1; angle = (double)gv*10.0; scale = 1.0/(1.0+gva*0.08); focus = std::clamp(1.0-gva*0.06,0.3,1.0); y = 0.0; break; }
        case Iceberg: { double ib = absd*0.3; x = d*kSpacing; z = -ib*0.15; angle = 0.0; scale = 1.0-ib*0.08; focus = std::clamp(1.0-ib*0.06,0.3,1.0); y = -ib*0.5; break; }
        case Jungle: { int jg = i%3-1; double jgd = (double)(i/3)-m_offset; double jga = std::abs(jgd); x = (double)jg*1.1; z = -jga*0.12; angle = (double)jg*15.0; scale = 1.0/(1.0+jga*0.1); focus = std::clamp(1.0-jga*0.08,0.3,1.0); y = (double)jg*0.1; break; }
        case Lava2D: { double lv = d*0.5; x = d*kSpacing+std::sin(lv)*0.15; z = -absd*0.08+std::cos(lv*1.3)*0.05; angle = std::sin(lv)*20.0; scale = 1.0/(1.0+absd*0.06); focus = std::clamp(1.0-absd*0.05,0.3,1.0); y = std::abs(std::sin(lv))*0.1; break; }
        case Meadow: { int md = i%4-1; double mdd = (double)(i/4)-m_offset; double mda = std::abs(mdd); x = (double)md*0.8; z = -mda*0.08; angle = 0.0; scale = 1.0-mda*0.05; focus = std::clamp(1.0-mda*0.05,0.3,1.0); y = 0.0; break; }
        case Ocean2D: { double oc2 = d*0.6; x = d*kSpacing+std::sin(oc2)*0.3; z = -absd*0.04+std::cos(oc2*1.5)*0.1; angle = std::sin(oc2)*15.0; scale = 1.0/(1.0+absd*0.04); focus = std::clamp(1.0-absd*0.04,0.3,1.0); y = std::sin(oc2*2.0)*0.12; break; }
        case Prairie: { int pr = i%4-1; double prd = (double)(i/4)-m_offset; double pra = std::abs(prd); x = (double)pr*1.0; z = -pra*0.06; angle = 0.0; scale = 1.0-pra*0.04; focus = std::clamp(1.0-pra*0.04,0.3,1.0); y = 0.0; break; }
        case Rapids: { double rp = d*0.8; x = d*kSpacing+std::sin(rp)*0.25; z = -absd*0.06+std::cos(rp)*0.08; angle = std::sin(rp)*30.0; scale = 1.0/(1.0+absd*0.06); focus = std::clamp(1.0-absd*0.05,0.3,1.0); y = std::abs(std::sin(rp))*0.15; break; }
        case Reef: { int rf = i%6; double rfA = (double)rf*1.047; double rfd = (double)(i/6)-m_offset; double rfa = std::abs(rfd); x = std::sin(rfA)*1.2; z = std::cos(rfA)*1.2-1.2-rfa*0.08; angle = -rfA*25.0; scale = 1.0/(1.0+rfa*0.08); focus = std::clamp(1.0-rfa*0.06,0.3,1.0); y = (rf%2==0?0.1:-0.1); break; }
        case Ridge: { double rg = absd*0.3; x = d*kSpacing; z = -rg*0.1; angle = d*10.0; scale = 1.0/(1.0+rg*0.08); focus = std::clamp(1.0-rg*0.06,0.3,1.0); y = std::sin(rg*M_PI)*0.15; break; }
        case River: { double rv = d*0.4; x = d*kSpacing+std::sin(rv)*0.2; z = -absd*0.06; angle = std::sin(rv)*15.0; scale = 1.0/(1.0+absd*0.05); focus = std::clamp(1.0-absd*0.04,0.3,1.0); y = 0.0; break; }
        case Sand: { double sn = d*0.3; x = d*kSpacing; z = -absd*0.04; angle = 0.0; scale = 1.0-absd*0.03; focus = std::clamp(1.0-absd*0.04,0.3,1.0); y = std::sin(sn)*0.05; break; }
        case Shore: { double sh = d*0.5; x = d*kSpacing+std::sin(sh)*0.15; z = -absd*0.04+std::cos(sh)*0.06; angle = std::sin(sh)*10.0; scale = 1.0-absd*0.03; focus = std::clamp(1.0-absd*0.04,0.3,1.0); y = std::cos(sh*1.5)*0.08; break; }
        case Valley: { double vl = absd*0.3; x = d*kSpacing; z = -vl*0.12; angle = 0.0; scale = 1.0-vl*0.06; focus = std::clamp(1.0-vl*0.06,0.3,1.0); y = -vl*vl*0.3; break; }
        case Vine: { double vn = d*0.4; x = d*kSpacing+std::sin(vn)*0.15; z = -absd*0.06; angle = std::sin(vn*2.0)*20.0; scale = 1.0/(1.0+absd*0.05); focus = std::clamp(1.0-absd*0.04,0.3,1.0); y = -absd*0.12; break; }
        case Volcano: { double vc = absd*0.4; double vcH = 0.5-vc*0.3; x = d*kSpacing; z = -vc*0.2; angle = 0.0; scale = 1.0-vc*0.12; focus = std::clamp(1.0-vc*0.1,0.2,1.0); y = vcH; break; }
        case Water2D: { double wt = d*0.7; x = d*kSpacing+std::sin(wt)*0.2; z = -absd*0.04+std::cos(wt*1.3)*0.06; angle = std::sin(wt)*12.0; scale = 1.0/(1.0+absd*0.04); focus = std::clamp(1.0-absd*0.04,0.3,1.0); y = std::sin(wt*2.0)*0.1; break; }
        case Woods: { int wd = i%3-1; double wdd = (double)(i/3)-m_offset; double wda = std::abs(wdd); x = (double)wd*1.0; z = -wda*0.1; angle = (double)wd*12.0; scale = 1.0/(1.0+wda*0.08); focus = std::clamp(1.0-wda*0.06,0.3,1.0); y = 0.0; break; }
        case Abyss: { double ab = absd*0.5; x = d*kSpacing; z = -ab*0.3; angle = 0.0; scale = 1.0/(1.0+ab*0.2); focus = std::clamp(1.0-ab*0.15,0.2,1.0); y = -ab*0.3; alpha = std::clamp(1.0-ab*0.2,0.0,1.0); break; }
        case Anchor2D: { int an = i%2; double anc = (double)(i/2)-m_offset; double ana = std::abs(anc); x = (an==0?-0.5:0.5); z = -ana*0.1; angle = (an==0?-15.0:15.0); scale = 1.0/(1.0+ana*0.1); focus = std::clamp(1.0-ana*0.08,0.3,1.0); y = (an==0?0.2:-0.2); break; }
        case Barrel: { double br2 = d*0.3; double brR = 0.5+absd*0.05; x = std::sin(br2)*brR*2.0; z = std::cos(br2)*brR*2.0-brR*2.0-absd*0.06; angle = -d*15.0; scale = 1.0/(1.0+absd*0.06); focus = std::clamp(1.0-absd*0.05,0.3,1.0); y = 0.0; break; }
        case Basket: { int bs2 = i%6; double bsA = (double)bs2*1.047; double bsd = (double)(i/6)-m_offset; double bsa = std::abs(bsd); x = std::sin(bsA)*1.3; z = std::cos(bsA)*1.3-1.3-bsa*0.08; angle = -bsA*30.0; scale = 1.0/(1.0+bsa*0.08); focus = std::clamp(1.0-bsa*0.06,0.3,1.0); y = (bs2%2==0?0.15:-0.15); break; }
        case Bell: { double be = d*0.6; x = d*kSpacing; z = -absd*0.08; angle = std::sin(be)*15.0; scale = 1.0/(1.0+absd*0.08); focus = std::clamp(1.0-absd*0.06,0.3,1.0); y = std::abs(std::sin(be))*0.2; break; }
        case Blade: { int bl2 = i%2; double bld2 = (double)(i/2)-m_offset; double bla2 = std::abs(bld2); x = (bl2==0?-1.0:1.0); z = -bla2*0.08; angle = (bl2==0?-15.0:15.0); scale = 1.0/(1.0+bla2*0.08); focus = std::clamp(1.0-bla2*0.06,0.3,1.0); y = 0.0; break; }
        case Block: { int bk = i%4-1; double bkd = (double)(i/4)-m_offset; double bka = std::abs(bkd); x = (double)bk*0.7; z = -bka*0.1; angle = 0.0; scale = 1.0-bka*0.05; focus = std::clamp(1.0-bka*0.05,0.3,1.0); y = 0.0; break; }
        case Board: { int bd2 = i%3-1; double bdd2 = (double)(i/3)-m_offset; double bda2 = std::abs(bdd2); x = (double)bd2*0.9; z = -bda2*0.06; angle = (double)bd2*8.0; scale = 1.0-bda2*0.04; focus = std::clamp(1.0-bda2*0.04,0.3,1.0); y = 0.0; break; }
        case Boat: { double bt = d*0.4; x = d*kSpacing+std::sin(bt)*0.15; z = -absd*0.08; angle = std::sin(bt)*15.0; scale = 1.0/(1.0+absd*0.06); focus = std::clamp(1.0-absd*0.05,0.3,1.0); y = std::abs(std::sin(bt))*0.1; break; }
        case Brick: { int br3 = i%3-1; double brd = (double)(i/3)-m_offset; double bra = std::abs(brd); double brOff = ((i/3)%2==0)?0.0:0.4; x = (double)br3*0.8+brOff; z = -bra*0.1; angle = 0.0; scale = 1.0/(1.0+bra*0.08); focus = std::clamp(1.0-bra*0.06,0.3,1.0); y = 0.0; break; }
        case Castle: { int ca = i%5-2; double cad = (double)(i/5)-m_offset; double caa = std::abs(cad); double caH = 0.6-std::abs((double)ca)*0.12; x = (double)ca*0.5; z = -caa*0.08; angle = 0.0; scale = caH/(1.0+caa*0.06); focus = std::clamp(caH-caa*0.04,0.3,1.0); y = caH-0.2; break; }
        case Chair: { int ch = i%2; double chd = (double)(i/2)-m_offset; double cha = std::abs(chd); x = (ch==0?-0.5:0.5); z = -cha*0.08; angle = (ch==0?-10.0:10.0); scale = 1.0-cha*0.05; focus = std::clamp(1.0-cha*0.05,0.3,1.0); y = 0.0; break; }
        case Chest: { int cht = i%2; double chtd = (double)(i/2)-m_offset; double chta = std::abs(chtd); x = (cht==0?-0.6:0.6); z = -chta*0.08; angle = 0.0; scale = 1.0-chta*0.05; focus = std::clamp(1.0-chta*0.05,0.3,1.0); y = 0.0; break; }
        case Crown2D: { int cr2 = i%5-2; double crd = (double)(i/5)-m_offset; double cra = std::abs(crd); double crH = 0.5-std::abs((double)cr2)*0.15; x = (double)cr2*0.5; z = -cra*0.08; angle = (double)cr2*10.0; scale = crH/(1.0+cra*0.06); focus = std::clamp(crH-cra*0.04,0.3,1.0); y = crH-0.15; break; }
        case Disc: { double dc = d*0.4; double dcR = 0.3+absd*0.12; x = std::sin(dc)*dcR*2.5; z = std::cos(dc)*dcR*2.5-dcR*2.5-absd*0.04; angle = -d*20.0; scale = 1.0/(1.0+absd*0.06); focus = std::clamp(1.0-absd*0.05,0.3,1.0); y = 0.0; break; }
        case Dome2D: { double dm2 = absd*0.3; double dm2H = std::cos(dm2*1.5)*0.4; x = d*kSpacing; z = -dm2*0.2; angle = 0.0; scale = 1.0-dm2*0.1; focus = std::clamp(1.0-dm2*0.08,0.3,1.0); y = dm2H; break; }
        case Drum2D: { double dr2 = d*0.3; double drR = 0.5+absd*0.05; x = std::sin(dr2)*drR*2.0; z = std::cos(dr2)*drR*2.0-drR*2.0-absd*0.05; angle = -d*15.0; scale = 1.0/(1.0+absd*0.05); focus = std::clamp(1.0-absd*0.04,0.3,1.0); y = 0.0; break; }
        case Flag2D: { double fl2 = d*0.5; x = d*kSpacing; z = -absd*0.06; angle = std::sin(fl2*2.0)*25.0; scale = 1.0/(1.0+absd*0.05); focus = std::clamp(1.0-absd*0.04,0.3,1.0); y = 0.2+std::abs(std::sin(fl2))*0.1; break; }
        case Frame: { int fr = i%4-1; double frd = (double)(i/4)-m_offset; double fra = std::abs(frd); x = (double)fr*0.8; z = -fra*0.08; angle = (double)fr*10.0; scale = 1.0/(1.0+fra*0.06); focus = std::clamp(1.0-fra*0.05,0.3,1.0); y = (fr%2==0?0.05:-0.05); break; }
        case Gate: { int ga = i%2; double gad = (double)(i/2)-m_offset; double gaa = std::abs(gad); x = (ga==0?-0.7:0.7); z = -gaa*0.08; angle = (ga==0?-20.0:20.0); scale = 1.0-gaa*0.05; focus = std::clamp(1.0-gaa*0.05,0.3,1.0); y = 0.0; break; }
        case Gem: { int gm = i%5-2; double gmd = (double)(i/5)-m_offset; double gma = std::abs(gmd); double gH = 0.5-std::abs((double)gm)*0.1; x = (double)gm*0.4; z = -gma*0.08; angle = (double)gm*15.0; scale = gH/(1.0+gma*0.06); focus = std::clamp(gH-gma*0.04,0.3,1.0); y = gH-0.2; break; }
        case Globe2D: { double gb = d*0.3; double gbR = 0.5+absd*0.04; x = std::sin(gb)*gbR*2.0; z = std::cos(gb)*gbR*2.0-gbR*2.0-absd*0.04; angle = -d*15.0; scale = 1.0/(1.0+absd*0.05); focus = std::clamp(1.0-absd*0.04,0.3,1.0); y = std::cos(gb)*0.15; break; }
        case Goblet: { int gb2 = i%2; double gbd = (double)(i/2)-m_offset; double gba = std::abs(gbd); x = (gb2==0?-0.4:0.4); z = -gba*0.1; angle = (gb2==0?-12.0:12.0); scale = 1.0/(1.0+gba*0.08); focus = std::clamp(1.0-gba*0.06,0.3,1.0); y = (gb2==0?0.2:-0.1); break; }
        case Hat: { int ht = i%3-1; double htd = (double)(i/3)-m_offset; double hta = std::abs(htd); x = (double)ht*0.6; z = -hta*0.08; angle = (double)ht*15.0; scale = 1.0/(1.0+hta*0.06); focus = std::clamp(1.0-hta*0.05,0.3,1.0); y = (ht==0?0.25:-0.05); break; }
        case Helmet: { int hm = i%3-1; double hmd = (double)(i/3)-m_offset; double hma = std::abs(hmd); x = (double)hm*0.7; z = -hma*0.08; angle = (double)hm*12.0; scale = 1.0/(1.0+hma*0.06); focus = std::clamp(1.0-hma*0.05,0.3,1.0); y = (hm==0?0.2:0.0); break; }
        case Lamp: { double lp = d*0.5; x = d*kSpacing; z = -absd*0.06; angle = std::sin(lp)*10.0; scale = 1.0/(1.0+absd*0.05); focus = std::clamp(1.0-absd*0.04,0.3,1.0); y = 0.3+std::abs(std::sin(lp))*0.1; break; }
        case Shield2D: { int sh2 = i%5-2; double shd = (double)(i/5)-m_offset; double sha = std::abs(shd); double shC = 1.0-std::abs((double)sh2)*0.2; x = (double)sh2*0.4; z = -sha*0.08; angle = (double)sh2*12.0; scale = shC/(1.0+sha*0.06); focus = std::clamp(shC-sha*0.04,0.3,1.0); y = shC*0.3-0.15; break; }
        case Throne: { int th = i%2; double thd = (double)(i/2)-m_offset; double tha = std::abs(thd); x = (th==0?-0.6:0.6); z = -tha*0.08; angle = (th==0?-8.0:8.0); scale = 1.0-tha*0.04; focus = std::clamp(1.0-tha*0.04,0.3,1.0); y = 0.3; break; }
        case Trophy: { int tr = i%2; double trd = (double)(i/2)-m_offset; double tra = std::abs(trd); x = (tr==0?-0.4:0.4); z = -tra*0.08; angle = (tr==0?-10.0:10.0); scale = 1.0/(1.0+tra*0.06); focus = std::clamp(1.0-tra*0.05,0.3,1.0); y = 0.2; break; }
        case Wheel2D: { double wh2 = d*0.4; double whR = 0.3+absd*0.1; x = std::cos(wh2)*whR*2.5; z = std::sin(wh2)*whR*2.5-whR*2.5-absd*0.05; angle = d*25.0; scale = 1.0/(1.0+absd*0.08); focus = std::clamp(1.0-absd*0.06,0.3,1.0); y = 0.0; break; }
        case Beacon: { double bc = d*0.6; x = d*kSpacing; z = -absd*0.06; angle = std::sin(bc)*20.0; scale = 1.0/(1.0+absd*0.05); focus = std::clamp(1.0-absd*0.04,0.3,1.0); y = std::abs(std::sin(bc))*0.3; alpha = std::clamp(0.5+std::sin(bc)*0.5,0.0,1.0); break; }
        case Canvas: { int cn2 = i%3-1; double cnd2 = (double)(i/3)-m_offset; double cna2 = std::abs(cnd2); x = (double)cn2*0.9; z = -cna2*0.06; angle = 0.0; scale = 1.0-cna2*0.04; focus = std::clamp(1.0-cna2*0.04,0.3,1.0); y = 0.0; break; }
        case Chasm: { double ch2 = absd*0.4; x = d*kSpacing; z = -ch2*0.2; angle = 0.0; scale = 1.0/(1.0+ch2*0.15); focus = std::clamp(1.0-ch2*0.12,0.2,1.0); y = -ch2*ch2*0.3; break; }
        case Core: { double co = d*0.4; double coR = 0.1+absd*0.1; x = std::cos(co)*coR*2.5; z = std::sin(co)*coR*2.5-coR*2.5-absd*0.08; angle = d*35.0; scale = 1.0/(1.0+absd*0.15); focus = std::clamp(1.0-absd*0.1,0.3,1.0); y = 0.0; break; }
        case Depth: { double dp2 = absd*0.5; x = d*kSpacing; z = -dp2*0.25; angle = 0.0; scale = 1.0/(1.0+dp2*0.2); focus = std::clamp(1.0-dp2*0.15,0.2,1.0); alpha = std::clamp(1.0-dp2*0.15,0.0,1.0); break; }
        case Edge: { int ed = i%2; double edd = (double)(i/2)-m_offset; double eda = std::abs(edd); x = (ed==0?-1.2:1.2); z = -eda*0.1; angle = (ed==0?-20.0:20.0); scale = 1.0/(1.0+eda*0.08); focus = std::clamp(1.0-eda*0.06,0.3,1.0); y = 0.0; break; }
        case Field: { int fl = i%4-1; double fld = (double)(i/4)-m_offset; double fla = std::abs(fld); x = (double)fl*0.9; z = -fla*0.06; angle = 0.0; scale = 1.0-fla*0.04; focus = std::clamp(1.0-fla*0.04,0.3,1.0); y = 0.0; break; }
        case Haze: { double hz = d*0.3; x = d*kSpacing; z = -absd*0.04; angle = std::sin(hz)*8.0; scale = 1.0-absd*0.03; focus = std::clamp(1.0-absd*0.03,0.2,1.0); y = 0.0; alpha = std::clamp(0.5+(1.0-absd*0.04)*0.5,0.0,1.0); break; }
        case Mist: { double ms = d*0.4; x = d*kSpacing+std::sin(ms)*0.1; z = -absd*0.04; angle = std::sin(ms)*5.0; scale = 1.0-absd*0.02; focus = std::clamp(1.0-absd*0.03,0.2,1.0); y = std::cos(ms)*0.05; alpha = std::clamp(0.3+(1.0-absd*0.03)*0.7,0.0,1.0); break; }
        case Smoke: { double sm = d*0.5; x = d*kSpacing+std::sin(sm)*0.2; z = -absd*0.04+std::cos(sm)*0.05; angle = std::sin(sm*2.0)*15.0; scale = 1.0-absd*0.02+std::sin(sm)*0.05; focus = std::clamp(1.0-absd*0.03,0.2,1.0); y = std::abs(std::sin(sm))*0.15; alpha = std::clamp(0.4+(1.0-absd*0.03)*0.6-0.2*std::abs(std::sin(sm)),0.0,1.0); break; }
        case Echo2D: { int ec = i%2; double ecd = (double)(i/2)-m_offset; double eca = std::abs(ecd); x = (ec==0?-0.3:0.3); z = -eca*0.1+(ec==0?0.05:-0.05); angle = (ec==0?-15.0:15.0); scale = 1.0/(1.0+eca*0.08); focus = std::clamp(1.0-eca*0.06,0.2,1.0); y = (ec==0?0.1:-0.1); alpha = std::clamp(1.0-(double)ec*0.3-eca*0.04,0.0,1.0); break; }
        default: // Coverflow
        {
            x     = d * kSpacing;
            {
                const double sign = (d >= 0) ? -1.0 : 1.0;
                double raw = sign * kMaxAngleDeg;
                angle = (absd < 0.5) ? raw * (absd / 0.5) : raw;
            }
            z     = -absd * kZStep;
            scale = 1.0 / (1.0 + absd * kScaleFalloff);
            focus = std::clamp(1.0 - absd / kVisibleSpan, 0.0, 1.0);
            break;
        }
        }

        glPushMatrix();
        glTranslatef((float)x, (float)y, (float)z);
        glRotatef((float)-angle, 0.0f, 1.0f, 0.0f);
        glRotatef((float)angleX, 1.0f, 0.0f, 0.0f);
        glScalef((float)scale, (float)scale, (float)scale);

        wxColour a = item.accent;
        float glow = (float)(0.15 + 0.25 * focus);
        glDisable(GL_TEXTURE_2D);

        glBegin(GL_QUADS);
            glColor4f(0.11f, 0.12f, 0.14f, (float)(0.95 * alpha));
            glVertex3f(-cardW/2, -cardH/2, 0.0f);
            glVertex3f( cardW/2, -cardH/2, 0.0f);
            glVertex3f( cardW/2,  cardH/2, 0.0f);
            glVertex3f(-cardW/2,  cardH/2, 0.0f);
        glEnd();

        glLineWidth((float)(1.0 + 2.0 * focus));
        glBegin(GL_LINE_LOOP);
            glColor4f(a.Red()/255.0f, a.Green()/255.0f, a.Blue()/255.0f, (float)((glow + 0.5f) * alpha));
            glVertex3f(-cardW/2, -cardH/2, 0.001f);
            glVertex3f( cardW/2, -cardH/2, 0.001f);
            glVertex3f( cardW/2,  cardH/2, 0.001f);
            glVertex3f(-cardW/2,  cardH/2, 0.001f);
        glEnd();

        if (item.texture)
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, item.texture);
            glColor4f(1.0f, 1.0f, 1.0f, (float)alpha);
            glBegin(GL_QUADS);
                glTexCoord2f(0, 1); glVertex3f(-cardW/2, -cardH/2, 0.002f);
                glTexCoord2f(1, 1); glVertex3f( cardW/2, -cardH/2, 0.002f);
                glTexCoord2f(1, 0); glVertex3f( cardW/2,  cardH/2, 0.002f);
                glTexCoord2f(0, 0); glVertex3f(-cardW/2,  cardH/2, 0.002f);
            glEnd();
        }

        glPopMatrix();
    }
};
