#pragma once


class tool {
public:
    double scanzone;
    virtual double get_height(double R, double depth);
    int samplesteps = 2;
};


class vbit:public tool {
public:
    vbit(double angle);
    virtual double get_height(double R, double depth);
private:
    double _angle;
    double _tan;
};

class flat:public tool {
public:
    flat(double diameter);
    virtual double get_height(double R, double depth);
private:
    double _diameter;
};