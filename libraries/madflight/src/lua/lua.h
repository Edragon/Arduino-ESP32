#pragma once

#if MF_LUA_ENABLE

#include "luawrap.h"
#include "../hal/hal.h" //xTaskCreate, vTaskDelete
#include "../bbx/bbx.h"

class LuaClass {
private:
  TaskHandle_t xHandle;
  static uint8_t* code;

  static void lua_task(void *pvParameters) {
    (void)pvParameters;
    luawrap_run((const char*)LuaClass::code); //should not return
    free(LuaClass::code);
    vTaskDelete(NULL);
  }

public:
  void begin() {
    int len = bbx.read("/madflight.lua", &LuaClass::code);
    if(len <= 0) {
      Serial.println("LUA: madflight.lua not found");
      return;
    }
    
    if(xTaskCreate(lua_task, "LUA", MF_FREERTOS_DEFAULT_STACK_SIZE, NULL, uxTaskPriorityGet(NULL), &xHandle) != pdPASS ){
      Serial.println("LUA: Task creation failed");
      return;
    }

    Serial.println("LUA: executing madflight.lua");
  }
};

uint8_t* LuaClass::code = nullptr;

#else // #if MF_LUA_ENABLE

class LuaClass {
public:
  void begin() {}
};

#endif // #if MF_LUA_ENABLE


LuaClass lua;
