/*
 * logfile header
 */
#ifndef LOGFILE_H
#define LOGFILE_H

#include "htsim.h"
#include "eventlist.h"

#include <string>
#include <unordered_map>

/*
 * Logfile is a class for specifying the log file format.
 * The loggers (loggers.h) face both
 *  1. the log file, using the base class Logger (defined here)
 *  2. the simulator, using the base classes in loggertypes.h
 */

#define MAX_RECORDS 100000

struct __attribute__((__packed__)) Record {
    double   time;
    uint32_t type;
    uint32_t id;
    uint32_t ev;
    double   val1;
    double   val2;
    double   val3;
};

class Logfile;

class Logfile
{
    public:
        Logfile(const std::string &filename,
                simtime_picosec start = 0,
                simtime_picosec end = ULLONG_MAX);
        ~Logfile();

        void write(const std::string &msg);
        void writeName(uint32_t id, std::string name);
        void writeRecordWithTxt(uint32_t type, uint32_t id, std::string evstr, uint32_t ev, 
            std::string val1str, double val1, std::string val2str, double val2, 
            std::string val3str, double val3);
        void writeRecord(uint32_t type, uint32_t id, uint32_t ev,
                double val1, double val2, double val3);
        void writeTxt(std::string txt);

    private:
        FILE *_trace_file;
        FILE *_id_file;
        FILE *_txt_file;

        simtime_picosec _start;
        simtime_picosec _end;

        struct Record *_records;
        uint32_t _nRecords;
        uint32_t _nTotalRecords;

        std::unordered_map<unsigned int, std::string> _evTypeMap = {{0, "QUEUE_EVENT"}, {1, "TCP_EVENT"}, {2, "TCP_STATE"}, {3, "TRAFFIC_EVENT"}, 
            {4, "QUEUE_RECORD"}, {5, "QUEUE_APPROX"}, {6, "TCP_RECORD"}, {11, "TCP_SINK"}};
        std::unordered_map<uint32_t, std::string> _idNameMap;
};

#endif /* LOGFILE_H */
