/* Copyright (C) 2001, 2002, 2003 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2001.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifndef LIST_H
#define LIST_H	1

/* Add element to the end of a circular, double-linked list.  */
#define CDBL_LIST_ADD_REAR(first, newp) \
  do {									      \
    __typeof (newp) _newp = (newp);					      \
    assert (_newp->next == NULL);					      \
    assert (_newp->previous == NULL);					      \
    if (unlikely ((first) == NULL))					      \
      (first) = _newp->next = _newp->previous = _newp;			      \
    else								      \
      {									      \
	_newp->next = (first);						      \
	_newp->previous = (first)->previous;				      \
	_newp->previous->next = _newp->next->previous = _newp;		      \
      }									      \
  } while (0)

/* Remove element from circular, double-linked list.  */
#define CDBL_LIST_DEL(first, elem) \
  do {									      \
    __typeof (elem) _elem = (elem);					      \
    /* Check whether the element is indeed on the list.  */		      \
    assert (first != NULL && _elem != NULL				      \
	    && (first != elem						      \
		|| ({ __typeof (elem) _runp = first->next;		      \
		      while (_runp != first)				      \
			if (_runp == _elem)				      \
			  break;					      \
			else						      \
		          _runp = _runp->next;				      \
		      _runp == _elem; })));				      \
    if (unlikely (_elem->next == _elem))				      \
      first = NULL;							      \
    else								      \
      {									      \
	_elem->next->previous = _elem->previous;			      \
	_elem->previous->next = _elem->next;				      \
	if (unlikely (first == _elem))					      \
	  first = _elem->next;						      \
      }									      \
     assert ((_elem->next = _elem->previous = NULL, 1));		      \
  } while (0)


/* Add element to the front of a single-linked list.  */
#define SNGL_LIST_PUSH(first, newp) \
  do {									      \
    __typeof (newp) _newp = (newp);					      \
    assert (_newp->next == NULL);					      \
    _newp->next = first;						      \
    first = _newp;							      \
  } while (0)


/* Add element to the rear of a circular single-linked list.  */
#define CSNGL_LIST_ADD_REAR(first, newp) \
  do {									      \
    __typeof (newp) _newp = (newp);					      \
    assert (_newp->next == NULL);					      \
    if (unlikely ((first) == NULL))					      \
      (first) = _newp->next = _newp;					      \
    else								      \
      {									      \
	_newp->next = (first)->next;					      \
	(first) = (first)->next = _newp;				      \
      }									      \
  } while (0)


#endif	/* list.h */
