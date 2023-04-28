### pi with floating point.

Big picture:

 - libpi gets compiled both:

    - without floating point (libpi/libpi.a, staff-start.o) 
    - with floating point (libpi/libpi-fp.a, staff-start-fp.o). 

 - libpi/libm: has math routines (libpi/libm/libm-pi.a)

 - you need to define  your CS240LX_2023_PATH (like you did with
   (`CS140E_2023_PATH`).  

   Since I use `tcsh` this looks like:

        setenv CS240LX_2023_PATH /home/engler/class/cs240lx-23spr/

   make sure to `source` your shell configuraton to refresh this
   value. (easy: logout and log back in)

An example floating point use is in this directory (`labs/config-float`).
Note the `Makefile` has two new lines:

    USE_FP=1
    LIBS += $(LIBM)

If you compile and run the example:

        % cat fp-example.c  
        // trivial example of using floating point + our simple math
        // library.
        #include "rpi.h"
        #include "rpi-math.h"
        void notmain(void) {
            double x = 3.1415;
            printk("hello from pi=%f float!!\n", x);
        
            double v[] = { M_PI, 0, M_PI/2.0, M_PI/2.0*3.0 };
            for(int i = 0; i < 4; i++)  {
                printk("COS(%f) = %f\n", v[i], cos(v[i]));
                printk("sin(%f) = %f\n", v[i], sin(v[i]));
            }
        }

You should get:

        hello from pi=3.141500 float!!
        COS(3.141592) = -1.0
        sin(3.141592) = 0.0
        COS(0.0) = 1.0
        sin(0.0) = 0.0
        COS(1.570796) = 0.0
        sin(1.570796) = 1.0
        COS(4.712388) = -0.0
        sin(4.712388) = -1.0
        DONE!!!
