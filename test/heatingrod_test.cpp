#include <thread>
#include <chrono>
#include <gtest/gtest.h>
#include <HeatingRod.h>

class HeatingRodTest: public ::testing::Test{
public:
    HeatingRodTest(){}

    virtual void SetUp() override {
    }

    virtual void TearDown() override {
    }
};

std::shared_ptr<HeatingRod> heatingRodGenerator(){
    auto hr = std::make_shared<HeatingRod>("heatingRod0",1000.f);
    hr->switch_power = [](bool power){std::cout << "\t>>> switch_power to " << power << std::endl;};
    std::cout << "hr memory: "<< hr.get() << std::endl;
    return hr;
}

TEST_F(HeatingRodTest, TurnOn0){
    auto heatingRod = heatingRodGenerator();
    bool result = heatingRod->allow_power(1200);
    EXPECT_FALSE(result);
    EXPECT_EQ(heatingRod->using_power(), 0.f);
}

TEST_F(HeatingRodTest, TurnOn1){
    auto heatingRod = heatingRodGenerator();
    heatingRod->read_temperature = []()->float{return 42.23f;};
    bool result = heatingRod->allow_power(1200);
    EXPECT_TRUE(result);
    float using_pwr_result = heatingRod->using_power();
    float using_pwr_expected = heatingRod->get_requesting_power().get_min();
    EXPECT_EQ(using_pwr_result, using_pwr_expected);
}

TEST_F(HeatingRodTest, TurnOn2){
    auto heatingRod = heatingRodGenerator();
    heatingRod->read_temperature = []()->float{return 53.23f;};
    bool result = heatingRod->allow_power(1200);
    EXPECT_FALSE(result);
    EXPECT_EQ(heatingRod->using_power(), 0.f);
}

TEST_F(HeatingRodTest, TurnOff0){
    auto heatingRod = heatingRodGenerator();
    heatingRod->read_temperature = []()->float{return 42.23f;};
    bool result = heatingRod->allow_power(0);
    EXPECT_TRUE(result);
    EXPECT_EQ(heatingRod->using_power(), 0.f);
}

TEST_F(HeatingRodTest, TurnOff1){
    auto heatingRod = heatingRodGenerator();
    heatingRod->read_temperature = []()->float{return 42.23f;};
    bool result = heatingRod->allow_power(1200);
    EXPECT_TRUE(result);
    EXPECT_EQ(heatingRod->using_power(), heatingRod->get_requesting_power().get_min());

    result = heatingRod->allow_power(0);
    EXPECT_TRUE(result);
    EXPECT_EQ(heatingRod->using_power(), 0.f);
}

TEST_F(HeatingRodTest, TurnOff2){
    auto heatingRod = heatingRodGenerator();
    heatingRod->timing.min_on = 1200;
    heatingRod->read_temperature = []()->float{return 42.23f;};
    EXPECT_TRUE(heatingRod->allow_power(1200));
    EXPECT_EQ(heatingRod->using_power(), heatingRod->get_requesting_power().get_min());

    std::vector<std::pair<bool,float>> result;

    const clock_t start = clock();
    while (start + 3000 > clock()){
        heatingRod->allow_power(0);
        result.push_back({heatingRod->on,heatingRod->using_power()});
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    std::vector<std::pair<bool,float>> expected = {
        {true, heatingRod->get_requesting_power().get_min()},
        {true, heatingRod->get_requesting_power().get_min()},
        {false, 0.f}
    };

    EXPECT_EQ(result, expected);
}

TEST_F(HeatingRodTest, TempBelowMin){
    auto heatingRod = heatingRodGenerator();
    heatingRod->read_temperature = []()->float{return 42.23f;};
    EXPECT_TRUE(heatingRod->allow_power(1200));
}

TEST_F(HeatingRodTest, TempBetweenMinAndMax){
    auto heatingRod = heatingRodGenerator();
    heatingRod->read_temperature = []()->float{return 47.23f;};
    EXPECT_TRUE(heatingRod->allow_power(1200));
}

TEST_F(HeatingRodTest, TempReachedMax){
    auto heatingRod = heatingRodGenerator();
    heatingRod->read_temperature = []()->float{return 53.23f;};
    EXPECT_FALSE(heatingRod->allow_power(1200));
}

class TestResult{
public:
    TestResult(bool _on, float _power, clock_t _time_on, clock_t _time_off):
        on(_on),
        power(_power),
        time_on(_time_on),
        time_off(_time_off)
    {};

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
            std::cout << "lhs.time_on: "<< time_on << ", rhs.time_on: "<<rhs.time_on << std::endl;
            return false;
        }

        if(abs(time_off - rhs.time_off) > tolerance)
        {
            std::cout << "lhs.time_off: "<< time_off << ", rhs.time_off: "<<rhs.time_off << std::endl;
            return false;
        }

        return true;
    }

    static const clock_t tolerance = 50;

private:
    bool on;
    float power;
    clock_t time_on;
    clock_t time_off;
};

TEST_F(HeatingRodTest, MaxTimeOn){
    auto heatingRod = heatingRodGenerator();
    float temperature = 42.23f;
    heatingRod->read_temperature = [temperature]() mutable -> float{
        // std::cout << "Temperature: " << temperature << std::endl;
        float result = temperature;
        temperature += 0.1f;
        return result;
    };
    heatingRod->timing.max_on = 2000;

    std::vector<TestResult> result;

    const clock_t start = clock();
    while (start + 3000 > clock()){
        heatingRod->allow_power(1200);
        result.push_back(TestResult(heatingRod->on,heatingRod->using_power(),heatingRod->on_time(), heatingRod->off_time()));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::vector<TestResult> expected = {
        TestResult(true, heatingRod->get_requesting_power().get_min(),0,-1),
        TestResult(true, heatingRod->get_requesting_power().get_min(),1000,-1),
        TestResult(false, 0.f,2000,0)
    };

    EXPECT_EQ(result, expected);
}

TEST_F(HeatingRodTest, MinTimeOn){
    auto heatingRod = heatingRodGenerator();
    float temperature = 42.23f;
    heatingRod->read_temperature = [temperature]() mutable -> float{
        // std::cout << "Temperature: " << temperature << std::endl;
        float result = temperature;
        temperature += 0.1f;
        return result;
    };
    heatingRod->timing.min_on = 1500;

    std::vector<TestResult> result;

    heatingRod->allow_power(1200);
    result.push_back(TestResult(heatingRod->on,heatingRod->using_power(),heatingRod->on_time(), heatingRod->off_time()));

    const clock_t start = clock();
    while (start + 3000 > clock()){
        heatingRod->allow_power(0);
        result.push_back(TestResult(heatingRod->on,heatingRod->using_power(),heatingRod->on_time(), heatingRod->off_time()));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::vector<TestResult> expected = {
        TestResult(true, heatingRod->get_requesting_power().get_min(), 0, -1),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 0, -1),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 1000, -1),
        TestResult(false, 0, 2000, 0),
    };

    EXPECT_EQ(result, expected);
}

TEST_F(HeatingRodTest, MinTimeOff){
    auto heatingRod = heatingRodGenerator();
    float temperature = 42.23f;
    heatingRod->read_temperature = [temperature]() mutable -> float{
        // std::cout << "Temperature: " << temperature << std::endl;
        float result = temperature;
        temperature += 0.1f;
        return result;
    };
    heatingRod->timing.min_off = 1200;

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
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::vector<TestResult> expected = {
        TestResult(true, heatingRod->get_requesting_power().get_min(), 0, -1),
        TestResult(false, 0, 1000, 0),
        TestResult(false, 0, 2000, 1000),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 0, 2000),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 1000, 3000),
    };

    EXPECT_EQ(result, expected);
}

TEST_F(HeatingRodTest, MaxMinTimeOn){
    auto heatingRod = heatingRodGenerator();
    float temperature = 42.23f;
    heatingRod->read_temperature = [temperature]() mutable -> float{
        // std::cout << "Temperature: " << temperature << std::endl;
        float result = temperature;
        temperature += 0.1f;
        return result;
    };
    heatingRod->timing.max_on = 2000;
    heatingRod->timing.min_off = 1200;

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
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::vector<TestResult> expected = {
        TestResult(true, heatingRod->get_requesting_power().get_min(),0,-1),
        TestResult(true, heatingRod->get_requesting_power().get_min(),1000,-1),
        TestResult(false, 0.f, 2000, 0),
        TestResult(false, 0.f, 3000, 1000),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 0, 2000),
        TestResult(true, heatingRod->get_requesting_power().get_min(), 1000, 3000)
    };

    EXPECT_EQ(result, expected);
}

int main(int argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
