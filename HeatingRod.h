#pragma once
#include <functional>
#include <PowerSink.h>
#include <TimeProvider.h>


class HeatingRod: public PowerSink{
public:
    HeatingRod(std::string name, float power);
    HeatingRod(const HeatingRod& other) = delete;
    virtual ~HeatingRod() = default;

    static const std::string type;

    virtual float using_power() const override;
    virtual bool allow_power(float power) override;

    struct Timing{
        milliseconds min_on{0};
        milliseconds max_on{1000*60*60*3}; // set zero for infinity
        milliseconds min_off{0};
        milliseconds on{0};
        milliseconds off{0};
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

    milliseconds on_time();
    milliseconds off_time();

    virtual Json::Value serialize() override;

    void enable_log();
    void disable_log();

    void set_time_provider(TimeProvider& timeprovider);

protected:
    TimeProvider* timeprovider;

private:
    bool turn_on();
    bool turn_off();
    bool check_max_on();

    float last_read_temperature;
    milliseconds time_turn_on{-1};
    milliseconds time_turn_off{-1};

    std::function<void(bool)> switch_power;

    bool first_allow = true;

    bool _enable_log{false};

    virtual void log(std::string message) const;
};
