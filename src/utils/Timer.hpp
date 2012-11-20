#ifndef TIMER_H
#define TIMER_H

#include "Types.hpp"

#include <map>
#include <ctime>

#define PROFILE_FUNCTION() PROFILE(__PRETTY_FUNCTION__)

#ifdef ENABLE_PROFILING
#define PROFILE(name) Timer ___t(name)
#else
#define PROFILE(name)
#endif

#define NAME_SPACING 50
#define CELL_SPACING 8

class Timer
{

public:
  Timer(rcString inName);
  ~Timer();

  static String GetReport(int inPrecision = 4);
  static String SpacePadding(rcString inString, int inSpaces);

private:
  static std::vector<String> sTasks;
  static std::map<String, Uint32> sTaskCounters;
  static std::map<String, double> sSumTimings;
  static std::map<String, double> sMinTimings;
  static std::map<String, double> sMaxTimings;

  inline double getRealTime() const
  {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return double(ts.tv_sec) + 1e-9 * double(ts.tv_nsec);
  }

  inline double getCpuTime() const
  {
    timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return double(ts.tv_sec) + 1e-9 * double(ts.tv_nsec);
  }

  bool mIsInitialized;
  double mStartTime;
  double mEndTime;
  cString mTaskName;
};

#endif
