#include "network_stats.h"

#include <utility>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>

#include "program_IO.h"

std::string DEFAULT_INTERFACE("eth0");

namespace
{
    // interfaces are found under this dir
    const std::string SYSFS_PATH("/sys/class/net/");
    // once have interface-specific dir, stats are under this dir
    const std::string STATS_DIR("/statistics/");

    // module/class name
    const std::string NAME("network_stats");

    enum
    {
        // # bytes to read from any given stats file: this should be
        // way more than we need, i.e. if we read this many, it's probably
        // bad
        READ_SIZE = 32
    };

    const char *rx_name(enum rx_fields r)
    {
        #define STRINGIFY(x) case x: return #x;
        switch (r)
        {
        STRINGIFY(RX_BYTES); STRINGIFY(RX_COMPRESSED); STRINGIFY(RX_CRC_ERRORS);
        STRINGIFY(RX_DROPPED); STRINGIFY(RX_ERRORS); STRINGIFY(RX_FIFO_ERRORS);
        STRINGIFY(RX_FRAME_ERRORS); STRINGIFY(RX_LENGTH_ERRORS);
        STRINGIFY(RX_MISSED_ERRORS); STRINGIFY(RX_OVER_ERRORS);
        STRINGIFY(RX_PACKETS);
        default: return "Unknown Rx!";
        }
        #undef STRINGIFY
    }

    const char *tx_name(enum tx_fields t)
    {
        #define STRINGIFY(x) case x: return #x;
        switch (t)
        {
        STRINGIFY(TX_ABORTED_ERRORS); STRINGIFY(TX_BYTES);
        STRINGIFY(TX_CARRIER_ERRORS); STRINGIFY(TX_COMPRESSED);
        STRINGIFY(TX_DROPPED); STRINGIFY(TX_ERRORS); STRINGIFY(TX_FIFO_ERRORS);
        STRINGIFY(TX_HEARTBEAT_ERRORS); STRINGIFY(TX_PACKETS);
        STRINGIFY(TX_WINDOW_ERRORS);
        default: return "Unknown Tx!";
        }
        #undef STRINGIFY
    }
}

#define CPRINT(fmt, args...) CPRINT_WITH_NAME(NAME, fmt, ##args)
#define ALWAYS(fmt, args...) ALWAYS_WITH_NAME(NAME, fmt, ##args)
#define ERROR(fmt, args...) ERROR_WITH_NAME(NAME, fmt, ##args)
#define RUNTIME(fmt, args...) RUNTIME_WITH_NAME(NAME, fmt, ##args)

////////////////////////////////////////////////////////////////////////////////
// static variable initialization
////////////////////////////////////////////////////////////////////////////////

std::map<rx_fields, std::string> network_stats::rx_fields_to_filename;
std::map<tx_fields, std::string> network_stats::tx_fields_to_filename;

////////////////////////////////////////////////////////////////////////////////
// static methods
////////////////////////////////////////////////////////////////////////////////

/**
    Build mapping from the stat we want to retrieve to the filename that holds
    that information
*/

void
network_stats::build_fields_to_filename_maps(void)
{
    using ::std::make_pair;

    CPRINT("Building stat to filename map\n");

#define RX_INSERT(a,b) rx_fields_to_filename.insert(make_pair(a, b))
#define TX_INSERT(a,b) tx_fields_to_filename.insert(make_pair(a, b))
    RX_INSERT(RX_BYTES, "rx_bytes");
    RX_INSERT(RX_COMPRESSED, "rx_compressed");
    RX_INSERT(RX_CRC_ERRORS, "rx_crc_errors");
    RX_INSERT(RX_DROPPED, "rx_dropped");
    RX_INSERT(RX_ERRORS, "rx_errors");
    RX_INSERT(RX_FIFO_ERRORS, "rx_fifo_errors");
    RX_INSERT(RX_FRAME_ERRORS, "rx_frame_errors");
    RX_INSERT(RX_LENGTH_ERRORS, "rx_length_errors");
    RX_INSERT(RX_MISSED_ERRORS, "rx_missed_errors");
    RX_INSERT(RX_OVER_ERRORS, "rx_over_errors");
    RX_INSERT(RX_PACKETS, "rx_packets");

    TX_INSERT(TX_ABORTED_ERRORS, "tx_aborted_errors");
    TX_INSERT(TX_BYTES, "tx_bytes");
    TX_INSERT(TX_CARRIER_ERRORS, "tx_carrier_errors");
    TX_INSERT(TX_COMPRESSED, "tx_compressed");
    TX_INSERT(TX_DROPPED, "tx_dropped");
    TX_INSERT(TX_ERRORS, "tx_errors");
    TX_INSERT(TX_FIFO_ERRORS, "tx_fifo_errors");
    TX_INSERT(TX_HEARTBEAT_ERRORS, "tx_heartbeat_errors");
    TX_INSERT(TX_PACKETS, "tx_packets");
    TX_INSERT(TX_WINDOW_ERRORS, "tx_window_errors");
#undef TX_INSERT
#undef RX_INSERT
}

////////////////////////////////////////////////////////////////////////////////
// Constructors and destructor
////////////////////////////////////////////////////////////////////////////////

/**
    Setup path so we know where to go to get information: do not open any stats
    files or anything to actually do this monitoring: we'll wait until the user
    tells us what they want.
*/

network_stats::network_stats(const std::string interface):
    interface_name_(interface),
    interface_stats_path_()
{
    // If first call, then initialize this static map
    if (rx_fields_to_filename.empty() || tx_fields_to_filename.empty())
        build_fields_to_filename_maps();

    // This is the path to the dir that holds interface info
    std::string interface_path(SYSFS_PATH + interface_name_);

    // open/close dir for the interface to (a) see if it exists (b) is a dir
    // I'm going to assume if we get this right, then we should be able to
    // expect a stats dir and the stats files.
    DIR *dir = opendir(C(interface_path));
    if (!dir)
        ERROR("Cannot find/access network stats path '%s' for interface '%s'",
              C(interface_path), C(interface));

    int ret = closedir(dir);
    if (ret)
        ERROR("Error closing dir @ '%s' for interface '%s'",
              C(interface_path), C(interface));

    interface_stats_path_ = interface_path + STATS_DIR;
    CPRINT("Got interface stats path as '%s'\n", C(interface_stats_path_));
}

/**
    Added destructor to netdata, so I think I don't need this anymore.
    My bad.
*/

network_stats::~network_stats(void)
{
    CPRINT("Shutting down\n");

    // close RX stuff
    std::map<rx_fields, netdata>::iterator rx_i = rx_to_update_.begin();
    const std::map<rx_fields, netdata>::iterator rx_e = rx_to_update_.end();
    for ( ; rx_i != rx_e; ++rx_i)
    {
        close(rx_i->second.fd);
    }

    // close TX stuff
    std::map<tx_fields, netdata>::iterator tx_i = tx_to_update_.begin();
    const std::map<tx_fields, netdata>::iterator tx_e = tx_to_update_.end();
    for ( ; tx_i != tx_e; ++tx_i)
    {
        close(tx_i->second.fd);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Private
////////////////////////////////////////////////////////////////////////////////

/**
    Retrieve from data map whatever value we have for the Rx stat 'r'.  This
    value is not updated at this time: it is only retrieved.

    If we are not monitoring the value 'r', then returns 0.
*/

uint64_t
network_stats::fetch_one_rx(rx_fields r) const
{
    std::map<rx_fields, netdata>::const_iterator i = rx_to_update_.find(r);
    return (i == rx_to_update_.end()) ? 0 : i->second.value;
}

/**
    As above, but for a Tx stat.
*/

uint64_t
network_stats::fetch_one_tx(tx_fields t) const
{
    std::map<tx_fields, netdata>::const_iterator i = tx_to_update_.find(t);
    return (i == tx_to_update_.end()) ? 0 : i->second.value;
}

/**
    Given the file descriptor to a statistics file, retrieve the ASCII data
    from that file and convert it into something numeric and return that.
*/

uint64_t
network_stats::update_one(int fd)
{
    // rewind to beginning to read new value
    off_t ret = lseek(fd, 0, SEEK_SET);
    if (ret == (off_t)-1)
        ERROR("lseek for file descriptor %d", fd);

    // read value and check for basic validity
    char rbuf[READ_SIZE];
    ssize_t r_ret = read(fd, rbuf, READ_SIZE);
    if (r_ret == READ_SIZE)
        RUNTIME("Wow, actually read %d bytes from fd %d", (int)r_ret, fd);
    if (r_ret <= 0)
        ERROR("Read %d bytes from fd %d", (int)r_ret, fd);
    // Turn value into a string
    rbuf[r_ret] = '\0';

    // try and convert ASCII to numeric data
    char *endptr;
    long value = strtol(rbuf, &endptr, 10); // expect value to be decimal
    if (errno && (errno != EINTR))
        ERROR("Unable to convert network stat value '%s' to long for fd %d",
             rbuf, fd);

    return (uint64_t)value;
}

////////////////////////////////////////////////////////////////////////////////
// Public
////////////////////////////////////////////////////////////////////////////////

void
network_stats::set_rx_stats_to_update(const std::set<rx_fields> &to_update)
{
    using std::make_pair;

    std::set<rx_fields>::const_iterator i = to_update.begin();
    const std::set<rx_fields>::const_iterator e = to_update.end();
    for ( ; i != e; ++i)
    {
        if (rx_to_update_.count(*i) != 0)
        {
            CPRINT("For '%s': already monitoring -- ignoring request\n",
                   rx_name(*i));
            continue;
        }

        // get path for stat
        std::string statfile_path(interface_stats_path_ + rx_fields_to_filename[*i]);
        CPRINT("For '%s': opening stats file @ '%s'\n",
               rx_name(*i), C(statfile_path));

        // open file for stat
        int fd = open(C(statfile_path), O_RDONLY);
        if (fd == -1)
            ERROR("For '%s': opening stats file '%s'",
                  rx_name(*i), C(statfile_path));

        CPRINT("For '%s': got file descriptor as %d\n", rx_name(*i), fd);
        rx_to_update_.insert(make_pair(*i, netdata(fd)));
    }
}

/**
    As above, but for Tx stat.

    TODO: I might be able to unify with above using a template.
*/

void
network_stats::set_tx_stats_to_update(const std::set<tx_fields> &to_update)
{
    using std::make_pair;

    std::set<tx_fields>::const_iterator i = to_update.begin();
    const std::set<tx_fields>::const_iterator e = to_update.end();
    for ( ; i != e; ++i)
    {
        if (tx_to_update_.count(*i) != 0)
        {
            CPRINT("For '%s': already monitoring -- ignoring request\n",
                   tx_name(*i));
            continue;
        }


        std::string statfile_path(interface_stats_path_ + tx_fields_to_filename[*i]);
        CPRINT("For %s: opening stats file @ '%s'\n",
               tx_name(*i), C(statfile_path));

        int fd = open(C(statfile_path), O_RDONLY);
        if (fd == -1)
            ERROR("For %s: Opening stats file '%s'",
                  tx_name(*i), C(statfile_path));

        CPRINT("For '%s': got file descriptor as %d\n", tx_name(*i), fd);
        tx_to_update_.insert(make_pair(*i, netdata(fd)));
    }
}


/**
    Update everything we're monitoring
*/

void
network_stats::update_all(void)
{
    update_receive_data();
    update_transmit_data();
}

void
network_stats::update_receive_data(void)
{
    std::map<rx_fields, netdata>::iterator i = rx_to_update_.begin();
    std::map<rx_fields, netdata>::iterator e = rx_to_update_.end();
    for ( ; i != e; ++i)
    {
//      CPRINT("Updating value for '%s' with fd %d\n", rx_name(i->first), i->second.fd);
        i->second.value = update_one(i->second.fd);
    }
}

/**

*/

void
network_stats::update_transmit_data(void)
{
    std::map<tx_fields, netdata>::iterator i = tx_to_update_.begin();
    std::map<tx_fields, netdata>::iterator e = tx_to_update_.end();
    for ( ; i != e; ++i)
    {
//      CPRINT("Updating value for '%s' with fd %d\n", tx_name(i->first), i->second.fd);
        i->second.value = update_one(i->second.fd);
    }
}

/**
    Should get zeros for fields we aren't monitoring
*/

receive_data
network_stats::get_receive_data(void) const
{
    receive_data r;
    r.bytes         = fetch_one_rx(RX_BYTES);
    r.compressed    = fetch_one_rx(RX_COMPRESSED);
    r.CRC_errors    = fetch_one_rx(RX_CRC_ERRORS);
    r.dropped       = fetch_one_rx(RX_DROPPED);
    r.errors        = fetch_one_rx(RX_ERRORS);
    r.FIFO_errors   = fetch_one_rx(RX_FIFO_ERRORS);
    r.frame_errors  = fetch_one_rx(RX_FRAME_ERRORS);
    r.length_errors = fetch_one_rx(RX_LENGTH_ERRORS);
    r.missed_errors = fetch_one_rx(RX_MISSED_ERRORS);
    r.over_errors   = fetch_one_rx(RX_OVER_ERRORS);
    r.packets       = fetch_one_rx(RX_PACKETS);
    return r;
}

transmit_data
network_stats::get_transmit_data(void) const
{
    transmit_data t;
    t.aborted_errors    = fetch_one_tx(TX_ABORTED_ERRORS);
    t.bytes             = fetch_one_tx(TX_BYTES);
    t.carrier_errors    = fetch_one_tx(TX_CARRIER_ERRORS);
    t.compressed        = fetch_one_tx(TX_COMPRESSED);
    t.dropped           = fetch_one_tx(TX_DROPPED);
    t.errors            = fetch_one_tx(TX_ERRORS);
    t.FIFO_errors       = fetch_one_tx(TX_FIFO_ERRORS);
    t.heartbeat_errors  = fetch_one_tx(TX_HEARTBEAT_ERRORS);
    t.packets           = fetch_one_tx(TX_PACKETS);
    t.window_errors     = fetch_one_tx(TX_WINDOW_ERRORS);
    return t;
}

uint64_t
network_stats::get_rx_bytes(void) const
{
    return fetch_one_rx(RX_BYTES);
}

uint64_t
network_stats::get_rx_packets(void) const
{
    return fetch_one_rx(RX_PACKETS);
}

uint64_t
network_stats::get_tx_bytes(void) const
{
    return fetch_one_tx(TX_BYTES);
}

uint64_t
network_stats::get_tx_packets(void) const
{
    return fetch_one_tx(TX_PACKETS);
}

#undef CPRINT
#undef ALWAYS
#undef ERROR
#undef RUNTIME

