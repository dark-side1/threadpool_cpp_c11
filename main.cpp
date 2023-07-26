#include <cstdio>
#include"thread_pool.hpp"

// 作为task的函数
float Sum(float end)
{
    float s = 0;
    float i = 0.0f;
    while (i < end) {
        s += i;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        i += 0.5;
    }
    return s;
}

// 作为task的函数对象
struct SumFunctor {
    float end;
    SumFunctor(float e) : end(e) {}

    float operator()()
    {
        float s = 0;
        float i = 0.0f;
        while (i < end) {
            s += i;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            i += 0.5;
        }
        return s;
    }
};

int main(int argc, char** argv) {
    ThreadPool pool;
    std::vector<std::future<float>> futures; // 用于获取task执行结果
    for (int i = 0; i < 20; i++) {
        // 添加lambda表达式形式的task，并将返回结果放到future，方便后续获取执行结果
        futures.emplace_back(pool.commit([](int k) -> float {
            float s = 0;
            for (int j = 0; j < k; j++) {
                s += j;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            return s;
            }, i));
    }
    // 获取执行结果
    for (auto& f : futures) {
        auto ret = f.get();
        printf("[lambda] result: %f\n", ret);
    }
    //futures.clear();
    //float i = 100;
    //while (i < 110) {
    //    // 添加函数形式的task
    //    futures.emplace_back(pool.commit(Sum, i));
    //    std::this_thread::sleep_for(std::chrono::seconds(1));
    //    i += 1.0f;
    //}
    //for (auto& f : futures) {
    //    auto ret = f.get();
    //    printf("[function] result: %f\n", ret);
    //}

    //futures.clear();
    //while (i < 120) {
    //    // 添加函数对象形式的task
    //    futures.emplace_back(pool.commit(SumFunctor(i)));
    //    std::this_thread::sleep_for(std::chrono::seconds(1));
    //    i += 1.0f;
    //}
    //for (auto& f : futures) {
    //    auto ret = f.get();
    //    printf("[functor] result: %f\n", ret);
    //}
    return 0;
}