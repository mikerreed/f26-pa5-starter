/**
 *  Copyright 2015 Mike Reed
 */

#include "GWindow.h"
#include "../../include/GBitmap.h"
#include "../../include/GCanvas.h"
#include "../../include/GRect.h"
#include <stdio.h>

GClick::GClick(GPoint loc, std::function<void(const GClick*)> func) : fFunc(func) {
    fCurr = fPrev = fOrig = loc;
    fState = kDown_State;
}

std::unique_ptr<GClick> GClick::Noop() {
    return std::make_unique<GClick>(GPoint{0,0}, [](const GClick*){});
}

GWindow::GWindow(GISize sz) {
    fSize = sz;

    uint32_t flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
    fWindow = SDL_CreateWindow("An SDL2 window",
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               fSize.width, fSize.height, flags);
    if (!fWindow) {
        printf("Can't create window: %s\n", SDL_GetError());
        return;
    }

    fRenderer = SDL_CreateRenderer(fWindow, -1, 0);

    fTexture = SDL_CreateTexture(fRenderer, SDL_PIXELFORMAT_ARGB8888,
                                            SDL_TEXTUREACCESS_STREAMING,
                                            fSize.width, fSize.height);

    fInvalEventType = SDL_RegisterEvents(1);
}

GWindow::~GWindow() {}

void GWindow::setTitle(const char title[]) {
    SDL_SetWindowTitle(fWindow, title);
}

void GWindow::pushEvent(int code) const {
    SDL_Event u;
    u.type = fInvalEventType;
    u.user.code = code;
    u.user.data1 = nullptr;
    u.user.data2 = nullptr;
    SDL_PushEvent(&u);
}

void GWindow::requestDraw() {
    if (!fNeedDraw) {
        fNeedDraw = true;
        this->pushEvent(42);
    }
}

bool GWindow::handleEvent(const SDL_Event& evt) {
    const auto inv = fCTM.invert().value_or(GMatrix());
    switch (evt.type) {
        case SDL_WINDOWEVENT:
            switch (evt.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                    this->handleResize(evt.window.data1, evt.window.data2);
                    return true;
            }
            break;
        case SDL_KEYDOWN: {
            unsigned sym = evt.key.keysym.sym;
            if (evt.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) {
                if (sym >= 'a' && sym <= 'z') {
                    sym += 'A' - 'a';
                }
            }
            if (this->onKeyPress(sym)) {
                return true;
            }
        } break;
        case SDL_KEYUP: {
            unsigned sym = evt.key.keysym.sym;
            if (evt.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) {
                if (sym >= 'a' && sym <= 'z') {
                    sym += 'A' - 'a';
                }
            }
            this->onKeyUp(sym);
            return true;
        }
        case SDL_MOUSEBUTTONDOWN:
            // seem to get wacky down events when entering a window on mac, but only when
            // .which is non-zero
            if (evt.button.which) {
                break;
            }
            fClick = this->onFindClickHandler(inv * GPoint{evt.button.x, evt.button.y});
            if (fClick) {
                return true;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (fClick) {
                fClick->fState = GClick::kUp_State;
                fClick->callback();
                this->requestDraw();
                fClick = nullptr;
                return true;
            }
            break;
        case SDL_MOUSEMOTION:
            if (fClick) {
                fClick->fState = GClick::kMove_State;
                fClick->fPrev = fClick->fCurr;
                fClick->fCurr = inv * GPoint{evt.motion.x, evt.motion.y};
                fClick->callback();
                this->requestDraw();
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

void GWindow::onUpdate(GCanvas* canvas) {
    canvas->save();
    canvas->concat(fCTM);
    this->onDraw(canvas);
    canvas->restore();
}

void GWindow::drawIntoTexture() {
    std::shared_ptr<GData> data;

    void* lockedPixels;
    int stride;
    if (SDL_LockTexture(fTexture, nullptr, &lockedPixels, &stride) == 0) {
        data = GData::Unmanaged(lockedPixels, fSize.height * stride);
    } else {
        lockedPixels = nullptr;   // signal that lock failed
        stride = fSize.width * sizeof(GPixel);
        data = GData::Uninitialized(fSize.height * stride);
    }

    this->onUpdate(GCreateCanvas(GBitmap(fSize, stride, data)).get());

    if (lockedPixels) {
        SDL_UnlockTexture(fTexture);
    } else {
        // we couldn't lock, so we need to copy the pixels into the texture
        SDL_UpdateTexture(fTexture, nullptr, data->data(), stride);
    }
}

static uint32_t compute_fast_keys_mask(const uint8_t keymap[]) {
    // these are in GFastKeys order, first in the low-bit
    const SDL_Scancode scancodes[] = {
        SDL_SCANCODE_LEFT,
        SDL_SCANCODE_RIGHT,
        SDL_SCANCODE_UP,
        SDL_SCANCODE_DOWN,
        SDL_SCANCODE_RETURN,
        SDL_SCANCODE_SPACE,
    };
    const int paircodes[] = {
        SDL_SCANCODE_LCTRL, SDL_SCANCODE_RCTRL, GFastKeys::ctrl_key,
        SDL_SCANCODE_LALT, SDL_SCANCODE_RALT,   GFastKeys::opt_key,
    };
    uint32_t shift = 0;
    uint32_t mask = 0;
    for (auto sc : scancodes) {
        assert(keymap[sc] == 0 || keymap[sc] == 1);
        mask |= ((uint32_t)keymap[sc] << shift);
        shift += 1;
    }
    for (size_t i = 0; i < std::size(paircodes); i += 3) {
        if (keymap[paircodes[i+0]] || keymap[paircodes[i+1]]) {
            mask |= paircodes[i+2];
        }
    }
    return mask;
}

uint32_t GWindow::pollFastKeys() const {
    // calling this often helps reduce latency for updating the keyboard state
    SDL_PumpEvents();
    return compute_fast_keys_mask(fKeyboardState);
}

void GWindow::handleResize(int w, int h) {
    if (fSize.width != w || fSize.height != h) {
        SDL_SetWindowSize(fWindow, w, h);
        fSize = {w, h};
        this->onResize(fSize);

        SDL_DestroyTexture(fTexture);
        fTexture = SDL_CreateTexture(fRenderer,
                                     SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     fSize.width, fSize.height);

        fNeedDraw = true;
    }
}

void GWindow::main_loop(GWindow* w) {
#ifdef __EMSCRIPTEN__
    // Check if the canvas was resized by JS
    {
        int cw, ch;
        emscripten_get_canvas_element_size("#canvas", &cw, &ch);
        w->handleResize((int)cw, (int)ch);
    }
#endif

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            w->fQuit = true;
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#endif
            return;
        }
        w->handleEvent(e);
    }

    if (w->fNeedDraw) {
        w->fNeedDraw = false;
        w->drawIntoTexture();

        SDL_RenderCopy(w->fRenderer, w->fTexture, nullptr, nullptr);
        SDL_RenderPresent(w->fRenderer);
    }
}

int GWindow::run() {
    if (!fWindow) {
        return -1;
    }

    int keymapSize;
    fKeyboardState = SDL_GetKeyboardState(&keymapSize);

    this->requestDraw();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg((em_arg_callback_func)GWindow::main_loop, this, 0, 1);
#else
    while (!fQuit) {
        GWindow::main_loop(this);
    }
#endif

    return 0;
}
