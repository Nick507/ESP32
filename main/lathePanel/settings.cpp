#include "settings.h"
#include "controller.h"
#include "grblConnect.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern "C" {
#include "../grbl/grbl.h"
#include "../grbl/settings.h"
#include "../grbl/system.h"

// External reference to GRBL settings
extern settings_t settings;
}

// Setting IDs for backlash compensation
#define SETTING_BACKLASH_X  ((setting_id_t)160)
#define SETTING_BACKLASH_Y  ((setting_id_t)161)
#define SETTING_BACKLASH_Z  ((setting_id_t)162)

// UI Components - using NamedUnitsValue for clickable editing (like winding.cpp)
// These directly bind to settings.axis[].backlash in GRBL's global settings structure
NamedUnitsValue xBacklashNUV(10, 10, nvFloat, &settings.axis[X_AXIS].backlash, "%7.3f", GUI_BUTTON_DEFAULT_FONT, "X Backlash", "mm", NULL);
NamedUnitsValue yBacklashNUV(10, 10 + 38, nvFloat, &settings.axis[Y_AXIS].backlash, "%7.3f", GUI_BUTTON_DEFAULT_FONT, "Y Backlash", "mm", NULL);
NamedUnitsValue zBacklashNUV(10, 10 + 38 * 2, nvFloat, &settings.axis[Z_AXIS].backlash, "%7.3f", GUI_BUTTON_DEFAULT_FONT, "Z Backlash", "mm", NULL);

// Forward declarations
void settingsBackButtonCB(UserEvent event, RectangleObject * obj);
void settingsSaveButtonCB(UserEvent event, RectangleObject * obj);

Button settingsBackButton(10, 260, 100, 50, "Back", GUI_BUTTON_DEFAULT_FONT, settingsBackButtonCB);
Button settingsSaveButton(370, 260, 100, 50, "Save", GUI_BUTTON_DEFAULT_FONT, settingsSaveButtonCB);

RectangleObject * settingsWindowObjects[] = {
    &xBacklashNUV,
    &yBacklashNUV,
    &zBacklashNUV,
    &settingsBackButton, &settingsSaveButton,
    NULL
};

Window settingsWindow(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, settingsWindowObjects);

// Redraw all widgets
static void refreshDisplay()
{
    xBacklashNUV.setNeedToRedraw(true);
    yBacklashNUV.setNeedToRedraw(true);
    zBacklashNUV.setNeedToRedraw(true);
}

// Save all settings to GRBL and persist to EEPROM
void settingsSaveButtonCB(UserEvent event, RectangleObject * obj)
{
    // Values are already updated in settings.axis[].backlash by the NumericValueEditor
    // We just need to persist to EEPROM/NVS
    settings_write_global();
    printf("Backlash settings saved: X=%.3f Y=%.3f Z=%.3f\n", 
           settings.axis[X_AXIS].backlash,
           settings.axis[Y_AXIS].backlash,
           settings.axis[Z_AXIS].backlash);
    
    // Go back to main window
    guiChangeWindow(&mainWindow);
}

// Back button callback
void settingsBackButtonCB(UserEvent event, RectangleObject * obj)
{
    // Explicitly navigate to main window (guiBackWindow won't work after numeric editor)
    guiChangeWindow(&mainWindow);
}

// Settings window button callback
void settingsWindowButtonCB(UserEvent event, RectangleObject * obj)
{
    // Values are directly bound to settings.axis[].backlash, so they're always current
    refreshDisplay();
    
    // Change to settings window
    guiChangeWindow(&settingsWindow);
}

// Initialize settings window
void settingsWindowInit()
{
    // Nothing to initialize - values are directly bound to GRBL settings
}
