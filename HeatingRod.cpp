#include "HeatingRod.h"
#include <iostream>

HeatingRod::HeatingRod(std::string name, float power): PowerSink(name){
    set_requesting_power(power, power);
}

float HeatingRod::using_power(){
    return get_allowed_power();
}

bool HeatingRod::turn_on(){
    clock_t time_now = clock();
    if(time_now > time_turn_off + timing.min_off){
        if(!on){
            time_turn_on = time_now;
            set_allowed_power(get_requesting_power().get_min());
            switch_power(true);
            on = true;
        }
        else{
            // already turned on
            std::cout << "already turned on" << std::endl;
        }
        state = State::ready;
        return true;
    }
    // blocked by cool download
    state = State::cool_down;
    std::cout << "blocked by cool down" << std::endl;
    return false;
}

bool HeatingRod::turn_off(){
    clock_t time_now = clock();
    if(time_now > time_turn_on + timing.min_on){
        if(on){
            time_turn_off = time_now;
            set_allowed_power(0);
            switch_power(false);
            on = false;
        }
        else{
            // already turned off
            std::cout << "already turned off" << std::endl;
        }
        state = State::ready;
        return true;
    }
    // blocked by min time on
    state = State::min_time_on;
    std::cout << "blocked by min time on" << std::endl;
    return false;
}

bool HeatingRod::check_max_on(){
    clock_t time_now = clock();
    if(time_now > time_turn_on + timing.max_on){
        return true;
    }
    return false;
}

bool HeatingRod::allow_power(float power){
    std::cout << "---------------------------" << std::endl;
    if(!switch_power)
        return false;

    if(!read_temperature)
        return false;

    if(check_max_on()){
        std::cout << "Max turned on -> force switch off" << std::endl;
        turn_off();
        return false;
    }

    if(power < get_requesting_power().get_min()){
        std::cout << "Power not enough" << std::endl;
        return turn_off();
    }
    std::cout << "Power enough" << std::endl;

    last_read_temperature = read_temperature();
    if(last_read_temperature > temperature_hysteresis.max){
        std::cout << "Heating not needed" << std::endl;
        turn_off();
        return false;
    }
    else if(last_read_temperature < temperature_hysteresis.min){
        std::cout << "Heating needed" << std::endl;
        return turn_on();
    }
    else{
        return true;
    }
}
