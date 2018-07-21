#define _USE_MATH_DEFINES
#include <math.h>

class GaussianModel {
public:
  GaussianModel() : _mx(1), _my(1), _sxx(0), _syy(0), _N(1) {
  }

  void update(double x, double y) {
    double omx = _mx;
    double omy = _my;
    double osxx = _sxx;
    double osyy = _syy;

    // count
    _N++;

    // means
    _mx = omx + (x - omx) / _N;
    _my = omy + (y - omy) / _N;

    // sum of squares
    _sxx = osxx + (x - omx) * (x - _mx);
    _syy = osyy + (y - omy) * (y - _my);
  }

  double queryLog(double x, double y) {
    double sigx = sqrt(_sxx / (_N - 1));
    double sigy = sqrt(_syy / (_N - 1));

    return -log(2.0 * M_PI * sigx * sigy) - (x - _mx) * (x - _mx) / (2.0 * sigx * sigx) -
           (y - _my) * (y - _my) / (2.0 * sigy * sigy);
  }

  double querySD(double x, double y) {
    double dx, dy;
    double sigx = sqrt(_sxx / (_N - 1));
    double sigy = sqrt(_syy / (_N - 1));

    dx = (_mx - x) / sigx;
    dy = (_my - y) / sigy;

    return sqrt(dx * dx + dy * dy);
  }
  // private:
  double _mx, _my;
  double _sxx, _syy;
  UINT _N;
};
