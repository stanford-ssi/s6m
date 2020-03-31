#pragma once

class System;

#include <Arduino.h>

#include "LoggerTask.hpp"
#include "RadioTask.hpp"

class System
{ 
public:

    class Tasks
    {
    public:
        LoggerTask logger = LoggerTask(1); //logs to USB/SD
        RadioTask radio = RadioTask(2);
    };

    Tasks tasks;
};

#include "main.hpp"