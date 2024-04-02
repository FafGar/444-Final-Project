#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <math.h>

#include "cube.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>


using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::mat3;
using std::vector;

int tessLevel;

GLuint program;
GLuint pointProgram;

int lastTime;
int nFrames;

vec3 **controlPoints;
vec3 **du, **dv;
int M = 10;
int N = 10; // the dimensions of the control points matrix for a total of MxN control points

GLuint buf = 0;
GLuint vao = 0;

void readShader(const char* fname, char *source)
{
	FILE *fp;
	fp = fopen(fname,"r");
	if (fp==NULL)
	{
		fprintf(stderr,"The shader file %s cannot be opened!\n",fname);
		glfwTerminate();
		exit(1);
	}
	char tmp[300];
	while (fgets(tmp,300,fp)!=NULL)
	{
		strcat(source,tmp);
		strcat(source,"\n");
	}
}

unsigned int loadShader(const char *source, unsigned int mode)
{
	GLuint id;
	id = glCreateShader(mode);

	glShaderSource(id,1,&source,NULL);

	glCompileShader(id);

	char error[1000];

	glGetShaderInfoLog(id,1000,NULL,error);
	printf("Compile status: \n %s\n",error);

	return id;
}

void initShaders()
{
	char *vsSource, *fsSource, *gsSource, *tcsSource, *tesSource, *pointVSource, *pointFSource;
	GLuint vs,fs,gs,tcs,tes,pointV,pointF;

	vsSource = (char *)malloc(sizeof(char)*20000);
	fsSource = (char *)malloc(sizeof(char)*20000);
	gsSource = (char *)malloc(sizeof(char)*20000);
	tcsSource = (char *)malloc(sizeof(char)*20000);
	tesSource = (char *)malloc(sizeof(char)*20000);
    pointVSource = (char *)malloc(sizeof(char)*20000);
	pointFSource = (char *)malloc(sizeof(char)*20000);


	vsSource[0]='\0';
	fsSource[0]='\0';
	gsSource[0]='\0';
	tcsSource[0]='\0';
	tesSource[0]='\0';
    pointVSource[0]='\0';
	pointFSource[0]='\0';

    pointProgram = glCreateProgram();

    readShader("shader/pointShader.vs", pointVSource);
    readShader("shader/pointShader.fs", pointFSource);

    pointV = loadShader(pointVSource,GL_VERTEX_SHADER);
    pointF = loadShader(pointFSource,GL_FRAGMENT_SHADER);

    glAttachShader(pointProgram,pointV);
    glAttachShader(pointProgram,pointF);
    
    glLinkProgram(pointProgram);

    glUseProgram(pointProgram);


	program = glCreateProgram();

	readShader("shader/waterVertex.glsl",vsSource);
	readShader("shader/waterFragment.glsl",fsSource);
	readShader("shader/waterGeometry.glsl",gsSource);
	readShader("shader/waterTessC.glsl",tcsSource);
	readShader("shader/waterTessE.glsl",tesSource);

	vs = loadShader(vsSource,GL_VERTEX_SHADER);
	fs = loadShader(fsSource,GL_FRAGMENT_SHADER);
	gs = loadShader(gsSource,GL_GEOMETRY_SHADER);
	tcs = loadShader(tcsSource,GL_TESS_CONTROL_SHADER);
	tes = loadShader(tesSource,GL_TESS_EVALUATION_SHADER);

	glAttachShader(program,vs);
	glAttachShader(program,fs);
	glAttachShader(program,gs);
	glAttachShader(program,tcs);
	glAttachShader(program,tes);

	glLinkProgram(program);

	glUseProgram(program);

	
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void findDerivatives()
{
	int i,j;
	float udist = 2.0; // a single patch covers 0<=u<=1, the derivatives are computed using the previous patch and the next patch
					   // given a total distance of 2.0 in the parameter space
	float vdist = 2.0;
	for (i=0;i<M;i++)
		for (j=0;j<N;j++)
		{
			dv[i][j].x=dv[i][j].y=dv[i][j].z=0;
			du[i][j].x=du[i][j].y=du[i][j].z=0;
		}
	for (i=0;i<M;i++)
		for (j=1;j<N-1;j++) // partial derivatives are approximated using differences
		{
			dv[i][j].x=(controlPoints[i][j+1].x-controlPoints[i][j-1].x)/vdist;
			dv[i][j].y=(controlPoints[i][j+1].y-controlPoints[i][j-1].y)/vdist;
			dv[i][j].z=(controlPoints[i][j+1].z-controlPoints[i][j-1].z)/vdist;
		
		}

	for (j=0;j<N;j++)
		for (i=1;i<M-1;i++) // partial derivatives are approximated using differences
		{
			du[i][j].x=(controlPoints[i+1][j].x-controlPoints[i-1][j].x)/udist;
			du[i][j].y=(controlPoints[i+1][j].y-controlPoints[i-1][j].y)/udist;
			du[i][j].z=(controlPoints[i+1][j].z-controlPoints[i-1][j].z)/udist;
		}

}

void relocatePoint(int _x, int _y,int pos){
    vector<GLfloat> patchData;

    controlPoints[_x][_y].y = pos;
    findDerivatives();
    int index = 0;
    for (int i=0;i<M-1;i++) {
        for (int j=0;j<N-1;j++) {
            // control points
            patchData.push_back(controlPoints[i][j].x);
            patchData.push_back(controlPoints[i][j].y);
            patchData.push_back(controlPoints[i][j].z);
            index+=3;
            patchData.push_back(controlPoints[i+1][j].x);
            patchData.push_back(controlPoints[i+1][j].y);
            patchData.push_back(controlPoints[i+1][j].z);
            index+=3;
            patchData.push_back(controlPoints[i+1][j+1].x);
            patchData.push_back(controlPoints[i+1][j+1].y);
            patchData.push_back(controlPoints[i+1][j+1].z);
            index+=3;
            patchData.push_back(controlPoints[i][j+1].x);
            patchData.push_back(controlPoints[i][j+1].y);
            patchData.push_back(controlPoints[i][j+1].z);
            index+=3;

            // now partial derivatives du
            patchData.push_back(du[i][j].x);
            patchData.push_back(du[i][j].y);
            patchData.push_back(du[i][j].z);
            index+=3;
            patchData.push_back(du[i+1][j].x);
            patchData.push_back(du[i+1][j].y);
            patchData.push_back(du[i+1][j].z);
            index+=3;
            patchData.push_back(du[i+1][j+1].x);
            patchData.push_back(du[i+1][j+1].y);
            patchData.push_back(du[i+1][j+1].z);
            index+=3;
            patchData.push_back(du[i][j+1].x);
            patchData.push_back(du[i][j+1].y);
            patchData.push_back(du[i][j+1].z);
            index+=3;

            // now partial derivatives dv
            patchData.push_back(dv[i][j].x);
            patchData.push_back(dv[i][j].y);
            patchData.push_back(dv[i][j].z);
            index+=3;
            patchData.push_back(dv[i+1][j].x);
            patchData.push_back(dv[i+1][j].y);
            patchData.push_back(dv[i+1][j].z);
            index+=3;
            patchData.push_back(dv[i+1][j+1].x);
            patchData.push_back(dv[i+1][j+1].y);
            patchData.push_back(dv[i+1][j+1].z);
            index+=3;
            patchData.push_back(dv[i][j+1].x);
            patchData.push_back(dv[i][j+1].y);
            patchData.push_back(dv[i][j+1].z);
            index+=3;

        }
    }


    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, patchData.size() * sizeof(float), patchData.data(), GL_STATIC_DRAW);

}

void wavIt(float t){
    vector<GLfloat> patchData;

    for (int i=0;i<M-1;i++) {
        for (int j=0;j<N-1;j++) {
        controlPoints[i][j].y = 4*sin(i + t) + 2*sin(j+t)*cos(j+t);
        }
    }
    findDerivatives();
    int index = 0;
    for (int i=0;i<M-1;i++) {
        for (int j=0;j<N-1;j++) {
            // control points
            patchData.push_back(controlPoints[i][j].x);
            patchData.push_back(controlPoints[i][j].y);
            patchData.push_back(controlPoints[i][j].z);
            index+=3;
            patchData.push_back(controlPoints[i+1][j].x);
            patchData.push_back(controlPoints[i+1][j].y);
            patchData.push_back(controlPoints[i+1][j].z);
            index+=3;
            patchData.push_back(controlPoints[i+1][j+1].x);
            patchData.push_back(controlPoints[i+1][j+1].y);
            patchData.push_back(controlPoints[i+1][j+1].z);
            index+=3;
            patchData.push_back(controlPoints[i][j+1].x);
            patchData.push_back(controlPoints[i][j+1].y);
            patchData.push_back(controlPoints[i][j+1].z);
            index+=3;

            // now partial derivatives du
            patchData.push_back(du[i][j].x);
            patchData.push_back(du[i][j].y);
            patchData.push_back(du[i][j].z);
            index+=3;
            patchData.push_back(du[i+1][j].x);
            patchData.push_back(du[i+1][j].y);
            patchData.push_back(du[i+1][j].z);
            index+=3;
            patchData.push_back(du[i+1][j+1].x);
            patchData.push_back(du[i+1][j+1].y);
            patchData.push_back(du[i+1][j+1].z);
            index+=3;
            patchData.push_back(du[i][j+1].x);
            patchData.push_back(du[i][j+1].y);
            patchData.push_back(du[i][j+1].z);
            index+=3;

            // now partial derivatives dv
            patchData.push_back(dv[i][j].x);
            patchData.push_back(dv[i][j].y);
            patchData.push_back(dv[i][j].z);
            index+=3;
            patchData.push_back(dv[i+1][j].x);
            patchData.push_back(dv[i+1][j].y);
            patchData.push_back(dv[i+1][j].z);
            index+=3;
            patchData.push_back(dv[i+1][j+1].x);
            patchData.push_back(dv[i+1][j+1].y);
            patchData.push_back(dv[i+1][j+1].z);
            index+=3;
            patchData.push_back(dv[i][j+1].x);
            patchData.push_back(dv[i][j+1].y);
            patchData.push_back(dv[i][j+1].z);
            index+=3;

        }
    }


    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, patchData.size() * sizeof(float), patchData.data(), GL_STATIC_DRAW);

}



static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
        tessLevel++;
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
        tessLevel--;

    // if (key == GLFW_KEY_W && action == GLFW_PRESS)
    //     relocatePoint(0,0,1);
    // if (key == GLFW_KEY_S && action == GLFW_PRESS)
    //     relocatePoint(0,0,-1);
}



void showFPS(GLFWwindow* window) {
        double currentTime = glfwGetTime();
        double delta = currentTime - lastTime;
	    char ss[500] = {};
		std::string wTitle = "Lab07";
        nFrames++;
        if ( delta >= 1.0 ){ // If last update was more than 1 sec ago
            double fps = ((double)(nFrames)) / delta;
            sprintf(ss,"%s running at %lf FPS. TessLevel = %d.",wTitle.c_str(),fps,tessLevel);
            glfwSetWindowTitle(window, ss);
            nFrames = 0;
            lastTime = currentTime;
        }
}



void readSurface(const char* filename)
{
	FILE *fp;
	int i,j;

	fp = fopen(filename,"r");

	fscanf(fp,"%d %d",&M,&N);

	controlPoints = (vec3 **)malloc(sizeof(vec3*)*(M));
	du = (vec3 **)malloc(sizeof(vec3*)*(M));
	dv = (vec3 **)malloc(sizeof(vec3*)*(M));
	for (i=0;i<M;i++)
	{
		controlPoints[i] = (vec3 *)malloc(sizeof(vec3)*(N));
		du[i] = (vec3 *)malloc(sizeof(vec3)*(N));
		dv[i] = (vec3 *)malloc(sizeof(vec3)*(N));
	}

	for (i=0;i<M;i++)
		for (j=0;j<N;j++)
		{
			fscanf(fp,"%f %f %f",&(controlPoints[i][j].x),&(controlPoints[i][j].y),&(controlPoints[i][j].z));
		}

    printf("%dx%d control points read.\n",M,N);

	findDerivatives();

    printf("Derivatives computed.\n");

}

void createSurface(float step)
{

	controlPoints = (vec3 **)malloc(sizeof(vec3*)*(M));
	du = (vec3 **)malloc(sizeof(vec3*)*(M));
	dv = (vec3 **)malloc(sizeof(vec3*)*(M));
	for (int i=0;i<M;i++)
	{
		controlPoints[i] = (vec3 *)malloc(sizeof(vec3)*(N));
		du[i] = (vec3 *)malloc(sizeof(vec3)*(N));
		dv[i] = (vec3 *)malloc(sizeof(vec3)*(N));
	}
    for(int i = 0; i<M; i++){
        for(int j = 0; j<N; j++){
            controlPoints[i][j] = vec3(step*i, 0, step*j);
        }
    }

    printf("%dx%d control points created.\n",M,N);

	findDerivatives();

    printf("Derivatives computed.\n");
}

int main(void)
{

    GLFWwindow* window;

    int i,j,c;

    GLint mvp_location,time_location;

	lastTime = 0;
	nFrames = 0;

    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window = glfwCreateWindow(800, 600, "Lab07", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);

	gladLoadGL();

	const GLubyte *renderer = glGetString( GL_RENDERER );
	const GLubyte *vendor = glGetString( GL_VENDOR );
	const GLubyte *version = glGetString( GL_VERSION );
	const GLubyte *glslVersion = glGetString( GL_SHADING_LANGUAGE_VERSION );

	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);

	printf("GL Vendor            : %s\n", vendor);
	printf("GL Renderer          : %s\n", renderer);
	printf("GL Version (string)  : %s\n", version);
	printf("GL Version (integer) : %d.%d\n", major, minor);
	printf("GLSL Version         : %s\n", glslVersion);
	
    initShaders();

	Cube cube = Cube(1.0);

    glEnable(GL_DEPTH_TEST);

    // setup buffers and send control points

    // In an F-surface patch we have 4 control points and 8 partial derivatives (du and dv) in these control points
    // making a total of 12 vec3 info per patch

    vector<GLfloat> patchData;

    createSurface(10);

    int index = 0;
    for (i=0;i<M-1;i++) {
        for (j=0;j<N-1;j++) {
            // control points
            patchData.push_back(controlPoints[i][j].x);
            patchData.push_back(controlPoints[i][j].y);
            patchData.push_back(controlPoints[i][j].z);
            index+=3;
            patchData.push_back(controlPoints[i+1][j].x);
            patchData.push_back(controlPoints[i+1][j].y);
            patchData.push_back(controlPoints[i+1][j].z);
            index+=3;
            patchData.push_back(controlPoints[i+1][j+1].x);
            patchData.push_back(controlPoints[i+1][j+1].y);
            patchData.push_back(controlPoints[i+1][j+1].z);
            index+=3;
            patchData.push_back(controlPoints[i][j+1].x);
            patchData.push_back(controlPoints[i][j+1].y);
            patchData.push_back(controlPoints[i][j+1].z);
            index+=3;

            // now partial derivatives du
            patchData.push_back(du[i][j].x);
            patchData.push_back(du[i][j].y);
            patchData.push_back(du[i][j].z);
            index+=3;
            patchData.push_back(du[i+1][j].x);
            patchData.push_back(du[i+1][j].y);
            patchData.push_back(du[i+1][j].z);
            index+=3;
            patchData.push_back(du[i+1][j+1].x);
            patchData.push_back(du[i+1][j+1].y);
            patchData.push_back(du[i+1][j+1].z);
            index+=3;
            patchData.push_back(du[i][j+1].x);
            patchData.push_back(du[i][j+1].y);
            patchData.push_back(du[i][j+1].z);
            index+=3;

            // now partial derivatives dv
            patchData.push_back(dv[i][j].x);
            patchData.push_back(dv[i][j].y);
            patchData.push_back(dv[i][j].z);
            index+=3;
            patchData.push_back(dv[i+1][j].x);
            patchData.push_back(dv[i+1][j].y);
            patchData.push_back(dv[i+1][j].z);
            index+=3;
            patchData.push_back(dv[i+1][j+1].x);
            patchData.push_back(dv[i+1][j+1].y);
            patchData.push_back(dv[i+1][j+1].z);
            index+=3;
            patchData.push_back(dv[i][j+1].x);
            patchData.push_back(dv[i][j+1].y);
            patchData.push_back(dv[i][j+1].z);
            index+=3;

        }
    }

    printf("size = %d\n",patchData.size());

    
    glGenBuffers(1, &buf);

    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, patchData.size() * sizeof(float), patchData.data(), GL_STATIC_DRAW);

    glGenVertexArrays( 1, &vao );
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, 0 );
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    glPatchParameteri(GL_PATCH_VERTICES, 12); // 4 control points with 2 partial derivatives (du and dv) at each control point

	float angle = 0;
    tessLevel = 8;
	
    vec3 cameraPos;
    mat4 mvp,view,projection,model;

    mvp_location = glGetUniformLocation(program,"MVP");

    glUniform1i(glGetUniformLocation(program,"TessLevel"), tessLevel);
    glUniform1f(glGetUniformLocation(program,"LineWidth"), 0.8f);
    glUniform4f(glGetUniformLocation(program,"LineColor"), 0.05f,0.0f,0.05f,1.0f);
    glUniform4f(glGetUniformLocation(program,"LightPosition"), 0.0f,0.0f,0.0f,1.0f);
    glUniform3f(glGetUniformLocation(program,"LightIntensity"), 1.0f,1.0f,1.0f);
    glUniform3f(glGetUniformLocation(program,"Kd"), 0.9f,0.9f,1.0f);

	glClearColor(0.5,0.5,0.5,1.0);

	float tPrev, rotSpeed, t, deltaT;

    angle = glm::pi<float>() / 3.0f;

    tPrev = 0;
    rotSpeed = glm::pi<float>()/8.0f;
	
    while (!glfwWindowShouldClose(window))
    {
        float ratio;
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        float w2 = width / 2.0f;
        float h2 = height / 2.0f;
        mat4 viewport = mat4( vec4(w2,0.0f,0.0f,0.0f),
                     vec4(0.0f,h2,0.0f,0.0f),
                     vec4(0.0f,0.0f,1.0f,0.0f),
                     vec4(w2+0, h2+0, 0.0f, 1.0f));

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		t = float(glfwGetTime());
		deltaT = t - tPrev;
    	if(tPrev == 0.0f) deltaT = 0.0f;
    	tPrev = t;

    	// angle += rotSpeed * deltaT;
    	// if (angle > glm::two_pi<float>()) angle -= glm::two_pi<float>();

        cameraPos = vec3(-50,90,-50);
        // cameraPos = vec3(50*2.0f * cos(angle), 60+1.5f, 50*2.0f * sin(angle));

        view = glm::lookAt(cameraPos, vec3(0.0f,0.0f,0.0f), vec3(0.0f,1.0f,0.0f));

        model = mat4(1.0f);
        //model = glm::translate(model, vec3(0.0f,-1.5f,0.0f));
        //model = glm::rotate(model,glm::radians(-90.0f), vec3(1.0f,0.0f,0.0f));

    	projection = glm::perspective(glm::radians(60.0f), (float)width/height, 1.0f, 1000.0f);

    	mvp = projection * view * model;

        mat4 mv = view * model;
        mat3 nm = mat3( vec3(mv[0]), vec3(mv[1]), vec3(mv[2]) );


        wavIt(t);
        glUseProgram(program);

    	glUniformMatrix4fv(glGetUniformLocation(program,"ModelViewMatrix"), 1, GL_FALSE, &(mv)[0][0]);
    	glUniformMatrix3fv(glGetUniformLocation(program,"NormalMatrix"), 1, GL_FALSE, &(nm)[0][0]);
    	glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &(mvp)[0][0]);
    	glUniformMatrix4fv(glGetUniformLocation(program,"ViewportMatrix"), 1, GL_FALSE, &(viewport)[0][0]);

        glUniform1i(glGetUniformLocation(program,"TessLevel"), tessLevel);

        glPatchParameteri(GL_PATCH_VERTICES, 12);

        glBindVertexArray(vao);
        glDrawArrays(GL_PATCHES, 0, ((M-1)*(N-1)*12));
        glBindVertexArray(0);



        glUseProgram(pointProgram);
        glUniformMatrix4fv(glGetUniformLocation(pointProgram,"projection"), 1, GL_FALSE, &(projection[0][0]));
        glUniformMatrix4fv(glGetUniformLocation(pointProgram,"view"), 1, GL_FALSE, &(view[0][0]));
        for (i=0;i<M;i++) {
            for (j=0;j<N;j++) {
                model = glm::translate(glm::mat4(1.0), controlPoints[i][j]);
                glUniformMatrix4fv(glGetUniformLocation(pointProgram,"model"), 1, GL_FALSE, &(model[0][0]));
                cube.render();
            }
        }


		showFPS(window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
	
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
