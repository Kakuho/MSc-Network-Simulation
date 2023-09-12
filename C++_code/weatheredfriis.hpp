#ifndef WEATHEREDFRIIS_HPP
#define WEATHEREDFRIIS_HPP

#include <iostream>

#include "ns3/object.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/random-variable-stream.h"
#include "ns3/type-id.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/integer.h"
#include <cmath>
#include <map>

using namespace ns3;

//NS_LOG_COMPONENT_DEFINE("WeatheredModels");

class WeatheredFriisPropagationLossModel: public PropagationLossModel{
  public:
    static TypeId GetTypeId(void);
    WeatheredFriisPropagationLossModel();
 
    // Delete copy constructor and assignment operator to avoid misuse
    WeatheredFriisPropagationLossModel(const WeatheredFriisPropagationLossModel& src) = delete;
    WeatheredFriisPropagationLossModel& operator=(const WeatheredFriisPropagationLossModel& src) = delete;
 
    void SetFrequency(double frequency);
    void SetSystemLoss(double systemLoss);
 
    void SetMinLoss(double minLoss);
 
    double GetMinLoss() const;
 
    double GetFrequency() const;
    double GetSystemLoss() const;

    void SetWeather(int weatherval); 
 
  private:
    double DoCalcRxPower(double txPowerDbm,
                         Ptr<MobilityModel> a,
                         Ptr<MobilityModel> b) const override;
    int64_t DoAssignStreams(int64_t stream) override;
 
    double DbmToW(double dbm) const;
 
    double DbmFromW(double w) const;
 
    double m_lambda;     
    double m_frequency;  
    double m_systemLoss; 
    double m_minLoss;    
    int8_t weather;
};

// ===================================================================== //


NS_OBJECT_ENSURE_REGISTERED(WeatheredFriisPropagationLossModel);

void WeatheredFriisPropagationLossModel::SetWeather(int weatherval){
  if(weatherval < 3){
    weather = weatherval;
  }
}

TypeId
WeatheredFriisPropagationLossModel::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::WeatheredFriisPropagationLossModel")
            .SetParent<PropagationLossModel>()
            .SetGroupName("Propagation")
            .AddConstructor<WeatheredFriisPropagationLossModel>()
            .AddAttribute(
                "Frequency",
                "The carrier frequency (in Hz) at which propagation occurs (default is 5.15 GHz).",
                DoubleValue(5.150e9),
                MakeDoubleAccessor(&WeatheredFriisPropagationLossModel::SetFrequency,
                                   &WeatheredFriisPropagationLossModel::GetFrequency),
                MakeDoubleChecker<double>())
            .AddAttribute("SystemLoss",
                          "The system loss",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&WeatheredFriisPropagationLossModel::m_systemLoss),
                          MakeDoubleChecker<double>())
            .AddAttribute("MinLoss",
                          "The minimum value (dB) of the total loss, used at short ranges.",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&WeatheredFriisPropagationLossModel::SetMinLoss,
                                             &WeatheredFriisPropagationLossModel::GetMinLoss),
                          MakeDoubleChecker<double>())
            .AddAttribute("WeatherVal",
                          "The weather effects on the model. 0 is normal, 1 is rainfall and 2 is snowfall",
                          IntegerValue(0),
                          MakeIntegerAccessor(&WeatheredFriisPropagationLossModel::weather),
                          MakeIntegerChecker<int8_t>());
    return tid;
}

WeatheredFriisPropagationLossModel::WeatheredFriisPropagationLossModel()
{
}

void
WeatheredFriisPropagationLossModel::SetSystemLoss(double systemLoss)
{
    m_systemLoss = systemLoss;
}

double
WeatheredFriisPropagationLossModel::GetSystemLoss() const
{
    return m_systemLoss;
}

void
WeatheredFriisPropagationLossModel::SetMinLoss(double minLoss)
{
    m_minLoss = minLoss;
}

double
WeatheredFriisPropagationLossModel::GetMinLoss() const
{
    return m_minLoss;
}

void
WeatheredFriisPropagationLossModel::SetFrequency(double frequency)
{
    m_frequency = frequency;
    static const double C = 299792458.0; // speed of light in vacuum
    m_lambda = C / frequency;
}

double
WeatheredFriisPropagationLossModel::GetFrequency() const
{
    return m_frequency;
}

double
WeatheredFriisPropagationLossModel::DbmToW(double dbm) const
{
    double mw = std::pow(10.0, dbm / 10.0);
    return mw / 1000.0;
}

double
WeatheredFriisPropagationLossModel::DbmFromW(double w) const
{
    double dbm = std::log10(w * 1000.0) * 10.0;
    return dbm;
}

double
WeatheredFriisPropagationLossModel::DoCalcRxPower(double txPowerDbm,
                                         Ptr<MobilityModel> a,
                                         Ptr<MobilityModel> b) const
{
    /*
     * Friis free space equation:
     * where Pt, Gr, Gr and P are in Watt units
     * L is in meter units.
     *
     *    P     Gt * Gr * (lambda^2)
     *   --- = ---------------------
     *    Pt     (4 * pi * d)^2 * L
     *
     * Gt: tx gain (unit-less)
     * Gr: rx gain (unit-less)
     * Pt: tx power (W)
     * d: distance (m)
     * L: system loss
     * lambda: wavelength (m)
     *
     * Here, we ignore tx and rx gain and the input and output values
     * are in dB or dBm:
     *
     *                           lambda^2
     * rx = tx +  10 log10 (-------------------)
     *                       (4 * pi * d)^2 * L
     *
     * rx: rx power (dB)
     * tx: tx power (dB)
     * d: distance (m)
     * L: system loss (unit-less)
     * lambda: wavelength (m)
     */
    double distance = a->GetDistanceFrom(b);
    if (distance < 3 * m_lambda)
    {
      std::cout  << "distance not within the far field region => inaccurate propagation loss value\n";
    }
    if (distance <= 0)
    {
        return txPowerDbm - m_minLoss;
    }
    double numerator = m_lambda * m_lambda;
    double denominator = 16 * M_PI * M_PI * distance * distance * m_systemLoss;
    double lossDb = -10 * log10(numerator / denominator);
    switch(weather){
      case 0: // normal weather
        return txPowerDbm - std::max(lossDb, m_minLoss) - 0;
      case 1: // rain
        return txPowerDbm - std::max(lossDb, m_minLoss) - 5;
      case 2: // snow
        return txPowerDbm - std::max(lossDb, m_minLoss) - 10;
    }
}

int64_t
WeatheredFriisPropagationLossModel::DoAssignStreams(int64_t stream)
{
    return 0;
}

#endif
