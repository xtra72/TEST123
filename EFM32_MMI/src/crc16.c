/*******************************************************************
**                                                                **
** CRC16 computation functions.                                   **
**                                                                **
*******************************************************************/

#include "crc16.h"

unsigned short CRC16_CalculatePolynomial(const unsigned char Byte, const unsigned short prevCrc, const unsigned short Polynom)
{
  unsigned short CRC = prevCrc ^ (((unsigned short)Byte) << 8);
  for (unsigned char i=8;i;i--)
  {
    if (CRC & 0x8000) 
      CRC = (CRC << 1) ^ Polynom;
    else
      CRC = CRC << 1;
  }
  return ((unsigned short)(CRC & 0xFFFF));
}

unsigned short CRC16_CalculateRangePolynomial(const unsigned char* Buffer, unsigned short length, unsigned short prevCrc, const unsigned short Polynom)
{
  while (length--)
    prevCrc = CRC16_CalculatePolynomial(*Buffer++, prevCrc,Polynom);
  return prevCrc;
}
