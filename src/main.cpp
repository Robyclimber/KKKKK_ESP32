#include "App.h"
#include "AppConstants.h"

namespace
{
App app;
}

void setup()
{
    Serial.begin(AppConstants::SerialBaudRate);
    delay(250);
    app.setup();
}

void loop()
{
    app.loop();
}
