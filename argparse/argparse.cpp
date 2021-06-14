//
//  argparse.cpp
//  argparse
//
//  Created by LARRYHOU on 2021/6/12.
//

#include "argparse.hpp"

#include <set>

using namespace argparse;

void ArgumentParser::parse(int argc, const char **argv)
{
    for (auto i = 0; i < argc; i++)
    {
        std::string item(argv[i]);
        if (item[0] == '-')
        {
            if (item == "--help" || item == "-h")
            {
                help(std::cout);
                exit(0);
            }
            
            auto match = arguments.find(item);
            if (match == arguments.end())
            {
                throw parse_exception(item + " is not supported");
            }
            
            auto arg = match->second;
            switch (arg->type)
            {
                case kOptionTypeRequired: case kOptionTypeOptional:
                {
                    if (i + 1 == argc)
                    {
                        throw parse_exception(item + " is lack of option value");
                    }

                    options.insert(std::make_pair(arg->name, arg->parse(argv[++i])));
                } break;
                
                case kOptionTypeFlag:
                {
                    options.insert(std::make_pair(arg->name, std::make_shared<ArgumentValue>(true)));
                } break;
            }
        }
        else
        {
            args.push_back(item);
        }
    }
    
    std::set<std::string> errors;
    for (auto iter = arguments.begin(); iter != arguments.end(); iter++)
    {
        auto &name = iter->second->name;
        if (iter->second->type == kOptionTypeRequired && options.find(name) == options.end())
        {
            if (errors.find(name) == errors.end())
            {
                std::cerr << "required option " + iter->second->name + "/" + iter->second->abbr << " is not set; ";
                errors.insert(name);
            }
        }
    }
    
    if (errors.size())
    {
        std::cerr << std::endl;
        exit(1);
    }
}

void ArgumentParser::parse(std::string args)
{
    std::vector<const char*> argv{args.c_str()};
    for (auto c = args.begin(); c != args.end(); c++)
    {
        if (*c == ' ')
        {
            *c = '\0';
            argv.push_back(&*++c);
        }
    }
    
    parse(static_cast<int>(argv.size()), argv.data());
}

void ArgumentParser::help(std::ostream &stream)
{
    char b[256];
    std::set<ArgumentOptionPtr> visits;
    for (auto iter = arguments.begin(); iter != arguments.end(); iter++)
    {
        if (visits.find(iter->second) == visits.end())
        {
            visits.insert(iter->second);
            
            auto arg = iter->second;
            sprintf(b, "%4s%-4s %-14s", "", arg->abbr.c_str(), arg->name.c_str());
            stream << b << " ";
            switch (arg->type) {
                case kOptionTypeOptional: { stream << "o"; } break;
                case kOptionTypeRequired: { stream << "r"; } break;
                case kOptionTypeFlag: { stream << "f"; } break;
            }
            
            stream << " " << arg->help;
            if (arg->type == kOptionTypeOptional)
            {
                stream << " (default=" << arg->value->str() << ")";
            }
            stream << std::endl;
        }
    }
}

void ArgumentParser::dump(std::ostream &stream)
{
    std::set<ArgumentOptionPtr> visits;
    for (auto iter = arguments.begin(); iter != arguments.end(); iter++)
    {
        if (visits.find(iter->second) == visits.end())
        {
            visits.insert(iter->second);
            
            auto arg = iter->second;
            auto iter = options.lower_bound(arg->name);
            if (iter == options.end())
            {
                if (arg->type == kOptionTypeOptional)
                {
                    stream << arg->name << "=" << arg->value->str() << " ";
                }
                continue;
            }
            
            while (iter != options.end() && iter->first == arg->name)
            {
                stream << arg->name << "=" << iter->second->str() << " ";
                ++iter;
            }
        }
    }
    
    for (auto iter = args.begin(); iter != args.end(); iter++)
    {
        stream << iter->c_str() << " ";
    }
    
    stream << std::endl;
}

void ArgumentParser::add(std::string name, std::string abbr, ValuePtr value, std::string help, std::function<void (ArgumentOptionPtr)> post)
{
    if (name == "--help" || abbr == "-h")
    {
        throw parse_exception("--help and -h option names are reserved");
    }
    
    auto o = std::make_shared<ArgumentOption>(value, name, abbr, help);
    arguments[name] = o;
    arguments[abbr] = o;
    
    if (post) { post(o); }
}
