#pragma once
#include <functional>
#include <ctime>
#include <PowerSink.h>

class HeatingRod: public PowerSink{
public:
    HeatingRod(std::string name, float power);
    virtual ~HeatingRod() = default;

    virtual float using_power() override;
    virtual bool allow_power(float power) override;

    struct Timing{
        clock_t min_on = 0;
        clock_t max_on = 5000;//std::numeric_limits<clock_t>::max();
        clock_t min_off = 0;
    } timing;

    struct TemperatureHysteresis{
        float min = 45;
        float max = 50;
        enum State{
            under,
            over
        } state = State::under;
    } temperature_hysteresis;

    void init_temperature_hysteresis();

    enum State{
        ready,
        cool_down,
        min_time_on
    } state = State::ready;

    std::function<void(bool)> switch_power;
    std::function<float()> read_temperature;

    bool on;

protected:

private:
    bool turn_on();
    bool turn_off();
    bool check_max_on();

    float last_read_temperature;
    clock_t time_turn_on = 0;
    clock_t time_turn_off = 0;
};
