CPP       := g++
CC        := gcc
INCLUDES  := -I./ -I/usr/include/czmq -I/usr/local/include
CFLAGS    := -Wall -O3 -g
CPPFLAGS  := -Wall -O3 -g
#CFLAGS    := -Wall -O0 -g
#CPPFLAGS  := -Wall -O0 -g
LDFLAGS   := -L/usr/local/lib -lczmq -lzmq 
ARCHIVE   := ar
OBJS      := rbtree.o xzmalloc.o planner.o
TAP_OBJS  := tap.o
DEPS      := $(OBJS:.o=.d)

all: libplanner.a libtap.a planner_test01 planner_test02

%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
%.o:%.cpp
	$(CPP) $(CPPFLAGS) $(INCLUDES) --std=c++11 -c $< -o $@

libplanner.a: $(OBJS)
	$(AR) -r $@ $^ 

libtap.a: $(TAP_OBJS)
	$(AR) -r $@ $^

planner_test01: planner_test01.o libplanner.a libtap.a
	$(CPP) $^ -o $@ -L./ -lplanner -ltap $(LDFLAGS)

planner_test02: planner_test02.o libplanner.a libtap.a
	$(CPP) $^ -o $@ -L./ -lplanner -ltap $(LDFLAGS)

clean:
	rm -fr *csv *gp *png *.o *breakpoints*  $(OBJS) libplanner.* libtap.a planner_test01 planner_test02

-include $(DEP)
