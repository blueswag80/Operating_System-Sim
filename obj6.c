/**
	obj6.c


	Simulator Project for COP 4600

	Objective 6 project that provides memory compaction and timer
	interrupts for round-robin scheduling.

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
	Compact memory to eliminate external fragmentation.

	The main idea is to continuously bubble the location of the first free block
	of memory up through memory and merge it with adjacent free blocks until
	only one large free block remains. The bubbling up is achieved by finding
	the used block in memory that is adjacent to the first free block and
	copying this used segment's instructions into the free block. The location
	of the free block is then set to directly follow the used segment.

	<b> Important Notes:</b>
	\li Neither the free block's nor the segment's size changes from moving
	them. The free block's size only changes when it is moved next to an
	adjacent free block and subsequently merged with it.
	\li Note that the algorithm does not break or return after merging a
	block. This is because it's possible that the next segment of the current
	user being checked may be the next segment to be copied into the
	newly-placed free block. However, Free_Mem->next may be NULL, but this
	will not be a problem if Merge_seg() is well-designed.

	<b> Algorithm: </b>
	\verbatim
	If no more space is available
		Return since compaction cannot be performed
	While more than 2 free memory blocks remain (Free_Mem->next != NULL)
		Find the PCB such that its segment is located after the first free block:
			For each PCB saved in Term_Table
				If the PCB was never used or the PCB has already terminated
					Skip it
				For each segment in the PCB's segment table
					If the segment directly follows the first free block
						Copy each instruction in memory from the segment into the free memory block
						Set start of segment to start of former free block
						Set start of free block to end of segment (using segment's new starting position)

						If the free block is now next to another free block
							Merge the two blocks into one larger free block
	\endverbatim
 */
void
Compact_mem( )
{
}

/**
	Service timer events.

	When a timer interrupt occurs, the active process is removed from the CPU,
	provided round-robin is the active scheduling policy; otherwise, the active
	process is allowed to continue.

	<b> Important Notes:</b>
	\li Timer interrupts should only occur with a preemptive scheduler like
	round-robin.
	\li Events of type TIMER_EVT will result in this call via
	Interrupt_Handler() in simulator.c after interrupt() in obj1.c sets the
	global Event variable.
 */
void
Timer_Service( )
{
}
