#include "HeatingRod.h"
#include <iostream>

const std::string HeatingRod::type = "heatingRod";

HeatingRod::HeatingRod(std::string name, float power): PowerSink(name){
    set_time_provider(*TimeProvider::getInstance());
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
    milliseconds time_now = timeprovider->get_time();
    bool init_condition = time_turn_off.count() < 0;
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
            log("already turned on");
        }
        state = State::ready;
        return true;
    }
    // blocked by cooldown
    state = State::cool_down;
    log("blocked by cool down");
    return false;
}

bool HeatingRod::turn_off(){
    milliseconds time_now = timeprovider->get_time();
    bool init_condition = time_turn_on.count() < 0;
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
            log("already turned off");
        }
        state = State::ready;
        return true;
    }
    // blocked by min time on
    state = State::min_time_on;
    log("blocked by min time on");
    return false;
}

bool HeatingRod::check_max_on(){
    if(time_turn_on.count() < 0)
        return false;
    if(timing.max_on.count() == 0)
        return false;
    milliseconds time_now = timeprovider->get_time();
    if(on && time_now > (time_turn_on + timing.max_on)){
        return true;
    }
    return false;
}

bool HeatingRod::allow_power(float power){
    log("---------------------------");
    if(!switch_power)
        return false;

    if(!read_temperature)
        return false;

    if(check_max_on()){
        log("Max turned on -> force switch off");
        turn_off();
        return false;
    }

    if(power < get_requesting_power().get_min()){
        log("Power not enough");
        return turn_off();
    }
    log("Power enough");

    last_read_temperature = read_temperature();
    if(last_read_temperature >= temperature_hysteresis.max){
        log("Heating not needed");
        turn_off();
        return false;
    }
    else if(last_read_temperature < temperature_hysteresis.min){
        log("Heating needed");
        return turn_on();
    }
    else{
        log("Inbetween hysteresis limits");
        if(first_allow){
            first_allow = false;
            return turn_on();
        }
        return false;
    }
}

milliseconds HeatingRod::on_time(){
    if(time_turn_on.count() < 0)
        return time_turn_on;
    if(!on)
        return timing.on;
    milliseconds time_now = timeprovider->get_time();
    milliseconds result = time_now - time_turn_on;
    timing.on = result;
    return result;
}

milliseconds HeatingRod::off_time(){
    if(time_turn_off.count() < 0)
        return time_turn_off;
    if(on)
        return timing.off;
    milliseconds time_now = timeprovider->get_time();
    milliseconds result = time_now - time_turn_off;
    timing.off = result;
    return result;
}

Json::Value HeatingRod::serialize(){
    Json::Value result = PowerSink::serialize();
    result["type"] = type;

    Json::Value timing_json;
    timing_json["on_time"] = static_cast<int32_t>(on_time().count());
    timing_json["off_time"] = static_cast<int32_t>(off_time().count());
    timing_json["min_on"] = static_cast<int32_t>(timing.min_on.count());
    timing_json["max_on"] = static_cast<int32_t>(timing.max_on.count());
    timing_json["min_off"] = static_cast<int32_t>(timing.min_off.count());
    result["timing"] = timing_json;

    Json::Value temperature_json;
    temperature_json["sensor"] = read_temperature();
    temperature_json["min"] = temperature_hysteresis.min;
    temperature_json["max"] = temperature_hysteresis.max;
    result["temperature"] = temperature_json;

    result["state"] = state;
    return result;
}

void HeatingRod::enable_log(){
    _enable_log = true;
}

void HeatingRod::disable_log(){
    _enable_log = false;
}

void HeatingRod::set_time_provider(TimeProvider& _timeprovider){
    timeprovider = &_timeprovider;
}

void HeatingRod::log(std::string message) const
{
    if(_enable_log)
        std::cout << "-- "<< name << ": " << message << std::endl;
}
