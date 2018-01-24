#ifndef _BRUSH_H
#define _BRUSH_H

#include "tablet.h"
#include <cmath>
#include <functional>


struct Vec {
    double x, y;

    Vec(const TabletEvent &evt) : x(evt.x), y(evt.y) { }
    Vec(double _x=0, double _y=0) : x(_x), y(_y) { }

    Vec operator-(const Vec &o) const {
        return Vec{x - o.x, y - o.y};
    }

    Vec operator+(const Vec &o) const {
        return Vec{x + o.x, y + o.y};
    }

    Vec operator*(double a) const {
        return Vec{x * a, y * a};
    }

    Vec operator/(double a) const {
        return Vec{x / a, y / a};
    }

    double len() const {
        return sqrt(x * x + y * y);
    }

    double sqlen() const {
        return x * x + y * y;
    }

    double dot(const Vec &o) const {
        return x * o.x + y * o.y;
    }

    Vec normalized() const {
        auto l = len();
        if (l < 1e-3) {
            return Vec{0, 0};
        }
        return Vec{x / l, y / l};
    }
};

template<int weight>
class Brush {
    TabletEvent _last_pos;

    double interpolate(double x, double y, double a) {
        return a*x + (1-a)*y;
    }

public:
    template<typename Buf>
    void draw(TabletEvent res, Buf &buffer) {
        auto p = Vec(_last_pos), c = Vec(res);
        auto l = c - p;
        if (!buffer.getPixel(res.x, res.y)) {
            return;
        }
        if (_last_pos.pressure < 10 || l.len() < 1e-3) {
            _last_pos = res;
            return;
        }

        auto n = Vec{l.y, -l.x};
        auto nlen = n.len();

        auto pwidth = 2*_last_pos.pressure/1000.;
        auto cwidth = 2*res.pressure/1000.;

        auto p1 = p - n * pwidth / nlen, p2 = p + n * pwidth / nlen;
        auto c1 = c - n * cwidth / nlen, c2 = p + n * cwidth / nlen;

        auto inside = [&](double x, double y) {
            Vec pt{x, y};
            // minimize (proj-pt)**2 subj to (proj-c, n)=0
            // (proj-c, n)=0 <=> (proj, n)= (c, n) = c0
            // F = (projx-ptx)**2+(projy-pty)**2+lambda*(projx*nx+projy*ny-c0) -> min
            // dF/dprojx = 2*projx-2*ptx+lambda*nx = 0; => projx = ptx-lambda*nx/2;
            // dF/dprojy = 2*projy-2*pty+lambda*ny = 0; => projy = pty-lambda*ny/2;
            // F = (ptx-lambda*nx/2-ptx)**2+(pty-lambda*ny/2-pty)**2+lambda*(()*nx+()*ny-c0) =
            //   = lambda**2*(nx**2/4+ny**2/4)+lambda*((ptx-lambda*nx/2)*nx+(pty-lambda*ny/2)*ny-c0) =
            //   = -lambda**2*(nx**2+ny**2)/4+lambda*(ptx*nx+pty*ny-c0)
            // dF/dlambda = -lambda*(nx**2+ny**2)/2+(ptx*nx+pty*ny-c0) = 0
            // lambda = 2*(pt-c, n)/(n**2)=2*(pt-c, n)
            // projx = ptx - 2*(pt-c, n)/(n**2)*nx/2 = ptx - (pt-c, n) * nx
            // projy = pty - (pt-c, n) * ny
            double distance = (pt - c).dot(n);
            auto projected = pt - n * distance / n.sqlen();
            double max_distance = interpolate(pwidth, cwidth, (c - projected).dot(c-p)/(c-p).sqlen());
            return fabs(distance) < nlen*max_distance-1e-3;
        };

        std::function<void(int, int, int, int, int)> warnock = [&buffer, &warnock, &inside](int mnx, int mny, int mxx, int mxy, int d) {
            bool in[] = {inside(mnx, mny), inside(mnx, mxy), inside(mxx, mny), inside(mxx, mxy)};
            if (in[0] && in[1] && in[2] && in[3]) {
                for (int y = mny; y <= mxy; ++y) {
                    // for (int x = mnx; x <= mxx; ++x) {
                    //     auto pxl = buffer.getPixel(x, y);
                    //     if (!pxl) {
                    //         break;
                    //     }
                    //     pxl[0] = pxl[1] = pxl[2] = pxl[3] = 255*weight;
                    // }
                    auto pxl = buffer.getPixel(0, y);
                    if (pxl) {
                        memset(pxl+4*mnx, weight*255, 4*(mxx-mnx+1));
                    }
                }
                return;
            }
            if (d > 1 && !in[0] && !in[1] && !in[2] && !in[3]) {
                return;
            }
            if (mnx > mxx || mny > mxy) {
                return;
            }
            if (mnx >= mxx && mny >= mxy) {
                return;
            }
            auto mix = (mnx + mxx) / 2;
            auto miy = (mny + mxy) / 2;
            warnock(mnx, mny, mix, miy, d+1);
            warnock(mix+1, mny, mxx, miy, d+1);
            warnock(mnx, miy+1, mix, mxy, d+1);
            warnock(mix+1, miy+1, mxx, mxy, d+1);
        };
        double mnx = std::min(p1.x, std::min(p2.x, std::min(c1.x, c2.x)));
        double mxx = std::max(p1.x, std::max(p2.x, std::max(c1.x, c2.x)));
        double mny = std::min(p1.y, std::min(p2.y, std::min(c1.y, c2.y)));
        double mxy = std::max(p1.y, std::max(p2.y, std::max(c1.y, c2.y)));

        warnock(floor(mnx), floor(mny), ceil(mxx), ceil(mxy), 0);

        _last_pos = res;
    }
};
#endif
