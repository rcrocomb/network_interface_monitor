#ifndef NETWORK_STATS_H
#define NETWORK_STATS_H

#include <string>
#include <map>
#include <set>

#include <stdint.h>
#include <unistd.h>

struct receive_data
{
    uint64_t bytes,
             compressed,
             CRC_errors,
             dropped,
             errors,
             FIFO_errors,
             frame_errors,
             length_errors,
             missed_errors,
             over_errors,
             packets;
};

struct transmit_data
{
    uint64_t aborted_errors,
             bytes,
             carrier_errors,
             compressed,
             dropped,
             errors,
             FIFO_errors,
             heartbeat_errors,
             packets,
             window_errors;
};


enum rx_fields
{
    RX_BYTES,
    RX_COMPRESSED,
    RX_CRC_ERRORS,
    RX_DROPPED,
    RX_ERRORS,
    RX_FIFO_ERRORS,
    RX_FRAME_ERRORS,
    RX_LENGTH_ERRORS,
    RX_MISSED_ERRORS,
    RX_OVER_ERRORS,
    RX_PACKETS
};

enum tx_fields
{
    TX_ABORTED_ERRORS,
    TX_BYTES,
    TX_CARRIER_ERRORS,
    TX_COMPRESSED,
    TX_DROPPED,
    TX_ERRORS,
    TX_FIFO_ERRORS,
    TX_HEARTBEAT_ERRORS,
    TX_PACKETS,
    TX_WINDOW_ERRORS
};

extern std::string DEFAULT_INTERFACE;

class network_stats
{
private:
    struct netdata
    {
        uint64_t value; // value pulled from stats file
        int fd;         // file descriptor for stats file

        netdata(void): value(0), fd(-1) {}
        netdata(int file_d, uint64_t v = 0): value(v), fd(file_d) {}
        ~netdata(void){}
    };

    static std::map<rx_fields, std::string> rx_fields_to_filename;
    static std::map<tx_fields, std::string> tx_fields_to_filename;

    std::string interface_name_;
    std::string interface_stats_path_;

    // Map from type of data to file descriptor used to update that kind of
    // data: map has entries only for those fields we are supposed to
    // update.
    std::map<rx_fields, netdata> rx_to_update_;
    std::map<tx_fields, netdata> tx_to_update_;
#if 0
    receive_data rx_data_;
    transmit_data tx_data_;
#endif
private:

    static void build_fields_to_filename_maps(void);

    uint64_t fetch_one_rx(rx_fields r) const;
    uint64_t fetch_one_tx(tx_fields t) const;
    uint64_t update_one(int fd);

    // uncopyable for now: would need to get open fds and suchlike (blick)
    network_stats(const network_stats &s);
    network_stats &operator =(const network_stats &s);

public:

    network_stats(const std::string interface = DEFAULT_INTERFACE);
    ~network_stats(void);

    void set_rx_stats_to_update(const std::set<rx_fields> &to_update);
    void set_tx_stats_to_update(const std::set<tx_fields> &to_update);
    void update_all(void);
    void update_receive_data(void);
    void update_transmit_data(void);

    // Fill out and return these, I guess
    receive_data get_receive_data(void) const;
    transmit_data get_transmit_data(void) const;

    uint64_t get_rx_bytes(void) const;
    uint64_t get_rx_packets(void) const;

    uint64_t get_tx_bytes(void) const;
    uint64_t get_tx_packets(void) const;
};

#endif  // NETWORK_STATS_H

