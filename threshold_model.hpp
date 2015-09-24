#ifndef HOUND_THRESHOLDMODEL
#define HOUND_THRESHOLDMODEL

class ThresholdModel {
public:
	ThresholdModel() : _maxstale(1) {}

	void update(double age, double stale) {
		UNREFERENCED_PARAMETER(age);
		if(stale > _maxstale) {
			_maxstale = stale;
		}
	}

	double queryLog(double age, double stale) {
		UNREFERENCED_PARAMETER(age);
		// XXX soft?
		if(stale > _maxstale) return 0.0;
		else return -20.0;
	};

	double querySD(double age, double stale) {
		UNREFERENCED_PARAMETER(age);
		return (stale > _maxstale) ? ((stale / _maxstale) - 1.0) : 0.0;
	}

//private:
	double _maxstale;
};

#endif
