#ifndef _CORE_QUEUE_H_
#define _CORE_QUEUE_H_

#if defined(__cplusplus)
extern "C" {
#endif

#if ( defined(__ARMCC_VERSION) || defined(_MSC_VER) || defined(__GNUC__)) && \
    !defined(inline) && !defined(__cplusplus)
    #define inline __inline
#endif

/*
 * Simple queue definitions.
 */
#define	SIMPLEQ_HEAD(name, type)					        \
struct name {								                \
    struct type *sqh_first;	/* first element */			    \
    struct type **sqh_last;	/* addr of last next element */	\
}

#define	SIMPLEQ_HEAD_INITIALIZER(head)					\
    { NULL, &(head).sqh_first }

#define	SIMPLEQ_ENTRY(type)						        \
struct {								                \
    struct type *sqe_next;	/* next element */			\
}

/*
 * Simple queue functions.
 */
#define	SIMPLEQ_INIT(head) do {						    \
    (head)->sqh_first = NULL;					        \
    (head)->sqh_last = &(head)->sqh_first;				\
} while (/*CONSTCOND*/0)

#define	SIMPLEQ_INSERT_HEAD(head, elm, field) do {			    \
    if (((elm)->field.sqe_next = (head)->sqh_first) == NULL)	\
        (head)->sqh_last = &(elm)->field.sqe_next;		        \
    (head)->sqh_first = (elm);					                \
} while (/*CONSTCOND*/0)

#define	SIMPLEQ_INSERT_TAIL(head, elm, field) do {			\
    (elm)->field.sqe_next = NULL;					        \
    *(head)->sqh_last = (elm);					            \
    (head)->sqh_last = &(elm)->field.sqe_next;			    \
} while (/*CONSTCOND*/0)

#define	SIMPLEQ_INSERT_AFTER(head, listelm, elm, field) do {		    \
    if (((elm)->field.sqe_next = (listelm)->field.sqe_next) == NULL)    \
        (head)->sqh_last = &(elm)->field.sqe_next;		                \
    (listelm)->field.sqe_next = (elm);				                    \
} while (/*CONSTCOND*/0)

#define	SIMPLEQ_REMOVE_HEAD(head, field) do {				                \
    if (((head)->sqh_first = (head)->sqh_first->field.sqe_next) == NULL)    \
        (head)->sqh_last = &(head)->sqh_first;			                    \
} while (/*CONSTCOND*/0)

#define	SIMPLEQ_REMOVE(head, elm, type, field) do {			    \
    if ((head)->sqh_first == (elm)) {				            \
    SIMPLEQ_REMOVE_HEAD((head), field);			                \
    } else {							                        \
    struct type *curelm = (head)->sqh_first;		            \
        while (curelm->field.sqe_next != (elm))			        \
            curelm = curelm->field.sqe_next;		            \
        if ((curelm->field.sqe_next =				            \
            curelm->field.sqe_next->field.sqe_next) == NULL)    \
                (head)->sqh_last = &(curelm)->field.sqe_next;   \
    }								                            \
} while (/*CONSTCOND*/0)

#define	SIMPLEQ_FOREACH(var, head, field)		    \
    for ((var) = ((head)->sqh_first);				\
        (var);							            \
        (var) = ((var)->field.sqe_next))

/*
 * Simple queue access methods.
 */
#define	SIMPLEQ_EMPTY(head)		((head)->sqh_first == NULL)
#define	SIMPLEQ_FIRST(head)		((head)->sqh_first)
#define	SIMPLEQ_NEXT(elm, field)	((elm)->field.sqe_next)


#if defined(__cplusplus)
}
#endif

#endif

