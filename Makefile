CPP           := g++ #clang++
BOOST_LDFLAGS := -L/usr/local/lib \
	         -lboost_system -lboost_filesystem -lboost_graph -lyaml-cpp -lreadline
LDFLAGS       := -O0 -g $(BOOST_LDFLAGS) -L../planner/Planner -lplanner -lczmq -lzmq
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
SCALES        := mini-5subsystems-fine \
		 medium-5subsystems-fine 
GRAPHS        := $(foreach m, $(MATCHERS), $(foreach s, $(SCALES), $(m).$(s)))

TARGETS       := resource-query grug2dot

all: $(TARGETS)

graphs: $(GRAPHS) 

resource-query: resource-query.o resource_gen.o resource_gen_spec.o jobspec.o
	$(CPP) $^ -o $@ $(LDFLAGS)

grug2dot: grug2dot.o resource_gen_spec.o
	$(CPP) $^ -o $@ $(LDFLAGS)

$(GRAPHS): resource-query
	mkdir -p graphs_dir/$(subst .,$(empty),$(suffix $@))
	mkdir -p graphs_dir/$(subst .,$(empty),$(suffix $@))/images
	$< --grug=conf/$(subst .,$(empty),$(suffix $@)).graphml \
		--matcher=$(basename $@) \
		--output=graphs_dir/$(subst .,$(empty),$(suffix $@))/$@
	if [ -f graphs_dir/$(subst .,$(empty),$(suffix $@))/$@.dot ]; \
	then \
		cd graphs_dir/$(subst .,$(empty),$(suffix $@)) \
		&& dot -Tsvg $@.dot -o images/$@.svg; \
	fi
	mkdir -p graphmls_dir/$(subst .,$(empty),$(suffix $@))
	$< --grug=conf/$(subst .,$(empty),$(suffix $@)).graphml \
		--matcher=$(basename $@) \
		--graph-format=graphml \
		--output=graphmls_dir/$(subst .,$(empty),$(suffix $@))/$@

%.o:%.cpp
	$(CPP) $(CPPFLAGS) $(INCLUDES) $< -c -o $@

.PHONY: clean

clean:
	rm -f $(OBJS) $(DEPS) $(TARGETS) *~ *.dot *.svg

clean-graphs: 
	rm -f -r graphs_dir/* graphmls_dir/*

-include $(DEPS)
