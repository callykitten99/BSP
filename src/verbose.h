#ifndef VERBOSE_H
#define VERBOSE_H

#define VERBOSE_1(stmt)
#define VERBOSE_2(stmt)
#define VERBOSE_3(stmt)
#define VERBOSE_4(stmt)

#ifndef VERBOSE
    #define VERBOSE 0
#else
    #if VERBOSE > 0
        #undef  VERBOSE_1
        #define VERBOSE_1(stmt) stmt
    #endif
    #if VERBOSE > 1
        #undef  VERBOSE_2
        #define VERBOSE_2(stmt) stmt
    #endif
    #if VERBOSE > 2
        #undef  VERBOSE_3
        #define VERBOSE_3(stmt) stmt
    #endif
    #if VERBOSE > 3
        #undef  VERBOSE_4
        #define VERBOSE_4(stmt) stmt
    #endif
#endif

#endif /* VERBOSE_H */