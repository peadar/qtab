// A pretty distilled JSON parser.
#ifndef PME_JSON_H
#define PME_JSON_H

#include <cctype>
#include <cmath>
#include <istream>
#include <sstream>

namespace JSON {

class InvalidJSON : public std::exception {
    std::string err;
public:
    const char *what() const throw() { return err.c_str(); }
    InvalidJSON(const std::string &err_) throw() : err(err_) {}
    ~InvalidJSON() throw() {};
};

enum Type { Array, Boolean, Null, Number, Object, String, Eof, JSONTypeCount };

static inline int
skipSpace(std::istream &l)
{
    while (!l.eof() && isspace(l.peek()))
        l.ignore();
    return l.eof() ? -1 : l.peek();
}

static inline char
expectAfterSpace(std::istream &l, char expected)
{
    char c = skipSpace(l);
    if (c != expected)
        throw InvalidJSON(std::string("expected '") + expected + "', got '" + c + "'");
    l.ignore();
    return c;
}

static inline void
skipText(std::istream &l, const char *text)
{
    for (size_t i = 0; text[i]; ++i) {
        char c;
        l.get(c);
        if (c != text[i])
            throw InvalidJSON(std::string("expected '") + text +  "'");
    }
}

static inline Type
peekType(std::istream &l)
{
    char c = skipSpace(l);
    switch (c) {
        case '{': return Object;
        case '[': return Array;
        case '"': return String;
        case '-': return Number;
        case 't' : case 'f': return Boolean;
        case 'n' : return Null;
        case -1: return Eof;
        default: {
            if (c >= '0' && c <= '9')
                return Number;
            throw InvalidJSON(std::string("unexpected token '") + char(c) + "' at start of JSON object");
        }
    }
}

template <typename Context> void parseObject(std::istream &l, Context &&ctx);
template <typename Context> void parseArray(std::istream &l, Context &&ctx);

template <typename I> I
parseInt(std::istream &l)
{
    int sign;
    char c;
    if (skipSpace(l) == '-') {
        sign = -1;
        l.ignore();
    } else {
        sign = 1;
    }
    I rv = 0;
    if (l.peek() == '0') {
        l.ignore(); // leading zero.
    } else if (isdigit(l.peek())) {
        while (isdigit(c = l.peek())) {
            rv = rv * 10 + c - '0';
            l.ignore();
        }
    } else {
        throw InvalidJSON("expected digit");
    }
    return rv * sign;
}

/*
 * Note that you can use parseInt instead when you know the value will be
 * integral.
 */

template <typename FloatType> static inline FloatType
parseFloat(std::istream &l)
{
    FloatType rv = parseInt<FloatType>(l);
    if (l.peek() == '.') {
        l.ignore();
        FloatType scale = rv < 0 ? -1 : 1;
        char c;
        while (isdigit(c = l.peek())) {
            l.ignore();
            scale /= 10;
            rv = rv + scale * (c - '0');
        }
    }
    if (l.peek() == 'e' || l.peek() == 'E') {
        l.ignore();
        int sign;
        char c = l.peek();
        if (c == '+' || c == '-') {
            sign = c == '+' ? 1 : -1;
            l.ignore();
            c = l.peek();
        } else if (isdigit(c)) {
            sign = 1;
        } else {
            throw InvalidJSON("expected sign or numeric after exponent");
        }
        auto exponent = sign * parseInt<int>(l);
        rv *= std::pow(10.0, exponent);
    }
    return rv;
}

template <typename Integer> Integer parseNumber(std::istream &i) { return parseInt<long double>(i); }
template <> double parseNumber<double> (std::istream &i) { return parseFloat<double>(i); }
template <> float parseNumber<float> (std::istream &i) { return parseFloat<float>(i); }
template <> long double parseNumber<long double> (std::istream &i) { return parseFloat<long double>(i); }

static std::string
parseString(std::istream &l)
{
    expectAfterSpace(l, '"');
    std::stringstream rv;
    for (;;) {
        char c;
        l.get(c);
        if (c == '"')
            break;
        if (c == '\\')
            l.get(c);
        rv <<  c;
    }
    return rv.str();
}

static inline bool
parseBoolean(std::istream &l)
{
    char c = skipSpace(l);
    switch (c) {
        case 't': skipText(l, "true"); return true;
        case 'f': skipText(l, "false"); return false;
        default: throw InvalidJSON("expected 'true' or 'false'");
    }
}

static inline void
parseNull(std::istream &l)
{
    skipSpace(l);
    skipText(l, "null");
}

static inline void // Parse any value but discard the result.
parseValue(std::istream &l)
{
    switch (peekType(l)) {
        case Array: parseArray(l, [](std::istream &l) -> void { parseValue(l); }); break;
        case Boolean: parseBoolean(l); break;
        case Null: parseNull(l); break;
        case Number: parseNumber<float>(l); break;
        case Object: parseObject(l, [](std::istream &l, std::string) -> void { parseValue(l); }); break;
        case String: parseString(l); break;
        default: throw InvalidJSON("unknown type for JSON construct");
    }
}

template <typename Context> void
parseObject(std::istream &l, Context &&ctx)
{
    expectAfterSpace(l, '{');
    for (;;) {
        std::string fieldName;
        char c;
        switch (c = skipSpace(l)) {
            case '"': // Name of next field.
                fieldName = parseString(l);
                expectAfterSpace(l, ':');
                ctx(l, fieldName);
                break;
            case '}': // End of this object
                l.ignore();
                return;
            case ',': // Separator to next field
                l.ignore();
                break;
            default: {
                throw InvalidJSON(std::string("unexpected character '") + char(c) + "' parsing object");
            }
        }
    }
}

template <typename Context> void
parseArray(std::istream &l, Context &&ctx)
{
    expectAfterSpace(l, '[');
    char c;
    if ((c = skipSpace(l)) == ']') {
        l.ignore();
        return; // empty array
    }
    for (size_t i = 0;; i++) {
        skipSpace(l);
        ctx(l);
        c = skipSpace(l);
        switch (c) {
            case ']':
                l.ignore();
                return;
            case ',':
                l.ignore();
                break;
            default:
                throw InvalidJSON(std::string("expected ']' or ',', got '") + c + "'");
        }
    }
}

std::string escape(std::string i)
{
    std::ostringstream o;
    for (auto c : i) {
        if (c == '\"' || c == '\\')
            o << '\\' << c;
        else if (unsigned(c) < 32 || unsigned(c) >= 0x7f && unsigned(c) < 0xa0)
            o << "\\u" << std::hex << unsigned(c);
        else
            o << c;
    }
    return o.str();
}
}


static inline std::ostream &
operator<<(std::ostream &os, const JSON::Type &t)
{
    switch (t) {
        case JSON::Array: os << "Array"; break;
        case JSON::Number: os << "Number"; break;
        case JSON::Object: os << "Object"; break;
        case JSON::String: os << "String"; break;
        default: throw JSON::InvalidJSON("not a JSON type");
    }
    return os;
}
#endif
