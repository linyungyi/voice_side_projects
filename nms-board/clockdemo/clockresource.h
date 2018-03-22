/*****************************************************************************
*  Filename : clockresource.h
*
*  Description : header file of Resource (timing reference) management
* 
*  Copyright 1999-2002 NMS Communications.
*****************************************************************************/

/* This structure is used to maintain a list of timing references
that will be used as the system clock source */
struct _ClkDemo_Timing_Reference;

typedef struct  _ClkDemo_Timing_Reference
    {
    int                              Board_id;  /* Board Number */
    int                              Trunk_id;  /* Trunk Number, 0 -> OSC */
    int                              Status;    /* Status of the timing reference GOOD/BAD */
    int                              Priority;  /* priority of this timing reference */
    struct _ClkDemo_Timing_Reference *Next;     /* pointer to the next timing reference */
    } CLKDEMO_TIMING_REFERENCE;

#define CLKRULE_FIRST_IN_PRIORITY                        0x00
#define CLKRULE_SECOND_IN_PRIORITY                       0x01
#define CLKRULE_FIRST_IN_SPECIFIC_BOARD                  0x10
#define CLKRULE_FIRST_IN_SPECIFIC_PRIORITY               0x11
#define CLKRULE_SECOND_IN_PRIORITY_SAME_BOARD            0x12
#define CLKRULE_FIRST_IN_PRIORITY_DIFF_BOARD             0x13
#define CLKRULE_FIRST_IN_LIST                            0x20

int ClkRsc_add_to_list(int board, int trunk, int proirity);
CLKDEMO_TIMING_REFERENCE * ClkRsc_remove_from_list(int board, int trunk);
CLKDEMO_TIMING_REFERENCE *ClkRsc_get_reference(int rule, int parm);
int ClkRsc_update_status(SWIHD swihd[]);
int ClkRsc_get_info(int board, int trunk, CLKDEMO_TIMING_REFERENCE **ref);





