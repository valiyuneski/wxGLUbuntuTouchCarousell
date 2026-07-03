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
            Refresh(false);
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

        double x     = d * kSpacing;
        double angle = std::clamp(d * 22.0, -kMaxAngleDeg, kMaxAngleDeg);
        double z     = -std::abs(d) * kZStep;
        double scale = 1.0 / (1.0 + std::abs(d) * kScaleFalloff);
        double focus = std::clamp(1.0 - std::abs(d) / kVisibleSpan, 0.0, 1.0);

        const float cardW = 1.6f, cardH = 1.0f;

        glPushMatrix();
        glTranslatef((float)x, 0.0f, (float)z);
        glRotatef((float)-angle, 0.0f, 1.0f, 0.0f);
        glScalef((float)scale, (float)scale, (float)scale);

        // Card plate.
        wxColour a = item.accent;
        float glow = (float)(0.15 + 0.25 * focus);
        glDisable(GL_TEXTURE_2D);

        glBegin(GL_QUADS);
            glColor4f(0.11f, 0.12f, 0.14f, 0.95f);
            glVertex3f(-cardW/2, -cardH/2, 0.0f);
            glVertex3f( cardW/2, -cardH/2, 0.0f);
            glVertex3f( cardW/2,  cardH/2, 0.0f);
            glVertex3f(-cardW/2,  cardH/2, 0.0f);
        glEnd();

        glLineWidth((float)(1.0 + 2.0 * focus));
        glBegin(GL_LINE_LOOP);
            glColor4f(a.Red()/255.0f, a.Green()/255.0f, a.Blue()/255.0f, glow + 0.5f);
            glVertex3f(-cardW/2, -cardH/2, 0.001f);
            glVertex3f( cardW/2, -cardH/2, 0.001f);
            glVertex3f( cardW/2,  cardH/2, 0.001f);
            glVertex3f(-cardW/2,  cardH/2, 0.001f);
        glEnd();

        // Label texture.
        if (item.texture)
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, item.texture);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
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
