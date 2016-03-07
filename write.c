/*
 * Redirecting standard output to UART1 module
 *
 * Tested on:
 * Microstick + Microstick Plus Development Board
 * PIC24HJ64GP502
 *
 * File name: write.c
 * Author:    Jozsef KOPJAK
 * Info:      info@microstickplus.com
 * Web:       http://www.microstickplus.com
 *
 * Last modification: 10-04-2013
 */

//#include <stdio.h>
#include "typedefs.h"
#include "uart1.h"

int write(int handle, void *buffer, unsigned int len) {
  int i;

  switch (handle)
  {
    case 0:		// stdin
    case 1:		// stdout
    case 2:		// stderr
      for (i = len; i; --i)
      {
	      vPutCharU1( *(BYTE*)buffer++ );
      }
      break;

    default:
      break;
  }
  return(len);
}
