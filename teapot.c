#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#define PI 3.14159265

//global variables containing polygon information for bunny
int face_count = 0;
GLfloat *final_vertex, *final_tex, *final_normal;


void load_materials(char *mat_file) {
	//open file
	FILE* fp = fopen(mat_file,"r"); 
	if (!fp) {
		fprintf(stderr, "Failed to open %s\n", mat_file);
		exit(1);
	}

	char prefix[20];
	char look[20];
	int x;
	x = fscanf(fp, "%s %s", prefix, look);

	float ambient[] = {0.0, 0.0, 0.0, 1.0};
	float diffuse[] = {0.0, 0.0, 0.0, 1.0};
	float specular[] = {0.0, 0.0, 0.0, 1.0};
	float shininess[] = {0.0};

	x = fscanf(fp, "%s %f %f %f", prefix, &ambient[0], &ambient[1], &ambient[2]);
	x = fscanf(fp, "%s %f %f %f", prefix, &diffuse[0], &diffuse[1], &diffuse[2]);
	x = fscanf(fp, "%s %f %f %f", prefix, &specular[0], &specular[1], &specular[2]);
	x = fscanf(fp, "%s %f", prefix, &shininess[0]);

	printf("%f %f %f %f\n", ambient[0], ambient[1], ambient[2], ambient[3]);
	printf("%f %f %f %f\n", diffuse[0], diffuse[1], diffuse[2], diffuse[3]);
	printf("%f %f %f %f\n", specular[0], specular[1], specular[2], specular[3]);
	printf("%f\n", shininess[0]);

	glMaterialfv(GL_FRONT,GL_AMBIENT,ambient);
	glMaterialfv(GL_FRONT,GL_DIFFUSE,diffuse);
	glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
	glMaterialfv(GL_FRONT,GL_SHININESS,shininess);

	fclose(fp);
}

void read_object_file(char* fileName) {

	//open file
	FILE* fp = fopen(fileName,"r"); 
	if (!fp) {
		fprintf(stderr, "Failed to open %s\n", fileName);
		exit(1);
	}

	//skip unnessesary
	char prefix[100];
	char material_file[100];
	int i, x;
	x = fscanf(fp, "%s %s", prefix, material_file);
	load_materials(material_file);
	
	//read in vertexes
	int vertex_count = 0;
	unsigned int allocated = 3 * sizeof(GLfloat);
	GLfloat *vertex = (GLfloat *) malloc(sizeof(GLfloat) * 3 * vertex_count);
	x = fscanf(fp, "%s", prefix);
	while (strcmp(prefix, "v") == 0) {
		if ((vertex_count+1) * 3 * sizeof(GLfloat) > allocated) {
			vertex = (GLfloat *) realloc(vertex, allocated * 2);
			allocated *= 2;
		}
		x = fscanf(fp, "%f %f %f", &vertex[vertex_count*3], 
			&vertex[vertex_count*3+1], &vertex[vertex_count*3+2]);
		x = fscanf(fp, "%s", prefix);
		vertex_count++;
	}

	//read in texture coords
	int tex_count = 0;
	allocated = 2 * sizeof(GLfloat);
	GLfloat *tex_coords = (GLfloat *) malloc(sizeof(GLfloat) * 2 * tex_count);
	while (strcmp(prefix, "vt") == 0) {
		if ((tex_count+1) * 2 * sizeof(GLfloat) > allocated) {
			tex_coords = (GLfloat *) realloc(tex_coords, allocated * 2);
			allocated *= 2;
		}
		x = fscanf(fp, "%f %f", &tex_coords[tex_count*2], 
			&tex_coords[tex_count*2+1]);
		x = fscanf(fp, "%s", prefix);
		tex_count++;
	}

	//read in vertex normals
	int normal_count = 0;
	allocated = 3 * sizeof(GLfloat);
	GLfloat *normal = (GLfloat *) malloc(sizeof(GLfloat) * 3 * normal_count);
	while (strcmp(prefix, "vn") == 0) {
		if ((normal_count+1) * 3 * sizeof(GLfloat) > allocated) {
			normal = (GLfloat *) realloc(normal, allocated * 2);
			allocated *= 2;
		}
		x = fscanf(fp, "%f %f %f", &normal[normal_count*3], 
			&normal[normal_count*3+1], &normal[normal_count*3+2]);
		x = fscanf(fp, "%s", prefix);
		normal_count++;
	}

	//read in tangents and bitangents
	int tang_count = 0;
	allocated = 3 * sizeof(GLfloat);
	GLfloat *tangent = (GLfloat *) malloc(sizeof(GLfloat) * 3 * tang_count);
	GLfloat *bitangent = (GLfloat *) malloc(sizeof(GLfloat) * 3 * tang_count);
	while (strcmp(prefix, "vx") == 0) {
		if ((tang_count+1) * 3 * sizeof(GLfloat) > allocated) {
			tangent = (GLfloat *) realloc(tangent, allocated * 2);
			bitangent = (GLfloat *) realloc(bitangent, allocated * 2);
			allocated *= 2;
		}
		x = fscanf(fp, "%f %f %f", &tangent[tang_count*3], 
			&tangent[tang_count*3+1], &tangent[tang_count*3+2]);
		x = fscanf(fp, "%s", prefix);
		x = fscanf(fp, "%f %f %f", &bitangent[tang_count*3], 
			&bitangent[tang_count*3+1], &bitangent[tang_count*3+2]);
		x = fscanf(fp, "%s", prefix);
		tang_count++;
	}

	//get material from material library
	char current_mtl[20];
	if (strcmp("usemtl", prefix) == 0) {
		x = fscanf(fp, "%s", current_mtl);
		x = fscanf(fp, "%s", prefix);
	}

	//read in face data
	face_count = 0;
	unsigned allocated_3 = 12 * sizeof(GLfloat);
	unsigned allocated_2 = 8 * sizeof(GLfloat);
	final_vertex = (GLfloat *) malloc(allocated_3);
	final_tex = (GLfloat *) malloc(allocated_2);
	final_normal = (GLfloat *) malloc(allocated_3);
	while (strcmp("f", prefix) == 0) {
		if ((face_count+1) * 12 * sizeof(GLfloat) > allocated_3) {
			final_vertex = (GLfloat *) realloc(final_vertex, allocated_3 * 2);
			final_tex = (GLfloat *) realloc(final_tex, allocated_2 * 2);
			final_normal = (GLfloat *) realloc(final_normal, allocated_3 * 2);
			allocated_3 *= 2;
			allocated_2 *= 2;
		}
		int i, vert, tex, norm;
		for (i = 0; i < 4; i++) {
			x = fscanf(fp, "%d/%d/%d", &vert, &tex, &norm);
			final_vertex[face_count*12+i*3] = vertex[(vert-1)*3];
			final_vertex[face_count*12+i*3+1] = vertex[(vert-1)*3+1];
			final_vertex[face_count*12+i*3+2] = vertex[(vert-1)*3+2];
			final_tex[face_count*8+i*2] = tex_coords[(tex-1)*2];
			final_tex[face_count*8+i*2+1] = tex_coords[(tex-1)*2+1];
			final_normal[face_count*12+i*3] = normal[(norm-1)*3];
			final_normal[face_count*12+i*3+1] = normal[(norm-1)*3+1];
			final_normal[face_count*12+i*3+2] = normal[(norm-1)*3+2];
		}
		x = fscanf(fp, "%s", prefix);
		face_count++;
	}


	x = fscanf(fp, "%s", prefix);
	//read in lid
	//vertices
	allocated = (vertex_count * 3) * sizeof(GLfloat);
	while (strcmp(prefix, "v") == 0) {
		if ((vertex_count+1) * 3 * sizeof(GLfloat) > allocated) {
			vertex = (GLfloat *) realloc(vertex, allocated * 2);
			allocated *= 2;
		}
		x = fscanf(fp, "%f %f %f", &vertex[vertex_count*3], 
			&vertex[vertex_count*3+1], &vertex[vertex_count*3+2]);
		x = fscanf(fp, "%s", prefix);
		vertex_count++;
	}

	//read in texture coords
	allocated = (tex_count * 2) * sizeof(GLfloat);
	while (strcmp(prefix, "vt") == 0) {
		if ((tex_count+1) * 2 * sizeof(GLfloat) > allocated) {
			tex_coords = (GLfloat *) realloc(tex_coords, allocated * 2);
			allocated *= 2;
		}
		x = fscanf(fp, "%f %f", &tex_coords[tex_count*2], 
			&tex_coords[tex_count*2+1]);
		x = fscanf(fp, "%s", prefix);
		tex_count++;
	}

	//read in vertex normals
	allocated = (normal_count * 3) * sizeof(GLfloat);
	while (strcmp(prefix, "vn") == 0) {
		if ((normal_count+1) * 3 * sizeof(GLfloat) > allocated) {
			normal = (GLfloat *) realloc(normal, allocated * 2);
			allocated *= 2;
		}
		x = fscanf(fp, "%f %f %f", &normal[normal_count*3], 
			&normal[normal_count*3+1], &normal[normal_count*3+2]);
		x = fscanf(fp, "%s", prefix);
		normal_count++;
	}

	//read in face data
	allocated_3 = (face_count * 12) * sizeof(GLfloat);
	allocated_2 = (face_count * 8) * sizeof(GLfloat);
	while (strcmp("f", prefix) == 0) {
		if ((face_count+1) * 12 * sizeof(GLfloat) > allocated_3) {
			final_vertex = (GLfloat *) realloc(final_vertex, allocated_3 * 2);
			final_tex = (GLfloat *) realloc(final_tex, allocated_2 * 2);
			final_normal = (GLfloat *) realloc(final_normal, allocated_3 * 2);
			allocated_3 *= 2;
			allocated_2 *= 2;
		}
		int i, vert, tex, norm;
		for (i = 0; i < 4; i++) {
			x = fscanf(fp, "%d/%d/%d", &vert, &tex, &norm);
			final_vertex[face_count*12+i*3] = vertex[(vert-1)*3];
			final_vertex[face_count*12+i*3+1] = vertex[(vert-1)*3+1];
			final_vertex[face_count*12+i*3+2] = vertex[(vert-1)*3+2];
			final_tex[face_count*8+i*2] = tex_coords[(tex-1)*2];
			final_tex[face_count*8+i*2+1] = tex_coords[(tex-1)*2+1];
			final_normal[face_count*12+i*3] = normal[(norm-1)*3];
			final_normal[face_count*12+i*3+1] = normal[(norm-1)*3+1];
			final_normal[face_count*12+i*3+2] = normal[(norm-1)*3+2];
		}
		x = fscanf(fp, "%s", prefix);
		face_count++;
	}

	//cleanup
	free(vertex);
	free(tex_coords);
	free(normal);
	free(tangent);
	free(bitangent);

	fclose(fp);
}
char *read_shader_program(char *filename) 
{
	FILE *fp;
	char *content = NULL;
	int fd, count;
	fd = open(filename,O_RDONLY);
	count = lseek(fd,0,SEEK_END);
	close(fd);
	content = (char *)calloc(1,(count+1));
	fp = fopen(filename,"r");
	count = fread(content,sizeof(char),count,fp);
	content[count] = '\0';
	fclose(fp);
	return content;
}

struct point {
	float x, y, z;
};

void setup_viewvolume()
{
	struct point eye, view, up;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0,1.0,0.1,20.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	eye.x = -4; eye.y = 3; eye.z = 1;
	view.x = 0; view.y = 0.3; view.z = 0.0;
	up.x = 0.0; up.y = 1.0; up.z = 0.0;
	gluLookAt(eye.x,eye.y,eye.z,view.x,view.y,view.z,up.x,up.y,up.z);
}

void normalize(struct point *p) {
	float mag = sqrt(p->x*p->x + p->y*p->y + p->z*p->z);
	p->x /= mag;
	p->y /= mag;
	p->z /= mag;
}

void init_lights()
{
	//set up hollywood lighting based on eye position
	struct point eye, light0, light1, light2;
	eye.x = 1; eye.y = 4; eye.z = 3;
	light0.x = eye.x * cos(PI/12) + eye.z * sin(PI/12);
	light0.y = eye.y;
	light0.z = eye.z * cos(PI/12) - eye.x * sin(PI/12);

	light1.x = eye.x * cos(-PI/3) + eye.z * sin(-PI/3);
	light1.y = eye.y;
	light1.z = eye.z * cos(-PI/3) - eye.x * sin(-PI/3);

	light2.x = -eye.x;
	light2.y = eye.y + .24;
	light2.z = -eye.z;

	//key light
	float light0_diffuse[] = { 0.60, 0.60, 0.60, 0.0 }; 
	float light0_specular[] = { 0.75, 0.75, 0.75, 0.0 }; 
	float light0_position[] = { light0.x, light0.y, light0.z, 1.0 };

	//fill light
	float light1_diffuse[] = { 0.55, 0.55, 0.4, 0.0 }; 
	float light1_specular[] = { 0.7, 0.7, 0.7, 0.0 }; 
	float light1_position[] = { light1.x, light1.y, light1.z, 1.0 };

	//back light
	float light2_diffuse[] = { 0.2, 0.2, 0.9, 0.0 }; 
	float light2_specular[] = { 1.25, 1.25, 1.25, 0.0 }; 
	float light2_position[] = { light2.x, light2.y, light2.z, 1.0 };
	
	//enable key light
	glLightfv(GL_LIGHT0,GL_POSITION,light0_position); 
	glLightfv(GL_LIGHT0,GL_DIFFUSE,light0_diffuse); 
	glLightfv(GL_LIGHT0,GL_SPECULAR,light0_specular); 

	//enable fill light
	glLightfv(GL_LIGHT1,GL_POSITION,light1_position); 
	glLightfv(GL_LIGHT1,GL_DIFFUSE,light1_diffuse); 
	glLightfv(GL_LIGHT1,GL_SPECULAR,light1_specular); 

	//enable back light
	glLightfv(GL_LIGHT2,GL_POSITION,light2_position); 
	glLightfv(GL_LIGHT2,GL_DIFFUSE,light2_diffuse); 
	glLightfv(GL_LIGHT2,GL_SPECULAR,light2_specular);
}

void init_material(int mat)
{
	if (mat == 0) {
		float mat_ambient[] = {0.0,0.0,0.0,0.0}; 
		float mat_diffuse[] = {0.8,0.8,0.7,1.0}; 
		float mat_specular[] = {0.15,0.15,0.15,1.0};
		float mat_shininess[] = {60.0}; 

		glMaterialfv(GL_FRONT,GL_AMBIENT,mat_ambient);
		glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_diffuse);
		glMaterialfv(GL_FRONT,GL_SPECULAR,mat_specular);
		glMaterialfv(GL_FRONT,GL_SHININESS,mat_shininess);
	}
	if (mat == 1) {
		float mat_ambient[] = {0.0,0.0,0.0,0.0}; 
		float mat_diffuse[] = {1.0,0.8,0.7,1.0}; 
		float mat_specular[] = {0.15,0.15,0.15,1.0};
		float mat_shininess[] = {60.0}; 
		
		glMaterialfv(GL_FRONT,GL_AMBIENT,mat_ambient);
		glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_diffuse);
		glMaterialfv(GL_FRONT,GL_SPECULAR,mat_specular);
		glMaterialfv(GL_FRONT,GL_SHININESS,mat_shininess);

	}
	if (mat == 2) {
		float mat_ambient[] = {0.0,0.0,0.0,0.0}; 
		float mat_diffuse[] = {1.0,0.8,0.7,1.0}; 
		float mat_specular[] = {0.15,0.15,0.15,1.0};
		float mat_shininess[] = {60.0}; 
		
		glMaterialfv(GL_FRONT,GL_AMBIENT,mat_ambient);
		glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_diffuse);
		glMaterialfv(GL_FRONT,GL_SPECULAR,mat_specular);
		glMaterialfv(GL_FRONT,GL_SHININESS,mat_shininess);

	}
	if (mat == 3) {
		float mat_ambient[] = {0.0,0.0,0.0,0.0}; 
		float mat_diffuse[] = {1.0,0.8,0.7,1.0}; 
		float mat_specular[] = {0.15,0.15,0.15,1.0};
		float mat_shininess[] = {60.0}; 
	}

}

unsigned int init_shaders() {
	GLint vertCompiled, fragCompiled;
	char *vs, *fs;
	GLuint v, f, p;
	int result = -1;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);
	vs = read_shader_program("teapot.vert");
	fs = read_shader_program("teapot.frag");
	glShaderSource(v,1,(const char **)&vs,NULL);
	glShaderSource(f,1,(const char **)&fs,NULL);
	free(vs);
	free(fs); 
	glCompileShader(v);
	glGetShaderiv(v,GL_COMPILE_STATUS,&result);
	if (result == GL_FALSE) {
		fprintf(stderr, "Vertex Shader: \n");
		GLint maxLength = 0;
		glGetShaderiv(v, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		GLchar* errorLog = (GLchar *) malloc(sizeof(GLchar) * maxLength);
		glGetShaderInfoLog(v, maxLength, &maxLength, &errorLog[0]);
		fprintf(stderr,"%s\n", errorLog);
		free(errorLog);
	}

	glCompileShader(f);
	glGetShaderiv(f,GL_COMPILE_STATUS,&result);
	if (result == GL_FALSE) {
		fprintf(stderr, "Fragment Shader: \n");
		GLint maxLength = 0;
		glGetShaderiv(f, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		GLchar* errorLog = (GLchar *) malloc(sizeof(GLchar) * maxLength);
		glGetShaderInfoLog(f, maxLength, &maxLength, &errorLog[0]);
		fprintf(stderr,"%s\n", errorLog);
		free(errorLog);
	}

	p = glCreateProgram();
	glAttachShader(p,f);
	glAttachShader(p,v);
	glLinkProgram(p);
	glUseProgram(p);
	return p;
}

GLuint vboId = 1;

void init_objects() {
	glGenBuffersARB(1, &vboId);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vboId);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(GL_FLOAT) * face_count * 24, NULL+0, GL_STATIC_DRAW_ARB);
	glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(GL_FLOAT) * face_count * 12, final_vertex);
	glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, sizeof(GL_FLOAT) * face_count * 12, sizeof(GL_FLOAT) * face_count * 12, final_normal);

	// When using VBOs, the final arg is a byte offset in buffer, not the address,
	// but gl<whatever>Pointer still expects an address type, hence the NULL.
	glVertexPointer(3,GL_FLOAT,3*sizeof(GLfloat),NULL+0);
	glNormalPointer(GL_FLOAT,3*sizeof(GLfloat),NULL+face_count*12*sizeof(GLfloat));

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	free(final_vertex);
	free(final_normal);
}

void render_scene()
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);	
	//coords for ground

	init_material(0);
	struct point bottom[4]={{-4,0.0,-4},{-4,0.0,4},{4,0.0,4},{4,0.0,-4}};
	glBegin(GL_QUADS);
	glNormal3f(0.0,1.0,0.0);
	int i;
	for(i=0;i<4;i++) glVertex3f(bottom[i].x,bottom[i].y,bottom[i].z);
	glEnd();

	init_material(4);
	glDrawArrays(GL_QUADS,0,face_count*4);
	glutSwapBuffers();
}

void end_program(unsigned char key, int x, int y)
{
	switch(key) {
        case 'q':
            exit(1);
        default:
            break;
	}
}

int main(int argc, char **argv)
{
	//read obj file into global arrays
	read_object_file("teapot.obj");

	glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH|GLUT_MULTISAMPLE);
	glutInitWindowSize(768,768);
	glutInitWindowPosition(100,100);
	glutCreateWindow("Project 3");
	glClearColor(0.25,0.25,0.25,0.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE_ARB);

	setup_viewvolume();
	init_lights();
	init_shaders();
	init_objects();

	glutDisplayFunc(render_scene);
	glutKeyboardFunc(end_program);
	glutMainLoop();

	free(final_tex);
	return 0;
}
