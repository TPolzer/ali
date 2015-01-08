#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <boost/circular_buffer.hpp>
#include <cmath>

using namespace std;
using namespace boost;

const static char * const ENABLE_ALI = "/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI0008:00/enable";
const static char * const BRIGHT = "/sys/devices/pci0000:00/0000:00:02.0/drm/card0/card0-eDP-1/intel_backlight/brightness";
const static char * const ALI = "/sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI0008:00/ali";
const static char * const AC = "/sys/bus/acpi/drivers/ac/ACPI0003:00/power_supply/AC0/online";
const static size_t samples = 10;
const static auto interval = std::chrono::seconds(4);
const static int MAX_ILLU = 6407;
const static int MIN_BRIGHT = 80;
const static int MIN_AC_BRIGHT = 150;
const static int MAX_BRIGHT = 937;
const static double cooldown = .95;
const static double exponent = 0.3;

static int getAli() {
    ifstream ali(ALI);
    int res;
    ali >> res;
    return res;
}

static int getBrightness() {
    ifstream brightness(BRIGHT);
    int res;
    brightness >> res;
    return res;
}

static void setBrightness(int value) {
    ofstream brightness(BRIGHT);
    brightness << value;
}

static int min_bright() {
    ifstream ac(AC);
    bool connected;
    ac >> connected;
    if(connected)
        return MIN_AC_BRIGHT;
    else
        return MIN_BRIGHT;
}

static double compute(double illumination) {
    double min = min_bright();
    return min + (MAX_BRIGHT-min)*pow(illumination/MAX_ILLU, exponent);
}

int main() {
    ios::sync_with_stdio(false);
    {
        ofstream enable(ENABLE_ALI);
        enable << 1 << endl;
    }
    circular_buffer<double> buf(samples);
    double average = 0;
    int expected = -1;
    double over = 0, inserted = 0;
    while(1) {
        over *= cooldown;
        double value = compute(getAli());
        int old = getBrightness();
        if(expected != -1 && old != expected) {
            over = 1;
            inserted = old;
            buf.clear();
        }
        value = over*inserted + (1-over)*value;
        if(buf.size() == samples) {
            average -= buf.back()/samples;
            buf.pop_back();
        } else {
            average *= buf.size();
            average /= buf.size() + 1;
        }
        buf.push_front(value);
        average += value / buf.size();
        expected = round(average);
        setBrightness(expected);
        
        this_thread::sleep_for( interval );
    }
}
