#include <json.h>
#include <iostream>

using namespace JSON;

const char *pad(size_t indent) {
    static const char spaces[] =
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    "
        "                    ";
    indent = std::min(4 * indent, sizeof spaces - 1);
    return spaces + sizeof spaces  - indent - 1;
}

template <typename numtype> static void pretty(std::istream &i, std::ostream &o, size_t indent);

template <typename numtype> void
prettyArray(std::istream &i, std::ostream &o, size_t indent)
{
    o << "[";
    size_t eleCount = 0;
    JSON::parseArray(i, [=, &eleCount, &o] (std::istream &i) -> void {
        o << (eleCount++ ? ", " : "") << "\n" << pad(indent + 1);
        pretty<numtype>(i, o, indent+1);
    });
    if (eleCount)
        o << "\n" << pad(indent);
    o << "]";
}

template <typename numtype> static void
prettyObject(std::istream &i, std::ostream &o, size_t indent)
{
    o << "{";
    int eleCount = 0;
    JSON::parseObject(i, [=, &eleCount, &o] (std::istream &i, std::string idx) -> void {
        if (eleCount++ == 0)
            o << "\n" << pad(indent + 1);
        else
            o << ", \n" << pad(indent + 1);
        o << "\"" << JSON::escape(idx) << "\": ";
        pretty<numtype>(i, o, indent + 1);
    });
    if (eleCount)
        o << "\n" << pad(indent);
    o << "}";
}

static void
prettyString(std::istream &i, std::ostream &o, size_t indent)
{
    o << "\"" << JSON::escape(JSON::parseString(i)) << "\"";
}

template <typename numtype> static void
prettyNumber(std::istream &i, std::ostream &o, size_t indent)
{
    o << JSON::parseNumber<numtype>(i);
}

static void
prettyBoolean(std::istream &i, std::ostream &o, size_t indent)
{
    o << (JSON::parseBoolean(i) ? "true" : "false");
}

template <typename numtype> static void
pretty(std::istream &i, std::ostream &o, size_t indent)
{
    switch (peekType(i)) {
        case Array: prettyArray<numtype>(i, o, indent); return;
        case Object: prettyObject<numtype>(i, o, indent); return;
        case String: prettyString(i, o, indent); return;
        case Number: prettyNumber<numtype>(i, o, indent); return;
        case Boolean: prettyBoolean(i, o, indent); return;
        case Eof: return;
    }
}


int
main(int argc, char *argv[])
{
    std::cin.tie(0);
    pretty<long> (std::cin, std::cout, 0);
    return 0;
}
