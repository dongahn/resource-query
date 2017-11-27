## Resource Query Utility

`resource-query` is a command-line utility that takes in a HPC resource request
written in Flux's Canonical Job Specification (RFC 14) and selects the best-matching
compute and other resources in accordance with a selection policy.

The goal of this utility is largely three-fold. First, as we are actively
developing a scalable batch-job scheduling infrastructure within Flux,
`resource-query` provides a way to exercise our infrastructure code to help with
our design decisions and development process in general. Second, this utility will
ultimately land into Flux and serve as our debugging and testing interface for our
scheduling infrastructure. Finally, `resource-query` is designed to facilitate
Exascale system-software co-design activities, and it does this by enabling
advanced scheduler developers and policy writers (e.g., IO bandwidth-aware
or power-aware policies) to test their HPC resource representation and selection
ideas in a much more simplified, easy-to-use environment.

In the following, we describe how `resource-query` builds on our scheduling
infrastructure and how users interact with this utility.

On start-up, `resource-query` reads in a resource-graph generation recipe written
in the GRUG format (see the GRUG section below) and populates a resource-graph
data store, representing HPC resources and their relationships according to
Flux Resource Model (RFC 4). Once the graph data store is populated, an
interactive command-line interface (cli) session is started for the user:

```
% resource-query --grug=conf/default --match-subsystems=CA --match-policy=high
% INFO: Loading a matcher: CA
resource-query>
```

The main cli command is `match.` Essentially, this command takes in
a job-specification or jobspec file name and either allocates or reserves the
best-matching resources.  `allocate`, a sub-command of `match` will try
to allocate the best-matching resources for the given jobspec.
`allocate_orelse_reserve`, the other sub-command will try to reserve the
resources into the future if the allocation cannot be created given the
current resource state.
By contrast, `allocate` will simply not allocate resources if matching
resources cannot be selected in the current resource state.

The following command allocated the best-matching resources for the
specification that test.jobspec contains. The output of an allocation
or reservation is a reversed tree shape where the root appears at
the last line. Each resource is annotated with the allocated or
reserved count and exclusive (x) vs. shared (s) access modes. For example,
`core31[1:x]` indicates that the 1 unit of `core31` has been exclusively
allocated or reserved. Similary, `memory1[2:x]` shows that the 2
units (i.e., GB) of `memory1` have been exclusive allocated. Please note
that the granularity of exclusive allocation/reservation is
the whole resource pool vertex, not anything less. So, if you want a more
fined-grained exclusive memory allocation, you should first represent
your memory pool vertices with smaller memory unit (e.g., 256MB).



```
resource-query> match allocate test.jobspec
      ---------------core31[1:x]
      ---------------core32[1:x]
      ---------------core33[1:x]
      ---------------core34[1:x]
      ---------------core35[1:x]
      ---------------gpu0[1:x]
      ---------------memory1[2:x]
      ---------------memory2[2:x]
      ---------------memory3[2:x]
      ------------socket0[1:x]
      ---------------core67[1:x]
      ---------------core68[1:x]
      ---------------core69[1:x]
      ---------------core70[1:x]
      ---------------core71[1:x]
      ---------------gpu1[1:x]
      ---------------memory5[2:x]
      ---------------memory6[2:x]
      ---------------memory7[2:x]
      ------------socket1[1:x]
      ---------node1[1:s]
      ------rack0[1:s]
      ---tiny0[1:s]
INFO: =============================
INFO: JOBID=1
INFO: RESOURCES=ALLOCATED
INFO: SCHEDULED AT=Now
INFO: =============================
```
 
The following command reserved the best-matching resources for the specification
given by test.jobspec.

```
resource-query> match allocate_orelse_reserve test.jobspec
      ---------------core31[1:x]
      ---------------core32[1:x]
      ---------------core33[1:x]
      ---------------core34[1:x]
      ---------------core35[1:x]
      ---------------gpu0[1:x]
      ---------------memory1[2:x]
      ---------------memory2[2:x]
      ---------------memory3[2:x]
      ------------socket0[1:x]
      ---------------core67[1:x]
      ---------------core68[1:x]
      ---------------core69[1:x]
      ---------------core70[1:x]
      ---------------core71[1:x]
      ---------------gpu1[1:x]
      ---------------memory5[2:x]
      ---------------memory6[2:x]
      ---------------memory7[2:x]
      ------------socket1[1:x]
      ---------node1[1:s]
      ------rack0[1:s]
      ---tiny0[1:s]
INFO: =============================
INFO: JOBID=3
INFO: RESOURCES=RESERVED
INFO: SCHEDULED AT=3600
INFO: =============================
```

test.jobspec:

```yaml
version: 1
resources:
  - type: node
    count: 1
    with:
      - type: slot
        count: 1
        label: default
        with:
          - type: socket
            count: 2
            with:
              - type: core
                count: 5
              - type: gpu
                count: 1
              - type: memory
                count: 6

attributes:
  system:
    duration: 3600
tasks:
  - command: app
    slot: default
    count:
      per_slot: 1
```

Internally, here is how `resource-query` uses our scheduling infrastructure
for matching. It simply passes a jobspec object into a traversal
interface of our infrastructure to traverse in a predefined order the
resource graph data store previously populated according to a GRUG file.
While traversing, the traverser calls back a matcher callback plugin
method on each visit event.
The matcher callback plugin represents a resource selection policy.
It evaluates the visiting resource vertex and passes its score to the
infrastructure, which then later uses the score information to determine
the best-matching resources to select.

Currently, `resource-query` supports only one traversal type as our scheduling
infrastructure only has one type: depth-first traversal on the
dominant subsystem and up-walk traversal on one or more auxiliary subsystems.
This capability will be expanded as more advanced traversal types are
developed.

The resource graph data are managed and organized around the concept
of subsystems (e.g., containment subsystem, power subsystem, network subsystem
and etc). A matcher subscribes to one of more these subsystems as its
dominant and/or auxiliary ones to perform matches on.
While testing has been done mostly on the containment subsystem, to position
us for future work, `resource-query` provides options for using a predefined
matcher that is configured to use a different set of subsystems.

`resource-query` provides an option for instantiating a different
resource-matching selection policy--e.g., select resources with high or low
IDs first. For more information about the options: `resource-query --help`
will print available options.  `resource-query> help` will print out messages
to explain cli commands.

## Generating Resources Using GraphML (GRUG)

GRUG is a GraphML-based language for specifying a resource-graph generation recipe.
`resource-query` can read in a GRUG file and populate its store
of the resource graph data conforming to Flux’s resource model
([RFC4](https://github.com/flux-framework/rfc/blob/master/spec_4.adoc)).
The goal of GRUG is to help Flux scheduler plug-in developers easily determine
the representation of this resource graph data (e.g., granularity of resource pools,
relationships between resources, and subsystems/hierarchies to use to organize
the resources) that are best suited for their scheduling objectives and algorithms.  
Without having to modify the source code of `resource-query` and our scheduling
infrastructure, developers can quickly test various resource graph representations
by only modifying the GRUG text file.

GraphML is an easy-to-use, XML-based graph specification language. GRUG uses
the vanilla GraphML schema (http://graphml.graphdrawing.org) with no extension,
and thereby familiarity with GraphML is the only prerequisite for fluent uses
of GRUG. We find that the following on-line GraphML materials are particularly
useful:

- [The GraphML File Format](http://graphml.graphdrawing.org)
- [GraphML Primer](http://graphml.graphdrawing.org/primer/graphml-primer.html)
- [Graph Markup Language](https://cs.brown.edu/~rt/gdhandbook/chapters/graphml.pdf)

### GRUG 
GRUG describes a resource-generation recipe as a graph. A vertex prescribes
how the corresponding resource pool (or simply resource as a shorthand) should
be generated;
an edge prescribes how the corresponding relationships between two resources
should be generated. The edge properties also allow a small recipe graph
to generate a large and more complex resource graph store.
A multiplicative edge has a scaling factor that will generate the specified
number of copies of the resources of the target type. An associative edge
allows a source resource to be associated with some of the already generated resources
in a specific manner.

The scheduling infrastructure walks this recipe graph using
the depth-first-search traversal and emits and stores the corresponding
resources and their relationship data into its resource graph store.  
The recipe graph must be a forest of trees whereby each tree represents
a distinct resource hierarchy or subsystem. We use the terms, hierarchy
and subsystem interchangeably.

A conforming GRUG file is composed of two sections: 1) recipe graph
definition and 2) recipe attributes declaration. We explain both
in the following sections.

### Recipe Graph Definition

A recipe graph definition is expressed as GraphML’s `graph` elements
consisting of two nested elements: `node` and `edge`. A `node` element
prescribes ways to generate a resource pool and an edge
for generating relationships (RFC 4). For example, given the following
definition,

```xml
<node id="socket">
     <data key="type">socket</data>
     <data key="basename">socket</data>
     <data key="size">1</data>
     <data key="subsystem">containment</data>
</node>

<node id="core">
    <data key="type">core</data>
    <data key="basename">core</data>
    <data key="size">1</data>
    <data key="subsystem">containment</data>
</node>
```
these `node` elements are the generation recipes
for a socket and compute-core resource (i.e., scalar), respectively.
And they belong to the containment hierarchy.


```xml
<edge id="socket2core" source="socket" target="core">
    <data key="e_subsystem">containment</data>
    <data key="relation">contains</data>
    <data key="rrelation">in</data>
    <data key="gen_method">MULTIPLY</data>
    <data key="multi_scale">2</data>
</edge>
```

Here, this `edge` element is the generation recipe for
the relationship between the socket and core resources. 
It specifies that for each socket resource, 2 new
core resources (i.e., MULTIPLY and 2) will be generated,
and the relationship is `contains` and the reverse relationship
is `in`.

A resource in one subsystem (e.g., power hierarchy) can be
associated with another subsystem (e.g., containment hierarchy),
and associative edges are used for this purpose.

```xml
<node id="pdu_power">
    <data key="type">pdu</data>
    <data key="basename">pdu</data>
    <data key="subsystem">power</data>
</node>

<edge id="powerpanel2pdu" source="powerpanel" target="pdu_power">
    <data key="e_subsystem">power</data>
    <data key="relation">drawn</data>
    <data key="rrelation">flows</data>
    <data key="gen_method">ASSOCIATE_IN</data>
    <data key="as_tgt_subsystem">containment</data>
</edge>
```

Here, this `edge` element is the generation recipe for
the relationship between `powerpanel` and `pdu` resource.
It specifies that a `powerpanel` resource will be associated
(i.e., `ASSOCIATE_IN`) with all of the `pdu` resources
that have already generated within the `containment` subsystem. 
The forward relationship is `drawn` and the reverse
relationship is `flows`.

Oftentimes, association with all resources of a type is not
sufficient to make a fine-grained association. For the case where the hierarchical paths of 
associating resources can be used to make associations, `ASSOCIATE_BY_PATH_IN` generation
method can be used.

```xml
<edge id="pdu2node" source="pdu_power" target="node_power">
    <data key="e_subsystem">power</data>
    <data key="relation">drawn</data>
    <data key="rrelation">flows</data>
    <data key="gen_method">ASSOCIATE_BY_PATH_IN</data>
    <data key="as_tgt_uplvl">1</data>
    <data key="as_src_uplvl">1</data>
</edge>
```

Here, the method is similar to the previous one except that
the association is only made with the `node` resources whose
hierarchical path at its parent level (i.e., `as_tgt_uplvl`=1)
is matched with the hierarchical path of the source resource
(also at the parent level, `as_src_uplvl`=1).

### Recipe Attributes Declaration 

This section appears right after the GraphML header and
before the recipe graph definition section.
To be a valid GRUG, this section must declare all attributes for both `node`
and `edge` elements. Currently, there are 16 attributes that must be 
declared. 5 for the `node` element and 11 for the `edge`
elements. You are encouraged to define the default value for
each attribute, which then can lead to more concise
recipe definitions. A graph element will inherit the default
attribute values unless it specifically overrides them.
The 16 attributes are listed in the following:

```xml
<-- attributes for the recipe node elements -->
<key id="root" for="node" attr.name="root" attr.type="int">
<key id="type" for="node" attr.name="type" attr.type="string"/>
<key id="basename" for="node" attr.name="basename" attr.type="string"/>
<key id="size" for="node" attr.name="size" attr.type="long"/>
<key id="subsystem" for="node" attr.name="subsystem" attr.type="string"/>

<-- attributes for the recipe edge elements -->
<key id="e_subsystem" for="edge" attr.name="e_subsystem" attr.type="string"/>
<key id="relation" for="edge" attr.name="relation" attr.type="string"/>
<key id="rrelation" for="edge" attr.name="rrelation" attr.type="string"/>
<key id="id_scope" for="edge" attr.name="id_scope" attr.type="int"/>
<key id="id_start" for="edge" attr.name="id_start" attr.type="int"/>
<key id="id_stride" for="edge" attr.name="id_stride" attr.type="int"/>
<key id="gen_method" for="edge" attr.name="gen_method" attr.type="string"/>
<key id="multi_scale" for="edge" attr.name="multi_scale" attr.type="int"/>
<key id="as_tgt_subsystem" for="edge" attr.name="as_tgt_subsystem" attr.type="string">
<key id="as_tgt_uplvl" for="edge" attr.name="as_tgt_uplvl" attr.type="int"/>
<key id="as_src_uplvl" for="edge" attr.name="as_src_uplvl" attr.type="int"/>
```

The `root` attribute specifies if a
resource is the root of a subsystem. If root, 1 must be assigned.

`id_scope`, `id_start` and `id_stride` specify how the id field of a
resource will be generated. The integer specified with `id_scope`
defines the scope in which the resource id should be generated. 
The scope is local to its ancestor level defined by `id_scope`.
If `id_scope` is higher than the most distant ancestor, then
the id space becomes global. 

For example,
if `id_scope`=0, the id of the generating resource will be local to its parent.
If `id_scope`=1, the id becomes local to its grand parent
For example, in `rack[1]->node[18]->socket[2]->core[8]` configuration,
if `id_scope` is 1, the id space of a core resource is local to
the node level instead of the socket level.
So, 16 cores in each node will have 0-15, instead of repeating
0-7 and 0-7, which will be the case if the `id_scope` is 0.


### Example GRUG Files
Example GRUG files can be found in `conf/` directory.
`medium-1subsystem-coarse.graphml` shows how one can model
a resource graph in a highly coarse manner with no additional
subsystem-based organization. `mini-5subsystems-fine.graphml` shows
one way to model a fairly complex resource graph with five
distinct subsystems to support the matchers of various types.

 
### GRUG Visualizer
`grug2dot` utility can be used to generate a GraphViz dot file
that can render the recipe graph. The dot file can be converted
into svg format by typing in `dot -Tsvg output.dot -o output.svg`:

```
Usage: grug2dot <genspec>.graphml
    Convert a GRUG resource-graph generator spec (<genspec>.graphml)
    to AT&T GraphViz format (<genspec>.dot). The output
    file only contains the basic information unless --more is given.

    OPTIONS:
    -h, --help
            Display this usage information

    -m, --more
            More information in the output file

```

## Documentation for Flux Scheduling Infrastructure Code Base
The source base has in-line documentation support for doxygen.
It can be generated:

```
$ cd doxy
$ doxygen doxy_conf.txt
$ cd ..
```
This will generate html, latex and man sub-directories under
the doc directory. Open doc/html/index.html using your favorite web
browser. NOTE for LLNL developers: It doesn't build on TOSS2 systems
because their compilers are too old. Please use a TOSS3 machine or your
own laptop (e.g. Mac OSX)

## Resource Selection Policy
Scheduler resource selection policy implementers can effect
their policies by deriving from our base match callback
class (`dfu_match_cb_t`) and overwriting one or more of its virtual methods.
The DFU traverser's `run ()` method calls back these methods
on well-defined graph vertex visit events and uses both 
match and score information to determine best matching.

Currently the supported visit events are: 

- preorder, postorder, slot, and finish graph events on the selected
dominant subsystem;
- preorder and postorder events on one or more selected
auxiliary subsystems.

`dfu_match_id_based.hpp` shows three demo match callback
implementations. They only overwrite `dom_finish_vtx ()` and
`dom_finish_graph ()` and `dom_finish_slot ()` to effect
their selection policies as
they just use one dominant subsystem: `containment`.
For example, the policy implemented in `high_first_t` provides
preferences towards higher IDs for resource selection; for example,
if node0 and node1 are both available and the user wanted only 1 node,
it will select node1. The following is the source listing for
its `dom_finish_vtx ()`. It is invoked when all of the subtree walk (on
the selected dominant subsystem) and up walk (on the selected
auxiliary subsystems) from the visiting vertex have been completed
and there are enough resource units that can satisfy
the job specification (i.e., method argument `resources`).

Note: up walk logic has not yet been fully tested and hardened.


```c++
 84     int dom_finish_vtx (vtx_t u, const subsystem_t &subsystem,
 85                         const std::vector<Flux::Jobspec::Resource> &resources,
 86                         const f_resource_graph_t &g, scoring_api_t &dfu)
 87     {
 88         int64_t score = MATCH_MET;
 89         int64_t overall;
 90
 91         for (auto &resource : resources) {
 92             if (resource.type != g[u].type)
 93                 continue;
 94
 95             // jobspec resource type matches with the visiting vertex
 96             for (auto &c_resource : resource.with) {
 97                 // test children resource count requirements
 98                 const std::string &c_type = c_resource.type;
 99                 unsigned int qc = dfu.qualified_count (subsystem, c_type);
100                 unsigned int count = select_count (c_resource, qc);
101                 if (count == 0) {
102                     score = MATCH_UNMET;
103                     break;
104                 }
105                 dfu.choose_accum_best_k (subsystem, c_resource.type, count);
106             }
107         }
108
109         // high id first policy (just a demo policy)
110         overall = (score == MATCH_MET)? (score + g[u].id + 1) : score;
111         dfu.set_overall_score (overall);
112         decr ();
113         return (score == MATCH_MET)? 0 : -1;
114     }
```

The scoring API object, `dfu`, contains relevant resource information
gathered as part of the subtree and up walks.

For example, you are visiting
a `socket` vertex and `dfu` contains a map of all of the resources
that are at its subtree, which may be 18 compute cores and 4 units
of 16GB.
Further, if the resource request was `slot[1]->socket[2]->core[4]`, 
the passed `resources` at the `socket` vertex visit level
would be `core[4]`. The method then checks the count satifiability
of the visiting `socket`'s child resource and then calls
`choose_accum_best_k ()` within `dfu` scoring API object
to choose the best matching 4 cores among
however many cores available.

`choose_accum_best_k ()` uses the scores that have already been
calculated during the subtree walk at the core resource level.
Because the default comparator of this method is `fold::greater`,
it sorts the cores in descending ID order.

If the visiting vertex satisfies the request, it sets the score
of the visiting vertex using `set_overall_score ()` method.
In this case, the score is merely the ID number of the visiting vertex.  

Similarly, `dom_finish_graph ()` performs the same logic
as `dom_finish_vertex ()` but has been introduced so that
we can perform a selection for the root resource vertex
(e.g., `cluster[1]`) without having to introduce special
casing within `dom_finish_vtx ()`.

Finally, `dom_finish_slot ()` is introduced so that the match
callback can provide score information on the discovered slots
using its comparator.
Note that, though, there exists no real `slot` resource vertex in the
resource graph, so you can't get a postorder visit event per each
slot. Instead, the DFU traverser by itself will perform the satisfiability
check on the child resource shape of each slot. But this matcher
callback method still provides an opportunity to the match
callback class to score all of the the child resources
of the discovered `slot`. This example
uses `choose_accum_all ()` method within the scoring API
object to sort all of the child resources of `slot` according
to its selection policy.

The Scoring API classes and implementation are entirely
located in `scoring_api.hpp`.

## Fully vs. Paritially Specified Resource Request

The resource section of a job specification can be fully
or partitially hierarchically specified. A fully specified request describes
the resource shape fully from the root to the requested resources with respect
to the resource graph data used by `resource-query`. A partially
specified resource request omits the prefix (i.e., from the root
to the highest-level resources in the request). For example, if the resource
graph data used by `resource-query` is the following,

![](resource.png)

then, the next fully hierarchically specifies
the resource request:

```yaml
version: 1
resources:
    - type: cluster
      count: 1
      with:
        - type: rack
          count: 1
          with:
            - type: node
              count: 1
              with:
                  - type: slot
                    count: 1
                    label: default
                    with:
                      - type: socket
                        count: 1
                        with:
                          - type: core
                            count: 1

```

By contrast, the following partially hierarchically specifies
the resource shape, as it omits from the `cluster` and `rack` levels.

```yaml
version: 1
resources:
  - type: node
    count: 1
    with:
        - type: slot
          count: 1
          label: default
          with:
            - type: socket
              count: 1
              with:
                - type: core
                  count: 1

```

Because the latter does not impose higher-level (i.e.,
`cluster` and `rack` levels) constraints, `node` type resources
will be evaluated by the match callbacks and all of them will be compared 
at once to select the highest scored node. 
On the other hand, with the higher-level constraints of the former specification,
`resource-query` will choose the highest-scored node at the `rack` level
in the same manner as how it enforces the lower-level constraints (e.g., `socket`).


## Limitations of Depth-First and Up (DFU) Traversal

You can implement a wide range of resource selection policy classes
using the DFU traversal, in particular in combination
with other mechanisms (e.g., choosing a different set and order of subsystems).
DFU, however, is a simple, one-pass traversal type and hence there are
inherient limitations associated with DFU, which may preclude you from
implementing certain policies.

For example, currently DFU cannot handle the following job specification even
if there is a rack that contains those nodes that can satisfy
either type of node requirements: one with more cores and burst buffers (bb)
and the other fewer cores with no advanced features.

```yaml
version: 1
resources:
  - type: cluster
    count: 1
    with:
      - type: rack
        count: 1
        with:
          - type: slot
            count: 2
            label: gpunode
            with:
              - type: node
                count: 1
                with:
                  - type: socket
                    count: 2
                    with:
                      - type: core
                        count: 18
                      - type: gpu
                        count: 1
                      - type: memory
                        count: 32
                  - type: bb
                    count: 768

              - type: node
                count: 1
                with:
                  - type: slot
                    count: 2
                    label: bicore
                      - type: socket
                        count: 1
                        with:
                          - type: core
                            count: 2
```

In general, to be able to handle a job specification
where resource requests of a same type appears
at the same hierarchical level (in this case compute `node` type
under the `rack` level), the traverser must be able to
perform a subtree walk for each of them to evaluate a match.
However, DFU does not have an ability to repeat certain subtree
walks and thus it cannot handle this matching problem.

Note that DFU can solve similar but slightly different matching problem:
different `node` types are contained within differently named rack types.
For example, the following job specification can be matched if the
underlying resource model labels the type of the rack with the beefy compute
nodes as `rack` and the other as `birack`.

```yaml
  - type: cluster
    count: 1
    with:
      - type: rack
        count: 1
        with:
          - type: slot
            count: 2
            label: gpunode
            with:
              - type: node
                count: 1
                with:
                  - type: socket
                    count: 2
                    with:
                      - type: core
                        count: 18
                      - type: gpu
                        count: 1
                      - type: memory
                        count: 32
                  - type: bb
                    count: 768

      - type: birack
        count: 1
        with:
          - type: slot
            count: 2
            label: bicorenode
            with:
              - type: node
                count: 1
                with:
                  - type: socket
                    count: 2
                    with:
                      - type: core
                        count: 2


```


When more advanced classes of resource selection policies are required,
you need to introduce new traversal types. For example, an ability
to traverse a subtree more than once for depth first walk
-- e.g., Loop-aware DFU -- can solve the examples shown above.
We designed our scheduling infrastructure to be extended. In fact,
it is our future work to extend our infrastracture with more capable
traversal types.

If you are interested in our earlier discussions on the different classes
of matching problems, please refer to
[this issue](https://github.com/flux-framework/flux-sched/issues/247#issuecomment-310551638)

