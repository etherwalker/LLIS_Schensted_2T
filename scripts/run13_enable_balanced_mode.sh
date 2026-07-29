#!/bin/bash

for gov in schedutil ondemand powersave; do echo $gov | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor 2>/dev/null && break; done