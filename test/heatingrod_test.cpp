#include <thread>
#include <chrono>
#include <gtest/gtest.h>
#include <HeatingRod.h>

class HeatingRodTest: public ::testing::Test{
public:
    HeatingRodTest(){}

    virtual void SetUp() override {
        heatingRod = std::move(std::make_unique<HeatingRod>("heatingRod0",1000.f));
        heatingRod->switch_power = [](bool power){std::cout << "\t>>> switch_power to " << power << std::endl;};
    }

    virtual void TearDown() override {
    }

    std::unique_ptr<HeatingRod> heatingRod;
};

TEST_F(HeatingRodTest, TempBelowMin){
    heatingRod->read_temperature = []()->float{return 42.23f;};
    EXPECT_TRUE(heatingRod->allow_power(1200));
}

TEST_F(HeatingRodTest, TempBetweenMinAndMax){
    heatingRod->read_temperature = []()->float{return 47.23f;};
    EXPECT_TRUE(heatingRod->allow_power(1200));
}

TEST_F(HeatingRodTest, TempReachedMax){
    heatingRod->read_temperature = []()->float{return 53.23f;};
    EXPECT_FALSE(heatingRod->allow_power(1200));
}
TEST_F(HeatingRodTest, MaxTimeOn){
    heatingRod->read_temperature = []()->float{return 42.23f;};
    heatingRod->timing.max_on = 2000;

    std::vector<std::pair<bool,HeatingRod::State>> result;

    const clock_t start = clock();
    while (start + 3000 > clock()){
        heatingRod->allow_power(1200);
        result.push_back({heatingRod->on,heatingRod->state});
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::vector<std::pair<bool,HeatingRod::State>> expected = {
        {true, HeatingRod::State::ready},
        {true, HeatingRod::State::ready},
        {false, HeatingRod::State::ready}
    };

    EXPECT_EQ(result, expected);
}

int main(int argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
