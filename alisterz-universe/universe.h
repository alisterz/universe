/****
          universe.h
                Clone this package from git://github.com/rtv/universe.git
          version 2
          Richard Vaughan
 ****/
/****
        alisterz version
 ****/
#include <vector>
#include <math.h> 
#include <stdio.h>
#include <stdlib.h>

//included for Ubuntu
#include <time.h>
#include <stdint.h>
#include <getopt.h>
#include <algorithm>
#include <iterator>
#include <pthread.h>

//include sockets header files
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <string.h>

#define GRAPHICS 1

// handy STL iterator macro pair. Use FOR_EACH(I,C){ } to get an iterator I to
// each item in a collection C.
#define VAR(V,init) __typeof(init) V=(init)
#define FOR_EACH(I,C) for(VAR(I,(C).begin());I!=(C).end();I++)
#define thread_no 4

namespace Uni {

    /** Convert radians to degrees. */
    inline double rtod(double r) {
        return ( r * 180.0 / M_PI);
    }

    /** Convert degrees to radians */
    inline double dtor(double d) {
        return ( d * M_PI / 180.0);
    }

    class Robot {
    public:

        class Pose {
        public:
            double x, y, a; // 2d position and orientation

            Pose(double x, double y, double a) : x(x), y(y), a(a) {
            }

            Pose() : x(0.0), y(0.0), a(0.0) {
            }

            // get a random pose

            static Pose Random() {
                return Pose(drand48() * Robot::worldsize,
                        drand48() * Robot::worldsize,
                        Robot::AngleNormalize(drand48() * (M_PI * 2.0)));
            }
        };

        class Speed {
        public:
            double v; // forward speed
            double w; // turn speed

            // constructor sets speeds to zero

            Speed() : v(0.0), w(0.0) {
            }
        };

        class Color {
        public:
            double r, g, b;

            Color(double r, double g, double b) :
            r(r), g(g), b(b) {
            }
        };

        // each robot has a vector of these to store its observations

        class Pixel {
        public:
            double range;
            Robot* robot;

            Pixel() : range(0.0), robot(NULL) {
            }
        };

        // STATIC DATA AND METHODS ------------------------------------------

        /** initialization: call this before using any other calls. */
        static void Init(int argc, char** argv);

        /** update all robots */
        static void UpdateAll();

        /** Normalize a length to within 0 to worldsize. */
        static double DistanceNormalize(double d);

        /** Normalize an angle to within +/_ M_PI. */
        static double AngleNormalize(double a);

        /** Start running the simulation. Does not return. */
        static void Run();

        static uint64_t updates; // number of simulation steps so far
        static uint64_t updates_max; // number of simulation steps to run before quitting (0 means infinity)
        static unsigned int sleep_msec; // number of milliseconds to sleep at each update
        static double worldsize; // side length of the toroidal world
        static double range; // sensor detects objects up tp this maximum distance
        static double fov; // sensor detects objects within this angular field-of-view about the current heading
        static unsigned int pixel_count; // number of pixels in sensor
        static std::vector<Robot*> population;
        static unsigned int population_size; // number of robots
        static bool paused; // runs only when this is false
        static bool show_data; // controls visualization of pixel data
        static int winsize; // initial size of the window in pixels
        static int displaylist; // robot body macro

        //new added static variables.
        static int counter;
        static std::vector< std::vector<int> > sections;
        static std::vector< std::vector< std::vector<int> > > sectionsxy;
        static int threadNumber;
        static int threadRatio;

        //sockets
        static struct sockaddr_in server_addr; //server address
        static struct sockaddr_in cli_addr;
        static struct hostent *server; //
        static int clientNumber; //number of client expected.
        static int client_id;
        static int server_socketfd;
        static int server_newsocketfd;
        static int server_portno;
        static int server_clilen;
        static int client_socketfd;
        static int client_newsocketfd;
        static int client_portno;
        static int client_clilen;

        //new implmentation for sockets
        static struct sockaddr_in *host_addrs;
        static struct sockaddr_in *connection_addrs;
        static struct hostent *hosts;
        static int hostNumber;
        static int *host_socketfd;
        static int *host_newsocketfd;
        static int *host_portno;
        static int *host_clilen;

#if GRAPHICS
        /** render all robots in OpenGL */
        static void DrawAll();
#endif

        // NON-STATIC DATA AND METHODS ------------------------------------------

        Pose pose; // robot is located at this pose
        Speed speed; // robot is moving this fast
        Color color; // robot's body has this color

        std::vector<Pixel> pixels; // robot's sensor data vector

        //new added. non static variables
        int robotNumber;
        int robotXSection;
        int robotYSection;

        // create a new robot with these parameters
        Robot(const Pose& pose, const Color& color);

        virtual ~Robot() {
        }

        // pure virtual - subclasses must implement this method
        virtual void Controller() = 0;

        // render the robot in OpenGL
        void Draw();

        // move the robot
        void UpdatePose();

        // update
        void UpdatePixels();

        int getRobotNumber();
        void removeRobot(int x, int y, int r);

        //threaded updatedPixel function
        static void *threadUpdatePixel(void *threadid);

        //single/multi thread to connect to all other hosts
        static void *threadConnection(void *connections);

        //single/multi thread to send updated population to all other hosts
        static void *threadSendPopulation();

        //return current robot's section
        int getRobotSection();

        //set robot's current section
        void setRobotSection(int s);

        //socket stuff
    };
}; // namespace Uni
