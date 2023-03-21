/*
 * logfile
 */
#include "logfile.h"

#include <stdlib.h>

using namespace std;

Logfile::Logfile(const string &filename, 
                 simtime_picosec start, 
                 simtime_picosec end)
                : _start(start), 
                _end(end), 
                _nRecords(0), 
                _nTotalRecords(0)
{
    string traceFile = filename + ".trace";
    _trace_file = fopen(traceFile.c_str(), "wbS");
    if (_trace_file == NULL) {
        cerr << "Failed to open logfile " << traceFile << endl;
        exit(1);
    }

    // Write a 4-byte #records entry at the start, to be overwritten later.
    fwrite(&_nTotalRecords, sizeof(uint32_t), 1, _trace_file);

    string idFile = filename + ".id";
    _id_file = fopen(idFile.c_str(), "wbS");
    if (_id_file == NULL) {
        cerr << "Failed to open logfile " << idFile << endl;
        exit(1);
    }
    
    string txtFile = filename + ".txt";
    _txt_file = fopen(txtFile.c_str(), "w");

    _records = (struct Record *) malloc(MAX_RECORDS * sizeof(struct Record));
}

Logfile::~Logfile()
{
    if (_trace_file != NULL) {
        // Flush remaining records in buffer to file.
        fwrite(_records, sizeof(struct Record), _nRecords, _trace_file);

        fseek(_trace_file, 0, SEEK_SET);
        fwrite(&_nTotalRecords, sizeof(uint32_t), 1, _trace_file);
        fclose(_trace_file);
    }

    if (_id_file != NULL) {
        fclose(_id_file);
    }

    fclose(_txt_file);
}

void 
Logfile::write(const string &msg)
{
    fputs(msg.c_str(), _id_file);
}

void
Logfile::writeName(uint32_t id, string name) {
    write(name + "=" + to_string(id) + "\n");
    _idNameMap[id] = name;
}

void
Logfile::writeRecordWithTxt(uint32_t type, uint32_t id, string evstr, uint32_t ev, string val1str, double val1, string val2str, double val2, string val3str, double val3) {
    fprintf(_txt_file, "%f: %s: %s: %s: %s-%f, %s-%f, %s-%f\n",timeAsSec(EventList::Get().now()), _evTypeMap[type].c_str(), _idNameMap[id].c_str(), 
        evstr.c_str(), val1str.c_str(), val1, val2str.c_str(), val2, val3str.c_str(), val3);
    writeRecord(type, id, ev, val1, val2, val3);
}

void
Logfile::writeRecord(uint32_t type, 
                     uint32_t id, 
                     uint32_t ev, 
                     double val1, 
                     double val2, 
                     double val3)
{
    simtime_picosec current_ts = EventList::Get().now();
    if (current_ts < _start || current_ts > _end) {
        return;
    }

    uint32_t i = _nRecords;

    _records[i].time = timeAsSec(current_ts);
    _records[i].type = type;
    _records[i].id   = id;
    _records[i].ev   = ev;
    _records[i].val1 = val1;
    _records[i].val2 = val2;
    _records[i].val3 = val3;

    _nRecords++;
    _nTotalRecords++;

    // Flush to file if buffer full.
    if (_nRecords == MAX_RECORDS) {
        fwrite(_records, sizeof(struct Record), MAX_RECORDS, _trace_file);
        _nRecords = 0;
    }
}
