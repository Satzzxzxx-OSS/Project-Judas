// FTFT8: independent state-based thermal accounting against the real solver.
// No production reaction/heat helper supplies the energy-budget oracle.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <string>
#include <glm/gtc/quaternion.hpp>
#include "AtmosphereField.h"
#include "CombustionWorld.h"
#include "ReferenceFrame.h"
namespace {
namespace fs = std::filesystem;
constexpr float dt = 1.0f / 60.0f;
constexpr double sigma = 5.670374419e-8;
constexpr double pi = 3.1415926535897932384626433832795;
constexpr double eps = std::numeric_limits<float>::epsilon();
int checks=0, failures=0, steps=0, cases=0;
fs::path output;
void Check(bool ok,const std::string& name) {
    ++checks; failures+=!ok;
    if (!ok) std::printf("FAIL %s\n",name.c_str());
}
struct Fixture {
    PhysicsWorld physics;
    AtmosphereParameters parameters;
    AtmosphereField atmosphere;
    ReferenceFrame frame;
    CombustionWorld thermal;
    explicit Fixture(AtmosphereParameters p={}) : parameters(p), atmosphere(p) { Check(physics.Init(),"physics init"); }
    BodyHandle Add(const glm::vec3& x,const CombustibleMaterial& m,float temperature) {
        const auto h=physics.CreateDynamicSphere(x,.1f,2,0,0); thermal.AddBody(h,m,temperature); return h;
    }
    void Advance(const RadiantHeater* heater=nullptr) { thermal.Step(dt,physics,atmosphere,frame,heater);++steps; }
};
struct Metrics {
    std::array<double,2> temperature{},fuel{};
    double stored=0,chemical=0,gas=0,heater=0,environment=0,budgetResidual=0;
    double roundBudget=0,maxStepResidual=0,maxPowerRelative=0,maxPairSum=0;
};
double Square(double x){return x*x;}
double Fourth(double x){return Square(Square(x));}
// Independent finite-polytrope relation from its two radial boundary values.
// These do not call AtmosphereField::Sample or its radial evaluators.
struct Gas {double density,temperature,oxidizer,speed;};
Gas GasOracle(const Fixture& f,const glm::vec3& position,const glm::vec3& velocity) {
    const glm::dvec3 r=glm::dvec3(position)-glm::dvec3(f.frame.originPosition);
    const double radius=glm::length(r);
    if(radius>=f.parameters.topRadius||radius<=0)return {0,0,0,0};
    const double q=(1.0/radius-1.0/f.parameters.topRadius)/(1.0/f.parameters.referenceRadius-1.0/f.parameters.topRadius);
    const double density=f.parameters.referenceDensity*std::pow(q,1.0/(double(f.parameters.polytropicExponent)-1));
    const glm::dvec3 vgas=glm::dvec3(f.frame.linearVelocity)+glm::cross(glm::dvec3(f.frame.angularVelocity),r);
    return {density,f.parameters.referenceTemperatureKelvin*q,density*f.parameters.oxidizerMassFraction,
            glm::length(glm::dvec3(velocity)-vgas)};
}
Metrics Combined(const char* name,bool reverse,const glm::quat& rotation,const glm::vec3& translation,const glm::vec3& boost) {
    ++cases;const int before=failures;Fixture f;
    f.frame.originPosition=translation;f.frame.orientation=rotation;f.frame.linearVelocity=boost;
    f.frame.angularVelocity=rotation*glm::vec3(0,0,.01f);
    std::array<CombustibleMaterial,2> m;
    m[0].heatCapacityJPerK=400;m[1].heatCapacityJPerK=700;
    m[0].initialFuelMassKg=.0015f;m[1].initialFuelMassKg=.002f;
    m[0].ignitionTemperatureK=400;m[1].ignitionTemperatureK=320;
    m[0].activationRangeK=m[1].activationRangeK=25;
    m[0].maximumFuelRateKgPerSecond=.002f;m[1].maximumFuelRateKgPerSecond=.0015f;
    m[0].heatReleaseJPerKg=1.0e6f;m[1].heatReleaseJPerKg=1.5e6f;
    m[0].retainedCombustionHeatFraction=.6f;m[1].retainedCombustionHeatFraction=.8f;
    m[0].radiativeAreaSquareMeters=.4f;m[1].radiativeAreaSquareMeters=.7f;
    m[0].emissivity=.6f;m[1].emissivity=.8f;
    m[0].convectionCoefficientWattsPerSquareMeterKelvin=2;m[1].convectionCoefficientWattsPerSquareMeterKelvin=3;
    const std::array<float,2> initialT={500,350};
    std::array<BodyHandle,2> body;std::array<glm::vec3,2> velocities, angular;
    for(int k=0;k<2;++k){const int i=reverse?1-k:k;
        const glm::vec3 r=rotation*glm::vec3(80,float(i)*2,0);
        body[i]=f.Add(translation+r,m[i],initialT[i]);
        velocities[i]=boost+glm::cross(f.frame.angularVelocity,r)+rotation*glm::vec3(2,.3f,-.2f);
        angular[i]=rotation*glm::vec3(.2f,-.1f,.3f);
        f.physics.SetLinearVelocity(body[i],velocities[i]);f.physics.SetAngularVelocity(body[i],angular[i]);
    }
    const RadiantHeater heater{translation+rotation*glm::vec3(80,-1,0),800};
    std::ofstream csv(output/(std::string(name)+".csv"));
    csv<<std::setprecision(17)<<"step,stored_J,chemical_J,gas_J,heater_J,environment_J,pair_sum_J,residual_J,round_budget_J,Ta_K,Tb_K,fuelA_kg,fuelB_kg\n";
    Metrics result;bool exhausted[2]={false,false};
    for(int step=0;step<240;++step){
        std::array<ThermalBodyState,2> old={*f.thermal.State(body[0]),*f.thermal.State(body[1])};
        f.Advance(&heater);
        double stored=0,chemical=0,gas=0,heating=0,environment=0,pairSum=0,roundBudget=0;
        const glm::dvec3 delta=glm::dvec3(f.physics.GetTransform(body[0]).position)-glm::dvec3(f.physics.GetTransform(body[1]).position);
        const double area=double(m[0].radiativeAreaSquareMeters)*m[1].radiativeAreaSquareMeters/(4*pi*glm::dot(delta,delta));
        const double pair=sigma*area*std::sqrt(double(m[0].emissivity)*m[1].emissivity)*(Fourth(old[0].temperatureKelvin)-Fourth(old[1].temperatureKelvin));
        for(int i=0;i<2;++i){
            const auto& s=*f.thermal.State(body[i]);const auto& material=m[i];
            const double burnt=old[i].remainingFuelMassKg-s.remainingFuelMassKg;
            const double released=burnt*material.heatReleaseJPerKg;
            stored+=double(material.heatCapacityJPerK)*(double(s.temperatureKelvin)-old[i].temperatureKelvin);
            chemical+=released;gas+=double(s.heatToGasWatts)*dt;heating+=double(s.heaterWattsReceived)*dt;
            environment+=double(s.environmentalHeatWattsReceived)*dt;pairSum+=double(s.pairwiseHeatWattsReceived)*dt;
            const Gas ambient=GasOracle(f,f.physics.GetTransform(body[i]).position,velocities[i]);
            const double convection=material.convectionCoefficientWattsPerSquareMeterKelvin*(ambient.density/f.parameters.referenceDensity)*
                (1+std::sqrt(ambient.speed))*material.radiativeAreaSquareMeters*(ambient.temperature-old[i].temperatureKelvin);
            const double radiation=material.emissivity*sigma*(material.radiativeAreaSquareMeters-area)*
                (Fourth(ambient.temperature)-Fourth(old[i].temperatureKelvin));
            const glm::dvec3 toHeater=glm::dvec3(f.physics.GetTransform(body[i]).position)-glm::dvec3(heater.worldPosition);
            const double absorbed=double(heater.powerWatts)*material.radiativeAreaSquareMeters/(4*pi*glm::dot(toHeater,toHeater));
            const double expectedPair=i==0?-pair:pair;
            const double powerScale=std::max(1.0,std::abs(convection)+std::abs(radiation)+std::abs(absorbed)+std::abs(expectedPair));
            const double powerError=std::max({std::abs(s.environmentalHeatWattsReceived-convection-radiation),
                std::abs(s.heaterWattsReceived-absorbed),std::abs(s.pairwiseHeatWattsReceived-expectedPair)})/powerScale;
            result.maxPowerRelative=std::max(result.maxPowerRelative,powerError);
            Check(powerError<=2e-5,std::string(name)+" independent exchange powers");
            Check(burnt>=0&&s.remainingFuelMassKg>=0&&s.remainingFuelMassKg<=material.initialFuelMassKg,std::string(name)+" finite fuel");
            Check(std::abs(double(s.heatOutputWatts)*dt-released)<=8*eps*std::max(1.0,released),std::string(name)+" fuel decrement owns chemical energy");
            Check(std::abs(double(s.heatToGasWatts)*dt-released*(1-double(material.retainedCombustionHeatFraction)))<=16*eps*std::max(1.0,released),
                std::string(name)+" retained and gas partition");
            const double oxygenAvailable=ambient.oxidizer*material.radiativeAreaSquareMeters*
                (material.oxygenTransportSpeedMetersPerSecond+material.airflowTransportFactor*ambient.speed)*dt;
            Check(burnt*material.oxygenRequiredKgPerKgFuel<=oxygenAvailable*(1+2e-5)+1e-12,std::string(name)+" supplied reservoir oxygen bounds reaction");
            Check(s.temperatureKelvin>100&&std::isfinite(s.temperatureKelvin),std::string(name)+" moderate coefficients remain finite");
            Check(s.heatExchangeScale==1&&s.limitedHeatExchangeJ==0,std::string(name)+" ordinary exchange is unmodified");
            if(s.remainingFuelMassKg==0){exhausted[i]=true;Check(s.burnRateKgPerSecond==0||old[i].remainingFuelMassKg>0,std::string(name)+" exhaustion stops reaction");}
            Check(f.physics.GetLinearVelocity(body[i])==velocities[i]&&f.physics.GetAngularVelocity(body[i])==angular[i],std::string(name)+" heat never edits rigid motion");
            // Each output T has one float rounding. Allow two eps*C*T plus
            // diagnostic power/partition float rounding. This measures the
            // state budget, not an assertion that floats conserve exact J.
            roundBudget+=2*eps*material.heatCapacityJPerK*std::max(double(s.temperatureKelvin),double(old[i].temperatureKelvin));
            roundBudget+=16*eps*(released+dt*(std::abs(s.heatToGasWatts)+std::abs(s.heaterWattsReceived)+
                std::abs(s.environmentalHeatWattsReceived)+std::abs(s.pairwiseHeatWattsReceived)));
        }
        const double residual=stored+gas-chemical-heating-environment;
        Check(std::abs(residual)<=roundBudget,std::string(name)+" stored plus gas equals chemical plus external heat");
        Check(pairSum==0,std::string(name)+" pair exchange cancels exactly in diagnostics");
        Check(heating<=double(heater.powerWatts)*dt*(1+2*eps),std::string(name)+" captured heater does not exceed external supply");
        result.stored+=stored;result.chemical+=chemical;result.gas+=gas;result.heater+=heating;result.environment+=environment;
        result.roundBudget+=roundBudget;result.maxStepResidual=std::max(result.maxStepResidual,std::abs(residual));
        result.maxPairSum=std::max(result.maxPairSum,std::abs(pairSum));
        csv<<step<<','<<stored<<','<<chemical<<','<<gas<<','<<heating<<','<<environment<<','<<pairSum<<','<<residual<<','<<roundBudget
           <<','<<f.thermal.State(body[0])->temperatureKelvin<<','<<f.thermal.State(body[1])->temperatureKelvin
           <<','<<f.thermal.State(body[0])->remainingFuelMassKg<<','<<f.thermal.State(body[1])->remainingFuelMassKg<<'\n';
    }
    for(int i=0;i<2;++i){const auto& state=*f.thermal.State(body[i]);result.temperature[i]=state.temperatureKelvin;result.fuel[i]=state.remainingFuelMassKg;
        Check(exhausted[i]&&state.burnRateKgPerSecond==0&&state.heatOutputWatts==0,std::string(name)+" finite reservoir exhausted within fixture");}
    result.budgetResidual=result.stored+result.gas-result.chemical-result.heater-result.environment;
    Check(std::abs(result.budgetResidual)<=result.roundBudget,std::string(name)+" cumulative state energy budget");
    Check(result.chemical>0&&result.gas>0&&result.heater>0&&result.environment<0,std::string(name)+" every declared energy channel exercised");
    std::printf("CASE %s %s stored_J=%.17g chemical_J=%.17g gas_J=%.17g heater_J=%.17g environment_J=%.17g residual_J=%.17g allowed_round_J=%.17g max_step_residual_J=%.17g max_independent_power_relative=%.17g\n",
        name,before==failures?"PASS":"FAIL",result.stored,result.chemical,result.gas,result.heater,result.environment,result.budgetResidual,result.roundBudget,result.maxStepResidual,result.maxPowerRelative);
    return result;
}
void ChemistryConditions(){
    for(int mode=0;mode<4;++mode){++cases;const int before=failures;AtmosphereParameters p;if(mode==1)p.oxidizerMassFraction=0;
        Fixture f(p);CombustibleMaterial m;m.heatCapacityJPerK=100;m.initialFuelMassKg=.1f;m.ignitionTemperatureK=405;
        m.activationRangeK=20;m.maximumFuelRateKgPerSecond=.001f;m.heatReleaseJPerKg=1e5f;m.retainedCombustionHeatFraction=.6f;
        m.radiativeAreaSquareMeters=.5f;m.emissivity=0;m.convectionCoefficientWattsPerSquareMeterKelvin=80;
        const float initial=mode==0?300:410;const glm::vec3 position=mode==2?glm::vec3(120,0,0):glm::vec3(80,0,0);
        const auto body=f.Add(position,m,initial);bool burnt=false,stopped=false;double lastFuel=m.initialFuelMassKg;
        for(int step=0;step<180;++step){f.Advance();const auto& s=*f.thermal.State(body);
            burnt|=s.burnRateKgPerSecond>0;
            if(mode<3)Check(s.burnRateKgPerSecond==0&&s.heatOutputWatts==0&&s.remainingFuelMassKg==double(m.initialFuelMassKg),"inactive chemistry has no hidden burn timer");
            if(mode==3&&s.previousTemperatureKelvin<=m.ignitionTemperatureK){stopped=true;Check(s.burnRateKgPerSecond==0&&s.remainingFuelMassKg==lastFuel,"cooling extinguishes with finite fuel still present");}
            lastFuel=s.remainingFuelMassKg;
        }
        if(mode==3)Check(burnt&&stopped&&lastFuel>0,"temperature-dependent extinction witnessed");
        const char* names[]={"below_ignition","zero_oxidizer","vacuum","cooling_extinction"};
        std::printf("CASE %s %s final_T=%.9g fuel=%.17g\n",names[mode],before==failures?"PASS":"FAIL",f.thermal.State(body)->temperatureKelvin,lastFuel);
    }
}
void HeaterNormalization(){++cases;const int before=failures;Fixture f;
    CombustibleMaterial m;m.initialFuelMassKg=0;m.radiativeAreaSquareMeters=2;m.emissivity=0;m.convectionCoefficientWattsPerSquareMeterKelvin=0;
    const auto a=f.Add({80,.1f,0},m,300),b=f.Add({80,-.1f,0},m,300);const RadiantHeater h{{80,0,0},100};f.Advance(&h);
    const auto& sa=*f.thermal.State(a);const auto& sb=*f.thermal.State(b);
    Check(sa.heaterWattsReceived==50&&sb.heaterWattsReceived==50,"overlapping view divides finite heater power equally");
    const double stored=m.heatCapacityJPerK*(double(sa.temperatureKelvin)+sb.temperatureKelvin-600);
    Check(std::abs(stored-100*double(dt))<=4*eps*m.heatCapacityJPerK*301,"heater-only stored energy accounts external input");
    std::printf("CASE heater_normalization %s stored_J=%.17g input_J=%.17g\n",before==failures?"PASS":"FAIL",stored,100*double(dt));
}
void StiffPassiveExchange(){
    for(int mode=0;mode<4;++mode){++cases;const int before=failures;Fixture f;
        CombustibleMaterial a;a.initialFuelMassKg=0;a.radiativeAreaSquareMeters=1;
        a.heatCapacityJPerK=mode==1?.01f:1;a.emissivity=mode==1?0:1;
        a.convectionCoefficientWattsPerSquareMeterKelvin=mode==1?100:0;
        CombustibleMaterial b=a;b.heatCapacityJPerK=100;
        const bool pair=mode>=2;
        BodyHandle first,second;
        if(mode==3){second=f.Add({80,.1f,0},b,100);first=f.Add({80,0,0},a,1500);}
        else {first=f.Add(mode==0?glm::vec3(120,0,0):glm::vec3(80,0,0),a,mode==1?100:1500);
            if(pair)second=f.Add({80,.1f,0},b,100);}
        const char* names[]={"stiff_vacuum_cooling","stiff_reservoir_heating","stiff_pair","stiff_pair_reversed"};
        std::ofstream csv(output/(std::string(names[mode])+".csv"));csv<<std::setprecision(17)<<"step,Ta,Tb,stored_J,external_J,pair_sum_J,residual_J\n";
        double worst=0,withheld=0;bool finiteBounded=true;
        for(int step=0;step<24;++step){const auto oldA=*f.thermal.State(first);
            const double oldB=pair?f.thermal.State(second)->temperatureKelvin:0;
            f.Advance();const auto& x=*f.thermal.State(first);
            withheld+=x.limitedHeatExchangeJ;
            if(step==0)Check(x.heatExchangeScale<1&&x.heatExchangeScale>0&&x.limitedHeatExchangeJ>0,
                std::string(names[mode])+" stiff approximation explicitly reported");
            const double tb=pair?f.thermal.State(second)->temperatureKelvin:0;
            const double ambient=mode==1?300:0;
            const double low=pair?std::min(double(oldA.temperatureKelvin),oldB):std::min(double(oldA.temperatureKelvin),ambient);
            const double high=pair?std::max(double(oldA.temperatureKelvin),oldB):std::max(double(oldA.temperatureKelvin),ambient);
            const bool bounds=std::isfinite(x.temperatureKelvin)&&x.temperatureKelvin>=low-1e-6&&x.temperatureKelvin<=high+1e-6&&
                (!pair||(std::isfinite(tb)&&tb>=low-1e-6&&tb<=high+1e-6));
            finiteBounded &= bounds;Check(bounds,std::string(names[mode])+" passive maximum principle");
            double stored=double(a.heatCapacityJPerK)*(double(x.temperatureKelvin)-oldA.temperatureKelvin);
            double external=double(x.environmentalHeatWattsReceived)*dt;
            double pairSum=double(x.pairwiseHeatWattsReceived)*dt;
            double scale=double(a.heatCapacityJPerK)*(std::abs(x.temperatureKelvin)+std::abs(oldA.temperatureKelvin))+std::abs(external)+std::abs(pairSum);
            if(pair){const auto& y=*f.thermal.State(second);stored+=double(b.heatCapacityJPerK)*(tb-oldB);
                external+=double(y.environmentalHeatWattsReceived)*dt;pairSum+=double(y.pairwiseHeatWattsReceived)*dt;
                scale+=double(b.heatCapacityJPerK)*(std::abs(tb)+std::abs(oldB))+dt*(std::abs(y.environmentalHeatWattsReceived)+std::abs(y.pairwiseHeatWattsReceived));}
            const double residual=stored-external;worst=std::max(worst,std::abs(residual));
            Check(std::abs(residual)<=8*eps*std::max(1.0,scale),std::string(names[mode])+" measured actual exchange closes state budget");
            Check(!pair||pairSum==0,std::string(names[mode])+" reciprocal pair exchange cancels");
            if(step==0)std::printf("STIFF FIRST %s oldA_K=%.17g oldB_K=%.17g nextA_K=%.17g nextB_K=%.17g stored_J=%.17g actual_external_J=%.17g residual_J=%.17g\n",
                names[mode],double(oldA.temperatureKelvin),oldB,double(x.temperatureKelvin),tb,stored,external,residual);
            csv<<step<<','<<x.temperatureKelvin<<','<<tb<<','<<stored<<','<<external<<','<<pairSum<<','<<residual<<'\n';
        }
        std::printf("CASE %s %s finite_bounded=%d worst_energy_residual_J=%.17g incident_withheld_J=%.17g\n",names[mode],before==failures?"PASS":"FAIL",int(finiteBounded),worst,withheld);
    }
}
void ThreeBodyStiffExchange(){
    std::array<float,3> reference{};
    for(bool reverse:{false,true}){++cases;const int before=failures;Fixture f;
        const std::array<float,3> capacity={.01f,1,100},initial={100,1500,800};std::array<BodyHandle,3> bodies;
        for(int k=0;k<3;++k){const int i=reverse?2-k:k;CombustibleMaterial m;m.initialFuelMassKg=0;
            m.heatCapacityJPerK=capacity[i];m.radiativeAreaSquareMeters=1;m.emissivity=1;m.convectionCoefficientWattsPerSquareMeterKelvin=0;
            bodies[i]=f.Add({80,float(i)*.1f,0},m,initial[i]);}
        double worst=0;
        for(int step=0;step<24;++step){std::array<float,3> old;
            for(int i=0;i<3;++i)old[i]=f.thermal.State(bodies[i])->temperatureKelvin;
            f.Advance();const double low=*std::min_element(old.begin(),old.end()),high=*std::max_element(old.begin(),old.end());
            double stored=0,external=0,pair=0,scale=0;
            for(int i=0;i<3;++i){const auto& x=*f.thermal.State(bodies[i]);
                Check(x.temperatureKelvin>=low-1e-4&&x.temperatureKelvin<=high+1e-4,"three-body passive maximum principle");
                stored+=double(capacity[i])*(double(x.temperatureKelvin)-old[i]);external+=double(x.environmentalHeatWattsReceived)*dt;
                pair+=double(x.pairwiseHeatWattsReceived)*dt;scale+=double(capacity[i])*(old[i]+x.temperatureKelvin)+std::abs(x.pairwiseHeatWattsReceived)*dt;}
            worst=std::max(worst,std::abs(stored-external));
            Check(std::abs(stored-external)<=8*eps*scale,"three unequal-capacity stored energy budget");
            Check(std::abs(pair)<=8*eps*scale,"shared multi-neighbor exchange sums to zero within diagnostic rounding");
        }
        for(int i=0;i<3;++i){const float t=f.thermal.State(bodies[i])->temperatureKelvin;
            if(!reverse)reference[i]=t;else Check(std::abs(t-reference[i])<=1e-3,"stiff three-body registration-order covariance");}
        Check(f.thermal.State(bodies[0])->temperatureKelvin>initial[0],"stiff three-body exchange actually transfers heat");
        std::printf("CASE three_body_stiff%s %s worst_energy_residual_J=%.17g\n",reverse?"_reversed":"",before==failures?"PASS":"FAIL",worst);
    }
}
void NoInputIdentity(){++cases;const int before=failures;Fixture f;
    CombustibleMaterial m;m.initialFuelMassKg=0;m.radiativeAreaSquareMeters=0;m.heatCapacityJPerK=300;
    const auto body=f.Add({80,0,0},m,.5f);f.Advance();const auto& s=*f.thermal.State(body);
    const double added=m.heatCapacityJPerK*(double(s.temperatureKelvin)-.5);
    Check(s.heatOutputWatts==0&&s.heaterWattsReceived==0&&s.pairwiseHeatWattsReceived==0&&s.environmentalHeatWattsReceived==0,"subkelvin identity genuinely has no energy source");
    Check(s.temperatureKelvin==.5f&&added==0,"positive temperature zero-input state remains unchanged");
    std::printf("CASE zero_input_positive_temperature %s initial_K=0.5 final_K=%.9g unexplained_J=%.17g\n",before==failures?"PASS":"FAIL",s.temperatureKelvin,added);
}
}
int main(int argc,char** argv){
    output="build/thermal-accounting";
    if(argc==3&&std::string(argv[1])=="--output")output=argv[2];
    else if(argc!=1){std::fprintf(stderr,"usage: thermal_accounting_tests [--output directory]\n");return 2;}
    fs::create_directories(output);
    const glm::quat identity(1,0,0,0),rotation=glm::angleAxis(.79f,glm::normalize(glm::vec3(1,-2,3)));
    const auto base=Combined("combined",false,identity,glm::vec3(0),glm::vec3(0));
    const auto reversed=Combined("registration_reversed",true,identity,glm::vec3(0),glm::vec3(0));
    const auto rotated=Combined("rotated_translated",false,rotation,glm::vec3(32,-16,24),glm::vec3(0));
    const auto boosted=Combined("common_boost",false,identity,glm::vec3(0),glm::vec3(30,-20,10));
    Check(base.temperature==reversed.temperature&&base.fuel==reversed.fuel&&base.budgetResidual==reversed.budgetResidual,"registration order has identical two-body result");
    for(int i=0;i<2;++i){Check(std::abs(base.temperature[i]-rotated.temperature[i])<=.002&&std::abs(base.fuel[i]-rotated.fuel[i])<=1e-8,"rotation translation preserves thermal scalars");
        Check(std::abs(base.temperature[i]-boosted.temperature[i])<=.002&&std::abs(base.fuel[i]-boosted.fuel[i])<=1e-8,"common velocity boost preserves thermal scalars");}
    ChemistryConditions();HeaterNormalization();StiffPassiveExchange();ThreeBodyStiffExchange();NoInputIdentity();
    std::printf("FTFT8 thermal accounting: %d cases, %d steps, %d checks, %d failures\n",cases,steps,checks,failures);
    return failures?1:0;
}
