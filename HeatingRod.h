#pragma once
#include <functional>
#include <ctime>
#include <PowerSink.h>

class HeatingRod: public PowerSink{
public:
    HeatingRod(std::string name, float power);
    HeatingRod(const HeatingRod& other) = delete;
    virtual ~HeatingRod() = default;

    virtual float using_power() override;
    virtual bool allow_power(float power) override;

    struct Timing{
        clock_t min_on = 0;
        clock_t max_on = 1000*60*60*3;
        clock_t min_off = 0;
    } timing;

    struct TemperatureHysteresis{
        float min = 55;
        float max = 65;
    } temperature_hysteresis;

    enum State{
        ready,
        cool_down,
        min_time_on
    } state = State::ready;

    void set_switch_power(std::function<void(bool)> switch_power);
    std::function<float()> read_temperature;

    bool on = false;

    clock_t on_time() const;
    clock_t off_time() const;

    virtual Json::Value serialize() override;

protected:

private:
    bool turn_on();
    bool turn_off();
    bool check_max_on();

    float last_read_temperature;
    clock_t time_turn_on = -1;
    clock_t time_turn_off = -1;

    std::function<void(bool)> switch_power;
};
