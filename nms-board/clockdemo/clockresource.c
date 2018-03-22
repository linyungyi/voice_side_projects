/*****************************************************************************
*  Filename : clockresource.c
*
*  Description : Resource (timing reference) management
*                It use static variables to hold status of interested timing references
*
* 
*  Copyright 1999-2002 NMS Communications.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "swidef.h"
#include "clockresource.h"

static CLKDEMO_TIMING_REFERENCE *head_of_list = NULL;
extern int _id2index(int board_id, int *index);
extern int _index2id(int index, int *board_id);


/***********************************************************
  ClkRsc_add_to_list:
          add a timing reference to the list
*************************************************************/
int ClkRsc_add_to_list(int board, int trunk, int priority)
    {
    CLKDEMO_TIMING_REFERENCE *current_ref, *previous_ref, *new_ref;
    
    previous_ref = NULL;
    /* search to see if in the list */
    for(current_ref = head_of_list; current_ref !=NULL; current_ref = current_ref->Next)
        {
        if(current_ref->Board_id == board && current_ref->Trunk_id == trunk)
            break;
        previous_ref = current_ref;
        
        }
    
    /* if in the list but different priority, remove the old one */
    if(current_ref != NULL && current_ref->Priority != priority)
        {
        /* if it is the head of list, set new head */
        if(previous_ref == NULL) 
            {
            head_of_list = current_ref->Next;
            }
        else
            {
            previous_ref->Next = current_ref->Next;
            }
        
        free(current_ref);
        }
    
    /* allocate structure and add into the list */
    
    new_ref = (CLKDEMO_TIMING_REFERENCE *) malloc(sizeof(CLKDEMO_TIMING_REFERENCE));
    if(new_ref == NULL)
        {
        printf("Can't malloc new timing reference\n");
        return -1;
        }
    
    new_ref->Board_id = board;
    new_ref->Trunk_id = trunk;
    new_ref->Priority = priority;
    new_ref->Status = MVIP95_TIMING_REF_STATUS_BAD;
    
    previous_ref = NULL;
    /* search for the last timing reference that has the same priority */
    for(current_ref = head_of_list; current_ref !=NULL; current_ref = current_ref->Next)
        {
        if(current_ref->Priority > priority)
            break;
        previous_ref = current_ref;
        }
    
    new_ref->Next = current_ref;
    if(previous_ref != NULL)
        previous_ref->Next = new_ref;
    else
        head_of_list = new_ref;
    
    return 0;
    
    }

/**********************************************************************************
  ClkRsc_remove_from_list:
  remove a timing reference from the list
  upon success, it returns the next timing reference in priority 
**********************************************************************************/
CLKDEMO_TIMING_REFERENCE * ClkRsc_remove_from_list(int board, int trunk)
    {
    CLKDEMO_TIMING_REFERENCE *current_ref, *previous_ref, *next_ref;
    
    previous_ref = NULL;
    /* search to see if in the list */
    for(current_ref = head_of_list; current_ref !=NULL; current_ref = current_ref->Next)
        {
        if(current_ref->Board_id == board && current_ref->Trunk_id == trunk)
            break;
        previous_ref = current_ref;
        
        }
    
    if(current_ref != NULL)
        {
        next_ref = current_ref->Next;
        /* if it is the head of list, set new head */
        if(previous_ref == NULL) 
            {
            head_of_list = next_ref;
            }
        else
            {
            previous_ref->Next = next_ref;
            }
        
        free(current_ref);
        return next_ref;
        }
    else
        return NULL;
    
    }

/******************************************************************
  ClkRsc_get_reference:
                     Used 'rule and/or priority' to search for the 
                     next timing reference
*********************************************************************/
CLKDEMO_TIMING_REFERENCE *ClkRsc_get_reference(int rule, int parm)   
    {
    CLKDEMO_TIMING_REFERENCE *current_ref, *rtn_ref;
    
    switch(rule)
        {
        case CLKRULE_FIRST_IN_PRIORITY:
            /* search the highest priority one */
            for(current_ref = head_of_list; current_ref !=NULL; current_ref = current_ref->Next)
                {
                if(current_ref->Status ==  MVIP95_TIMING_REF_STATUS_GOOD)
                    break;
                }
            
            return current_ref;
            
        case CLKRULE_SECOND_IN_PRIORITY:
            for(current_ref = head_of_list; current_ref !=NULL; current_ref = current_ref->Next)
                {
                if(current_ref->Status ==  MVIP95_TIMING_REF_STATUS_GOOD)
                    break;
                }
            /* if there is a highest priority */
            if(current_ref)
                {
                rtn_ref = current_ref;
                
                /* start search from the one next to the highest priority */
                for(current_ref = rtn_ref->Next; current_ref !=NULL; current_ref = current_ref->Next)
                    {
                    if(current_ref->Status ==  MVIP95_TIMING_REF_STATUS_GOOD)
                        break;
                    }
                }
            
            return current_ref;
            
        case CLKRULE_SECOND_IN_PRIORITY_SAME_BOARD:
            /* find the first one */
            for(current_ref = head_of_list; current_ref !=NULL; current_ref = current_ref->Next)
                {
                if( current_ref->Board_id ==  (parm & 0xffff)            &&
                    current_ref->Trunk_id !=  (parm >> 16)               &&
                    current_ref->Status   ==  MVIP95_TIMING_REF_STATUS_GOOD)
                    break;
                }
            return current_ref;
            
        case CLKRULE_FIRST_IN_PRIORITY_DIFF_BOARD:
            /* find the first reference that does not belong to a specific board */
            /* the parm pass in should be the board number  */
            for(current_ref = head_of_list; current_ref !=NULL; current_ref = current_ref->Next)
                {
                if( current_ref->Board_id   !=  parm                      &&
                    current_ref->Status ==  MVIP95_TIMING_REF_STATUS_GOOD)
                    break;
                }
            
            return current_ref;
            
        case CLKRULE_FIRST_IN_SPECIFIC_BOARD:      
            /* the parm pass in should be the Board number */
            /* start search from the head */
            for(current_ref = head_of_list; current_ref !=NULL; current_ref = current_ref->Next)
                {
                if( current_ref->Board_id ==  parm                       &&
                    current_ref->Status   ==  MVIP95_TIMING_REF_STATUS_GOOD)
                    break;
                }
            
            return current_ref;
        case CLKRULE_FIRST_IN_SPECIFIC_PRIORITY:      
            /* the parm pass in should be the intended priority */
            /* start search from the head */
            for(current_ref = head_of_list; current_ref !=NULL; current_ref = current_ref->Next)
                {
                if( current_ref->Priority ==  parm                       &&
                    current_ref->Status   ==  MVIP95_TIMING_REF_STATUS_GOOD)
                    break;
                }
            
            return current_ref;
        case CLKRULE_FIRST_IN_LIST:
            return head_of_list;
            break;
        default:
            return NULL;
        }
    
    }

/********************************************************************
  ClkRsc_set_status:
                  Update the status of the timing reference
*********************************************************************/
int ClkRsc_update_status(SWIHD swihd[])
    {
    CLKDEMO_TIMING_REFERENCE *current_ref;
    unsigned i, rc;
    SWI_QUERY_TIMING_REFERENCE_ARGS query_args;
    int old_status, ret; 
    
    ret = 0;
    /* query only the interested trunk */
    for(current_ref = head_of_list; current_ref !=NULL; current_ref = current_ref->Next)
        {
        
        /* backup old one for later compare */
        old_status = current_ref->Status;
        
        /* search for the swi handle */
        rc = _id2index(current_ref->Board_id, &i);
        
        if(rc != 0 || swihd[i] == 0)
            {
            current_ref->Status = MVIP95_TIMING_REF_STATUS_BAD;
            }
        
        /* OSC is always Good */
        else if(current_ref->Trunk_id == 0)
            {
            current_ref->Status = MVIP95_TIMING_REF_STATUS_GOOD;
            }
        else
            {
            /* don't know which field been used, fill them all up */
            query_args.size              = sizeof(SWI_QUERY_TIMING_REFERENCE_ARGS);
            query_args.referencesource   = MVIP95_SOURCE_NETWORK;
            query_args.network           = current_ref->Trunk_id;
            
            rc = swiGetTimingReference( swihd[i],
                MVIP95_SOURCE_NETWORK,
                (DWORD)   current_ref->Trunk_id,
                &query_args,
                sizeof(SWI_QUERY_TIMING_REFERENCE_ARGS) );
            /* if fail mark it Bad */
            if (rc != 0)
                {
                printf ("\n ERROR 0x%x: H110 query timing reference, Board %u Trunk %u \n", 
                    rc, current_ref->Board_id, current_ref->Trunk_id);
                
                current_ref->Status = MVIP95_TIMING_REF_STATUS_BAD;
                }
            else
                {
                current_ref->Status = query_args.status;
                }
            }
        /* flag that something has changed */
        if(current_ref->Status != old_status) ret = 1;
        
        }
    return ret;
    
    }

/********************************************************************
  ClkRsc_get_info:
                   read back the information of a timing reference
*********************************************************************/
int ClkRsc_get_info(int board, int trunk, CLKDEMO_TIMING_REFERENCE **ref)
    {
    CLKDEMO_TIMING_REFERENCE *current_ref;
    
    /* search to see if in the list */
    for(current_ref = head_of_list; current_ref !=NULL; current_ref = current_ref->Next)
        {
        if(current_ref->Board_id == board && current_ref->Trunk_id == trunk)
            break;
        }
    
    if(current_ref != NULL)
        {
        *ref = current_ref;
        return 0;
        }
    else
        {
        ref = NULL;
        return -1;
        }
    
    }