#include <iostream>
#include <string>
#include <unordered_map>

#include "jobspec.hpp"

using namespace std;

void Resource::parse_yaml_count(const YAML::Node &cnode)
{
    /* count can have an unsigned interger value */
    if (cnode.IsScalar()) {
        count.min = cnode.as<unsigned>();
        count.max = count.min;
        return;
    }

    /* or count may be a more complicated verbose form */
    if (!cnode.IsMap()) {
        throw invalid_argument("count is not a mapping");
    }

    /* Verify existance of required entries */
    if (!cnode["min"]) {
        throw invalid_argument("Key \"min\" missing from count");
    }
    if (!cnode["max"]) {
        throw invalid_argument("Key \"max\" missing from count");
    }
    if (!cnode["operator"]) {
        throw invalid_argument("Key \"operator\" missing from count");
    }
    if (!cnode["operand"]) {
        throw invalid_argument("Key \"operand\" missing from count");
    }

    /* Validate values of entries */
    count.min = cnode["min"].as<unsigned>();
    if (count.min < 1) {
        throw invalid_argument("\"min\" must be greater than zero");
    }

    count.max = cnode["max"].as<unsigned>();
    if (count.max < 1) {
        throw invalid_argument("\"max\" must be greater than zero");
    }
    if (count.max < count.min) {
        throw invalid_argument("\"max\" must be greater than or equal to \"min\"");
    }

    count.oper = cnode["operator"].as<char>();
    switch (count.oper) {
    case '+':
    case '-':
    case '^':
        break;
    default:
        throw invalid_argument("Invalid count operator");
    }

    count.operand = cnode["operand"].as<int>();
}

vector<Resource> parse_yaml_resources(const YAML::Node &resources);

Resource::Resource(const YAML::Node &resnode)
{
    int field_count = 0;

    /* The resource must be a mapping */
    if (!resnode.IsMap()) {
        throw invalid_argument("resource is not a mapping");
    }
    if (resnode.size() < 2 || resnode.size() > 10) {
        throw invalid_argument("impossible number of entries in resource mapping");
    }
    if (!resnode["type"]) {
        throw invalid_argument("Key \"type\" missing from resource");
    }
    type = resnode["type"].as<string>();
    field_count++;
    if (!resnode["count"]) {
        throw invalid_argument("Key \"count\" missing from resource");
    }
    parse_yaml_count(resnode["count"]);
    field_count++;
    if (resnode["unit"]) {
        field_count++;
        unit = resnode["unit"].as<string>();
    }
    if (resnode["exclusive"]) {
        field_count++;
        std::string x = resnode["exclusive"].as<string>();
        if (x == "true")
            exclusive = true;
    }
    if (resnode["with"]) {
        field_count++;
        with = parse_yaml_resources(resnode["with"]);
    }
    if (resnode["label"]) {
        field_count++;
        unit = resnode["label"].as<string>();
    }
    if (resnode["id"]) {
        field_count++;
        id = resnode["id"].as<string>();
    }

    if (field_count != resnode.size()) {
        throw invalid_argument("Unrecognized key in resource mapping");
    }
}

Task::Task(const YAML::Node &tasknode)
{
    /* The task node must be a mapping */
    if (!tasknode.IsMap()) {
        throw invalid_argument("task is not a mapping");
    }
    if (tasknode.size() < 3 || tasknode.size() > 5) {
        throw invalid_argument("impossible number of entries in task mapping");
    }
    if (!tasknode["command"]) {
        throw invalid_argument("Key \"command\" missing from task");
    }
    command = tasknode["command"].as<string>();

    /* Import slot */
    if (!tasknode["slot"]) {
        throw invalid_argument("Key \"slot\" missing from task");
    }
    YAML::Node tmpnode = tasknode["slot"];
    if (!tmpnode.IsMap()) {
        throw invalid_argument("\"slot\" in task is not a mapping");
    }
    if (tmpnode.size() != 1) {
        throw invalid_argument("\"slot\" must contain a single key/value pair");
    }
    slot_type = tmpnode.begin()->first.as<string>();
    slot_value = tmpnode.begin()->second.as<string>();

    /* Import count mapping */
    if (tasknode["count"]) {
        YAML::Node count = tasknode["count"];
        if (!count.IsMap()) {
            throw invalid_argument("\"count\" in task is not a mapping");
        }
        for (auto&& entry : count) {
            count[entry.first.as<string>()] = entry.second.as<string>();
        }
    }

    /* Import distribution if it is present */
    if (tasknode["distribution"]) {
        distribution = tasknode["distribution"].as<string>();
    }

    /* Import attributes mapping if it is present */
    if (tasknode["attributes"]) {
        YAML::Node attrs = tasknode["attributes"];
        if (!attrs.IsMap()) {
            throw invalid_argument("\"attributes\" in task is not a mapping");
        }
        for (auto&& attr : attrs) {
            attributes[attr.first.as<string>()] = attr.second.as<string>();
        }
    }
}

vector<Task> parse_yaml_tasks(const YAML::Node &tasks)
{
    vector<Task> taskvec;

    /* "tasks" must be a sequence */
    if (!tasks.IsSequence()) {
        throw invalid_argument("\"tasks\" is not a sequence");
    }

    for (auto&& task : tasks) {
        taskvec.push_back(Task(task));
    }

    return taskvec;
}

vector<Resource> parse_yaml_resources(const YAML::Node &resources)
{
    vector<Resource> resvec;

    /* "resources" must be a sequence */
    if (!resources.IsSequence()) {
        throw invalid_argument("\"resources\" is not a sequence");
    }

    for (auto&& resource : resources) {
        resvec.push_back(Resource(resource));
    }

    return resvec;
}


Jobspec::Jobspec(const YAML::Node &top)
{
    /* The top yaml node of the jobspec must be a mapping */
    if (!top.IsMap()) {
        throw invalid_argument("Top level of jobspec is not a mapping");
    }
    /* There must be exactly four entries in the mapping */
    if (top.size() != 4) {
        throw invalid_argument("Top mapping in jobspec must have four entries");
    }
    /* The four keys must be the following */
    if (!top["version"]) {
        throw invalid_argument("Missing key \"version\" in top level mapping");
    }
    if (!top["resources"]) {
        throw invalid_argument("Missing key \"resource\" in top level mapping");
    }
    if (!top["tasks"]) {
        throw invalid_argument("Missing key \"tasks\" in top level mapping");
    }
    if (!top["attributes"]) {
        throw invalid_argument("Missing key \"attributes\" in top level mapping");
    }

    /* Import version */
    if (!top["version"].IsScalar()) {
        throw invalid_argument("\"version\" must be an unsigned integer");
    }
    version = top["version"].as<unsigned int>();

    /* Import attributes mappings */
    YAML::Node attrs = top["attributes"];
    if (!attrs.IsMap()) {
        throw invalid_argument("\"attributes\" is not a mapping");
    }
    for (auto&& i : attrs) {
        attributes[i.first.as<string>()];
        if (!i.second.IsMap()) {
            throw invalid_argument("value of one of the attributes not a mapping");
        }
        for (auto&& j : i.second) {
            attributes[i.first.as<string>()][j.first.as<string>()] =
                j.second.as<string>();
        }
    }

    /* Import resources section */
    resources = parse_yaml_resources(top["resources"]);

    /* Import tasks section */
    tasks = parse_yaml_tasks(top["tasks"]);
}

