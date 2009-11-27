/****
   universe.c
   version 2
   Richard Vaughan  
 ****/
/****
        alisterz version
 ****/

#include <assert.h>
#include <unistd.h>

#include "universe.h"
using namespace Uni;

const char* PROGNAME = "universe";

#if GRAPHICS
#include <glut/glut.h> // OS X users need <glut/glut.h> instead
#endif

// initialize static members
double Robot::worldsize(1.0);
double Robot::range(0.1);
double Robot::fov(dtor(270.0));
std::vector<Robot*> Robot::population;
unsigned int Robot::population_size(100);
unsigned int Robot::pixel_count(8);
unsigned int Robot::sleep_msec(50);
uint64_t Robot::updates(0);
uint64_t Robot::updates_max(0.0);
bool Robot::paused(false);
int Robot::winsize(600);
int Robot::displaylist(0);
bool Robot::show_data(true);
int Robot::threadNumber(4);
int Robot::threadRatio(population_size / threadNumber);

//new added initialization
int Robot::counter = 0;
std::vector< std::vector<int> > Robot::sections(10, std::vector<int>(0, 0));
//vector of vector of vector
std::vector< std::vector< std::vector<int> > > Robot::sectionsxy(10, std::vector< std::vector<int> >(10, std::vector<int>(0, 0)));
sockaddr_in Robot::server_addr;
sockaddr_in Robot::cli_addr;
hostent* Robot::server;
int Robot::server_socketfd(-1);
int Robot::server_newsocketfd(0);
int Robot::server_portno(60225);
int Robot::server_clilen(0);
int Robot::client_socketfd(-1);
int Robot::client_newsocketfd(0);
int Robot::client_portno(60225);
int Robot::client_clilen(0);
int Robot::client_id(0);


char usage[] = "Universe understands these command line arguments:\n"
        "  -? : Prints this helpful message.\n"
        "  -c <int> : sets the number of pixels in the robots' sensor.\n"
        "  -d  Disables drawing the sensor field of view. Speeds things up a bit.\n"
        "  -f <float> : sets the sensor field of view angle in degrees.\n"
        "  -p <int> : set the size of the robot population.\n"
        "  -r <float> : sets the sensor field of view range.\n"
        "  -s <float> : sets the side length of the (square) world.\n"
        "  -u <int> : sets the number of updates to run before quitting.\n"
        "  -w <int> : sets the initial size of the window, in pixels.\n"
        "  -z <int> : sets the number of milliseconds to sleep between updates.\n"
        "  -t <int> : Enable server mode. sets the number of clients\n"
        "  -g <char []> : Enable client mode. Connect to the server provided.\n";

#if GRAPHICS
// GLUT callback functions ---------------------------------------------------

// update the world - this is called whenever GLUT runs out of events
// to process

static void idle_func(void) {
    Robot::UpdateAll();
}

static void timer_func(int dummy) {
    glutPostRedisplay(); // force redraw
}

// draw the world - this is called whenever the window needs redrawn

static void display_func(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    Robot::DrawAll();
    glutSwapBuffers();

    // run this function again in about 50 msec
    glutTimerFunc(20, timer_func, 0);
}

static void mouse_func(int button, int state, int x, int y) {
    if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN)) {
        Robot::paused = !Robot::paused;
    }
}

// render all robots in OpenGL

void Robot::DrawAll() {
    FOR_EACH(r, population)
            (*r)->Draw();
}

#endif // GRAPHICS

Robot::Robot(const Pose& pose,
        const Color& color)
: pose(pose),
speed(),
color(color),
pixels() {
    // add myself to the static vector of all robots
    population.push_back(this);
    pixels.resize(pixel_count);
    robotNumber = counter;
    counter++;
    printf("Robot Number:%d\nRobot X:%e\nRobot Y:%e\nRobot A:%d\n\n", robotNumber, pose.x, pose.y, (int) rtod(AngleNormalize(pose.a)));
    robotYSection = (int) (pose.y * 10 / worldsize);
    robotXSection = (int) (pose.x * 10 / worldsize);
    sectionsxy[robotYSection][robotXSection].push_back(robotNumber);
    //sections[robotXSection].push_back(robotNumber);

}

void Robot::Init(int argc, char** argv) {
    // seed the random number generator with the current time
    srand48(1);

    // parse arguments to configure Robot static members
    int c;
    printf("[Uni] thread number: %d\n", threadNumber);
    while ((c = getopt(argc, argv, "?dp:s:f:r:c:u:z:w:g:t:")) != -1)
        switch (c) {

            case 'p':
                population_size = atoi(optarg);
                printf("[Uni] population_size: %d\n", population_size);
                break;

            case 's':
                worldsize = atof(optarg);
                printf("[Uni] worldsize: %.2f\n", worldsize);
                break;

            case 'f':
                fov = dtor(atof(optarg)); // degrees to radians
                printf("[Uni] fov: %.2f\n", fov);
                break;

            case 'r':
                range = atof(optarg);
                printf("[Uni] range: %.2f\n", range);
                break;

            case 'c':
                pixel_count = atoi(optarg);
                printf("[Uni] pixel_count: %d\n", pixel_count);
                break;

            case 'u':
                updates_max = atol(optarg);
                printf("[Uni] updates_max: %lu\n", (long unsigned) updates_max);
                break;

            case 'z':
                sleep_msec = atoi(optarg);
                printf("[Uni] sleep_msec: %d\n", sleep_msec);
                break;

#if GRAPHICS
            case 'w': winsize = atoi(optarg);
                printf("[Uni] winsize: %d\n", winsize);
                break;

            case 'd': show_data = false;
                puts("[Uni] hide data");
                break;
#endif			
            case '?':
                puts(usage);
                exit(0); // ok
                break;
            case 't':
                printf("Server\n");
                server_socketfd = socket(AF_INET, SOCK_STREAM, 0);
                server_addr.sin_family = AF_INET;
                server_addr.sin_addr.s_addr = INADDR_ANY;
                server_addr.sin_port = htons(server_portno);
                if (bind(server_socketfd, (struct sockaddr *) & server_addr,
                        sizeof (server_addr)) < 0) {
                    printf("Server: ERROR on binding.\n");
                }

                //loop, accept connections until max client reached.
                listen(server_socketfd, 5);
                printf("Waiting for Client Connection....\n");
                server_clilen = sizeof (cli_addr);
                server_newsocketfd = accept(server_socketfd, (struct sockaddr *) & cli_addr, (socklen_t *) & server_clilen);
                client_id = 0;
                /*char buffer[1024];
                int n;
                n = read(server_newsocketfd,buffer,sizeof(buffer));
                printf("Client: %s\n",buffer);
                n = write(server_newsocketfd,"Hi!",3);
			
                printf("Server Socket FD:%d\n",server_socketfd);*/
                break;
            case 'g':
                client_id = 1;
                printf("Client %d \n", atoi(optarg));
                if (server_socketfd >= 0) {
                    printf("Client: Universe in server mode, cannot connect to other Universe.\n");
                } else {
                    client_socketfd = socket(AF_INET, SOCK_STREAM, 0);
                    if (client_socketfd < 0) {
                        printf("Client: ERROR on opening port.\n");
                    }
                    server = gethostbyname(optarg);
                    if (server == NULL) {
                        printf("Client: ERROR on connecting server.\n");
                        exit(0);
                    }
                    bzero((char *) & server_addr, sizeof (server_addr));
                    server_addr.sin_family = AF_INET;
                    server_addr.sin_addr = *((struct in_addr *) server->h_addr);
                    server_addr.sin_port = htons(client_portno);
                    if (connect(client_socketfd, (struct sockaddr *) & server_addr, sizeof (server_addr)) < 0) {
                        printf("Client: ERROR on connecting \n");
                    }
                    /*printf("Please enter your message:");
                    char buffer[1024];
                    bzero(buffer,1024);
                    fgets(buffer,1023,stdin);
                    n = write(client_socketfd,buffer,1023);
                    n = read(client_socketfd,buffer,1023);
                    printf("Server: %s\n",buffer);*/
                }
                break;

            default:
                fprintf(stderr, "[Uni] Option parse error.\n");
                puts(usage);
                exit(-1); // error
        }
    threadRatio = population_size / threadNumber;
#if GRAPHICS
    // initialize opengl graphics
    glutInit(&argc, argv);
    glutInitWindowSize(winsize, winsize);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutCreateWindow(PROGNAME);
    glClearColor(0.8, 0.8, 1.0, 1.0);
    glutDisplayFunc(display_func);
    glutTimerFunc(50, timer_func, 0);
    glutMouseFunc(mouse_func);
    glutIdleFunc(idle_func);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(1.0 / Robot::worldsize, 1.0 / Robot::worldsize, 1);

    // define a display list for a robot body
    double h = 0.01;
    double w = 0.01;

    glPointSize(4.0);

    displaylist = glGenLists(1);
    glNewList(displaylist, GL_COMPILE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBegin(GL_POLYGON);
    glVertex2f(h / 2.0, 0);
    glColor3f(0, 0, 0); // black
    glVertex2f(-h / 2.0, w / 2.0);
    glVertex2f(-h / 2.0, -w / 2.0);
    glEnd();

    glEndList();
#endif // GRAPHICS
}

void Robot::UpdatePixels() {
    double radians_per_pixel = fov / (double) pixel_count;

    double halfworld = worldsize * 0.5;

    // initialize pixels vector

    FOR_EACH(it, pixels) {
        it->range = Robot::range; // maximum range
        it->robot = NULL; // nothing detected
    }

    // check every robot in the world to see if it is detected

    //std::vector<Robot*> neighbors;

    //fix range issue.
    int view_range = (int) (10 * Robot::range);
    int* range_x = new int[2];
    int* range_y = new int[2];
    //x
    range_x[0] = robotXSection - view_range;
    range_x[1] = robotXSection;
    range_x[2] = robotXSection + view_range;
    if (range_x[0] < 0)
        range_x[0] = 9;
    if (range_x[2] > 9)
        range_x[2] = 0;
    //y
    range_y[0] = robotYSection - view_range;
    range_y[1] = robotYSection;
    range_y[2] = robotYSection + view_range;
    if (range_y[0] < 0)
        range_y[0] = 9;
    if (range_y[2] > 9)
        range_y[2] = 0;
	
	//start loop to populate pixels
    for (int i = 0; i <= 2; i++) {
        for (int j = 0; j <= 2; j++) {
            for (std::vector<int>::iterator it = sectionsxy[range_y[i]][range_x[j]].begin(); it != sectionsxy[range_y[i]][range_x[j]].end(); ++it) {
                Robot* other = population[*it];

                // discard if it's the same robot
                if (other == this)
                    continue;

                // discard if it's out of range. We put off computing the
                // hypotenuse as long as we can, as it's relatively expensive.

                double dx = other->pose.x - pose.x;

                // wrap around torus
                if (dx > halfworld)
                    dx -= worldsize;
                else if (dx < -halfworld)
                    dx += worldsize;

                if (fabs(dx) > Robot::range)
                    continue; // out of range

                double dy = other->pose.y - pose.y;

                // wrap around torus
                if (dy > halfworld)
                    dy -= worldsize;
                else if (dy < -halfworld)
                    dy += worldsize;

                if (fabs(dy) > Robot::range)
                    continue; // out of range

                double range = hypot(dx, dy);
                if (range > Robot::range)
                    continue;

                // discard if it's out of field of view
                double absolute_heading = atan2(dy, dx);
                double relative_heading = AngleNormalize((absolute_heading - pose.a));
                if (fabs(relative_heading) > fov / 2.0)
                    continue;

                // find which pixel it falls in
                int pixel = floor(relative_heading / radians_per_pixel);
                pixel += pixel_count / 2;
                pixel %= pixel_count;

                assert(pixel >= 0);
                assert(pixel < (int) pixel_count);

                // discard if we've seen something closer in this pixel already.
                //if(pixels[pixel].range == range)
                //printf("Range is same");
                if (pixels[pixel].range < range)
                    continue;
                // if we made it here, we see this other robot in this pixel.
                pixels[pixel].range = range;
                pixels[pixel].robot = other;
            }
        }
    }
}

void Robot::UpdatePose() {
    // move according to the current speed
    double dx = speed.v * cos(pose.a);
    double dy = speed.v * sin(pose.a);
    ;
    double da = speed.w;

    pose.x = DistanceNormalize(pose.x + dx);
    pose.y = DistanceNormalize(pose.y + dy);
    pose.a = AngleNormalize(pose.a + da);

    int tmp_section_x = (int) (pose.x * 10 / worldsize);
    int tmp_section_y = (int) (pose.x * 10 / worldsize);
    if (tmp_section_x != robotXSection || tmp_section_y != robotYSection) {
        removeRobot(robotXSection, robotYSection, robotNumber);
        robotXSection = tmp_section_x;
        robotYSection = tmp_section_y;
        sectionsxy[robotYSection][robotXSection].push_back(robotNumber);
    }
    //generate message for socket
    double doubleMsg[4];
    doubleMsg[0] = (double) this->getRobotNumber();
    doubleMsg[1] = pose.x;
    doubleMsg[2] = pose.y;
    doubleMsg[3] = pose.a;
    //int n;
    if (client_id == 0) {
        //n = write(server_newsocketfd,doubleMsg,4*sizeof(double));
    } else {
        //n = write(client_socketfd,doubleMsg,4*sizeof(double));
    }
}

void Robot::UpdateAll() {
    bool print = true;
    // if we've done enough updates, exit the program
    if (updates_max > 0 && updates > updates_max) {
        if (print) {
            printf("\n**********************************\n\n");
            printf("Final Robot Positions \n\n");
            printf("Thread No. : %d\n", threadNumber);
            printf("THread Ratio : %d\n", threadRatio);

            FOR_EACH(r, population) {
                printf("Robot Number : %d\n", (*r)->getRobotNumber());
                printf("Robot X : %e\n", (*r)->pose.x);
                printf("Robot Y: %e\n", (*r)->pose.y);
                //printf("Robot Section: %d\n", (*r)->getRobotSection());
                printf("\n");
            }

        }
        exit(1);
    }

    if (!Robot::paused) {

        FOR_EACH(r, population)
                (*r)->UpdatePose();
        pthread_t threads[threadNumber];
        int rc;
        int i;

        //update pixel with threads

        for (i = 0; i < threadNumber; i++) {
            rc = pthread_create(&threads[i], NULL, threadUpdatePixel, (void *) i);
        }
        for (i = 0; i < threadNumber; i++) {
            rc = pthread_join(threads[i], NULL);
        }

        /*
        int bound_min = 0;
        int bound_max = population_size;
        if(client_id == 0){
                bound_max = population_size/2;
        }
        if(client_id == 1){
                bound_min = population_size/2;
        }
        //printf("Bound Min: %d \n",bound_min);
        //printf("Bound Max: %d \n",bound_max);
        //printf("Running.\n");
        int p = (int) population_size;
		
        for(int i = bound_min;i<bound_max;i++)
        {
                population[i]->UpdatePose();
        }
        int counter=0;
        int diff = bound_max-bound_min;
        while(counter<p-diff)
        {
                double x,y,a;
                double doubleMsg[4];
                int r;
                if(client_id == 0){
                        read(server_newsocketfd,doubleMsg,4*sizeof(double));
                }
                else
                {
                        read(client_socketfd,doubleMsg,4*sizeof(double));
                }
                r = (int)doubleMsg[0];
                population[r]->pose.x = doubleMsg[1];
                population[r]->pose.y = doubleMsg[2];
                population[r]->pose.a = doubleMsg[3];
                //printf("Robot : %d \nX: %e\nY:%e\na:%ef\n",r,x,y,a);
                //printf("Robot: %d\n",r);
                int tmp_section = (int)(doubleMsg[1]*10);
                if(tmp_section!=population[r]->getRobotSection())
                {
                        population[r]->removeRobot(population[r]->getRobotSection(),population[r]->getRobotNumber());
                        population[r]->setRobotSection(tmp_section);
                        sections[population[r]->getRobotSection()].push_back(population[r]->getRobotNumber());
                  }
                counter++;
        }/*/

        int bound_min(0);
        int bound_max(population_size);
        //update pixels
        for (i = bound_min; i < bound_max; i++) {
            //printf("Robot: %d\n",population[i]->getRobotNumber());
            //population[i]->UpdatePixels();
        }

        //update controllers
        for (i = bound_min; i < bound_max; i++) {
            population[i]->Controller();
        }
    }

    ++updates;

    // possibly snooze to save CPU and slow things down
    if (sleep_msec > 0)
        usleep(sleep_msec * 1e3);
}

// draw a robot

void Robot::Draw() {
#if GRAPHICS
    glPushMatrix();
    glTranslatef(pose.x, pose.y, 0);
    glRotatef(rtod(pose.a), 0, 0, 1);

    glColor3f(color.r, color.g, color.b);

    // draw the pre-compiled triangle for a body
    glCallList(displaylist);

    if (Robot::show_data) {
        // render the sensors
        double rads_per_pixel = fov / (double) pixel_count;
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        for (unsigned int p = 0; p < pixel_count; p++) {
            double angle = -fov / 2.0 + (p + 0.5) * rads_per_pixel;
            double dx1 = pixels[p].range * cos(angle + rads_per_pixel / 2.0);
            double dy1 = pixels[p].range * sin(angle + rads_per_pixel / 2.0);
            double dx2 = pixels[p].range * cos(angle - rads_per_pixel / 2.0);
            double dy2 = pixels[p].range * sin(angle - rads_per_pixel / 2.0);

            glColor4f(1, 0, 0, pixels[p].robot ? 0.2 : 0.05);

            glBegin(GL_POLYGON);
            glVertex2f(0, 0);
            glVertex2f(dx1, dy1);
            glVertex2f(dx2, dy2);
            glEnd();
        }
    }

    glPopMatrix();
#endif // GRAPHICS
}

void Robot::Run() {
#if GRAPHICS
    glutMainLoop();
#else
    while (1)
        UpdateAll();
#endif
}

/** Normalize a length to within 0 to worldsize. */
double Robot::DistanceNormalize(double d) {
    while (d < 0) d += worldsize;
    while (d > worldsize) d -= worldsize;
    return d;
}

/** Normalize an angle to within +/_ M_PI. */
double Robot::AngleNormalize(double a) {
    while (a < -M_PI) a += 2.0 * M_PI;
    while (a > M_PI) a -= 2.0 * M_PI;
    return a;
}


//remove a robot from a section.

void Robot::removeRobot(int x, int y, int r) {
    //printf("Remove Robot Number %d From Section %d \n", r,s);
    std::vector<int>::iterator it = find(sectionsxy[y][x].begin(), sectionsxy[y][x].end(), r);
    if (it != sectionsxy[y][x].end()) {
        sectionsxy[y][x].erase(it);
        //printf("Robot Number %d is removed \n", r);
    }
    //	else
    //printf("Robot is not found! \n");
}

int Robot::getRobotNumber() {
    return robotNumber;
}

void *Robot::threadUpdatePixel(void *ptr) {
    /*(int)ptr;
    int a = id;*/
    //	printf("Hello World From %d. ",(int)ptr);
    long int left = (long int) threadRatio * (long int) ptr;
    long int right = (long int) threadRatio * ((long int) ptr + 1);
    //	printf("My range is %d to %d\n",left,right);
    for (long int i = left; i < right; i++) {
        //		printf("Thread %d: Updating Robot No[%d]\n",(int)ptr,i);
        population[i]->UpdatePixels();
    }
    //printf("Hello World From %d. My range is %d to %d\n",(int)ptr,left, right);
    //printf("Thread %d finished.\n",(int)ptr);
    pthread_exit(NULL);
}

int Robot::getRobotSection() {
    return robotXSection;
}

void Robot::setRobotSection(int s) {
    robotXSection = s;
}
