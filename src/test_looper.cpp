#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include "os/Looper.h"
#include "os/Handler.h"
#include "os/MessageQueue.h"
#include "utils/SystemClock.h"
#include "ui/WindowManager.h"

using namespace setu::os;


int looper_test_main() {
    if (!setu::os::Looper::myLooper()) {
        Looper::prepareMainLooper();
    }
    Looper* looper = Looper::getMainLooper();
    Handler handler(looper);

    std::vector<std::string> output;

    long long now = setu::uptimeMillis();

    // 1. Multiple delays in time order (not enqueue order)
    handler.postDelayed([&]() { output.push_back("B"); }, 100);
    handler.postDelayed([&]() { output.push_back("A"); }, 50);
    handler.postDelayed([&]() { output.push_back("C"); }, 150);

    // 2. Re-entrancy: posting a task mid-execution
    handler.postDelayed([&]() {
        output.push_back("D-runs-first");
        handler.post([&]() {
            output.push_back("E-posted-mid-execution");
        });
    }, 200);

    // Run until 300ms has passed
    long long start = setu::uptimeMillis();
    while (setu::uptimeMillis() - start < 300) {
        looper->loopOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Dump output
    for (const auto& s : output) {
        std::cout << s << std::endl;
    }
    
    return 0;
}
