#include "programflow.h"
#include "main.cpp"

void flow_handler()
{
    current_state = next_state;
    switch(current_state)
    {
        case(IDLE):
            state_idle();
            break;

        case(MASTER_PRESS):
            next_state = MASTER_PRESS;
            state_master_button_press();
            break;

        case(TOGGLE_LASER):
            next_state = IDLE;
            state_toggle_laser();

        case(TAKE_SHOT):
            next_state = IDLE;
            state_take_shot();
            break;

        case(ACCEL_CALIB):
            next_state = IDLE;
            state_accel_calibration();
            break;

        case(MAG_CALIB):
            next_state = IDLE;
            state_mag_calibration();
            break;

        case(LASER_ALIGN):
            next_state = IDLE;
            state_laser_align();
            break;
    }
}

void state_idle(){
    char cmd[30];
    blehandler.shared_bledata.read_command(cmd);
    if (strcmp(cmd, "calibrate accel") == 0 )
    {
        next_state = ACCEL_CALIB;
    } else if (strcmp(cmd, "calibrate mag") == 0 )
    {
        magnetometer.init();
        next_state = MAG_CALIB;
    } else if (strcmp(cmd, "align laser") == 0 )
    {
        next_state = LASER_ALIGN;
    } else if (interrupt_button_pressed)
    {
        next_state = MASTER_PRESS;
        reset_flow_interrupt_flags();
        start_shot_interrupt_timer();
    }
}

void state_master_button_press()
{
    if (interrupt_button_released)
    {
        stop_shot_interrupt_timer();
        reset_flow_interrupt_flags();
        next_state = TOGGLE_LASER;
    } else if (interrupt_get_shot)
    {
        stop_shot_interrupt_timer();
        reset_flow_interrupt_flags();
        next_state = TAKE_SHOT;
    }
}

bool state_accel_calibration()
{
    if (!CALIB_FUNC())
    {
        next_state = ACCEL_CALIB;
    }
}

bool state_mag_calibration()
{
    magnetometer.update();
    magnetometer.add_calibration_data();
    if (touchRead(4))
    {
        next_state = IDLE;
        magnetometer.calibrate();
    } else{
        next_state = MAG_CALIB;
    }
}

void state_take_shot()
{
    sensorhandler.update();
    save_shot_to_flash();
    save_shot_to_BLE();
    shot_ID += 1;
}

void state_laser_align(){
    if (interrupt_button_released)
    {
        stop_shot_interrupt_timer();
        reset_flow_interrupt_flags();
        lidar.toggle_laser();
    } else if (interrupt_get_shot)
    {
        stop_shot_interrupt_timer();
        reset_flow_interrupt_flags();
        sensorhandler.update();
    }

    if (!sensorhandler.add_laser_calibration())
    {
        next_state = LASER_ALIGN;
    }
}

void save_shot_to_flash()
{
    Vector3d shot_data; // Variable to hold shot data
    node *n = (struct node*)malloc(sizeof(node)); // Node struct to hold data to be saved
    char str_id[4]; // String formatted shot id

    // Get the shot data
    shot_data = sensorhandler.get_shot_data();

    // Populate node object
    debug(DEBUG_MAIN, "Assigning values to node object");
    n->id = shot_ID;
    n->heading = shot_data(0);
    n->inclination = shot_data(1);
    n->distance = shot_data(2);

    // Populate str_id with string version of int id
    debug(DEBUG_MAIN, "Writing to file");
    sprintf(str_id,"%d",shot_ID);
    write_to_file(current_file_name,str_id,n);
    
    debug(DEBUG_MAIN, "Finished saving data, returning...");
}

void save_shot_to_BLE()
{
    Vector3d shot_data; // Variable to hold shot data
    node *n = (struct node*)malloc(sizeof(node)); // Node struct to hold data to be saved
    char str_id[4]; // String formatted shot id

    // Get the shot data
    shot_data = sensorhandler.get_shot_data();

    // Write data to BLE
    snprintf(str_id,sizeof(str_id),"%d",shot_ID);
    read_from_file(current_file_name,str_id,&n1);
    blehandler.shared_bledata.write_data(&n1);
    blehandler.update();
}

void state_toggle_laser()
{
    lidar.toggle_laser();
}

void state_reset()
{
    next_state = IDLE;
}