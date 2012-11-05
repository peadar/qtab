/*
 * A primitive framework for parsing JSON.
 * To use:
 *
 * Step 1:
 *  Create an object of a some type Lookable, that allows you to read
 *  content one byte at a time.  The type needs, two methods,
 *  "int sgetc()" , and "int sbumpc()". The sgetc() method peeks at the next
 *  byte in the stream and returns it. The "sbumpc()" method consumes the
 *  next byte in the stream and returns it. This interface is compatbile with
 *  std::streambuf.
 *
 * Step 2:
 *  Call one of the parse functions . The JSON stream in Lookable
 *  must match the type inferred by the function. Eg, if you call "parseInt()",
 *  then the JSON stream must present an integer next.
 *  The parse functions are:
 *      JSON::parseString()
 *      JSON::parseFloat()
 *      JSON::parseInt()
 *      JSON::parseArray()
 *      JSON::parseObject()
 *
 *  The first argument is a reference to the Lookable above.
 *
 *  The parsers for scalar types (string, number) take a reference to a similar
 *  C++ type (eg, parseString(std::string &) that gets populated as their second
 *  argument.
 *
 *  The methods for the non-scalar types (arrays and objects) take a second argument
 *  of a further templated type: The objects of these templated types are called with
 *  either the name of the field of the object to be parsed, or the index into the
 *  array being parsed. The user can provide a function or a functor object to 
 *  satsfy the requirements.
 * 
 *
 * Notes:
 *  JSON::peekType() allows you to get some details of the type of the next object
 *  in the stream, for those cases where the context alone is not enough to 
 *  infer type
 *
 *  "JSON::parseValue()" will skip over any content you don't need to process. For
 *  example, if you are presented with a field of an object in "parseField()"
 *  with a name you don't recognise, you can simply call "parseValue()" to ignore the
 *  data for that field.
 */

#ifndef PME_JSON_H
#define PME_JSON_H

#include <iostream>
#include <sstream>
#include <string>
#include <math.h>
#include "exception.h"
namespace JSON {

enum Type {
    Array,
    Boolean,
    Null,
    Number,
    Object,
    String,
    None
};

template <typename Lookable, typename Context> void parseObject(Lookable &l, Context &ctx);

// Thrown for ill-formed JSON content.
class InvalidJSON : public Exception { };

template <typename Lookable> char
peekChar(Lookable &l)
{
    int rc = l.sgetc();
    if (rc == Lookable::traits_type::eof()) {
        InvalidJSON fmt;
        fmt << "I/O error reading JSON";
        throw fmt;
    }
    return rc;
}

template <typename Lookable> char
readChar(Lookable &l)
{
    char c = peekChar();
    l.sbumpc();
    return c;
}

template <typename Lookable> char
skipspace(Lookable &l)
{
    char c;
    while (isspace(c = peekChar(l)))
        c = readChar(l);
    return c;
}

template <typename Lookable> Type
peekType(Lookable &l)
{
    char c = skipspace(l);
    switch (c) {
        case '{': return Object;
        case '[': return Array;
        case '"': return String;
        case '-': return Number;
        case 't' : case 'f': return Boolean;
        case 'n' : return Null;
        default: {
            if (c >= '0' && c <= '9')
                return Number;
            InvalidJSON fmt;
            fmt << "unexpected token '" << c << "'";
            throw fmt;
        }
    }
}

template <typename Lookable> void
skipText(Lookable &l, const char *text)
{
    for (size_t i = 0; text[i]; ++i) {
        char c = readChar(l);
        if (c != text[i]) {
            InvalidJSON fmt;
            fmt << "expected '" << text <<  "'";
            throw fmt;
        }
    }
}

template <typename Lookable> void
parseBoolean(Lookable &l, bool &in)
{
    char c = skipspace(l);
    static const char trueText[] = "true";
    static const char falseText[] = "false";
    const char *text;

    bool value;
    switch (c) {
        case 't': text = trueText, value = true; break;
        case 'f': text = falseText, value = false; break;
        default: {
            InvalidJSON fmt;
            fmt << "expected 'true' or 'false'";
            throw fmt;
        }
    }
    skipText(l, text);
    in = value;
}

template <typename Lookable> void
parseNull(Lookable &l)
{
    skipspace(l);
    skipText(l, "null");
}

template <typename Lookable> void
parseString(Lookable &l, std::string &in)
{
    char c = skipspace(l);
    if (c != '"') {
        InvalidJSON fmt;
        fmt << "expected start of string, got " << c;
        throw fmt;
    }
    c = readChar(l);
    std::stringstream strm;
    for (;;) {
        c = readChar(l);
        if (c == '"')
            break;
        if (c == '\\')
            c = readChar(l);
        strm <<  c;
    }
    in = strm.str();
}

template <typename Lookable, typename I> void
parseInt(Lookable &l, I &in)
{
    int sign = 1;
    char c = peekChar(l);
    if (c == ' ') { // skip over white space
        readChar(l);
        c = peekChar(l);
    }
    if (c == '-') {
        readChar(l);
        c = peekChar(l);
        sign = -1;
    }
    for (in = 0; isdigit(c); c = peekChar(l)) {
        c = readChar(l);
        in *= 10;
        in += c - '0';
    };
    in *= sign;
}

template <typename Lookable> void
parseFloat(Lookable &l, double &in)
{
    long intPart;
    parseInt(l, intPart);
    in = intPart;
    char c = peekChar(l);
    if (c == '.') {
        c = readChar(l);
        int frac = 0;
        int scale = 1;
        while (isdigit(c = peekChar(l))) {
            c = readChar(l);
            frac = frac * 10 + c - '0';
            scale *= 10;
        }
        in += double(frac) / scale * (in < 0 ? - 1 : 1);
    }
    if (c == 'e' || c == 'E') {
        c = readChar(l);
        int sign;
        c = peekChar(l);
        if (c == '+' || c == '-') {
            sign = c;
            readChar(l);
            c = peekChar(l);
        } else if (isdigit(c)) {
            sign = '+';
        } else {
            InvalidJSON fmt;
            fmt << "expected sign or numeric after exponent";
            throw fmt;
        }
        int exponent;
        parseInt(l, exponent);
        switch (sign) {
            case '+': in *= pow(10.0, exponent); break;
            case '-': in /= pow(10.0, exponent); break;
        }
    }
}

template <typename Lookable> void
parseValue(Lookable &l)
{
    switch (peekType(l)) {
        case None: {
            InvalidJSON fmt;
            fmt << "unknown type for JSON construct";
            throw fmt;
        }
        case Object: {
            static std::function<void(Lookable &, std::string)> skipField = 
                [](Lookable &l, std::string) -> void { parseValue(l); };
            parseObject(l, skipField);
            break;
        }
        case String: {
            std::string str;
            parseString(l, str);
            break;
        }
        case Array: {
            static std::function<void(Lookable &, size_t)> skipElement = 
                [](Lookable &l, size_t) -> void { parseValue(l); };
            parseArray(l, skipElement);
            break;
        }
        case Number: {
            double d;
            parseFloat(l, d);
            break;
        }
        case Boolean: {
            bool b;
            parseBoolean(l, b);
            break;
        }
        case Null: {
            parseNull(l);
            break;
        }
    }
}

template <typename Lookable, typename Context> void
parseObject(Lookable &l, Context &ctx)
{
    skipspace(l);
    char c;
    c = readChar(l);
    if (c != '{') {
        InvalidJSON fmt;
        fmt << "expected '{', got '" << c << "'";
        throw fmt;
    }
    for (;;) {
        std::string fieldName;
        switch (c = skipspace(l)) {
            case '"': // Name of next field.
                parseString(l, fieldName);
                skipspace(l);
                c = readChar(l);
                if (c != ':') {
                    InvalidJSON fmt;
                    fmt << "expected ':', got '" << char(c) << "'";
                    throw fmt;
                }
                ctx(l, fieldName);
                break;
            case '}': // End of this object
                readChar(l);
                return;
            case ',': // Separator to next field
                readChar(l);
                break;
            default: {
                InvalidJSON fmt;
                fmt << "unexpected character '" << char(c) << "' parsing object";
                throw fmt;
            }
        }
    }
}

template <class Lookable, typename Context> void
parseArray(Lookable &l, Context &ctx)
{
    char c = skipspace(l);
    if (c != '[') {
        InvalidJSON fmt;
        fmt << "expected '['";
        throw fmt;
    }
    c = readChar(l);
    if ((c = skipspace(l)) == ']') {
        readChar(l);
        return; // empty array
    }
    for (size_t i = 0;; i++) {
        skipspace(l);
        ctx(l, i);
        skipspace(l);
        c = readChar(l);
        switch (c) {
            case ']':
                return;
            case ',':
                break;
            default: {
                InvalidJSON fmt;
                fmt << "expected ']' or ',', got '" << c << "'";
                throw fmt;
            }
        }
    }
}

template <class Looker
         , typename Element
         , typename Parser = void (*)(Looker &, Element &)
         , typename Container = std::vector<Element> >
struct ElementsOf {
    Parser p;
    Container *vec;
public:
    void operator() (Looker &l, size_t) const {
        vec->resize(vec->size() + 1);
        p(l, vec->back());
    }
    ElementsOf(Parser p_, Container &vec_) : p(p_), vec(&vec_) {}
};

static inline std::ostream &
operator<<(std::ostream &os, const Type &t)
{
    switch (t) {
        case Array: os << "Array"; break;
        case Object: os << "Object"; break;
        case Number: os << "Number"; break;
        case String: os << "String"; break;
        case NOTYPE:
        default: {
            Exception e;
            e << "inavlid type";
            throw e;
        }
    }
    return os;
}
}

#endif
