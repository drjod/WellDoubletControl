#!/bin/bash

simulator_folder=../testingEnvironment/rzcluster/ogs/ogs_kb1
wdc_folder=$simulator_folder/Libs/WellDoubletControl

mkdir -p $wdc_folder

cp build/src/libwellDoubletControl.a $wdc_folder
cp src/*.h $wdc_folder
cp build/wdc_config.h $wdc_folder
