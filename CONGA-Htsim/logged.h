#ifndef LOGGED_H
#define LOGGED_H

/* A class that Logs */
class Logged
{
public:
    Logged(const std::string &name)
    {
        _name = name;
        id = LASTIDNUM;
        Logged::LASTIDNUM++;
    }

    virtual ~Logged() {}

    void setName(const std::string &name) { _name = name; }
    virtual const std::string& str() { return _name; };

    uint32_t id;
private:
    static uint32_t LASTIDNUM;
    std::string _name;
};

#endif 