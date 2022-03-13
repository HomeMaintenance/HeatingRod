#include "HeatingRod.h"
#include <iostream>

const std::string HeatingRod::type = "heatingRod";

HeatingRod::HeatingRod(std::string name, float power): PowerSink(name){
    set_requesting_power(power, power);
}

float HeatingRod::using_power(){
    return get_allowed_power();
}

void HeatingRod::set_switch_power(std::function<void(bool)> _switch_power){
    if(!_switch_power)
        return;
    switch_power = _switch_power;
    switch_power(false);
}

bool HeatingRod::turn_on(){
    clock_t time_now = clock();
    bool init_condition = time_turn_off < 0;
    if(time_now >= (time_turn_off + timing.min_off) || init_condition){
        if(!on){
            off_time(); // update off duration
            time_turn_on = time_now;
            set_allowed_power(get_requesting_power().get_min());
            switch_power(true);
            on = true;
        }
        else{
            // already turned on
            // std::cout << "already turned on" << std::endl;
        }
        state = State::ready;
        return true;
    }
    // blocked by cool download
    state = State::cool_down;
    // std::cout << "blocked by cool down" << std::endl;
    return false;
}

bool HeatingRod::turn_off(){
    clock_t time_now = clock();
    bool init_condition = time_turn_on < 0;
    if(time_now >= (time_turn_on + timing.min_on) || init_condition){
        if(on){
            on_time(); // update on duration
            time_turn_off = time_now;
            set_allowed_power(0);
            switch_power(false);
            on = false;
        }
        else{
            // already turned off
            // std::cout << "already turned off" << std::endl;
        }
        state = State::ready;
        return true;
    }
    // blocked by min time on
    state = State::min_time_on;
    // std::cout << "blocked by min time on" << std::endl;
    return false;
}

bool HeatingRod::check_max_on(){
    if(time_turn_on < 0)
        return false;
    if(timing.max_on == 0)
        return false;
    clock_t time_now = clock();
    if(on && time_now > (time_turn_on + timing.max_on)){
        return true;
    }
    return false;
}

bool HeatingRod::allow_power(float power){
    // std::cout << "---------------------------" << std::endl;
    if(!switch_power)
        return false;

    if(!read_temperature)
        return false;

    if(check_max_on()){
        // std::cout << "Max turned on -> force switch off" << std::endl;
        turn_off();
        return false;
    }

    if(power < get_requesting_power().get_min()){
        // std::cout << "Power not enough" << std::endl;
        return turn_off();
    }
    // std::cout << "Power enough" << std::endl;

    last_read_temperature = read_temperature();
    if(last_read_temperature > temperature_hysteresis.max){
        // std::cout << "Heating not needed" << std::endl;
        turn_off();
        return false;
    }
    else if(last_read_temperature < temperature_hysteresis.min){
        // std::cout << "Heating needed" << std::endl;
        return turn_on();
    }
    else{
        return true;
    }
}

clock_t HeatingRod::on_time(){
    if(time_turn_on < 0)
        return time_turn_on;
    if(!on)
        return timing.on;
    clock_t time_now = clock();
    clock_t result = time_now - time_turn_on;
    timing.on = result;
    return result;
}

clock_t HeatingRod::off_time(){
    if(time_turn_off < 0)
        return time_turn_off;
    if(on)
        return timing.off;
    clock_t time_now = clock();
    clock_t result = time_now - time_turn_off;
    timing.off = result;
    return result;
}

Json::Value HeatingRod::serialize(){
    Json::Value result = PowerSink::serialize();
    result["type"] = type;

    Json::Value timing_json;
    timing_json["on_time"] = static_cast<int32_t>(on_time());
    timing_json["off_time"] = static_cast<int32_t>(off_time());
    timing_json["min_on"] = static_cast<int32_t>(timing.min_on);
    timing_json["max_on"] = static_cast<int32_t>(timing.max_on);
    timing_json["min_off"] = static_cast<int32_t>(timing.min_off);
    result["timing"] = timing_json;

    Json::Value temperature_json;
    temperature_json["sensor"] = read_temperature();
    temperature_json["min"] = temperature_hysteresis.min;
    temperature_json["max"] = temperature_hysteresis.max;
    result["temperature"] = temperature_json;

    result["state"] = state;
    return result;
}
