//
//  main.cpp
//  argparse
//
//  Created by LARRYHOU on 2021/6/12.
//

#include <iostream>

#include "argparse.hpp"

struct ColorARGB {
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

namespace argparse { /* custom more types parsing */
template<> struct Type<uint32_t> { static uint32_t parse(const char* s) { return static_cast<uint32_t>(std::atoll(s)); } };
template<> struct Type<ColorARGB>
{
    static ColorARGB parse(const char* s)
    {
        auto v = static_cast<uint32_t>(strtol(s, nullptr, 0));
        return *(ColorARGB *)&v;
    }
};
}

class ColorValue: public argparse::Value
{
    ColorARGB value;
public:
    ColorValue(uint32_t v)
    {
        memcpy(&value, &v, sizeof(v));
        layout.resize(sizeof(value));
        memcpy(layout.data(), &value, sizeof(value));
        char b[32];
        sprintf(b, "{a:%x, r:%x, g:%x, b:%x}", value.a, value.r, value.g, value.b);
        repr = b;
    }
};

class TemperatureValue: public argparse::Value
{
    double value;
public:
    TemperatureValue(double v): value(v)
    {
        layout.resize(sizeof(value));
        memcpy(layout.data(), &value, sizeof(value));
        repr = std::to_string(value) + "℃";
    }
};

int main(int argc, const char * argv[])
{
    argparse::ArgumentParser parser;
    parser.add_flag("--verbose", "-v", "print verbose message");
    parser.add_optional<int>("--thread-count", "-c", 4, "thread count");
    parser.add_optional<int64_t>("--tt-count", "-tc", 20, "tt count");
    parser.add_optional<double>("--pi", "-p", 3.1415926, "pi");
    parser.add_optional<float>("--quality", "-q", 0.75, "quality", [](std::string s)
    {
        return std::make_shared<argparse::ArgumentValue>(static_cast<float>(100 * std::atof(s.c_str())));
    });
    parser.add_optional<double, TemperatureValue>("--temperature", "-t", 36.5, "--temperature", [](std::string s)
    {
        return std::make_shared<TemperatureValue>(std::atof(s.c_str()));
    });
    parser.add_optional<uint32_t, ColorValue>("--color", "-l", 0x123456FF, "--color", [](std::string s)
    {
        printf("++ %lld %s\n", strtoll(s.c_str(), nullptr, 0), s.c_str());
        return std::make_shared<ColorValue>(strtoll(s.c_str(), nullptr, 0));
    });
    parser.add_optional<bool>("--bool", "-b", false, "bool flag");
    parser.add_required<std::string>("--file", "-f", "input file for processing");
    parser.parse(argc - 1, argv + 1);
    
    std::cout
    << "--verbose=" << parser.get<bool>("--verbose") << " "
    << "--quality=" << parser.get<float>("--quality") << " "
    << "--thread-count=" << parser.get<int>("-c") << " "
    << "--tt-count=" << parser.get<long long>("-tc") << " "
    << "--bool=" << parser.get<int>("-b") << " "
    << "--pi=" << parser.get<double>("-p") << " "
    << "--file=" << parser.get<std::string>("-f") << " "
    << "--temperature=" << parser.get<double>("-t") << " "
    << "--color=" << (int)parser.get<ColorARGB>("--color").a << " "
    << std::endl;
    
    auto files = parser.all<std::string>("-f");
    
    parser.dump();
    return 0;
}
