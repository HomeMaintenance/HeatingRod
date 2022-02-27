#include "HeatingRod.h"

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
        }
        else{
            // already turned on
        }
        return true;
    }
    // blocked by cool down
    return false;
}

bool HeatingRod::turn_off(){
    clock_t time_now = clock();
    if(time_now > time_turn_on + timing.min_on){
        if(on){
            time_turn_off = time_now;
            set_allowed_power(0);
            switch_power(false);
        }
        else{
            // already turned off
        }
        return true;
    }
    // blocked by min time on
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
    if(!switch_power)
        return false;

    if(!read_temperature)
        return false;

    if(check_max_on()){
        turn_off();
        return false;
    }

    last_read_temperature = read_temperature();
    if(last_read_temperature > temperature_hysteresis.max){
        turn_off();
    }
    else if(last_read_temperature < temperature_hysteresis.min){
        if(power >= get_requesting_power().get_min()){
            return turn_on();
        }
        else{
            turn_off();
        }
    }
    return false;
}
