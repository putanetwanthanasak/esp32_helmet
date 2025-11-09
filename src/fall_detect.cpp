#include "fall_detect.h"
#include <config.h>
#include <math.h>

enum State { IDLE, IMPACT_WIN, TILT_HOLD, FALL_DONE };
static State st = IDLE;
static unsigned long mark = 0;
static float lastA=0, lastR=0, lastP=0;
static unsigned long lastT=0;

void fallDetectBegin(){ st=IDLE; lastA=lastR=lastP=0; lastT=0; }

bool fallDetected(const SensorData& s){
  unsigned long now=millis();
  float dt = (lastT==0) ? 0.02f : (now-lastT)/1000.0f;
  if (dt <= 0) dt = 0.001f;
  lastT = now;

  float jerk = fabsf(s.A - lastA) / dt;
  float dR = fabsf(s.roll - lastR);
  float dP = fabsf(s.pitch - lastP);
  float omega = fmaxf(dR, dP) / dt;

  lastA = s.A; lastR = s.roll; lastP = s.pitch;

  bool impact = (s.A > IMPACT_A_T_MS2) || (jerk > IMPACT_JERK_T) || (omega > OMEGA_T_DPS) || s.shock;
  bool tilt   = (fabs(s.roll) > TILT_THRESH_DEG) || (fabs(s.pitch) > TILT_THRESH_DEG);

  switch(st){
    case IDLE:
      if (impact){ st=IMPACT_WIN; mark=now; }
      break;

    case IMPACT_WIN:
      if (tilt){ st=TILT_HOLD; mark=now; }
      else if (now - mark > ARM_WINDOW_MS){ st=IDLE; }
      break;

    case TILT_HOLD:
      if (tilt && (now - mark > TILT_HOLD_MS)){ st=FALL_DONE; }
      else if (!tilt){ st=IDLE; }
      break;

    case FALL_DONE:
      st=IDLE;
      return true;
  }
  return false;
}