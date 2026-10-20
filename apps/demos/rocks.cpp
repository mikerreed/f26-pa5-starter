/**
 *  Copyright 2015 Mike Reed
 */

#include "GWindow.h"
#include "../GTime.h"
#include "../../include/GBitmap.h"
#include "../../include/GCanvas.h"
#include "../../include/GColor.h"
#include "../../include/GRandom.h"
#include "../../include/GRect.h"
#include "../../include/GShader.h"

#include <optional>
#include <vector>

GBitmap gMoonRock;

extern float DrawStr(GCanvas*, GPoint origin, const std::string&, float size, const GColor&);

static void draw_donut(GCanvas* canvas, const GRect& outer, const GRect& inner, GColor c) {
    GPaint paint(c);
    GRect r = {outer.left, outer.top, outer.right, inner.top};
    canvas->drawRect(r, paint);
    r.top = inner.bottom; r.bottom = outer.bottom;
    canvas->drawRect(r, paint);
    r = {outer.left, inner.top, inner.left, inner.bottom};
    canvas->drawRect(r, paint);
    r.left = inner.right; r.right = outer.right;
    canvas->drawRect(r, paint);
}

static void draw_poly_hairlines(GCanvas* canvas, const GPoint pts[], size_t n, bool closed, const GPaint& paint) {
    if (n > 1) {
        for (size_t i = 1; i < n; ++i) {
            canvas->drawLine(pts[i - 1], pts[i], paint);
        }
        canvas->drawLine(pts[n - 1], pts[0], paint);
    }
}

static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

static float cross(GVector a, GVector b) {
    return a.x * b.y - a.y * b.x;
}

static bool contains(const GPoint pts[], size_t count, GPoint p, float radians) {
    p = GMatrix::Rotate(-radians) * p;

    int pos = 0, neg = 0;
    for (size_t i = 0; i < count; ++i) {
        const GPoint a = pts[i],
                     b = pts[(i+1) % count];
        const auto cx = cross(p - a, b - a);
        pos |= cx > 0;
        neg |= cx < 0;
    }
    return pos == 0 || neg == 0;
}

static float wrap_around(float value, float min, float max) {
    if (value > max) {
        value = min + (value - max);
    } else if (value < min) {
        value = max + (value - min);
    }
    return value;
}

static GPoint wrap_around(const GRect& r, GPoint p) {
    return { wrap_around(p.x, r.left, r.right), wrap_around(p.y, r.top, r.bottom) };
}

static GVector normalize(GVector v) {
    const float len = v.length();
    return {v.x/len, v.y/len};
}

static GVector rotate_cw(GVector c) { return  {-c.y,  c.x}; }

static GVector rotate(GVector v, float radians) {
    return GMatrix::Rotate(radians) * v;
}

struct Bullet {
    static constexpr float radius = 2;
    GPoint  m_origin;
    GVector m_direction;
    float   m_timeRemaining;
};

static GRect operator+(const GRect& r, GVector v) {
    return r.offset(v);
}

struct BulletList {
    std::vector<Bullet> m_bullets;

    void add(GPoint origin, GVector dir) {
        constexpr float kLifetime = 0.5;
        m_bullets.push_back({origin, dir, kLifetime});
    }

    void draw(GCanvas* canvas) const {
        const auto r = Bullet::radius;
        GPaint paint(GColor_white);
        const GRect rect = {-r, -r, r, 1};
        for (auto b : m_bullets) {
            canvas->drawRect(rect + b.m_origin, paint);
        }
    }

    void tick(float secs) {
        const float speed = 300;
        for (size_t i = m_bullets.size(); i --> 0;) {
            auto& b = m_bullets[i];
            b.m_origin += b.m_direction * (speed * secs);
            b.m_timeRemaining -= secs;

            if (b.m_timeRemaining <= 0) {
                m_bullets.erase(m_bullets.begin() + i);
            }
        }
    }
};

struct State {
    GRect   m_bounds;
    GRandom m_rand;
    std::shared_ptr<GShader> m_shader;

    float nextF() { return m_rand.nextF(); }
    float nextSF() { return 1 - m_rand.nextF()*2; }

    GRect mirrorBounds() const { return m_bounds.outset(40, 40); }
};

constexpr int MAX_GENERATIONS = 3;

struct Rock {
    State*              m_state;
    std::vector<GPoint> m_poly;
    float               m_radius;
    GPoint              m_origin;
    GVector             m_velocity; // pixels per second
    float               m_spin;     // radians per second
    float               m_rotation;
    int                 m_generation;

    Rock(State* state, GPoint origin, GVector velocity, float spin, float radius, size_t sides)
        : m_state(state)
        , m_radius(radius)
        , m_origin(origin)
        , m_velocity(velocity)
        , m_spin(spin)
        , m_rotation(0)
        , m_generation(1)
    {
        m_poly.resize(sides);
        const float angleGap = 2 * gFloatPI / sides;
        for (size_t i = 0; i < sides; ++i) {
            const float theta = i * angleGap * (1 + m_state->nextSF()/100);
            const float rad = radius * (1 - m_state->nextSF()/80);
            m_poly[i] = GVector{ rad * std::cos(theta), rad * std::sin(theta) };
        }
    }

    void draw(GCanvas* canvas) const {
        GPaint paint(m_state->m_shader);
        if (!paint.peekShader()) {
            paint.setHairline();
            paint.setColor(GColor_white);
        }
        auto dodraw = [&](GVector v) {
            canvas->save();
            canvas->translate(v);
            canvas->rotate(m_rotation);
            if (paint.isHairline()) {
                draw_poly_hairlines(canvas, m_poly.data(), m_poly.size(), true, paint);
            } else {
                canvas->drawConvexPolygon(m_poly.data(), m_poly.size(), paint);
            }
            canvas->restore();
        };

        auto compute_mirror = [](float x, float radius, float min, float max) -> float {
            if (x + radius > max) {
                return min - max;
            } else if (x < min + radius) {
                return max - min;
            }
            return 0;
        };
        const GRect& r = m_state->mirrorBounds();
        const float dx = compute_mirror(m_origin.x, m_radius, r.left, r.right),
                    dy = compute_mirror(m_origin.y, m_radius, r.top, r.bottom);

        dodraw(m_origin);
        if (dx != 0) {
            dodraw(m_origin + GVector{dx, 0});
            if (dy != 0) {
                dodraw(m_origin + GVector{dx, dy});
            }
        }
        if (dy != 0) {
            dodraw(m_origin + GVector{0, dy});
        }
    }

    void tick(float secs) {
        m_rotation += m_spin * secs;
        m_origin = wrap_around(m_state->mirrorBounds(), m_origin + m_velocity * secs);
    }

    bool collide(GPoint center, float radius, std::vector<Rock>* list) {
        if (contains(m_poly.data(), m_poly.size(), center - m_origin, m_rotation)) {
            if (m_generation < MAX_GENERATIONS) {
                auto spawn = [list, this](GVector vel, float spin) {
                    constexpr float dt = 0.5f;  // travel away from origin
                    const GPoint neworigin = m_origin + vel * dt;
                    const float newradius = m_radius*3/5;

                    list->emplace_back(m_state, neworigin, vel, spin, newradius, m_poly.size());
                    list->back().m_generation = m_generation + 1;
                };
                spawn(rotate(m_velocity,  gFloatPI/4) * 1.5f, m_spin * 1.5f);
                spawn(rotate(m_velocity, -gFloatPI/4) * 1.5f, -m_spin * 1.5f);
            }
            return true;
        }
        return false;
    }
};

struct Ship {
    enum class Turn {
        none, cw, ccw,
    };

    State*  m_state;
    GPoint  m_origin;
    GVector m_direction;
    float   m_speed;
    bool    m_thrusting;
    Turn    m_turn = Turn::none;
    double  m_lastFireTime = 0;

    void init(State* state) {
        m_state = state;
        m_origin = state->m_bounds.center();
        m_direction = {1, 0};
        m_speed = 0;
        m_thrusting = false;
        m_lastFireTime = 0;
    }

    void fire(BulletList* list, double now) {
        constexpr double kBulletTimeGap = 1.0/12;
        if (m_lastFireTime + kBulletTimeGap < now) {
            list->add(m_origin + m_direction * 25, m_direction);
            m_lastFireTime = now;
        }
    }

    void draw(GCanvas* canvas) const {
        const float h = -10;
        static const GPoint pts[] {
            {20, 0}, {-10, -h}, {-5, 0}, {-10, h},
        };
        GPaint paint(GColor_white);
        paint.setHairline();

        auto mx = GMatrix(m_direction, rotate_cw(m_direction),m_origin);
        canvas->save();
        canvas->concat(mx);
        draw_poly_hairlines(canvas, pts, std::size(pts), true, paint);
        canvas->restore();
    }

    void update(Turn turn) {
        m_turn = turn;
    }

    void tick(float secs) {
        const float turningSpeed = gFloatPI * 2;
        m_origin += m_direction * (m_speed * secs);

        if (m_turn != Turn::none) {
            const float rad = turningSpeed * secs * (m_turn == Turn::cw ? 1 : -1);
            m_direction = GMatrix::Rotate(rad) * m_direction;
        }
    }
};

struct Game {
    State             m_state;
    std::vector<Rock> m_rocks;
    BulletList        m_bullets;
    Ship              m_ship;
    double            m_now;

    void init(State s) {
        m_state = s;
        m_rocks.clear();
        m_ship.init(&m_state);
        m_now = 0;

        GPaint paint;
        paint.setHairline();
        paint.setColor(GColor_white);

        auto rnd = [&]() { return m_state.nextF(); };
        auto srnd = [&]() { return m_state.nextSF(); };
        auto signrnd = [&]() { return m_state.nextF() > 0 ? 1 : -1; };

        const auto r = m_state.m_bounds.inset(20, 20);
        const int n = 3;
        for (int i = 0; i < n; ++i) {
            GPoint origin = {
                lerp(r.left, r.right, rnd()),
                lerp(r.top, r.bottom, rnd()),
            };
            const float speed = 70 * (1 + srnd()/3);
            GVector velocity = normalize({srnd(), srnd()}) * speed;
            const float spin = 0.25f * (1 + srnd()/5) * signrnd();
            const float radius = m_state.m_bounds.width()/7;
            m_rocks.emplace_back(
                &m_state, origin, velocity, spin, radius, 4 + (i % 5)
            );
        }

    }

    void toggleBitmaps() {
        if (m_state.m_shader) {
            m_state.m_shader = nullptr;
        } else {
            auto lm = GMatrix::Scale(0.25, 0.25)
                      * GMatrix::Translate(-gMoonRock.width() * 0.5f, -gMoonRock.height() * 0.5f);
            m_state.m_shader = GShader::Bitmap(gMoonRock, lm);
        }
    }

    void draw(GCanvas* canvas, GISize dim) {
        canvas->fillRect(m_state.m_bounds, GColor_black);

        for (const auto& b : m_rocks) {
            b.draw(canvas);
        }
        m_ship.draw(canvas);
        m_bullets.draw(canvas);

        // erase the margin
        draw_donut(canvas, GRect{0, 0, dim.width, dim.height}, m_state.m_bounds, GColor_white);
    }

    void checkForCollisions() {
        for (size_t i = m_bullets.m_bullets.size(); i --> 0;) {
            const auto b = m_bullets.m_bullets[i];
            for (size_t j = m_rocks.size(); j --> 0;) {
                if (m_rocks[j].collide(b.m_origin, Bullet::radius, &m_rocks)) {
                    m_rocks.erase(m_rocks.begin() + j);
                    m_bullets.m_bullets.erase(m_bullets.m_bullets.begin() + i);
                    break;  // only get to kill 1 rock with 1 bullet
                }
            }
        }
    }

    void tick() {
        this->checkForCollisions();

        const auto now = GTime::Secs();
        if (m_now == 0) {
            m_now = now - 1/30.0;   // not sure how to get started
        }
        float dt = static_cast<float>(now - m_now);
        m_now = now;

        for (auto& rock : m_rocks) {
            rock.tick(dt);
        }

        m_ship.tick(dt);
        m_bullets.tick(dt);
    }
};

class TestWindow : public GWindow {
    Game    m_game;
    bool    m_playing = true;
public:
    TestWindow(GISize sz) : GWindow(sz) {
        m_game.init({{30, 30, 530, 530}, {}});
    }

protected:
    void onDraw(GCanvas* canvas) override {
        const auto now = GTime::Secs();
        const auto mask = this->pollFastKeys();

        auto turn = Ship::Turn::none;
        if (mask & GFastKeys::left_arrow) {
            turn = Ship::Turn::ccw;
        } else if (mask & GFastKeys::right_arrow) {
            turn = Ship::Turn::cw;
        }
        m_game.m_ship.update(turn);

        if (mask & GFastKeys::space_key) {
            m_game.m_ship.fire(&m_game.m_bullets, now);
        }

        if (m_playing) {
            m_game.tick();
        }

        m_game.draw(canvas, this->size());

        this->requestDraw();
    }

    bool onKeyPress(uint32_t sym) override {
        switch (sym) {
            case 'g':
                m_playing = !m_playing;
                break;
            case 't':
                m_game.toggleBitmaps();
                break;
            default:
                break;
        }
        return false;
    }

    std::unique_ptr<GClick> onFindClickHandler(GPoint loc) override {
        return nullptr;
    }

private:
    typedef GWindow INHERITED;
};

int main(int argc, char const* const* argv) {
    auto bm = GBitmap::ReadFromFile("apps/moonrock.png");
    if (!bm) {
        fprintf(stderr, "Looking for apps/moonrock.png\n");
        return -1;
    }
    gMoonRock = *bm;
    GWindow* wind = new TestWindow({600, 600});

    return wind->run();
}
