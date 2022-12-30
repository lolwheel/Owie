#ifndef FILTER_H
#define FILTER_H

// Based on
// http://www.schwietering.com/jayduino/filtuino/index.php?characteristic=bu&passmode=lp&order=2&usesr=usesr&sr=1&frequencyLow=0.0025&noteLow=&noteHigh=&pw=pw&calctype=float&run=Send
// With alpha=0.0025 and observed sample rate of the voltage messages of 1Hz,
// we get low pass period of 400 seconds.
//
// Low pass butterworth filter order=2 alpha1=0.0025
class LowPassFilter {
 public:
  LowPassFilter() = default;

 private:
  bool initialized = false;

  float v[3] = {0};

  void setTo(float value) {
    const float val = value / 4;
    v[0] = val;
    v[1] = val;
    v[2] = val;
  }

 public:
  float get() { return (v[0] + v[2]) + 2 * v[1]; }
  void step(float x) {
    if (!initialized) {
      initialized = true;
      setTo(x);
      return;
    }
    v[0] = v[1];
    v[1] = v[2];
    v[2] = (6.100617875806624291e-5 * x) + (-0.97803050849179629100 * v[0]) +
           (1.97778648377676402603 * v[1]);
  }
};

#endif