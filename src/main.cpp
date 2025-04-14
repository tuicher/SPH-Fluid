// main.cpp
#include "./graphics/Renderer.h"
#include "./physics/PBF_System.h"

int main()
{
    //PBF_System system = PBF_System();

    Renderer app(1280, 720, "PBF-Fluid");
    app.Run();

    return 0;
}
