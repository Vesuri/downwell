#ifndef DOWNWELL_CORE_CURRENCY_STEP_SOUND_H
#define DOWNWELL_CORE_CURRENCY_STEP_SOUND_H
namespace downwell {
// objCurrency Step blocks 0..4. Selection occurs once on the first admitted
// Step, after scrCurrencySpawn's size override, not during Create or a bounce.
struct CurrencyStepSound {
    bool allSet=false;
    int bounceSound=14,mySound=0;
    template<class Random> void begin(int size,Random& random){
        if(allSet)return;
        bounceSound=size==0?168+int(random.chooseIndexSmall(3)):163+int(random.chooseIndexSmall(2));
        mySound=bounceSound;allSet=true;
    }
};
}
#endif
