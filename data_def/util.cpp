#include "util.h"

#include "absl/strings/str_format.h"

namespace flight_panel {
namespace data {
// Decode transponder code.
// The code is encoded with BCO16 encoding.
// The double value is an integer. Lower 16 bits encodes the numbers.
// mask = 0xf; digit3 = int(val)>>12 & mask;
std::string DecodeTransponder(double value) {
  return absl::StrFormat("%X", int(value));
}

SimData ToSimData(const SimVars& src) {
  SimData data;
  // Instrument vars
  Instrument* instruments = data.mutable_instruments();
  instruments->set_indicated_airspeed(src.asiAirspeed);
  instruments->set_bank_angle(src.adiBank);
  instruments->set_pitch_angle(src.adiPitch);
  instruments->set_kohlsman_setting_hg(src.altKollsman);
  instruments->set_indicated_altitude(src.altAltitude);
  instruments->set_heading_indicator_deg(src.hiHeading);
  instruments->set_vertical_speed(src.vsiVerticalSpeed);
  instruments->set_turn_indicator_rate(src.tcRate);
  instruments->set_turn_coordinator_ball(src.tcBall);

  // Radio data
  Avionics* avionics = data.mutable_avionics();
  avionics->mutable_com_radio_1()->set_active_freq(src.com1Freq);
  avionics->mutable_com_radio_1()->set_standby_freq(src.com1Standby);
  avionics->mutable_com_radio_2()->set_active_freq(src.com2Freq);
  avionics->mutable_com_radio_2()->set_standby_freq(src.com2Standby);
  avionics->mutable_nav_radio_1()->set_active_freq(src.nav1Freq);
  avionics->mutable_nav_radio_1()->set_standby_freq(src.nav1Standby);
  avionics->mutable_nav_radio_2()->set_active_freq(src.nav2Freq);
  avionics->mutable_nav_radio_2()->set_standby_freq(src.nav2Standby);
  avionics->set_transponder_code(DecodeTransponder(src.transponderCode));

  // Aircraft data
  AircraftInfo* aircraft = data.mutable_aircraft_info();
  aircraft->set_model(src.aircraft);
  aircraft->set_call_sign(src.atcCallSign);

  // Control data
  AircraftControls* controls = data.mutable_aircraft_controls();
  controls->set_elevator_trim_indicator(src.tfElevatorTrimIndicator);
  controls->set_flaps_count(src.tfFlapsCount);
  controls->set_flaps_pos(src.tfFlapsIndex);
  controls->set_gear_pos(src.gearPosition);
  controls->set_parking_brake_on(src.parkingBrakeOn);
  

  // engine data
  EngineData* engine = data.mutable_engine_data();
  engine->set_rpm(src.rpmEngine);
  engine->set_rpm_percent(src.rpmPercent);
  engine->set_engine_elapsed_time(src.rpmElapsedTime);
  engine->set_fuel_left_level(src.fuelLeft);
  engine->set_fuel_right_level(src.fuelRight);


  // Game environment
  GameData* game = data.mutable_game_data();
  game->set_connected(src.connected);
  return data;
}

}  // namespace data
}  // namespace flight_panel
