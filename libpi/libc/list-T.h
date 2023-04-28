#ifndef __LIST_T_H__
#define __LIST_T_H__

// engler, cs140e: brain-dead generic list with a single next pointer.
// 
// - pfx: prepend to all helper routines.
// - E_T: the type name of the linked struct
// - <next>: name of field to hold the next element
#define gen_list_T(pfx, E_T, next)     \
    /* iterator support. */                                             \
    static inline E_T *pfx ## _next(E_T *l)  { return l->next; }   \
                                                                        \
    /* remove from front of list. */                                    \
    static inline E_T *pfx ## _pop(E_T **l) {                            \
        demand(l, bad input);                                           \
        if(!*l)                                                          \
            return 0;                                                   \
        E_T *e = *l;                                                    \
        *l = e->next;                                                   \
        return e;                                                       \
    }                                                                   \
                                                                        \
    /* insert at head (for LIFO) */                                     \
    static inline void pfx ## _push(E_T **l, E_T *e) {                   \
        e->next = *l;                                                   \
        *l = e;                                                         \
    }                                                                   \
    static inline void pfx ## _init(E_T **l) { *l = 0; }

#endif
