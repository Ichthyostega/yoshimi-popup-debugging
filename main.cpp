#include <iostream>
#include <math.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Tooltip.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Dial.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Menu_Window.H>
#include <string>

template<class T>
inline T limit(T val, T min, T max)
{
    return val < min ? min : (val > max ? max : val);
}

namespace {
    template<unsigned int base, bool fraction=false>
    struct PowerFunction
    {
        static_assert(base > 0, "0^x is always zero");

        static float invoke(float exponent)
        {
            return expf(LN_BASE * exponent);
        }
        static const float LN_BASE;
    };

    template<unsigned int base, bool fraction>
    const float PowerFunction<base,fraction>::LN_BASE = log(fraction? 1.0/base : double(base));
}

/* compute base^exponent for a fixed integral base */
template<unsigned int base>
inline float power(float exponent)
{
    return PowerFunction<base>::invoke(exponent);
}

const int tooltip_grid = 146;
const int tooltip_faint_text = 67;
const int tooltip_text = 66;
const int tooltip_curve = 177;
const int tooltip_major_grid = 105;
const int knob_ring = 144;
const int knob_low = 244;
const int knob_high = 207;
const int knob_lit = 199;
const int knob_point = 145;

Fl_Window* other_window;

class DynTooltip : private Fl_Menu_Window {
 public:
  DynTooltip();
  ~DynTooltip();

  void dynshow(float timeout=0);

  void setValue(float);
  void setOnlyValue(bool onlyval);

  void tipHandle(int event);

  void draw() override;

 private:
  void reposition();
  void update() {
    // Intentionally left empty
    size(286, 200);
    redraw();
  }

  float currentValue;
  bool onlyValue;
  bool positioned;
};

// Whether or not a dynamic tooltip was shown recently
static bool _recent;

/* Delayed display of tooltip - callbackk*/
static void delayedShow(void* dyntip){
    if (DynTooltip* tip = (DynTooltip*) dyntip)
       tip->dynshow(0);
}

static void resetRecent(void*){
    _recent = false;
}

DynTooltip::DynTooltip():Fl_Menu_Window(1,1)
{
    onlyValue = false;
    positioned = false;

    set_override(); // place window on top
    end();
    hide();
}

DynTooltip::~DynTooltip(){
    Fl::remove_timeout(delayedShow);
    Fl::remove_timeout(resetRecent);
}

inline void DynTooltip::reposition()
{
    if (!positioned)
    {
        const int cursor_y = Fl::event_y_root();

        // Check if tooltip would extend below screen
        // (in practice it is more lenient, but better to be safe than to have an invisible tooltip)
        if (cursor_y + 20 + h() > Fl::h())
        {
            position(Fl::event_x_root(), cursor_y - 20 - h());
        }
        else
        {
            position(Fl::event_x_root(), cursor_y + 20);
        }
        positioned = true;
    }
}

void DynTooltip::dynshow(float timeout)
{
    if (timeout <= 0){
        Fl::remove_timeout(delayedShow, this);
        _recent = true;
        reposition();
        update();
        Fl_Menu_Window::show();
    }
    else
    {
        Fl::add_timeout(timeout, delayedShow, this);
    }
}

void DynTooltip::setValue(float val)
{
    if (val != currentValue)
    {
        currentValue = val;
        if (positioned)
            update();
    }
}

void DynTooltip::setOnlyValue(bool onlyval)
{
    if (onlyValue != onlyval)
    {
        onlyValue = onlyval;
        if (positioned)
            update();
    }
}

void DynTooltip::tipHandle(int event)
{
    switch(event)
    {
    case FL_ENTER:
        Fl::remove_timeout(resetRecent);
        setOnlyValue(false);
        dynshow(_recent ? Fl_Tooltip::hoverdelay() : Fl_Tooltip::delay());
        break;
    case FL_PUSH:
    case FL_DRAG:
    case FL_MOUSEWHEEL:
        Fl::remove_timeout(delayedShow);
        Fl::remove_timeout(resetRecent);
        setOnlyValue(true);
        dynshow(0);
        break;
    case FL_LEAVE:
    case FL_RELEASE:
    case FL_HIDE:
        Fl::remove_timeout(delayedShow);
        Fl::add_timeout(Fl_Tooltip::hoverdelay(),resetRecent);
        hide();
        break;
    }
}

void custom_graphics(float val,int W,int H)
{
    int x0,y0,i;
    int _w = 256, _h = 128;
    float x,y,p;
    x0 = W / 2 - (_w / 2);
    y0 = H;

    {
        /* The scale centers around the factor 1 vertically
           and is logarithmic in both dimensions. */

        int margin = 28;
        _h -= margin;
        _w -= margin * 2;
        x0 += margin * 1.25;
        y0 -= margin * 0.75;

        float cy = y0 - _h / 2;

        int j = 1;
        const float lg1020 = log10(20); /* Lower bound = 20hz*/
        const float rx = _w / (log10(20000) - lg1020); /* log. width ratio */
        const float ry = (_h / 2) / log10(100000);

        std::string hzMarkers[] = {"20", "100", "1k", "10k"};
        std::string xMarkers[] = {"x10","x100","x1k","x10k","10%","1%","0.1%","0.01%"};

        /* Scale lines */

        fl_font(fl_font(),8);
        for (i = 0; i < 4; i++) /* 10x / 10%, 100x / 1% ... */
        {
            y = ry * (i + 1);
            fl_color(tooltip_grid);
            fl_line(x0, cy - y, x0 + _w, cy - y);
            fl_line(x0, cy + y, x0 + _w, cy + y);
            fl_color(tooltip_faint_text);
            fl_draw(xMarkers[i].c_str(), x0 - 28, (cy - y - 4), 24, 12,
                    Fl_Align(FL_ALIGN_RIGHT));
            fl_draw(xMarkers[i + 4].c_str(), x0 - 28, (cy + y - 4), 24, 12,
                    Fl_Align(FL_ALIGN_RIGHT));
        }

        /* Hz lines */

        fl_color(tooltip_grid); /* Lighter inner lines*/

        for (i = 10;i != 0; i *= 10)
        {
            for (j = 2; j < 10; j++)
            {
                x = x0 + rx * (log10(i * j) - lg1020) + 1;
                fl_line(x, y0, x, y0 - _h);
                if (i * j >= 20000)
                {
                    i = 0;
                    break;
                }
            }
        }

        fl_font(fl_font(),10);
        for (i = 0; i < 4; i++) /* 20, 100, 1k, 10k */
        {
            x = x0 + (i == 0 ?  0 : ((float)i + 1 - lg1020) * rx);
            fl_color(tooltip_major_grid); /* Darker boundary lines */
            fl_line(x, y0, x, y0 - _h);
            fl_color(tooltip_text);
            fl_draw(hzMarkers[i].c_str(), x - 20, y0 + 4, 40, 12,
                    Fl_Align(FL_ALIGN_CENTER));
        }
        /* Unit marker at the lower right of the graph */
        fl_draw("Hz", x0 + _w, y0 + 4, 20, 12, Fl_Align(FL_ALIGN_LEFT));

        /* Vertical center line */
        fl_color(38);
        fl_line(x0 - margin, cy, x0 + _w, cy);

        /* Function curve */
        fl_color(tooltip_curve);
        if ((int)val == 0)
        {
            fl_line(x0, cy, x0 + _w, cy);
        }
        else
        {
            const float p = ((int)val / 64.0f) * 3.0;

std::cout<<"Popup-draw: cairo_make_current(other_window)"<<std::endl;
            Fl::cairo_make_current(other_window);
std::cout<<"Popup-draw: cairo_make_current(Fl_Window::current()))"<<std::endl;

            /* Cairo not necessary, but makes it easier to read the graph */
#ifndef YOSHIMI_CAIRO_LEGACY
            cairo_t *cr = Fl::cairo_make_current(Fl_Window::current());

#else
            // Legacy solution : retrieve drawing surface from XServer
            cairo_surface_t* Xsurface = cairo_xlib_surface_create
                (fl_display, fl_window, fl_visual->visual,
                 Fl_Window::current()->w(), Fl_Window::current()->h());
            cairo_t *cr = cairo_create (Xsurface);
#endif
            cairo_save(cr);
            cairo_set_source_rgb(cr, 1, 0, 0);
            cairo_set_line_width(cr, 1.5);
            cairo_move_to(cr, x0, cy - ry * log10(power<50>(p)));
            cairo_line_to(cr, x0 + _w, cy - ry * log10(powf(0.05, p)));
            cairo_stroke(cr);
            cairo_restore(cr);
#ifndef YOSHIMI_CAIRO_LEGACY
            Fl::cairo_flush(cr);
#else
            cairo_surface_destroy(Xsurface);
            cairo_destroy(cr);
#endif
        }
    }
}

void DynTooltip::draw()
{

    const int mw = 3;//Fl_Tooltip::margin_width();
    const int mh = 3;//Fl_Tooltip::margin_height();

    int x = mw, y = mh;
    int _w = w() - mw * 2;

    draw_box(FL_BORDER_BOX, 0, 0, w(), h(), Fl_Tooltip::color());
    fl_color(Fl_Tooltip::textcolor());
    fl_font(Fl_Tooltip::font(), Fl_Tooltip::size());

    /* Draw tooltip text */
    if (!onlyValue)
    {
        auto tipTextH = 40;
        fl_draw("foobar", x, y, _w, tipTextH, FL_ALIGN_CENTER);
        y += tipTextH;
    }

    /* Draw formatted tooltip value */
    auto valTextH = 10;
    fl_draw("valuetext", x, y, _w, valTextH,
            Fl_Align(FL_ALIGN_CENTER | FL_ALIGN_WRAP));

    /* Draw additional graphics */
    if (true)
        custom_graphics(/*graphicsType,*/ currentValue, w(), h() - mh);
}

class WidgetPDial : public Fl_Dial {
public:
  WidgetPDial(int x,int y, int w, int h, const char *label=0);
  ~WidgetPDial();

  int handle(int event) override;

  void draw() override;

void value(double val) /* override */
{
    Fl_Valuator::value(val);
    dyntip->setValue(val);
    dyntip->setOnlyValue(true);
}

double value()
{
    return Fl_Valuator::value();
}

private:
  DynTooltip *dyntip;
  double oldvalue;
};

WidgetPDial::WidgetPDial(int x,int y, int w, int h, const char *label) : Fl_Dial(x,y,w,h,label)
{
    Fl_Group *save = Fl_Group::current();
    dyntip = new DynTooltip();
    Fl_Group::current(save);
    maximum(127);
}

WidgetPDial::~WidgetPDial()
{
    delete dyntip;
}

int WidgetPDial::handle(int event)
{
    double dragsize, v, min = minimum(), max = maximum();
    int my, mx;
    int res = 0;

    switch (event)
    {
    case FL_PUSH:
    case FL_DRAG: // done this way to suppress warnings
        if (event == FL_PUSH)
        {
            Fl::belowmouse(this); /* Ensures other widgets receive FL_RELEASE */
            do_callback();
            oldvalue = value();
        }
        my = -((Fl::event_y() - y()) * 2 - h());
        mx = ((Fl::event_x() - x()) * 2 - w());
        my = (my + mx);

        dragsize = 200.0;
        if (Fl::event_state(FL_CTRL) != 0)
            dragsize *= 10;
        else if (Fl::event_button() == 2)
            dragsize *= 3;
        if (Fl::event_button() != 3)
        {
            v = oldvalue + my / dragsize * (max - min);
            v = clamp(v);
            value(v);
            value_damage();
            if (this->when() != 0)
                do_callback();
        }
        res = 1;
        break;
    case FL_MOUSEWHEEL:
        if (!Fl::event_inside(this))
        {
            return 1;
        }
        my = - Fl::event_dy();
        dragsize = 25.0f;
        if (Fl::event_state(FL_CTRL) != 0)
            dragsize *= 5; // halved this for better fine resolution
        value(limit(value() + my / dragsize * (max - min), min, max));
        value_damage();
        if (this->when() != 0)
            do_callback();
        res = 1;
        break;
    case FL_ENTER:
        res = 1;
        break;
    case FL_HIDE:
    case FL_UNFOCUS:
        break;
    case FL_LEAVE:
        res = 1;
        break;
    case FL_RELEASE:
        if (this->when() == 0)
            do_callback();
        res = 1;
        break;
    }

    dyntip->setValue(value());
    dyntip->tipHandle(event);
    return res;
}

void WidgetPDial::draw()
{
    float scale = 1.0f;
    int cx = x() * scale, cy = y() * scale, sx = w() * scale, sy = h() * scale;
    double d = (sx>sy)?sy:sx; // d = the smallest side -2
    double dh = d/2;
       /*
        * doing away with the fixed outer band. It's out of place now!
        * fl_color(170,170,170);
        * fl_pie(cx - 2, cy - 2, d + 4, d + 4, 0, 360);
        */
    double val = (value() - minimum()) / (maximum() - minimum());
#ifndef YOSHIMI_CAIRO_LEGACY
std::cout<<"Widget-draw: cairo_make_current(other_window)"<<std::endl;
    Fl::cairo_make_current(other_window);
std::cout<<"Widget-draw: cairo_make_current(window())"<<std::endl;
    cairo_t* cr = Fl::cairo_make_current(window());
               // works both with Wayland and X11

#else
    // Legacy solution : retrieve drawing surface from XServer
    cairo_surface_t* Xsurface = cairo_xlib_surface_create
        (fl_display, fl_window, fl_visual->visual,Fl_Window::current()->w() * scale,
        Fl_Window::current()->h() * scale);
    cairo_t* cr = cairo_create (Xsurface);
#endif
    cairo_save(cr);
    cairo_translate(cr,cx+dh,cy+dh);
    //relative lengths of the various parts:
    double rCint = 10.5/35;
    double rCout = 13.0/35;
    double rHand = 8.0/35;
    double rGear = 15.0/35;

    unsigned char r,g,b;
    //drawing base dark circle
    Fl::get_color(knob_ring, r, g, b); // 51,51,51
    if (active_r())
    {
       /*
        * cairo_pattern_create_rgb(r/255.0,g/255.0,b/255.0);
        * The above line seems to be wrong and draws black
        * regardless of the selection.
        * The line below works.
        * Will G.
        */
        cairo_set_source_rgb(cr,r/255.0,g/255.0,b/255.0);
    }
    else
    {
        cairo_set_source_rgb(cr,0.4,0.4,0.4);
    }
    cairo_arc(cr,0,0,dh,0,2*M_PI);
    cairo_fill(cr);

    cairo_pattern_t* pat;
    Fl::get_color(knob_low, r, g, b);
    float R1 = r/255.0; // 186
    float G1 = g/255.0; // 198
    float B1 = b/255.0; // 211

    Fl::get_color(knob_high, r, g, b);
    float R2 = r/255.0; // 231
    float G2 = g/255.0; // 235
    float B2 = b/255.0; // 239

    //drawing the inner circle:
    pat = cairo_pattern_create_linear(0.5*dh,0.5*dh,0,-0.5*dh);
    cairo_pattern_add_color_stop_rgba (pat, 0, 0.8*R1, 0.8*G1, 0.8*B1, 1);
    cairo_pattern_add_color_stop_rgba (pat, 1, R2, G2, B2, 1);
    cairo_set_source (cr, pat);
    cairo_arc(cr,0,0,d*rCout,0,2*M_PI);
    cairo_fill(cr);
    //drawing the outer circle:
    pat = cairo_pattern_create_radial(2.0/35*d,6.0/35*d,2.0/35*d,0,0,d*rCint);
    cairo_pattern_add_color_stop_rgba (pat, 0, R2, G2, B2, 1);
    cairo_pattern_add_color_stop_rgba (pat, 1, R1, G1, B1, 1);
    cairo_set_source (cr, pat);
    cairo_arc(cr,0,0,d*rCint,0,2*M_PI);
    cairo_fill(cr);
    //drawing the "light" circle:
    int linewidth = int(2.0f * sx / 30);
    if (linewidth < 2)
        linewidth = 2;
    if (active_r())
    {
        Fl::get_color(knob_lit, r, g, b); // 0, 197, 255
        cairo_set_source_rgb(cr,r/255.0,g/255.0, b/255.0); //light blue
    }
    else
    {
        cairo_set_source_rgb(cr,0.6,0.7,0.8);
    }
    cairo_set_line_width (cr, linewidth);
    cairo_new_sub_path(cr);
    cairo_arc(cr,0,0,d*rGear,0.75*M_PI,+val*1.5*M_PI+0.75*M_PI);
    cairo_stroke(cr);
    //drawing the hand:
    if (active_r())
    {
        if (selection_color() == 8)
            selection_color(knob_point);
        Fl::get_color(selection_color(), r, g, b); // 61, 61, 61
        cairo_set_source_rgb(cr,r/255.0,g/255.0,b/255.0);
    }
    else
    {
        cairo_set_source_rgb(cr,111.0/255,111.0/255,111.0/255);
    }
    cairo_rotate(cr,val*3/2*M_PI+0.25*M_PI);
    cairo_set_line_width (cr, linewidth);
    cairo_move_to(cr,0,0);
    cairo_line_to(cr,0,d*rHand);
    cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
    cairo_stroke (cr);
    //freeing the resources
    cairo_pattern_destroy(pat);
    cairo_restore(cr);
#ifndef YOSHIMI_CAIRO_LEGACY
    // Fltk handles the lifecycle of cr in fltk >= 1.4.0
    Fl::cairo_flush(cr);
#else
    cairo_surface_destroy(Xsurface);
    cairo_destroy(cr);
#endif
}

int main(int argc, char* argv[]) {
  other_window = new Fl_Window(340, 180);
  other_window->end();

  Fl_Window *window = new Fl_Window(340, 180);
  auto *dial = new WidgetPDial(20, 40, 300, 100);
  window->end();

  window->show(argc, argv);
  return Fl::run();
}
