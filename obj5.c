/**
	obj5.c


	Simulator Project for COP 4600

	Objective 5 project that calculates OS statistics. 

	Revision List:
		Original Design:  Dr. David Workman, 1990
		Revised by Tim Hughey and Mark Stephens, 1993
		Revised by Wade Spires, Spring 2005
		Minor Revisions by Sean Szumlanski, Spring 2010
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "osdefs.h"
#include "externs.h"

/**
	Compute all simulation statistics and store them in the appropriate
	variables and data structures for display.  It is called by Clean_Up().

	<b> Algorithm: </b>
	\verbatim
	For each user
		Calculate total processing time
		Calculate total job blocked time
		Calculate total job wait time
		Calculate total job execution time
		Calculate efficiency for each process

	For each device
		Calculate response time for each device
		Calculate total idle time for each device
		Calculate utilization for each device

	Calculate average user execution time
	Calculate average user logon time
	Calculate average user blocked time
	Calculate average user wait time
	Calculate response time for CPU
	Calculate total idle time for CPU
	Calculate total utilization for CPU
	\endverbatim
 */
void
Calc_Stats( )
{
}
