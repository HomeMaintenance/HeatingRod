#include <thread>
#include <chrono>
#include <gtest/gtest.h>
#include <HeatingRod.h>

class HeatingRodTest: public ::testing::Test{
public:
    HeatingRodTest(){}

    virtual void SetUp() override {
        heatingRod = std::move(std::make_unique<HeatingRod>("heatingRod0",1000.f));
        heatingRod->switch_power = [](bool power){};
    }

    virtual void TearDown() override {
    }

    std::unique_ptr<HeatingRod> heatingRod;
};

TEST_F(HeatingRodTest, TempBelowMin){
    heatingRod->read_temperature = []()->float{return 42.23f;};
    heatingRod->init_temperature_hysteresis();
    EXPECT_TRUE(heatingRod->allow_power(1200));
}

TEST_F(HeatingRodTest, TempBetweenMinAndMax){
    heatingRod->read_temperature = []()->float{return 47.23f;};
    heatingRod->init_temperature_hysteresis();
    EXPECT_FALSE(heatingRod->allow_power(1200));
}

TEST_F(HeatingRodTest, TempReachedMax){
    heatingRod->read_temperature = []()->float{return 53.23f;};
    heatingRod->init_temperature_hysteresis();
    EXPECT_FALSE(heatingRod->allow_power(1200));
}

TEST(RandomTest, myTest){
    HeatingRod heatingRod0 = HeatingRod("heatingRod0",1000);
    heatingRod0.switch_power = [](bool power){};
    heatingRod0.read_temperature = []()->float{return 53.23;};

    clock_t start = clock();

    while (start + 10000 > clock())
        heatingRod0.allow_power(1200);
        std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main(int argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
