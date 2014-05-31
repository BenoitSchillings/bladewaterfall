
#include <GL/glut.h>
#include <unistd.h>     // Header file for sleeping.
#include <stdio.h>      // Header file for standard file i/o.
#include <stdlib.h>     // Header file for malloc/free.
#include <memory.h>
#include <math.h>
#include <stdarg.h>

const int startwinsize = 400; // Starting window width & height, in pixels
const int ESCKEY = 27;        // ASCII value of escape character

#define WIDTH 1024 
const int img_width = WIDTH;
const int img_height = 768;
GLubyte the_image[img_height*img_width];
double zoom = 1.0;
const double maxzoom = 10.0;
const double minzoom = -10.0;
int last_x = 0;
int last_y = 0;
int display_mode = 0;       //0 = waterfall. 1 = 3d
int frame = 0;


GLfloat     vectors[img_height * img_width * 3];
GLuint     indices[img_height * img_width];
GLubyte     colors[3 * img_height * img_width];

//------------------------------------------------------------------
#define uchar unsigned char
//------------------------------------------------------------------
extern void retune(float delta);
extern int init_fft(int argc, char **argv);
extern uchar *do_input();
extern float get_frequency();
extern double nanotime();
//------------------------------------------------------------------


void __print(char *message, float pos_x, float pos_y)
{
    glPushMatrix();
    glTranslatef(pos_x/32.0 - 16, -pos_y/33.0 + 12.2, 0);
    glColor4f(1, 1.0, 1.0, 1);
    glScalef(0.003, 0.003, 0.003);
    
	while (*message) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN,*message++);
	}
    glPopMatrix();
}

//------------------------------------------------------------------

int gprint(float pos_x, float pos_y, const char * format, ... )
{
    char buf[512];
    
    va_list args;
    va_start(args, format);
    vsprintf(buf,format, args);// This still uses standaes formating
    va_end(args);
    __print(buf, pos_x, pos_y);
    return 0;
}


//------------------------------------------------------------------

float rot_x = 0;
float rot_y = 0;

float k = 0;
void ddraw()
{
    double t0, t1;
    
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    glEnable(GL_LINE_SMOOTH);
    //glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.0);
    
    
    glTranslatef(0, 0, -8);
    glRasterPos2d(-16, -12);
    
    t0 = nanotime();
    
    glPixelZoom(zoom, zoom);
    if (display_mode == 0) {
        glDrawPixels(img_width, img_height,
                     GL_LUMINANCE, GL_UNSIGNED_BYTE,
                     the_image);
        t1 = nanotime();
        
        gprint(30, 27,  "Frequency: %f Mhz\n", get_frequency()/1e6);
        gprint(230, 27,  "time: %f\n", t1-t0);
        glFlush();
        
    }
    
    
    
    
    if (display_mode == 2) {
        glLoadIdentity();
        glTranslatef(0, -4, -14);
        
        glRotatef(rot_y + 30, 1, 0, 0);
        glRotatef(rot_x, 0, 1, 0);
        
        
        glColor4f(1, 1.0, 1.0, 0.08);
        
        int x,z;
        glPointSize(0);
        glBegin(GL_POINTS);
        for (z = 2; z < img_height; z++) {
            for (x = 0; x < img_width; x ++) {
                float target_x = (x/(WIDTH*1.0)) - 0.5;
                float target_z = (z/1000.0) - 0.5;
                target_x *= 44.0;
                target_z *= 24.0;
                
                float height = the_image[(img_height-z)*WIDTH + x];
                if (x % 30 == 0) {
                    glColor3f(height/255.0, height/255.0, height/255.0);
                    glVertex3f(target_x, sqrt(height/18.0), target_z);
                }
            }
        }
        glEnd();
        glFlush();
        t1 = nanotime();
        
        gprint(30, 27,  "Frequency: %f Mhz\n", get_frequency()/1e6);
        gprint(230, 27,  "timez: %f\n", t1-t0);
        
        glFlush();
    }
    
    
    if (display_mode == 1) {
        glEnableClientState( GL_VERTEX_ARRAY );
        
        glLoadIdentity();
        glTranslatef(0, -4, -14);
        
        glRotatef(rot_y + 30, 1, 0, 0);
        glRotatef(rot_x, 0, 1, 0);
        
        glColor4f(1, 1.0, 1.0, 0.1);
        
        int x,z;
        glPointSize(0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_POINT_SMOOTH);
        glEnableClientState( GL_COLOR_ARRAY );
        int idx = 0;
        
        
        for (z = 0; z < img_height; z++) {
            for (x = 0; x < img_width; x++) {
                float target_x = (x/1000.0) - 0.5;
                float target_z = (z/1000.0) - 0.5;
                target_x *= 44.0;
                target_z *= 64.0;
                target_z -= 10;
                float height = the_image[(img_height-z)*WIDTH + x];
                colors[idx] = height;
                colors[idx+1] = height;
                colors[idx+2] = height;
                
                vectors[idx++] = target_x;
                
                vectors[idx++] = height/58.0;
                vectors[idx] = target_z;
                idx++;
            }
        }
        glColorPointer( 3, GL_UNSIGNED_BYTE, 0, colors );
        glVertexPointer(3, GL_FLOAT, 0, vectors);
        glDrawElements(GL_POINTS, sizeof(indices) / sizeof(GLuint), GL_UNSIGNED_INT, indices);
    }
    
    glFlush();
    t1 = nanotime();
    
    
    gprint(230, 27,  "timez: %f\n", t1-t0);
    
    glFlush();
    
    frame++;
    
}

void display()
{
    ddraw();
    glFlush(); 
    glutSwapBuffers(); 
    return;
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Draw image
    glRasterPos2d(0, 0);
    glPixelZoom(zoom, zoom);
    glDrawPixels(img_width, img_height,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 the_image);
    
    //ddraw();
    // Draw instructions
    glColor3d(0.0, 0.0, 0.0);
    
    glutSwapBuffers();
}

//------------------------------------------------------------------

// reshape
// The GLUT reshape function
void reshape(int w, int h)
{
    w = WIDTH;
    h = 256;
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, w, 0.0, h, 100, -100);
}

//------------------------------------------------------------------

// keyboard
// The GLUT keyboard function
void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
        case ESCKEY:  // ESC: Quit
            exit(0);
            break;
        case ' ':     // Space: reset zoom
            display_mode ^= 1;
            glutPostRedisplay();
            break;
    }
}

//------------------------------------------------------------------

// special
// The GLUT special function
void special(int key, int x, int y)
{
    switch(key)
    {
        case GLUT_KEY_RIGHT: // Zoom goes up
            retune(800000);
            break;
        case GLUT_KEY_LEFT:  // Zoom goes down
            retune(-800000);
            break;
    }
}

//------------------------------------------------------------------

void makeimage()
{
    int cnt = img_width * img_height;
    
    for (int i = 0; i < cnt; i++) {
        the_image[i] = i;
    }
}

//------------------------------------------------------------------
const int avg_count = 1;
//------------------------------------------------------------------

int   cur_line = 0;
float sum[img_width];

void sum_line(uchar *scanline)
{
    for (int i = 0; i < img_width; i++) {
        sum[i] += *scanline++;
    }
}

//------------------------------------------------------------------

void add_line(int n, uchar *scanline)
{
    uchar   *dest_ptr;
    
    sum_line(scanline);
    
    cur_line++;
    
    if (cur_line == avg_count) {
        cur_line = 0;
        int bcnt = ((img_height-1) * img_width);
        
        int cnt = img_width;
        dest_ptr = the_image + bcnt;
        
        for (int i = 0; i < img_width; i++) {
            *dest_ptr++ = sum[i]/avg_count;
            sum[i] = 0;
            //memcpy(dest_ptr, scanline, cnt);
        }
        
        double t0, t1;
        t0 = nanotime();
        memmove(the_image, the_image + (img_width), bcnt);
        t1 = nanotime();
    }
}

//------------------------------------------------------------------


int n = 0;

//------------------------------------------------------------------
uchar read_in[WIDTH];
FILE *input_file = NULL;
long position = 0;

//------------------------------------------------------------------

void idle(int tmp)
{
    int     cnt;
    
    
    if (input_file == NULL) {
        input_file = fopen("./buffer.bin", "rb");
    }
    
    long avail = fseek(input_file, 0, SEEK_END);
    avail = ftell(input_file);
    avail /= WIDTH;
    
    int max = 516;
    
    while (avail > position && max) {
        fseek(input_file, position * WIDTH, SEEK_SET);
        cnt = fread(read_in, sizeof(uchar), WIDTH, input_file);
        
        position++;
        
        add_line(0, read_in);
        max--;
    }
    
    glutPostRedisplay();
    
    glutTimerFunc(3, idle, 0);
}

//------------------------------------------------------------------

void mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
    {
    }
    last_x = x;
    last_y = y;
}

//------------------------------------------------------------------


void motion(int x, int y)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -8);
    rot_y = (y - 430)/12.0;
    rot_x = (x - 512)/29.0;
    
    last_x = x;
    last_y = y;
    
    
    glRotatef(rot_y, 1, 0, 0);
    glRotatef(rot_x, 0, 1, 0);
    
}

//------------------------------------------------------------------

void init()
{
    glClearColor(1.0, 1.0, 1.0, 0.0);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    makeimage();
}

//------------------------------------------------------------------

void grid_init() {
    
    printf("init\n");
    // Set the current clear color to sky blue and the current drawing color to
    // white.
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glColor3f(1.0, 1.0, 1.0);
    
    // Tell the rendering engine not to draw backfaces.  Without this code,
    // all four faces of the tetrahedron would be drawn and it is possible
    // that faces farther away could be drawn after nearer to the viewer.
    // Since there is only one closed polyhedron in the whole scene,
    // eliminating the drawing of backfaces gives us the realism we need.
    // THIS DOES NOT WORK IN GENERAL.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Set the camera lens so that we have a perspective viewing volume whose
    // horizontal bounds at the near clipping plane are -2..2 and vertical
    // bounds are -1.5..1.5.  The near clipping plane is 1 unit from the camera
    // and the far clipping plane is 40 units away.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-2, 2, -1.5, 1.5, 1, 340);
    glEnable(GL_BLEND);
    // Set up transforms so that the tetrahedron which is defined right at
    // the origin will be rotated and moved into the view volume.  First we
    // rotate 70 degrees around y so we can see a lot of the left side.
    // Then we rotate 50 degrees around x to "drop" the top of the pyramid
    // down a bit.  Then we move the object back 3 units "into the screen".
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -5);
    glRotatef(rot_y, 1, 0, 0);
    glRotatef(rot_x, 0, 1, 0);
}




int main(int argc, char ** argv)
{
    init_fft(argc, argv);
  
    for (int i = 0; i < 256; i++) {
        colors[i * 3 + 0] = i;
        colors[i * 3 + 1] = i;
        colors[i * 3 + 2] = i;
    }

    for (int i =0; i < img_width * img_height; i++) {
        indices[i] = i;
    }
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    
    glutInitWindowSize(WIDTH, 800);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("benoit waterfall");
    
    
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    
    glutTimerFunc(100, idle, 0);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutDisplayFunc(display);
    grid_init();
    
    glutMainLoop();
    
    return 0;
}

