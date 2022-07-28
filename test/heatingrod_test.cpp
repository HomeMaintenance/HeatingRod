#include <thread>
#include <chrono>
#include <gtest/gtest.h>
#include <HeatingRod.h>
#include <TimeProviderSim.h>

class HeatingRodTest: public ::testing::Test{
public:
    HeatingRodTest(){}

    virtual void SetUp() override {
        time = TimeProviderSim::getInstance();
        heatingRod = std::make_unique<HeatingRod>("heatingRod0",1000.f);
        heatingRod->set_time_provider(*time);
        heatingRod->timing.max_on = std::chrono::duration<int,std::milli>(0); // infinite
        heatingRod->temperature_hysteresis.max = 50;
        heatingRod->temperature_hysteresis.min = 45;
        heatingRod->set_switch_power([this](bool power){
            std::cout << "\t>>> switch_power to " << power << std::endl;
            switch_pwr_calls.push_back(power);
        });
        std::cout << "hr memory: "<< heatingRod.get() << std::endl;
    }

    virtual void TearDown() override {
    }

    std::unique_ptr<HeatingRod> heatingRod;
    std::vector<bool> switch_pwr_calls;
    TimeProvider* time;
};


TEST_F(HeatingRodTest, TurnOn0){
    // Temperature function is not set
    bool result = heatingRod->allow_power(1200);
    EXPECT_FALSE(result);
    EXPECT_EQ(heatingRod->using_power(), 0.f);
    std::vector<bool> exp_sw_pwr_calls = {false};
    EXPECT_EQ(switch_pwr_calls, exp_sw_pwr_calls);
}

TEST_F(HeatingRodTest, TurnOn1){
    // Temperature is too low
    heatingRod->read_temperature = []()->float{return 42.23f;};
    bool result = heatingRod->allow_power(1200);
    result = result && heatingRod->allow_power(1200);
    result = result && heatingRod->allow_power(1200);
    result = result && heatingRod->allow_power(1200);
    result = result && heatingRod->allow_power(1200);
    EXPECT_TRUE(result);
    float using_pwr_result = heatingRod->using_power();
    float using_pwr_expected = heatingRod->get_requesting_power().get_min();
    EXPECT_EQ(using_pwr_result, using_pwr_expected);

    std::vector<bool> exp_sw_pwr_calls = {false, true};
    EXPECT_EQ(switch_pwr_calls, exp_sw_pwr_calls);
}

TEST_F(HeatingRodTest, TurnOn2){
    // Temperature is high enough
    heatingRod->read_temperature = []()->float{return 53.23f;};
    bool result = heatingRod->allow_power(1200);
    EXPECT_FALSE(result);
    EXPECT_EQ(heatingRod->using_power(), 0.f);

    std::vector<bool> exp_sw_pwr_calls = {false};
    EXPECT_EQ(switch_pwr_calls, exp_sw_pwr_calls);
}

TEST_F(HeatingRodTest, TurnOff0){
    // Got no power -> do nothing
    heatingRod->read_temperature = []()->float{return 42.23f;};
    bool result = heatingRod->allow_power(0);
    result = result && heatingRod->allow_power(0);
    result = result && heatingRod->allow_power(0);
    result = result && heatingRod->allow_power(0);
    result = result && heatingRod->allow_power(0);
    EXPECT_TRUE(result);
    EXPECT_EQ(heatingRod->using_power(), 0.f);

    std::vector<bool> exp_sw_pwr_calls = {false};
    EXPECT_EQ(switch_pwr_calls, exp_sw_pwr_calls);
}

TEST_F(HeatingRodTest, TurnOff1){
    // Turn on and off quickly
    heatingRod->read_temperature = []()->float{return 42.23f;};
    bool result = heatingRod->allow_power(1200);
    EXPECT_TRUE(result);
    EXPECT_EQ(heatingRod->using_power(), heatingRod->get_requesting_power().get_min());

    result = heatingRod->allow_power(0);
    EXPECT_TRUE(result);
    EXPECT_EQ(heatingRod->using_power(), 0.f);

    std::vector<bool> exp_sw_pwr_calls = {false, true, false};
    EXPECT_EQ(switch_pwr_calls, exp_sw_pwr_calls);
}

TEST_F(HeatingRodTest, TurnOff2){
    // Test min time on
    heatingRod->timing.min_on = milliseconds(1200);
    heatingRod->read_temperature = []()->float{return 42.23f;};
    EXPECT_TRUE(heatingRod->allow_power(1200));
    EXPECT_EQ(heatingRod->using_power(), heatingRod->get_requesting_power().get_min());

    std::vector<std::pair<bool,float>> result;

    const milliseconds start = time->get_time();
    while (start + milliseconds(3000) > time->get_time()){
        heatingRod->allow_power(0);
        result.push_back({heatingRod->on,heatingRod->using_power()});
        time->sleep_for(std::chrono::milliseconds(1000));
    }

    std::vector<std::pair<bool,float>> expected = {
        {true, heatingRod->get_requesting_power().get_min()},
        {true, heatingRod->get_requesting_power().get_min()},
        {false, 0.f}
    };

    EXPECT_EQ(result, expected);

    std::vector<bool> exp_sw_pwr_calls = {false, true, false};
    EXPECT_EQ(switch_pwr_calls, exp_sw_pwr_calls);
}

TEST_F(HeatingRodTest, TempBelowMin){
    heatingRod->read_temperature = []()->float{return 42.23f;};
    EXPECT_TRUE(heatingRod->allow_power(1200));

    std::vector<bool> exp_sw_pwr_calls = {false, true};
    EXPECT_EQ(switch_pwr_calls, exp_sw_pwr_calls);
}

TEST_F(HeatingRodTest, TempBetweenMinAndMax){
    heatingRod->read_temperature = []()->float{return 47.23f;};
    EXPECT_TRUE(heatingRod->allow_power(1200));

    std::vector<bool> exp_sw_pwr_calls = {false, true};
    EXPECT_EQ(switch_pwr_calls, exp_sw_pwr_calls);
}

TEST_F(HeatingRodTest, TempReachedMax){
    heatingRod->read_temperature = []()->float{return 53.23f;};
    EXPECT_FALSE(heatingRod->allow_power(1200));

    std::vector<bool> exp_sw_pwr_calls = {false};
    EXPECT_EQ(switch_pwr_calls, exp_sw_pwr_calls);
}

class TestResult{
public:
    TestResult(bool _on, float _power, milliseconds _time_on, milliseconds _time_off):
        on(_on),
        power(_power),
        time_on(_time_on),
        time_off(_time_off)
    {};

    TestResult(bool _on, float _power, int _time_on, int _time_off):
        on(_on),
        power(_power)
    {
        time_on = milliseconds(_time_on);
        time_off = milliseconds(_time_off);
    };

    bool operator==(const TestResult& rhs) const {
        if(on != rhs.on){
            std::cout << "lhs.on: "<< on << ", rhs.on: "<<rhs.on << std::endl;
            return false;
        }

        if(power != rhs.power){
            std::cout << "lhs.power: "<< power << ", rhs.power: "<<rhs.power << std::endl;
            return false;
        }

        if(abs(time_on - rhs.time_on) > tolerance)
        {
            std::cout << "lhs.time_on: "<< time_on.count() << ", rhs.time_on: "<<rhs.time_on.count() << std::endl;
            return false;
        }

        if(abs(time_off - rhs.time_off) > tolerance)
        {
            std::cout << "lhs.time_off: "<< time_off.count() << ", rhs.time_off: "<<rhs.time_off.count() << std::endl;
            return false;
        }

        return true;
    }

    static inline const milliseconds tolerance{50};

private:
    bool on{false};
    float power{0.f};
    milliseconds time_on{0};
    milliseconds time_off{0};
};

TEST_F(HeatingRodTest, MaxTimeOn){
    float temperature = 42.23f;
    heatingRod->read_temperature = [temperature]() mutable -> float{
        // std::cout << "Temperature: " << temperature << std::endl;
        float result = temperature;
        temperature += 0.1f;
        return result;
    };
    heatingRod->timing.max_on = milliseconds(200);

    std::vector<TestResult> result;

    const milliseconds start = time->get_time();
    while (start + milliseconds(700) > time->get_time()){
        heatingRod->allow_power(1200);
        result.push_back(TestResult(heatingRod->on,heatingRod->using_power(),heatingRod->on_time(), heatingRod->off_time()));
        time->sleep_for(std::chrono::milliseconds(102));
    }

    std::vector<TestResult> expected = {
        TestResult(true, heatingRod->get_requesting_power().get_min(), 0, -1),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 100, -1),
        TestResult(false, 0, 200, 0),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 0, 100),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 100, 100),
        TestResult(false, 0, 200, 0),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 0, 100)
    };

    EXPECT_EQ(result, expected);

    std::vector<bool> exp_sw_pwr_calls = {false,true,false,true,false,true};
    EXPECT_EQ(switch_pwr_calls, exp_sw_pwr_calls);
}

TEST_F(HeatingRodTest, MinTimeOn){
    float temperature = 42.23f;
    heatingRod->read_temperature = [temperature]() mutable -> float{
        // std::cout << "Temperature: " << temperature << std::endl;
        float result = temperature;
        temperature += 0.1f;
        return result;
    };
    heatingRod->timing.min_on = milliseconds(300);

    std::vector<TestResult> result;

    std::vector<float> pwr_steps = {
        1200,
        0,
        0
    };

    for(const auto& pwr: pwr_steps){
        heatingRod->allow_power(pwr);
        heatingRod->allow_power(0);
        result.push_back(TestResult(heatingRod->on,heatingRod->using_power(),heatingRod->on_time(), heatingRod->off_time()));
        time->sleep_for(std::chrono::milliseconds(200));
    }

    std::vector<TestResult> expected = {
        TestResult(true, heatingRod->get_requesting_power().get_min(), 0, -1),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 200, -1),
        TestResult(false, 0, 400, 0),
    };

    EXPECT_EQ(result, expected);

    std::vector<bool> exp_sw_pwr_calls = {false,true,false};
    EXPECT_EQ(switch_pwr_calls, exp_sw_pwr_calls);
}

TEST_F(HeatingRodTest, MinTimeOff){
    float temperature = 42.23f;
    heatingRod->read_temperature = [temperature]() mutable -> float{
        // std::cout << "Temperature: " << temperature << std::endl;
        float result = temperature;
        temperature += 0.1f;
        return result;
    };
    heatingRod->timing.min_off = milliseconds(240);

    std::vector<TestResult> result;

    std::vector<float> pwr_steps = {
        1200,
        0,
        1200,
        1200,
        1200
    };

    for(const auto& pwr: pwr_steps){
        heatingRod->allow_power(pwr);
        result.push_back(TestResult(heatingRod->on,heatingRod->using_power(),heatingRod->on_time(), heatingRod->off_time()));
        time->sleep_for(std::chrono::milliseconds(200));
    }

    std::vector<TestResult> expected = {
        TestResult(true, heatingRod->get_requesting_power().get_min(), 0, -1),
        TestResult(false, 0, 200, 0),
        TestResult(false, 0, 200, 200),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 0, 400),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 200, 400)
    };

    EXPECT_EQ(result, expected);

    std::vector<bool> exp_sw_pwr_calls = {false,true,false,true};
    EXPECT_EQ(switch_pwr_calls, exp_sw_pwr_calls);
}

TEST_F(HeatingRodTest, MaxMinTimeOn){
    float temperature = 42.23f;
    heatingRod->read_temperature = [temperature]() mutable -> float{
        // std::cout << "Temperature: " << temperature << std::endl;
        float result = temperature;
        temperature += 0.1f;
        return result;
    };
    heatingRod->timing.max_on = milliseconds(400);
    heatingRod->timing.min_off = milliseconds(240);

    std::vector<TestResult> result;

    std::vector<float> pwr_steps = {
        1200,
        1200,
        1200,
        1200,
        1200,
        1200
    };

    for(const auto& pwr: pwr_steps){
        heatingRod->allow_power(pwr);
        result.push_back(TestResult(heatingRod->on,heatingRod->using_power(),heatingRod->on_time(), heatingRod->off_time()));
        time->sleep_for(std::chrono::milliseconds(203));
    }

    std::vector<TestResult> expected = {
        TestResult(true, heatingRod->get_requesting_power().get_min(),0,-1),
        TestResult(true, heatingRod->get_requesting_power().get_min(),200,-1),
        TestResult(false, 0.f, 400, 0),
        TestResult(false, 0.f, 400, 200),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 0, 400),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 200, 400)
    };

    EXPECT_EQ(result, expected);

    std::vector<bool> exp_sw_pwr_calls = {false, true, false, true};
    EXPECT_EQ(switch_pwr_calls, exp_sw_pwr_calls);
}

int main(int argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
