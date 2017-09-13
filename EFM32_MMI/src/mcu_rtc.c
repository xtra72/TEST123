/*******************************************************************
**                                                                **
** RTC management functions                                       **
**                                                                **
*******************************************************************/

#include "mcu_rtc.h"
#include "system.h"

static unsigned long _RTC_Counter;

unsigned long RTCGetSeconds(void)
{
  unsigned long N = ((_RTC_Counter) ? _RTC_Counter : 86400UL) + SystemGetSystemSeconds();
  return N;
}

void RTCSetSeconds(const unsigned long seconds)
{
  _RTC_Counter = seconds - SystemGetSystemSeconds();
}
