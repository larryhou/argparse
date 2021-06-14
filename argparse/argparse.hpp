//
//  argparse.hpp
//  argparse
//
//  Created by LARRYHOU on 2021/6/12.
//

#ifndef argparse_hpp
#define argparse_hpp

#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>

namespace argparse {

template<typename T> struct Type { static T parse(const char* s); };
template<> struct Type<std::string> { static std::string parse(const char* s) { return s; } };
template<> struct Type<int> { static int parse(const char* s) { return static_cast<int>(strtoll(s, nullptr, 0)); } };
template<> struct Type<long long> { static long long parse(const char* s) { return strtoll(s, nullptr, 0); } };
template<> struct Type<double> { static double parse(const char* s) { return strtod(s, nullptr); } };
template<> struct Type<float> { static float parse(const char* s) { return strtof(s, nullptr); } };
template<> struct Type<bool>
{
    static bool parse(const char* s)
    {
        if (!strcmp(s, "true") || !strcmp(s, "TRUE") || !strcmp(s, "1")) { return true; }
        return false;
    }
};

class Value
{
protected:
    std::vector<char> layout;
    std::string repr;
    
public:
    const char* get() const { return layout.data(); }
    const char* str() const { return repr.c_str(); }
};

struct ArgumentValue: public Value
{
    template<typename T> ArgumentValue(T v)
    {
        layout.resize(sizeof(T));
        memcpy(layout.data(), &v, sizeof(T));
        repr = std::to_string(v);
    }
    
    template<> ArgumentValue(std::string v)
    {
        layout.resize(v.size() + 1);
        memcpy(layout.data(), v.c_str(), v.size() + 1);
        repr = v;
    }
};

using ValuePtr = std::shared_ptr<Value>;

template<typename T>
struct ArgumentType
{
    static ValuePtr parse(std::string s)
    {
        T v = Type<T>::parse(s.c_str());
        return std::make_shared<ArgumentValue>(v);
    }
};

template<> struct ArgumentType<std::string>
{
    static ValuePtr parse(std::string s) { return std::make_shared<ArgumentValue>(s); }
};

#include <exception>
class parse_exception: public std::exception
{
    std::string message;
public:
    parse_exception(std::string message): message(message) {}
    const char * what() const _NOEXCEPT { return message.c_str(); }
};

enum OptionType
{
    kOptionTypeRequired = 0,
    kOptionTypeOptional,
    kOptionTypeFlag,
};

struct ArgumentOption
{
    ValuePtr value;
    std::string name;
    std::string abbr;
    std::string help;
    OptionType type;
    
    std::function<ValuePtr(std::string)> parse;
    
    ArgumentOption(ValuePtr value, std::string name, std::string abbr, std::string help)
        :value(value), name(name), abbr(abbr), help(help) { }
};

using ArgumentOptionPtr = std::shared_ptr<ArgumentOption>;

class ArgumentParser
{
    std::map<std::string, ArgumentOptionPtr> arguments;
    std::multimap<std::string, ValuePtr> options;
    std::vector<std::string> args;
    
public:
    template<typename T, typename ValueType = ArgumentValue>
    void add_optional(std::string name, std::string abbr, T value, std::string help, std::function<ValuePtr(std::string)> parse = nullptr)
    {
        add(name, abbr, std::make_shared<ValueType>(value), help, [&](ArgumentOptionPtr option)
        {
            option->type = kOptionTypeOptional;
            option->parse = parse ? parse : ArgumentType<T>::parse;
        });
    }
    
    template<typename T>
    void add_required(std::string name, std::string abbr, std::string help, std::function<ValuePtr(std::string)> parse = nullptr)
    {
        add(name, abbr, nullptr, help, [&](ArgumentOptionPtr option)
        {
            option->type = kOptionTypeRequired;
            option->parse = parse ? parse : ArgumentType<T>::parse;
        });
    }
    
    void add_flag(std::string name, std::string abbr, std::string help)
    {
        add(name, abbr, std::make_shared<ArgumentValue>(false), help, [](ArgumentOptionPtr option)
        {
            option->type = kOptionTypeFlag;
        });
    }
    
    void parse(int argc, const char **argv);
    void parse(std::string args);
    
    template<typename T>
    T get(std::string name)
    {
        auto match = arguments.find(name);
        if (match == arguments.end()) { return T(); }
        
        auto arg = match->second;
        {
            auto iter = options.find(arg->name);
            if (iter == options.end())
            {
                if (arg->type == kOptionTypeOptional)
                {
                    return resolve<T>(arg->value->get());
                }
                
                return T();
            }
            
            return resolve<T>(iter->second->get());
        }
    }
    
    template<typename T>
    std::vector<T> all(std::string name)
    {
        auto match = arguments.find(name);
        if (match == arguments.end()) { return {}; }
        
        std::vector<T> result;
        auto arg = match->second;
        {
            auto iter = options.lower_bound(arg->name);
            if (iter == options.end())
            {
                if (arg->type == kOptionTypeOptional) { result.push_back(resolve<T>(arg->value->get())); }
                return result;
            }
            
            while (iter != options.end() && iter->first == arg->name)
            {
                result.push_back(resolve<T>(iter->second->get()));
                ++iter;
            }
        }
        
        return result;
    }
    
    void dump(std::ostream &stream = std::cout);
    
private:
    void add(std::string name, std::string abbr, ValuePtr value, std::string help, std::function<void(ArgumentOptionPtr)> post);
    void help(std::ostream &stream = std::cout);
    
    template<typename T> T resolve(const char *ptr) { return *(T *)ptr; }
    template<> std::string resolve(const char *ptr) { return ptr; }
};
}

#endif /* argparse_hpp */
