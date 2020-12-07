#pragma once

#include <stdio.h>

#include <vector>

enum elements {
    Abstract,
    Container,
    Raw_gcode
};

class element {
public:
    enum elements type;
    
    /* bounding box */
    double minX, minY, minZ, maxX, maxY, maxZ;
    virtual void append_gcode(FILE *file) = 0;
    
private:
};

class container: public element {
public:
    container(const container &obj);
    container(void);
    
    ~container(void);
    
    void push(class element * element);
    
    virtual void append_gcode(FILE *file);
    
private:
    std::vector<class element *> elements;
};


class raw_gcode: public element {
public:
    raw_gcode(const raw_gcode &obj);
    raw_gcode(const char *str);
    ~raw_gcode(void); 

    virtual void append_gcode(FILE *file);
private:
    char *gcode_string;
};
