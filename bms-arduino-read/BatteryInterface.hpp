// for BatteryInterface2
#define MON2_RESP_LEN 27
#define MON2_REQ_MSG {0x7F, 0x10, 0x02, 0x06, 0x11, 0x58}; // {SOI, ADD, VER, LEN, FUN, CKS}
#define MON2_REQ_MSG_LEN 6

struct BatteryInfo {
  int16_t batt_current;
  int16_t batt_voltage;
  int16_t batt_soc;
  int16_t batt_crg_cyc;
  int16_t batt_status;
  int16_t batt_warning;
  int16_t batt_cap_left;
};

enum COMM_STATES {
  COMM_FREE = 0,
  COMM_SEND = 1,
  COMM_BUSY = 2,
};

//////////////////////////////////////////////////////////////////////////////
class BatteryInterface2
{
public:
  /*
   * Class constructor
   */
  BatteryInterface2(Stream& port, const int txenpin = 0) {
      serial_port_ = &port;
      tx_en_pin_ = txenpin;
      comm_state_ = COMM_FREE;
  }

  /*
   * Set up tx_en_pin to OUTPUT and begin serial_port_ before this line!
   */
  void init() {
    digitalWrite(tx_en_pin_, LOW);
  }

  /*
   * As serial readings can be quite slow, this function is meant to be
   * read periodically.
   * 
   * The users needs to take care of transition of COMM_FREE to COMM_SEND
   * and call queryMon2() just before the last byte transmitted out on Serial.
   * This requirement was needed as it was intended that the code is not
   * blocking, so the user needs to call queryMon2() just before the last byte
   * is sent so the block by flush() is minimised. If the call is too late,
   * and the tx_en_pin_ is LOW too late, data would be lost.
   * 
   * Two varints of this exist for now. The former writes out to an array dest,
   * while the latter only writes to her own private member.
   */
  int queryMon2(uint8_t* dest) {

    switch (comm_state_) {
        case COMM_FREE:
          comm_state_ = COMM_SEND;
          bytes_received_ = 0;
          digitalWrite(tx_en_pin_, HIGH);
          clearRecvBuff(); // flush rx buffer
          clearRecvData(); // clear outstanding received data
          serial_port_->write(mon2_query_msg_, MON2_REQ_MSG_LEN);
          break;
        case COMM_SEND:
          // calling queryMon2 when it is in COMM_SEND may block
          serial_port_->flush(); // this blocks
          digitalWrite(tx_en_pin_, LOW);
          comm_state_ = COMM_BUSY;
          break;
        case COMM_BUSY:
          // if not ready
          while (serial_port_->available() > 0) {
            // read incoming byte
            mon2_resp_temp_msg_[bytes_received_] = serial_port_->read();
            dest[bytes_received_] = mon2_resp_temp_msg_[bytes_received_];
            if (++bytes_received_ == MON2_RESP_LEN) {
              // copy to variable that would be parsed
              for (uint8_t i = 0; i < MON2_RESP_LEN; i++) {
                mon2_resp_msg_[i] = mon2_resp_temp_msg_[i];
              }
              comm_state_ = COMM_FREE;
            }
          }
          break;
    }
    return comm_state_;
  }

  int queryMon2() {

    switch (comm_state_) {
        case COMM_FREE:
          comm_state_ = COMM_SEND;
          bytes_received_ = 0;
          digitalWrite(tx_en_pin_, HIGH);
          clearRecvBuff(); // flush rx buffer
          clearRecvData(); // clear outstanding received data
          serial_port_->write(mon2_query_msg_, MON2_REQ_MSG_LEN);
          break;
        case COMM_SEND:
          // calling queryMon2 when it is in COMM_SEND may block
          serial_port_->flush(); // this blocks
          digitalWrite(tx_en_pin_, LOW);
          comm_state_ = COMM_BUSY;
          break;
        case COMM_BUSY:
          // if not ready
          while (serial_port_->available() > 0) {
            // read incoming byte
            mon2_resp_temp_msg_[bytes_received_] = serial_port_->read();
            if (++bytes_received_ == MON2_RESP_LEN) {
              // copy to variable that would be parsed
              for (uint8_t i = 0; i < MON2_RESP_LEN; i++) {
                mon2_resp_msg_[i] = mon2_resp_temp_msg_[i];
              }
              comm_state_ = COMM_FREE;
            }
          }
          break;
    }
    return comm_state_;
  }

  /*
   * Aborts queryMon2, particularly when waiting for response
   */
  int abortLastCall() {
    if (comm_state_ == COMM_SEND) return 1; // can't abort when sending, as flush blocks

    clearRecvBuff();
    clearRecvData();
    bytes_received_ = 0;
    comm_state_ = COMM_FREE;
    return 0;
  }

  /*
   * Get communication state
   */
  int getCommState() {
    return comm_state_;
  }

  /*
   * Get last valid battery monitor 2 message
   */
  BatteryInfo getBattInfo() {
    BatteryInfo batt_info;
    batt_info.batt_current = (mon2_resp_msg_[10] << 8) | mon2_resp_msg_[9];
    batt_info.batt_voltage = (mon2_resp_msg_[16] << 8) | mon2_resp_msg_[15];
    batt_info.batt_soc     = int16_t(0x64 * int32_t((mon2_resp_msg_[22] << 8) | mon2_resp_msg_[21])/((mon2_resp_msg_[24] << 8) | mon2_resp_msg_[23]));
    batt_info.batt_crg_cyc = (mon2_resp_msg_[20] << 8) | mon2_resp_msg_[19];
    batt_info.batt_status  = ((mon2_resp_msg_[25] >> 6 & 1) << 7) | // Bit 08: charge MOSFET on flag
                             ((mon2_resp_msg_[25] >> 7 & 1) << 6) | // Bit 07: discharge MOSFET on flag

                             ((mon2_resp_msg_[5]      & 1) << 1) |  // Bit 02: is charging flag
                              (mon2_resp_msg_[5] >> 4 & 1);         // Bit 01: is discharging flag

    batt_info.batt_warning = ((mon2_resp_msg_[5] >> 1 & 1) << 14) | // Bit 15: charge overcurrent flag
                             ((mon2_resp_msg_[5] >> 5 & 1) << 13) | // Bit 14: discharge overcurrent flag
                             ((mon2_resp_msg_[5] >> 6 & 1) << 12) | // Bit 13: discharge short-circuit flag

                             ((mon2_resp_msg_[6]      & 1) << 11) | // Bit 12: battery detection open circuit flag
                             ((mon2_resp_msg_[6] >> 1 & 1) << 10) | // Bit 11: temperature sensing open circuit flag
                             ((mon2_resp_msg_[6] >> 4 & 1) << 9) |  // Bit 10: battery overvoltage flag
                             ((mon2_resp_msg_[6] >> 5 & 1) << 8) |  // Bit 09: battery undervoltage flag
                             ((mon2_resp_msg_[6] >> 6 & 1) << 7) |  // Bit 08: total battery voltage too high flag
                             ((mon2_resp_msg_[6] >> 7 & 1) << 6) |  // Bit 07: total battery voltage too low flag

                             ((mon2_resp_msg_[7] >> 2 & 1) << 5) |  // Bit 06: charge overtemperature flag
                             ((mon2_resp_msg_[7] >> 3 & 1) << 4) |  // Bit 05: discharge overtemperature flag
                             ((mon2_resp_msg_[7] >> 4 & 1) << 3) |  // Bit 04: charge undertemperature flag
                             ((mon2_resp_msg_[7] >> 5 & 1) << 2) |  // Bit 03: discharge undertemperature flag
                             ((mon2_resp_msg_[7] >> 6 & 1) << 1) |  // Bit 02: charge temperature difference too large flag
                              (mon2_resp_msg_[7] >> 7 & 1);         // Bit 01: discharge temperature difference too large flag

    batt_info.batt_cap_left = (mon2_resp_msg_[22] << 8) | mon2_resp_msg_[21];
  return batt_info;
  }

private:
  uint8_t mon2_query_msg_[MON2_REQ_MSG_LEN] = MON2_REQ_MSG; // {SOI, ADD, VER, LEN, FUN, CKS}
  uint8_t mon2_resp_temp_msg_[MON2_RESP_LEN];
  uint8_t mon2_resp_msg_[MON2_RESP_LEN];

  uint8_t comm_state_;
  uint8_t bytes_received_ = 0;

  Stream *serial_port_;
  int tx_en_pin_;

  /*
   * Clears buffer of Serial port
   */
  void clearRecvBuff() {
    while (serial_port_->available() > 0) serial_port_->read(); // flush rx buffer
  }

  /*
   * Clears temporary holding data from Serial buffer
   */
  void clearRecvData() {
    for (uint8_t i = 0; i < MON2_RESP_LEN; i++) {
      mon2_resp_temp_msg_[i] = 0x00u;
    }
  }
};
