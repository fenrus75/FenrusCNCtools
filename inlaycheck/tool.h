#pragma once


class tool {
public:
    double scanzone;
    virtual double get_height(double R, double depth);
    int samplesteps = 2;
    int number;
};


class vbit:public tool {
public:
    vbit(double angle);
    virtual double get_height(double R, double depth);
    double get_height_static(double R, double depth);
    double _angle;
private:
    double _tan;
    double _invtan;
};

class flat:public tool {
public:
    flat(double diameter);
    virtual double get_height(double R, double depth);
private:
    double _diameter;
    double _radius;
};