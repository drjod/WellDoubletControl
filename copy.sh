#!/bin/bash

simulator_file=../ogs_kb1
wdc_folder=$simulator_file/Libs/WellDoubletControl

mkdir -p $wdc_folder

cp build/src/libwellDoubletControl.a $wdc_folder
cp src/wellDoubletControl.cpp $wdc_folder
cp src/wellDoubletControl.h $wdc_folder
cp src/comparison.h $wdc_folder
cp build/wdc_config.h $wdc_folder  

