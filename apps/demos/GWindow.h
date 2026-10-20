/**
 *  Copyright 2015 Mike Reed
 */

#ifndef GWindow_DEFINED
#define GWindow_DEFINED

#include <SDL2/SDL.h>
#include <functional>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include "../../include/GBitmap.h"
#include "../../include/GMatrix.h"
#include "../../include/GPoint.h"
#include "../../include/GRect.h"

class GCanvas;
class GClick;
class GIRect;

enum GFastKeys {
    left_arrow =  1 << 0,
    right_arrow = 1 << 1,
    up_arrow =    1 << 2,
    down_arrow =  1 << 3,
    return_key =  1 << 4,
    space_key =   1 << 5,

    ctrl_key  = 1 << 6,
    opt_key   = 1 << 7,
};

class GWindow {
public:
    int run();

    void requestDraw();

protected:
    GWindow(GISize);
    virtual ~GWindow();

    GISize size() const { return fSize; }
    GRect bounds() const { return GRect::WH((float)fSize.width, (float)fSize.height); }

    uint32_t pollFastKeys() const;

    virtual void onUpdate(GCanvas*);
    virtual void onDraw(GCanvas*) {}
    virtual void onResize(GISize) {}
    virtual bool onKeyPress(uint32_t) { return false; }
    virtual void onKeyUp(uint32_t) {}
    virtual void onFastKeys(uint32_t mask) {}
    virtual std::unique_ptr<GClick> onFindClickHandler(GPoint) { return nullptr; }

    void setTitle(const char title[]);

    GMatrix ctm() const { return fCTM; }
    void setCtm(const GMatrix& ctm) {
        if (fCTM != ctm) {
            this->requestDraw();
        }
        fCTM = ctm;
    }

    static void main_loop(GWindow*);

private:
    std::unique_ptr<GClick> fClick;
    
    GISize fSize;
    bool fNeedDraw;
    bool fQuit = false;

    SDL_Window*   fWindow;
    SDL_Renderer* fRenderer;
    SDL_Texture*  fTexture;
    const uint8_t* fKeyboardState;

    uint32_t fInvalEventType;

    GMatrix fCTM;

    bool handleEvent(const SDL_Event&);
    void pushEvent(int code) const;
    void drawIntoTexture();
    void handleResize(int w, int h);
};

class GClick {
public:
    GClick(GPoint, std::function<void(const GClick*)>);
    
    enum State {
        kDown_State,
        kMove_State,
        kUp_State
    };
    
    State state() const { return fState; }
    GPoint curr() const { return fCurr; }
    GPoint prev() const { return fPrev; }
    GPoint orig() const { return fOrig; }

    void callback() { fFunc(this); }

    static std::unique_ptr<GClick> Noop();

private:
    GPoint  fCurr, fPrev, fOrig;
    State   fState;
    std::function<void(const GClick*)> fFunc;

    friend class GWindow;
};

#endif
