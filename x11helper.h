#ifndef X11HELPER_H
#define X11HELPER_H

extern "C" {
struct _XDisplay;
}
class X11Helper
{
public:
    X11Helper();
    static void doit(struct _XDisplay *, unsigned long);
};

#endif // X11HELPER_H
