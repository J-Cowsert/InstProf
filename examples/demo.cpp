#include <instprof.h>

#include <thread>

static void do_work(int items) {
    IP_FUNC_SCOPE();
    for (int i = 0; i < items; ++i) {
        IP_NAMED_SCOPE("item");
        volatile long x = 0;                     // stand-in for real work
        for (int j = 0; j < 5000; ++j) x += j;
    }
}

int main() {
    IP_FUNC_SCOPE();

    std::thread t([] { do_work(50); });
    do_work(100);
    t.join();
}
