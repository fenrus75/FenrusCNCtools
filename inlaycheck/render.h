#pragma once



class render {
public:

    render(const char *filename);
    
    void load(void);
    
    /* gcode language */
    double width_mm, height_mm, depth_mm;
    void update_pixel(double X, double Y, double H);


    /* internal representation */
    int	   pixels_per_mm;
    int    width, height;
    int    minX,minY,maxX,maxY;
    double *pixels;
    double deepest;
    
    int mm_to_x(double X); 
    int mm_to_y(double Y); 
    double x_to_mm(int x); 
    double y_to_mm(int y); 
        
    void update_pixel(int x, int y, double H);
    
    void save_as_pgm(const char *filename);

    void crop(void);
    void cut_out(void);    
    void flip_over(void);
    
    double get_height(int x, int y);
    void set_offsets(int x, int y);
    
private:
    double ratio_x, ratio_y;
    class tool *tool;
    const char *fname;
    
    double cX, cY, cZ;
    int offsetX = 0;
    int offsetY = 0;
    double offsetZ = 0.0;
    
    void setup_canvas(void);
    
    void parse_line(const char *line);
    void parse_g_line(const char *line);
    void movement(double X1, double Y1, double Z1, double X2, double Y2,double Z2);
    void tooltouch(double X, double Y, double Z);
};