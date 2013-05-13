#include "test.h"

class maddTest : public Test
{
    const char *function() const { return "madd(2,3)"; }

    Mat computeBaseline(const Mat &src) const
    {
        Mat dst;
        src.convertTo(dst, src.depth(), 2, 3);
        return dst;
    }
};

int main(int argc, char *argv[])
{
    (void) argc; (void) argv;
    maddTest t;
    return t.run();
}
