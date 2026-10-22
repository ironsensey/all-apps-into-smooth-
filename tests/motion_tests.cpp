#define APP_MOTION_CORE_TEST
#include "../native-app-motion.wh.cpp"
#include <cstdlib>
#include <iostream>
#include <atomic>
#include <thread>

namespace {
std::uint64_t checks=0;
void Require(bool condition,const char* description) {
    ++checks;
    if (!condition) { std::cerr << "FAILED: " << description << '\n'; std::exit(1); }
}
bool Near(float a,float b,float tolerance=0.0001f) { return std::abs(a-b)<=tolerance; }
}
int main() {
    using namespace AppMotion;
    Require(RevealStateMatches(true,true,true),"a minimized window may still have WS_VISIBLE; it must qualify for restore");
    Require(RevealStateMatches(true,true,false),"an iconic hidden window can be restored");
    Require(!RevealStateMatches(true,false,true),"ordinary activation is not replayed as restore");
    Require(!RevealStateMatches(false,true,true),"a minimized app is not classified as a first launch");
    Require(RevealStateMatches(false,false,false),"hidden non-iconic launch qualifies");
    Require(!RevealStateMatches(false,false,true),"visible windows do not get a second launch animation");
    Require(CanRouteRestore(true,true,true,true,false,false),"ready responsive minimized app can route");
    Require(!CanRouteRestore(true,false,true,true,false,false),"already visible switching stays native");
    Require(!CanRouteRestore(true,true,false,true,false,false),"unconfirmed source dispatcher stays native");
    Require(!CanRouteRestore(true,true,true,false,false,false),"missing renderer stays native");
    Require(!CanRouteRestore(true,true,true,true,true,false),"new click during pending restore is handled natively");
    Require(!CanRouteRestore(true,true,true,true,false,true),"known hung window stays native");
    Require(!CanRouteRestore(false,true,true,true,false,false),"disabled feature stays native");
    Require(DisplayContentAllowed(true,false,0,true,0),"normal new window with unsupported affinity getter can close-animate");
    Require(!DisplayContentAllowed(true,false,0,false,0),"unknown snapshot permission is never inferred from a failed getter");
    Require(DisplayContentAllowed(false,false,0,false,0),"live DWM reveal can use compositor-enforced protection without screen capture");
    Require(!DisplayContentAllowed(true,true,0x11,true,0),"explicit excluded-from-capture setting wins over a cached default");
    Require(!DisplayContentAllowed(false,false,0,true,0x11),"tracked protected window cannot use a custom live reveal");
    Require(DisplayContentAllowed(true,true,0,true,0x11),"successful current getter supersedes an old tracked setting");
    for (int i=0;i<200;++i) {
        std::atomic<int> state{Pending};
        bool claimed=false;
        auto cas=[&](int desired,int expected) { state.compare_exchange_strong(expected,desired); return expected; };
        std::thread source([&]{ claimed=ClaimRestore(cas); });
        const bool fallback=ClaimRestoreFallback(cas);
        source.join();
        Require(claimed!=fallback,"exactly one of source restore and native deadline fallback may claim the request");
        Require(state==(claimed ? Started : Cancelled),"restore cancellation has one stable winning state");
        Require(!ClaimRestore(cas) && !ClaimRestoreFallback(cas),"claimed restore cannot be replayed by either side");
    }
    struct RecoveryPlatform {
        bool claimed=false,owns=true,uncloakSucceeds=true,preferenceSucceeds=true;
        bool removed=false,released=false;
        int visibilityCalls=0,preferenceCalls=0;
        bool Claim() { if (claimed) return false; claimed=true; return true; }
        bool OwnsGeneration() { return owns; }
        bool Uncloak() { ++visibilityCalls; return uncloakSucceeds; }
        bool RestoreTransitions() { ++preferenceCalls; return preferenceSucceeds; }
        void ReleaseClaim() { claimed=false; released=true; }
        void RemoveToken() { removed=true; }
    };
    for (bool visibility:{false,true}) for (bool preferences:{false,true}) {
        RecoveryPlatform platform;
        platform.uncloakSucceeds=visibility; platform.preferenceSucceeds=preferences;
        RestoreOpening(platform);
        Require(platform.visibilityCalls==1 && platform.preferenceCalls==1,
                "preference recovery is attempted even when uncloaking fails");
        Require(platform.removed==(visibility&&preferences),"ownership removed only after complete recovery");
        Require(platform.released==!(visibility&&preferences),"failed cross-process recovery permits local watchdog retry");
        if (!visibility || !preferences) {
            platform.uncloakSucceeds=true; platform.preferenceSucceeds=true;
            RestoreOpening(platform);
            Require(platform.removed,"source watchdog can retry failed broker recovery");
        }
    }
    RecoveryPlatform once;
    RestoreOpening(once); RestoreOpening(once);
    Require(once.visibilityCalls==1 && once.preferenceCalls==1,"recovery claimed only once");
    RecoveryPlatform reused; reused.owns=false;
    RestoreOpening(reused);
    Require(reused.visibilityCalls==0 && reused.preferenceCalls==0,"old request cannot alter a new window generation");
    for (int initial:{Pending,Ready,Started,Complete,Cancelled}) {
        std::atomic<int> state{initial};
        auto cas=[&](int desired,int expected) {
            state.compare_exchange_strong(expected,desired); return expected;
        };
        Require(AcceptOrCancel(cas)==(initial==Ready),"handoff accepts only a published ready state");
        Require(state==(initial==Pending ? Cancelled : initial),"deadline only cancels pending work");
    }
    for (int i=0;i<200;++i) {
        std::atomic<int> state{Pending};
        bool published=false;
        auto cas=[&](int desired,int expected) {
            state.compare_exchange_strong(expected,desired); return expected;
        };
        std::thread renderer([&]{ published=PublishReady(cas); });
        const bool accepted=AcceptOrCancel(cas);
        renderer.join();
        Require(published==accepted,"cancellation race cannot accept unprepared work");
        Require(state==(accepted ? Ready : Cancelled),"cancelled work cannot become a late overlay");
    }
    // An overshooting or non-monotone curve would cause a visible reversal or
    // invalid opacity. Test the actual shared curves, including out-of-range t.
    for (Curve curve:{Curve::Smooth,Curve::Out,Curve::Linear}) {
        Require(Ease(-1,curve)==0 && Ease(2,curve)==1,"progress clamped outside the interval");
        float previous=0;
        for (int i=0;i<=1000;++i) {
            const float t=i/1000.0f;
            const float value=Ease(t,curve);
            Require(std::isfinite(value) && value>=0 && value<=1,"opacity stays finite and bounded");
            Require(value+0.000001f>=previous,"curve never reverses direction");
            previous=value;
        }
        // DWM live-opening math and DirectComposition closing polynomials must
        // describe the same progress across different allowed durations.
        for (int ms:{60,80,120,150,200,350,400}) {
            const float duration=ms/1000.0f;
            for (auto pair:{std::pair{1.0f,0.0f},std::pair{1.0f,0.96f},std::pair{0.9f,1.0f}}) {
                const auto c=Coefficients(pair.first,pair.second,duration,curve);
                for (int i=0;i<=100;++i) {
                    const float t=i/100.0f,x=t*duration;
                    const float actual=((c.d*x+c.c)*x+c.b)*x+c.a;
                    const float expected=pair.first+(pair.second-pair.first)*Ease(t,curve);
                    Require(Near(actual,expected),"compositor polynomial matches the live curve");
                }
            }
        }
    }
    // Exercise portrait/ultrawide/4K geometry and small budgets: the upload
    // must fit the limit, never grow the source and never acquire zero extent.
    for (int w:{1,31,100,1920,2560,3840,7680,16384}) {
        for (int h:{1,31,100,1080,1440,2160,4320,16384}) {
            for (std::uint64_t budget:{1ULL,1000ULL,1'000'000ULL,4'000'000ULL,8'000'000ULL}) {
                const auto fit=FitImage(w,h,budget);
                Require(fit.width>0 && fit.height>0 && fit.width<=w && fit.height<=h,
                        "snapshot bounds do not expand or collapse");
                Require(static_cast<std::uint64_t>(fit.width)*fit.height<=budget,
                        "snapshot respects the pixel budget");
                Require(ValidImage(fit.width,fit.height,
                        static_cast<std::uint64_t>(fit.width)*fit.height*4),"valid fitted upload accepted");
            }
        }
    }
    Require(FitImage(0,10,10).width==0,"zero width rejected");
    Require(FitImage(-1,10,10).width==0,"negative width rejected");
    Require(FitImage(10,10,0).width==0,"zero budget rejected");
    Require(!ValidImage(100,100,39999),"truncated packet rejected");
    Require(!ValidImage(100,100,40001),"inconsistent packet rejected");
    Require(!ValidImage(0,100,0),"empty packet rejected");
    Require(!ValidImage(20000,1,80000),"oversize extent rejected");
    Require(!ValidImage(4000,4000,64'000'000),"oversize upload rejected");
    Require(Clamp(-500,60,350)==60 && Clamp(999999,60,350)==350,
            "duration settings stay in the supported range");
    std::cout << checks << " app-motion geometry/curve checks passed\n";
}
