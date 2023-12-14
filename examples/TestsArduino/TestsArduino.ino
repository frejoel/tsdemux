/**
 * @file ArduinoTests.ino
 * @author Phil Schatzmann 
 * @brief Executes all tests for Arduino
 * This works only on environments that can handle symbolic links properly: linux, os/x
 * @version 0.1
 * @date 2023-09-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "all_tests.h"

void setup(){
    Serial.begin(119200);
    
    test_get_version();

    TESTdata_context();
    TESTparse_cat();
    TESTparse_descriptors();
    TESTparse_packet_headers();
    TESTparse_pat();
    TESTparse_pes_header();
    TESTparse_pmt();
    TESTparse_table();
    TESTregister_pid();
    TESTset_event_callback();
}

void loop(){
}