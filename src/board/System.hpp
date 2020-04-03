#pragma once

class System;

#include <Arduino.h>

#include "LoggerTask.hpp"
#include "RadioTask.hpp"
#include "TestTask.hpp"
#include "RXTask.hpp"

class System
{
public:
    class Tasks
    {
    public:
        LoggerTask logger = LoggerTask(5); //logs to USB/SD
        RadioTask radio = RadioTask(2);    //controls radio
        TestTask test = TestTask(3);       //test
        RXTask rx = RXTask(4);
    };

    Tasks tasks;
};

#include "main.hpp"