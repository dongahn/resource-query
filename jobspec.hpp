#ifndef jobspec_hpp
#define jobspec_hpp

#include <iostream>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

class Resource {
public:
    std::string type;
    struct {
        unsigned min;
        unsigned max;
        char oper = '+';
        int operand = 1;
    } count;
    std::string unit;
    std::string label;
    std::string id;
    bool exclusive = false;
    std::vector<Resource> with;
    std::unordered_map<std::string, int64_t> user_data;

    Resource(const YAML::Node&);

private:
    void parse_yaml_count(const YAML::Node&);
};

class Task {
public:
    std::string command;
    std::string slot_type;
    std::string slot_value;
    std::unordered_map<std::string, std::string> count;
    std::string distribution;
    std::unordered_map<std::string, std::string> attributes;

    Task(const YAML::Node&);
};

class Jobspec {
public:
    unsigned int version;
    std::vector<Resource> resources;
    std::vector<Task> tasks;
    std::unordered_map<std::string,
                       std::unordered_map<std::string,
                                          std::string>> attributes;
    Jobspec(const YAML::Node&);
    Jobspec(std::istream &is): Jobspec{YAML::Load(is)} {}
    Jobspec(std::string &s): Jobspec{YAML::Load(s)} {}
};

#endif
