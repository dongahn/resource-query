CPP           := g++ #clang++
BOOST_LDFLAGS := -L/usr/local/lib \
	         -lboost_system -lboost_filesystem -lboost_graph -lyaml-cpp -lreadline
LDFLAGS       := -O0 -g $(BOOST_LDFLAGS) -L./planner -lplanner -lczmq -lzmq
CPPFLAGS      := -O0 -g -std=c++11 -MMD -MP
INCLUDES      := -I/usr/include -I/usr/local/include
OBJS          := resource-query.o \
                 grug2dot.o \
                 resource_gen.o \
                 resource_gen_spec.o \
                 jobspec.o 
MATCHERS      := CA \
                 IBA \
                 IBBA \
                 PFS1BA \
                 PA \
                 C+IBA \
                 C+PFS1BA \
                 C+PA \
                 IB+IBBA \
                 C+P+IBA \
                 ALL
DEPS          := $(OBJS:.o=.d)

TARGETS       := planner resource-query grug2dot

all: $(TARGETS)

graphs: $(GRAPHS) 

resource-query: resource-query.o resource_gen.o resource_gen_spec.o jobspec.o
	$(CPP) $^ -o $@ $(LDFLAGS)

grug2dot: grug2dot.o resource_gen_spec.o
	$(CPP) $^ -o $@ $(LDFLAGS)

planner:
	$(MAKE) -C $@

%.o:%.cpp
	$(CPP) $(CPPFLAGS) $(INCLUDES) $< -c -o $@

.PHONY: clean planner

clean:
	cd planner && make clean && \
	rm -f $(OBJS) $(DEPS) esource-query grug2dot *~ *.dot *.svg

-include $(DEPS)
