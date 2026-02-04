#include "stdexcept"

#include "app/app.h"

int main()
{
    try
    {
        App app;
        app.run();
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "Fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}
