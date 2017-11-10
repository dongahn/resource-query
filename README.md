## Resource Query Utility

`resource-query` is a command-line utility that takes in a HPC resource request
written in Flux's Canonical Job Specification (RFC 14) and selects the best-
matching compute and other resources in accordance with a selection policy.

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
specification that test.jobspec contains.

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
      ---tiny0[0:s]
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
      ---tiny0[0:s]
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
  - type: slot
    count: 4
    label: default
    with:
      - type: node
        count: 1
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

NOTE: both `resource-query` and scheduling infrastructure work within Flux
are in very early development stages; please set your expectations accordingly.

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

The resource service strawman walks this recipe graph using
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
We have minimal support for doxygen documentation. It can be generated:

```
$ cd doxy
$ doxygen doxy_conf.txt
$ cd ..
```
This will generate html, latex and man sub-directories under
the doc directory. Open doc/html/index.html using your favorite web
browser. NOTE for LLNL developers: It doesn't build on TOSS2 systems
because their compilers are old. Please use a TOSS3 machine or your
own laptop (e.g. Mac OSX)

## How to Use Scoring API to Effect Your Resource Selection Policy
TBD
