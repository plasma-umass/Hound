#define _USE_MATH_DEFINES
#include <math.h>

class GaussianStaleness {
public:
	GaussianStaleness() : _my(1), _syy(0), _N(1) {}

	// IGNORES x (age)
	void update(double x, double y) {
		double omy = _my;
		double osyy = _syy;

		// count
		_N++;

		// means
		_my = omy + (y - omy)/_N;

		// sum of squares
		_syy = osyy + (y - omy)*(y-_my);
	}

	double queryLog(double x, double y) {
		double sigy = sqrt(_syy/(_N - 1));

		return -log(2.0 * M_PI * sigy) - (y - _my)*(y - _my)/(2.0*sigy*sigy);
	}

//private:
	double _my;
	double _syy;
	unsigned int _N;
};