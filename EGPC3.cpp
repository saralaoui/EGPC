/*****************************************************************************
 File: TD5.cpp
 
 Sarah FDILI ALAOUI, UniversitŽ Paris 11
 
 Copyright (C) 2010 University Paris 11 
 This file is provided without support, instruction, or implied
 warranty of any kind.  University Paris 11 makes no guarantee of its
 fitness for a particular purpose and is not liable under any
 circumstances for any damages or loss whatsoever arising from the use
 or inability to use this file or items derived from it.
 ******************************************************************************/
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>           
#include <GL/glu.h>         
#include <GL/glut.h>    
#include <GL/glx.h>
#include <GL/glext.h>
#include "all-include.h"

#include <stdio.h>      
#include <stdlib.h>     
#include <math.h>
#include <string.h>
#include <iostream>

#include <Cg/cgGL.h>

#include "cg-lib.h"
#include <png.h>
#include "texpng.h"

#define WIDTH  512
#define HEIGHT 512

#define RED   1
#define GREEN 1
#define BLUE  1
#define ALPHA 1

#define KEY_ESC 27
#define PI 3.1415926535898

// display lists
#define My_QuadRECT2D 1
#define My_Particles 2
#define My_Points 3

//Pour la liaison OSC, dŽfinitio du socket et du buffer
UdpTransmitSocket transmitSocket( IpEndpointName( ADDRESS, OUT_PORT ) );
char buffer[OUTPUT_BUFFER_SIZE];

// gestion des changements de position en fonction de la souris
int angle_x = 0, angle_y = 0;
int mouse_pos_x = 0, mouse_pos_y = 0;

// gestion du frustum
float top = 2.0;
float bottom = -2.0;
float ratio = 1.0;
float near = 0.04;
int width = WIDTH;
int height = HEIGHT;

// frame no
int FrameNo = 0;
int latence = 10;

// textures
#define    MAX_TEXTURES 7
#define    POSITION_1_TEXTURE 0
#define    POSITION_2_TEXTURE 1
#define    POSITION_3_TEXTURE 2
#define    EDGE_TEXTURE 3
#define    FORCE_TEXTURE 4
#define    FORCE_MASS_TEXTURE 5
#define    DECAL_TEXTURE 6

#define IN_PORT 7000


unsigned int g_Texture[MAX_TEXTURES];
int        previousTexturePosition = POSITION_2_TEXTURE;
int        currentTexturePosition = POSITION_3_TEXTURE;
int        newTexturePosition = POSITION_1_TEXTURE;

// default material: white
GLfloat white_transparent[4] = {1.0,1.0,1.0,0.0};

//Param¶tres ˆ gerer par le biais d'un controle gestuel

//raideur supplÚmentaire
float Kplus = 0;
//longueure initiale supplÚmentaire
float Linit = 0;
//amortissement total supplÚmentaire
float Amort = 0;
//constante de viscositÚ
float mu = 0;
//masse uniforme supplÚmentaire
float MassePlus = 1;

//nombre de masses et de ressorts choisis
int NB_MASSES=8;
int NB_EDGES=8;
int NB_MASSESREEL=4;

// les parametres de control 
float indice =8;
float raideur[8];
float elongation[8];
float masses[4];





///////////////////////////////////////
// FBO frame buffer objects

GLuint g_frameBuffer;
GLuint g_depthRenderBuffer = 0;


///////////////////////////////////////
// verification d'erreur

void checkGLErrors(const char *label);
bool checkFramebufferStatus();
void cgErrorCallback(void);

///////////////////////////////////////
// donnŽes Cg

// contexte GC
CGcontext cgContext;

// profil GC
CGprofile cgVertexProfile;
CGprofile cgFragmentProfile;

// programmes GC
// pass 1: forces
CGprogram cgQuadVertexProgram;            // TD5-quad-VP.cg
CGprogram cgForceFragmentProgram;         // TD5-force-FS.cg
// pass 2-3 : cumul des forces
CGprogram cgVertexLeftForcesProgram;         // TD5-forces-gauche-VP.cg
CGprogram cgVertexRightForcesProgram;        // TD5-forces-droite-VP.cg
CGprogram cgFragmentLeftCumulForcesProgram;  // TD5-cumul-force-gauche-FS.cg
CGprogram cgFragmentRightCumulForcesProgram; // TD5-cumul-force-droite-FS.cg
// pass 4: positions
CGprogram cgPositionsFragmentProgram;     // TD5-positions-FS.cg
// pass 5: dessin
CGprogram cgDessinVertexProgram;          // TD5-dessin-VP.cg
CGprogram cgDessinFragmentProgram;        // TD5-dessin-FS.cg

// paramŽtres Cg pour le programme de calcul des forces
// pass 1: forces
CGparameter texture_FragmentProgram;                       // texture
CGparameter edge_Index_K_L0_ForceFragmentProgram;          // edge_Index_K_L0
CGparameter mass_Position_ForceFragmentProgram;            // mass_Position
CGparameter KplusFragmentShaderParam;						// raideur supplementaire pour les ressorts
CGparameter LinitFragmentShaderParam;						// raideur supplementaire pour les ressorts
CGparameter AmortFragmentShaderParam;						// amortissement supplementaire pour les ressorts

// pass 2-3 : cumul des forces
CGparameter edge_Index_K_L0_LeftForcesVertexShaderParam;   // edge_Index_K_L0
CGparameter edge_Index_K_L0_RightForcesVertexShaderParam;  // edge_Index_K_L0
CGparameter edge_Force_LeftFragmentShaderParam;            // edge_force
CGparameter edge_Force_RightFragmentShaderParam;           // edge_force
// pass 4: positions
// paramŽtres Cg pour le programme de calcul des positions
CGparameter mass_PrevPos_PositionsFragmentShaderParam;   // prev_mass_positions
CGparameter mass_CurrPos_PositionsFragmentShaderParam;   // curr_mass_positions
CGparameter mass_Forces_PositionsFragmentShaderParam;      // edge_forces
CGparameter scale_PositionsFragmentShaderParam;            // scale
CGparameter muFragmentShaderParam;							// constante de viscositÚ
CGparameter MassePlusFragmentShaderParam;					// masse supplementaire  pour les masses

// pass 5: dessin
// paramŽtres Cg pour le programme de dessin
CGparameter largeurParticlesVertexShaderParam;             // largeur
CGparameter profondeurParticlesVertexShaderParam;          // profondeur
CGparameter mass_Positions_DessinVertexShaderParam;        // mass_positionsV
CGparameter decal_DessinFragmentShaderParam;               // decal_particules

///////////////////////////////////////
// protos de fonctions
void init_scene();
void render_scene();
GLvoid initGL();
GLvoid initCg();
GLvoid window_display();
void window_timer( int step );
GLvoid window_reshape(GLsizei width, GLsizei height); 
GLvoid window_key(unsigned char key, int x, int y); 
GLvoid window_mouseFunc(int button, int state, int x, int y);
GLvoid window_motionFunc(int x, int y);

// display lists
void MakeQuadRECT2D( void );
void MakeParticles( void );
void MakePoints( void );

// creation de la texture vide
void CreateTexture( int textureId , int sizeX , int sizeY );

/////////////////////////////////////
//			OSC link
//////////////////////////////////////
 void *ConnectListener(void *message);

// CJ
GLfloat * dataFloatTrace = NULL; 
GLfloat * dataFloatEdge = NULL; 
GLfloat * dataFloatMasses = NULL;

///////////////////////////////////////
// PROGRAMME PRINCIPAL

int main(int argc, char **argv)  {  
	
	
	dataFloatTrace = new GLfloat[ 512 * 512 * 4 ];
	
	for( int ind = 0 ; ind < NB_MASSESREEL ; ind++ ) {
		raideur[ ind ] = 0;
		elongation [ind ] =2;
	}
	for( int ind = NB_MASSESREEL ; ind < NB_EDGES ; ind++ ) {
		raideur[ ind ] = 0;
		elongation [ind ] =0.01;
	}
	
	
	// initialisation  des paramtres de GLUT en fonction
	// des arguments sur la ligne de commande
	glutInit(&argc, argv);
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
	
	// dŽfinition et crŽation de la fentre graphique
	glutInitWindowSize(width, height);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("breathing");
	
	// initialisation de OpenGL de Cg et de la scne
	initGL();  
	initCg();  
	init_scene();
	//glutFullScreen();
	 	//Pthread OSC 
	 	pthread_t listener_thread;
	 	int thread_id;
	 	char *nothing;
	 	// crÚe un thread qui execute en parall¶le la fonction ConnectListener ci-dessous avec des attributs par dÚfaut
	 	thread_id = pthread_create( &listener_thread, NULL, ConnectListener , (void*) nothing);
	
	
	
	// choix des procŽdures de callback pour 
	// le tracŽ graphique
	glutDisplayFunc(&window_display);
	// le redimensionnement de la fentre
	glutReshapeFunc(&window_reshape);
	// la gestion des ŽvŽnements clavier
	glutKeyboardFunc(&window_key);
	// la gestion des clicks souris
	glutMouseFunc(&window_mouseFunc);
	// la gestion des dŽplacement souris
	glutMotionFunc(&window_motionFunc);
	// fonction appelŽe rŽgulirement entre deux gestions d«ŽvŽnements
	glutTimerFunc(0,&window_timer,FrameNo);
	
	// la boucle prinicipale de gestion des ŽvŽnements utilisateur
	glutMainLoop();  
	
	return 1;
}

// fonction OSC 
 void *ConnectListener(void *message) {
 	//dÚfini la mÚthode de traitement des donnÚes 
 	SarahPacketListener listener;
 	//dÚfini la communication UDP et permet de recevoir les pacquets
 	UdpListeningReceiveSocket s(
 								IpEndpointName( IpEndpointName::ANY_ADDRESS, IN_PORT ),
 								&listener );


 	std::cout << "listener activated"<< endl;

 	s.RunUntilSigInt();
 }


///////////////////////////////////////
// INITIALISATION DE OpenGL

GLvoid initGL()  {
	glClearColor(1,1,1,1);        
	
	// DŽfinit un modle d«ombrage
	glShadeModel(GL_SMOOTH);
	
	// Z Buffer pour la suppression des parties cachŽes
	glEnable(GL_DEPTH_TEST);
	
}

///////////////////////////////////////
// INITIALISATION DE Cg

GLvoid initCg()  {
	char filename[256];
	char filename2[256];
	
	// register the error callback once the context has been created
	cgSetErrorCallback(cgErrorCallback);
	
	// crŽation d'un contexte pour les programmes Cg
	cgContext = cgCreateContext();
	// + vŽrification
	if (cgContext == NULL) {
		printf( "Erreur de chargement du contexte Cg\n" ); exit( 0 );
	}
	cgGLRegisterStates(cgContext);
	cgGLSetManageTextureParameters(cgContext,CG_TRUE);
	
	
	// le dernier profil pour les vertex
	cgVertexProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
	cgVertexProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
	// + vŽrification
	if (cgVertexProfile == CG_PROFILE_UNKNOWN) {
		printf( "Profil de vertex invalide\n" ); exit( 0 );
	}
	cgGLSetOptimalOptions(cgVertexProfile);
	
	// le dernier profil pour les fragments
	cgFragmentProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
	cgFragmentProfile = cgGetProfile("fp40");
	// + vŽrification
	if (cgFragmentProfile == CG_PROFILE_UNKNOWN) {
		printf( "Profil de fragment invalide\n" ); exit( 0 );
	}
	cgGLSetOptimalOptions(cgFragmentProfile);
	
	/////////////////////////////////////////////////
	// pass 1: forces
	// compilation des programmmes de vertex et de fragment de calcul des forces
	
	// compile le programme de vertex ˆ partir d'un fichier
	strcpy( filename , "src/TD5-quad2-VP.cg" );
	printf( "Chargement du programme de vertex %s\n" , filename );
	cgQuadVertexProgram 
	= cgCreateProgramFromFile( cgContext,        // le context Cg courant
							  CG_SOURCE,        // programme source
							  // vs. CG_OBJECT pour un
							  // programme objet
							  filename,         // le nom du fichier
							  cgVertexProfile,  // le profil
							  "main",           // le programme d'entrŽe
							  NULL );           // autres paramtres
	// + vŽrification
	if( cgQuadVertexProgram == NULL ) {
		// Cg nous renvoie un pointeur vers une erreur
		CGerror Error = cgGetError();
		
		// dont on affiche la chaine - gŽnŽralement peu informative...
		printf( "Chargement incorrect du programme de vertex de calcul forces %s (%s)\n" ,
			   filename , cgGetErrorString(Error) ); 
		exit( 0 );
	}
	
	// compile le programme de fragment ˆ partir d'un fichier
	strcpy( filename , "src/TD5-force2-FS.cg" );
	printf( "Chargement du programme de fragment %s\n" , filename );
	cgForceFragmentProgram 
	= cgCreateProgramFromFile( cgContext,        // le context Cg courant
							  CG_SOURCE,        // programme source
							  // vs. CG_OBJECT pour un
							  // programme objet
							  filename,         // le nom du fichier
							  cgFragmentProfile,  // le profil
							  "main",           // le programme d'entrŽe
							  NULL );           // autres paramtres
	// + vŽrification
	if( cgForceFragmentProgram == NULL ) {
		// Cg nous renvoie un pointeur vers une erreur
		CGerror Error = cgGetError();
		
		// dont on affiche la chaine - gŽnŽralement peu informative...
		printf( "Chargement incorrect du programme de fragment de calcul de forces %s (%s)\n" ,
			   filename , cgGetErrorString(Error) ); 
		exit( 0 );
	}
	
	/////////////////////////////////////////////////
	// pass 2-3 : cumul des forces
	// compilation des programmmes de vertex et de fragment de cumul des forces
	
	// compile le programme de vertex ˆ partir d'un fichier
	strcpy( filename , "src/TD5-forces-gauche2-VP.cg" );
	printf( "Chargement du programme de vertex %s\n" , filename );
	cgVertexLeftForcesProgram 
	= cgCreateProgramFromFile( cgContext,        // le context Cg courant
							  CG_SOURCE,        // programme source
							  // vs. CG_OBJECT pour un
							  // programme objet
							  filename,         // le nom du fichier
							  cgVertexProfile,  // le profil
							  "main",           // le programme d'entrŽe
							  NULL );           // autres paramtres
	strcpy( filename2 , "src/TD5-forces-droite2-VP.cg" );
	printf( "Chargement du programme de vertex %s\n" , filename2 );
	cgVertexRightForcesProgram 
	= cgCreateProgramFromFile( cgContext,        // le context Cg courant
							  CG_SOURCE,        // programme source
							  // vs. CG_OBJECT pour un
							  // programme objet
							  filename2,         // le nom du fichier
							  cgVertexProfile,  // le profil
							  "main",           // le programme d'entrŽe
							  NULL );           // autres paramtres
	// + vŽrification
	if( cgVertexLeftForcesProgram == NULL 
	   || cgVertexRightForcesProgram == NULL ) {
		// Cg nous renvoie un pointeur vers une erreur
		CGerror Error = cgGetError();
		
		// dont on affiche la chaine - gŽnŽralement peu informative...
		printf( "Chargement incorrect du programme de vertex de cumul de forces %s ou %s (%s)\n" ,
			   filename , filename2 , cgGetErrorString(Error) ); 
		exit( 0 );
	}
	
	// compile le programme de fragment ˆ partir d'un fichier
	strcpy( filename , "src/TD5-cumul-force-gauche2-FS.cg" );
	printf( "Chargement du programme de fragment %s\n" , filename );
	cgFragmentLeftCumulForcesProgram 
	= cgCreateProgramFromFile( cgContext,        // le context Cg courant
							  CG_SOURCE,        // programme source
							  // vs. CG_OBJECT pour un
							  // programme objet
							  filename,         // le nom du fichier
							  cgFragmentProfile,  // le profil
							  "main",           // le programme d'entrŽe
							  NULL );           // autres paramtres
	strcpy( filename2 , "src/TD5-cumul-force-droite2-FS.cg" );
	printf( "Chargement du programme de fragment %s\n" , filename2 );
	cgFragmentRightCumulForcesProgram 
	= cgCreateProgramFromFile( cgContext,        // le context Cg courant
							  CG_SOURCE,        // programme source
							  // vs. CG_OBJECT pour un
							  // programme objet
							  filename2,         // le nom du fichier
							  cgFragmentProfile,  // le profil
							  "main",           // le programme d'entrŽe
							  NULL );           // autres paramtres
	// + vŽrification
	if( cgFragmentLeftCumulForcesProgram == NULL 
	   || cgFragmentRightCumulForcesProgram == NULL ) {
		// Cg nous renvoie un pointeur vers une erreur
		CGerror Error = cgGetError();
		
		// dont on affiche la chaine - gŽnŽralement peu informative...
		printf( "Chargement incorrect du programme de fragment de cumul de forces gauche %s ou droite %s (%s)\n" , filename , filename2 , cgGetErrorString(Error) ); 
		exit( 0 );
	}
	
	/////////////////////////////////////////////////
	// pass 4: positions
	// compilation des progs de vertex et de fragment de calcul des positions
	
	// mme programme de vertex que le calcul des forces
	
	// compile le programme de fragment ˆ partir d'un fichier
	strcpy( filename , "src/TD5-positions3-FS.cg" );
	printf( "Chargement du programme de fragment %s\n" , filename );
	cgPositionsFragmentProgram 
	= cgCreateProgramFromFile( cgContext,        // le context Cg courant
							  CG_SOURCE,        // programme source
							  // vs. CG_OBJECT pour un
							  // programme objet
							  filename,         // le nom du fichier
							  cgFragmentProfile,  // le profil
							  "main",           // le programme d'entrŽe
							  NULL );           // autres paramtres
	// + vŽrification
	if( cgPositionsFragmentProgram == NULL ) {
		// Cg nous renvoie un pointeur vers une erreur
		CGerror Error = cgGetError();
		
		// dont on affiche la chaine - gŽnŽralement peu informative...
		printf( "Chargement incorrect du programme de fragment %s (%s)\n" ,
			   filename , cgGetErrorString(Error) ); 
		exit( 0 );
	}
	
	/////////////////////////////////////////////////
	// pass 5: dessin
	// compilation des programmmes de vertex et de fragment de dessin
	
	// compile le programme de vertex ˆ partir d'un fichier
	strcpy( filename , "src/TD5-dessin2-VP.cg" );
	printf( "Chargement du programme de vertex %s\n" , filename );
	cgDessinVertexProgram 
	= cgCreateProgramFromFile( cgContext,        // le context Cg courant
							  CG_SOURCE,        // programme source
							  // vs. CG_OBJECT pour un
							  // programme objet
							  filename,         // le nom du fichier
							  cgVertexProfile,  // le profil
							  "main",           // le programme d'entrŽe
							  NULL );           // autres paramtres
	// + vŽrification
	if( cgDessinVertexProgram == NULL ) {
		// Cg nous renvoie un pointeur vers une erreur
		CGerror Error = cgGetError();
		
		// dont on affiche la chaine - gŽnŽralement peu informative...
		printf( "Chargement incorrect du programme de vertex de dessin %s (%s)\n" ,
			   filename , cgGetErrorString(Error) ); 
		exit( 0 );
	}
	
	// compile le programme de fragment ˆ partir d'un fichier
	strcpy( filename , "src/TD5-dessin2-FS.cg" );
	printf( "Chargement du programme de fragment %s\n" , filename );
	cgDessinFragmentProgram 
	= cgCreateProgramFromFile( cgContext,        // le context Cg courant
							  CG_SOURCE,        // programme source
							  // vs. CG_OBJECT pour un
							  // programme objet
							  filename,         // le nom du fichier
							  cgFragmentProfile,  // le profil
							  "main",           // le programme d'entrŽe
							  NULL );           // autres paramtres
	// + vŽrification
	if( cgDessinFragmentProgram == NULL ) {
		// Cg nous renvoie un pointeur vers une erreur
		CGerror Error = cgGetError();
		
		// dont on affiche la chaine - gŽnŽralement peu informative...
		printf( "Chargement incorrect du programme de fragment %s (%s)\n" ,
			   filename , cgGetErrorString(Error) ); 
		exit( 0 );
	}
	
	/////////////////////////////////////////////////
	// pass 1: forces
	// chargement des programmmes de vertex et de fragment de calcul des forces
	
	// chargement du programme de vertex de quad
	cgGLLoadProgram(cgQuadVertexProgram);
	
	// chargement du programme de fragment de calcul des forces
	cgGLLoadProgram(cgForceFragmentProgram);
	
	//  et liens avec ses paramtres
	edge_Index_K_L0_ForceFragmentProgram 
	= cgGetNamedParameter(cgForceFragmentProgram, "edge_Index_K_L0");
	mass_Position_ForceFragmentProgram
	= cgGetNamedParameter(cgForceFragmentProgram, "mass_Position");
	KplusFragmentShaderParam 
	= cgGetNamedParameter(cgForceFragmentProgram, "Kplus");
	LinitFragmentShaderParam 
	= cgGetNamedParameter(cgForceFragmentProgram, "Linit");
	AmortFragmentShaderParam 
	= cgGetNamedParameter(cgForceFragmentProgram, "Amort");
	
	
	if (!edge_Index_K_L0_ForceFragmentProgram 
		|| !mass_Position_ForceFragmentProgram
		|| !KplusFragmentShaderParam
		|| !LinitFragmentShaderParam
		|| !AmortFragmentShaderParam) {
		printf("Erreur dans le chargement des paramtres du programme de fragment de calcul des forces\n");
		exit(0);
	}
	
	/////////////////////////////////////////////////
	// pass 2-3 : cumul des forces
	// chargement des programmmes de vertex et de fragment de cumul des forces
	
	// chargement du programme de vertex de cumul des forces gauche
	cgGLLoadProgram(cgVertexLeftForcesProgram);
	
	//  et liens avec ses paramtres
	edge_Index_K_L0_LeftForcesVertexShaderParam 
	= cgGetNamedParameter(cgVertexLeftForcesProgram, "edge_Index_K_L0");
	
	if (!edge_Index_K_L0_LeftForcesVertexShaderParam ) {
		printf("Erreur dans le chargement des paramtres du programme de vertex de forces gauche\n");
		exit(0);
	}
	
	// chargement du programme de vertex de cumul des forces droite
	cgGLLoadProgram(cgVertexRightForcesProgram);
	
	//  et liens avec ses paramtres
	edge_Index_K_L0_RightForcesVertexShaderParam 
	= cgGetNamedParameter(cgVertexRightForcesProgram, "edge_Index_K_L0");
	
	if (!edge_Index_K_L0_RightForcesVertexShaderParam  ) {
		printf("Erreur dans le chargement des paramtres du programme de vertex de forces droite\n");
		exit(0);
	}
	
	// chargement du programme de fragment de cumul des forces gauche
	cgGLLoadProgram(cgFragmentLeftCumulForcesProgram);
	
	//  et liens avec ses paramtres
	edge_Force_LeftFragmentShaderParam
	= cgGetNamedParameter(cgFragmentLeftCumulForcesProgram, "edge_forces");
	
	if (!edge_Force_LeftFragmentShaderParam ) {
		printf("Erreur dans le chargement des paramtres du programme de fragment de forces gauche\n");
		exit(0);
	}
	
	// chargement du programme de fragment de cumul des forces droite
	cgGLLoadProgram(cgFragmentRightCumulForcesProgram);
	
	//  et liens avec ses paramtres
	edge_Force_RightFragmentShaderParam
	= cgGetNamedParameter(cgFragmentRightCumulForcesProgram, "edge_forces");
	
	if (!edge_Force_RightFragmentShaderParam ) {
		printf("Erreur dans le chargement des paramtres du programme de fragment de forces droite\n");
		exit(0);
	}
	/////////////////////////////////////////////////
	// pass 4: positions
	// chargement du programmmes de fragment de calcul des positions
	
	// mme programme de vertex que les forces
	
	// chargement du shader de fragment positions gauche
	cgGLLoadProgram(cgPositionsFragmentProgram);
	
	//  et liens avec ses paramtres
	mass_Forces_PositionsFragmentShaderParam 
	= cgGetNamedParameter(cgPositionsFragmentProgram, "edge_forces");
	mass_PrevPos_PositionsFragmentShaderParam
	= cgGetNamedParameter(cgPositionsFragmentProgram, "prev_mass_positions");
	mass_CurrPos_PositionsFragmentShaderParam
	= cgGetNamedParameter(cgPositionsFragmentProgram, "curr_mass_positions");
	scale_PositionsFragmentShaderParam 
	= cgGetNamedParameter(cgPositionsFragmentProgram, "scale");
	MassePlusFragmentShaderParam 
	= cgGetNamedParameter(cgPositionsFragmentProgram, "MassePlus");
	muFragmentShaderParam 
	= cgGetNamedParameter(cgPositionsFragmentProgram, "mu");
	if ( !scale_PositionsFragmentShaderParam 
		|| !mass_Forces_PositionsFragmentShaderParam 
		|| !mass_PrevPos_PositionsFragmentShaderParam 
		|| !mass_CurrPos_PositionsFragmentShaderParam
		|| !MassePlusFragmentShaderParam
		|| !muFragmentShaderParam) {
		printf("Erreur dans le chargement des paramtres du programme de fragment de positions\n");
		exit(0);
	}
	
	/////////////////////////////////////////////////
	// pass 5: dessin
	// chargement des programmmes de vertex et de fragment de dessin
	
	// chargement du programme de vertex
	cgGLLoadProgram(cgDessinVertexProgram);
	
	//  et liens avec ses paramtres
	largeurParticlesVertexShaderParam 
	= cgGetNamedParameter(cgDessinVertexProgram, "largeur"); 
	profondeurParticlesVertexShaderParam 
	= cgGetNamedParameter(cgDessinVertexProgram, "profondeur"); 
	mass_Positions_DessinVertexShaderParam
	= cgGetNamedParameter(cgDessinVertexProgram, "mass_positionsV");
	
	if (!largeurParticlesVertexShaderParam 
		|| !profondeurParticlesVertexShaderParam 
		|| !mass_Positions_DessinVertexShaderParam ) {
		printf("Erreur dans le chargement des paramtres du programme de vertex de dessin\n");
		exit(0);
	}
	
	// chargement du shader de fragment dessin gauche
	cgGLLoadProgram(cgDessinFragmentProgram);
	
	//  et liens avec ses paramtres
	decal_DessinFragmentShaderParam 
	= cgGetNamedParameter(cgDessinFragmentProgram, "decal_particules");
	if ( !decal_DessinFragmentShaderParam ) {
		printf("Erreur dans le chargement des paramtres du programme de fragment de dessin\n");
		exit(0);
	}
}

///////////////////////////////////////
// CHARGEMENT DES DONNƒES DANS LA GPU: display lists et textures

void init_scene() {
	//////////////////////////////////////////////////////
	// initialisation FBO: frame buffer object 
	// + initialisation des textures de positions
	// Creation d'un frame-buffer object
	glGenFramebuffersEXT( 1, &g_frameBuffer );
	
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, g_frameBuffer );
	CreateTexture( POSITION_1_TEXTURE , 512 , 512 );
	CreateTexture( FORCE_TEXTURE , 512 , 512 );
	CreateTexture( FORCE_MASS_TEXTURE , 512 , 512 );
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	
	CreateTexture( EDGE_TEXTURE , 512 , 512 );
	
	CreateTextureRECTPNG( &g_Texture[DECAL_TEXTURE] , "textures/atom.png" );
	
	//  CreateTextureRECTPNG( &g_Texture[DECAL_TEXTURE] , "textures/atom3.png" );
	
	
	printf( "Texture DECAL_TEXTURE No %d ID (%d)\n" , DECAL_TEXTURE , g_Texture[DECAL_TEXTURE] );
	
	// initialise les display lists 
	MakeQuadRECT2D();
	MakeParticles();
	MakePoints();
	
}

///////////////////////////////////////
// FONCTIONS DE CALL-BACK DE GLUT

// fonction de call-back appelŽe rŽgulirement

void window_timer( int step ) 
{
	glutTimerFunc(latence,&window_timer,++FrameNo);
	
	//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//   glLoadIdentity();
	
	glutPostRedisplay();
	
	if( FrameNo % 25 == 0 ) {
		printf( "Frame %d\n" , FrameNo );
	}
}

// fonction de call-back de glut pour l«affichage dans la fentre

GLvoid window_display() {
	render_scene();
	
	// trace la scne grapnique qui vient juste d'tre dŽfinie
	glFlush();
}

// fonction de call-back de glut pour le redimensionnement de la fentre

GLvoid window_reshape( GLsizei newWidth, GLsizei newHeight ) {  
	width = newWidth;
	height = newHeight;
	
	ratio = (float) width / (float) height;
	
	glViewport(0, 0, 512, 512);
}

// fonction de call-back de glut pour la gestion des ŽvŽnements clavier

GLvoid window_key(unsigned char key, int x, int y)  {  
	switch (key) {    
		case KEY_ESC:  
			exit(1);                    
			break; 
		default:
			printf ("La touche %d n«est pas active.\n", key);
			break;
	}     
}

// fonction de call-back de glut de gestion des clicks souris

GLvoid window_mouseFunc(int button, int state, int x, int y) {
	if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
		mouse_pos_x = x;
		mouse_pos_y = y;
	}
}

// fonction de call-back de glut de gestion des dŽplacement de la souris

GLvoid window_motionFunc(int x, int y) {
	angle_x += y - mouse_pos_y;
	angle_y += x - mouse_pos_x;
	
	mouse_pos_x = x;
	mouse_pos_y = y;
	
	glutPostRedisplay();
}


// compilation de la display list des quads
// de rendu auxiliaire pour une texture
// pour une texture RECT (coordonnŽes de texture entre 
// 0 et width) dans le FS
// et via la normale pour une texture 2D (coordonnŽes 
// de texture entre 0 et 1) dans le VP
void MakeQuadRECT2D( void ) {
	glNewList(My_QuadRECT2D, GL_COMPILE);
	
	glBegin(GL_POLYGON);
	glNormal3f(0.0 , 0.0 , 0.0);
	glTexCoord2f(0.0 , 0.0);
	glVertex3f( -1.0 , -1.0 , 0.0 ); 
	
	glNormal3f(1.0 , 0.0 , 0.0);
	glTexCoord2f(width , 0.0);
	glVertex3f( 1.0 , -1.0 , 0.0);
	
	glNormal3f(1.0 , 1.0 , 0.0);
	glTexCoord2f(width , width);
	glVertex3f( 1.0 , 1.0 , 0.0);
	
	glNormal3f(0.0 , 1.0 , 0.0);
	glTexCoord2f(0.0 , width);
	glVertex3f( -1.0 , 1.0 , 0.0);
	glEnd();
	
	glEndList();
}

///////////////////////////////////////
// compilation de la display list des points pour les particules

void MakeParticles( void ) {
	// particules sous formes de quads: on donne 
	// deux indices dans la position: l'indice de la masse
	// et l'indice de position autour de la masse
	glNewList(My_Particles, GL_COMPILE);
	//particule relative au ressort
	for( int ind = 0 ; ind < NB_MASSESREEL ; ind++ ) {
		glBegin(GL_POLYGON);
		glTexCoord2f(0.0 , 0.0);
		glVertex3f(  (float)(ind) , 3.0 , -0.01);
		
		glTexCoord2f(128.0 , 0.0);
		glVertex3f(  (float)(ind) , 1.0 , -0.01 ); 
		
		glTexCoord2f(128.0 , 128.0);
		glVertex3f(  (float)((ind+1) % (NB_MASSESREEL)) , 1.0 , -0.01); 
		
		glTexCoord2f(0.0 , 128.0);
		glVertex3f(  (float)((ind+1) % (NB_MASSESREEL)) , 3.0 , -0.01); 
		glEnd();
	}
	
	
	
	/*for( int ind = NB_MASSES ; ind < NB_EDGES ; ind++ ) {
		glBegin(GL_POLYGON);
		glTexCoord2f(0.0 , 0.0);
		glVertex3f(  4.0 , 0.0 , -0.1 );
		
		glTexCoord2f(128.0 , 0.0);
		glVertex3f(  4.0 , 1.0 , -0.1 ); 
		
		glTexCoord2f(128.0 , 128.0);
		glVertex3f(  (float)((ind)%NB_MASSES) , 1.0 , -0.1 ); 
		
		glTexCoord2f(0.0 , 128.0);
		glVertex3f(  (float)((ind)%NB_MASSES) , 0.0 , -0.1 ); 
		glEnd();
	}*/
	
	//particule relative aux masses 
	for( int ind = 0 ; ind < NB_MASSESREEL ; ind++ ) {
		glBegin(GL_POLYGON);
		glTexCoord2f(0.0 , 0.0);
		glVertex4f(  (float)(ind) , 0.0 , 0.0, -2.0 );
		
		glTexCoord2f(128.0 , 0.0);
		glVertex4f(  (float)(ind) , 1.0 , 0.0, -2.0 ); 
		
		glTexCoord2f(128.0 , 128.0);
		glVertex4f(  (float)(ind) , 2.0 , 0.0, -2.0 ); 
		
		glTexCoord2f(0.0 , 128.0);
		glVertex4f(  (float)(ind) , 3.0 , 0.0, -2.0 ); 
		glEnd();
	}
	glEndList();
}

///////////////////////////////////////
// compilation de la display list des points pour 

void MakePoints( void ) {
	// 1 point correspond ˆ une masse au bout d'une arte
	// il est rendu ˆ la position dans la texture de force
	// (donc aux coordonnŽes de texture d'une masse dans 
	// une texture de donnŽes de masses)
	glNewList(My_Points, GL_COMPILE);
	glPointSize( 1.0 );
	glDisable( GL_POINT_SMOOTH );
	// chaque arete est rendue deux fois: une pour la masse
	// de gauche, une pour la masse de droite
	// les masses sont positionnŽes au bon endroit du frame buffer
	// par le programme de vertex de gauche ou de droite selon
	// la passe 2 ou 3
	// on passe l'indice de la masse par la position
	glBegin(GL_POINTS);
	for( int ind = 0 ; ind < NB_EDGES ; ind++ ) {
		glVertex3f( (float)(ind) , 0.0 , 0.0  ); 
	}
	glEnd();
	glEndList();
}

///////////////////////////////////////
// creation d'une texture vide
void CreateTexture( int textureId , int sizeX , int sizeY ) {
	// Return from the function if no file name was passed in
	// Load the image and store the data
	GLfloat * dataFloat = NULL; 
	
	switch( textureId ) {
		case POSITION_1_TEXTURE:
		{
			/////////////////////////////////////////////////////////////////////
			// texture qui sert ˆ stocker les informations sur les aretes
			// (indices des masses, k, l0
			// utilisŽe en shader de vertex et de fragment
			// doit donc permettre le vertex texture fetch
			// donc GL_TEXTURE_2D et format GL_RGBA_FLOAT32_ATI
			dataFloatMasses = new GLfloat[ sizeX * sizeY * 4 ];
			
			// If we can't load the file, quit!
			if(dataFloatMasses == NULL) {		  
				printf( "Texture allocation error!" ); throw 425;
			}
			
			float alpha = M_PI/4;
			// mass location
			// couronne
			for( int ind = 0 ; ind < NB_MASSES ; ind++ ) {
				float R =  2;
				float alpha =(M_PI/4) + (float)ind * (M_PI/2);
				dataFloatMasses[ ind * 4 ]     = 0;
				dataFloatMasses[ ind * 4 + 1 ] = 0;
				dataFloatMasses[ ind * 4 + 2 ] = 0;
				dataFloatMasses[ ind * 4 + 3 ] = 1.0;
			}		
			
			dataFloatMasses[ 0 ] = 1;
			dataFloatMasses[ 1 ] = 1;
			dataFloatMasses[ 4 ] = -1;
			dataFloatMasses[ 5 ] = 1;
			dataFloatMasses[ 8 ] = -1;
			dataFloatMasses[ 9 ] = -1;
			dataFloatMasses[ 12 ] = 1;
			dataFloatMasses[ 13 ] = -1;
			
			dataFloatMasses[ 16 ] = -1.01;
			dataFloatMasses[ 17 ] = 1.01;
			//dataFloatMasses[ 19 ] = -1.0;
			dataFloatMasses[ 20 ] = -1.01;
			dataFloatMasses[ 21 ] = -1.01;
			//dataFloatMasses[ 23 ] = -1.0;
			dataFloatMasses[ 24 ] = 1.01;
			dataFloatMasses[ 25 ] = -1.01;
			//dataFloatMasses[ 27 ] = -1.0;
			dataFloatMasses[ 28 ] = 1.01;
			dataFloatMasses[ 29 ] = 1.01;
			//dataFloatMasses[ 31 ] = -1.0;
			
			for( int i = NB_MASSES ; i < sizeX * sizeY ; i++ ) {
				dataFloatMasses[ i * 4 ] = 0;
				dataFloatMasses[ i * 4 + 1 ] = 0;
				dataFloatMasses[ i * 4 + 2 ] = 0;
				dataFloatMasses[ i * 4 + 3 ] = 1.0;
			}
			
			// Generate a texture with the associative texture ID stored in the array
			glGenTextures(3, &g_Texture[POSITION_1_TEXTURE] );
			printf( "Texture No %d ID (%d)\n" , textureId , g_Texture[POSITION_1_TEXTURE] );
			printf( "Texture No %d ID (%d)\n" , textureId , g_Texture[POSITION_2_TEXTURE] );
			printf( "Texture No %d ID (%d)\n" , textureId , g_Texture[POSITION_3_TEXTURE] );
			
			for( int indTex = POSITION_1_TEXTURE ; indTex <= POSITION_3_TEXTURE ; 
				indTex++ ) {
				// Bind the texture to the texture arrays index and init the texture
				glEnable( GL_TEXTURE_2D );
				glBindTexture( GL_TEXTURE_2D, g_Texture[indTex]);
				
				/////////////////////////////////////////////////////////////////////
				// textures des positions 
				// textures des positions avec un ping pong entre trois
				// cibles d'Žcriture pour le FBO: la position prŽcŽdente, 
				// la position courante et la position antŽrieure. 
				// On a besoin de ces trois valeurs pour
				// calculer le dŽplacement dans le schŽma d'intŽgration de Verlet
				// VTF (vertex texture fetch) donc format GL_RGBA_FLOAT32_ATI
				// attachŽe ˆ un FBO 
				// initialisŽe par les valeurs stockŽes dans dataFloat
				glTexImage2D( GL_TEXTURE_2D , 0, GL_RGBA_FLOAT32_ATI , 
							 sizeX, sizeY, 
							 // dataFloat est NULL pour POSITION_3_TEXTURE
							 // qui n'a pas besoin d'etre initialisŽe
							 0, GL_RGBA, GL_FLOAT, (const void *)dataFloatMasses );
				
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
								GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
								GL_NEAREST );
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
				glBindTexture( GL_TEXTURE_2D, 0);
				
				

				
			} 
		}
			break;
		case FORCE_TEXTURE:
		{
			// Generate a texture with the associative texture ID stored in the array
			glGenTextures(1, &g_Texture[textureId] );
			printf( "Texture No %d ID (%d)\n" , textureId , g_Texture[textureId] );
			
			glEnable( GL_TEXTURE_RECTANGLE_ARB );
			// Lie la texture avec une cible rectangulaire (por FBO) 
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, g_Texture[textureId]);
			
			/////////////////////////////////////////////////////////////////////
			// texture qui sert ˆ stocker les forces avant de les cumuler
			// attachŽe ˆ un FBO et sans initialisation nŽcessaire
			glTexImage2D(  GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB , 
						 sizeX, sizeY, 0, GL_RGBA, GL_FLOAT, 0 );
			glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, 
							GL_NEAREST );
			glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, 
							GL_NEAREST );
			glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0);
		}
			break;
		case FORCE_MASS_TEXTURE:
		{
			// Generate a texture with the associative texture ID stored in the array
			glGenTextures(1, &g_Texture[textureId] );
			printf( "Texture No %d ID (%d)\n" , textureId , g_Texture[textureId] );
			
			glEnable( GL_TEXTURE_RECTANGLE_ARB );
			// Lie la texture avec une cible rectangulaire (por FBO) 
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, g_Texture[textureId]);
			
			/////////////////////////////////////////////////////////////////////
			// texture qui sert ˆ cumuler les forces 
			// comme le cumul se fait par blending accumulatif,
			// la prŽcision doit tre 16f (le blending sur les textures 
			// 32f clampe les valeurs ˆ 0..1)
			// attachŽe ˆ un FBO et sans initialisation nŽcessaire
			glTexImage2D( GL_TEXTURE_RECTANGLE_ARB , 0, GL_RGBA16F_ARB , 
						 sizeX, sizeY, 0, GL_RGBA, GL_FLOAT, 0 );
			glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, 
							GL_NEAREST );
			glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, 
							GL_NEAREST );
			glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0);
		}
			break;
		case EDGE_TEXTURE:
		{
			// Generate a texture with the associative texture ID stored in the array
			glGenTextures(1, &g_Texture[textureId] );
			printf( "Texture No %d ID (%d)\n" , textureId , g_Texture[textureId] );
			
			glEnable( GL_TEXTURE_2D );
			// Lie la texture avec une cible carrŽe
			glBindTexture(GL_TEXTURE_2D, g_Texture[textureId]);
			
			/////////////////////////////////////////////////////////////////////
			// texture qui sert ˆ stocker les informations sur les aretes
			// (indices des masses, k, l0
			// utilisŽe en shader de vertex et de fragment
			// doit donc permettre le vertex texture fetch
			// donc format GL_RGBA_FLOAT32_ATI
			dataFloatEdge = new GLfloat[ sizeX * sizeY * 4 ];
			
			// If we can't load the file, quit!
			if(dataFloatEdge == NULL) {		  
				printf( "Texture allocation error!" ); throw 425;
			}
			
			// mass1 index, mass2 index, rest length, elasticity
			// edge #1-4
			
			//les 4 premire masses sont en carrŽ
				
				for( int i = 0 ; i < NB_MASSESREEL ; i++ ) {
					dataFloatEdge[ i * 4 ]     = (GLfloat)(i % NB_MASSESREEL);
					dataFloatEdge[ i * 4 + 1 ] = (GLfloat)((i + 1) % NB_MASSESREEL);
					dataFloatEdge[ i * 4 + 2 ] = 0;
					dataFloatEdge[ i * 4 + 3 ] = 2;
				}
	
			
			for( int i = NB_MASSESREEL ; i < NB_EDGES ; i++ ) {
				dataFloatEdge[ i * 4 ]     = (GLfloat)i ;
				dataFloatEdge[ i * 4 + 1 ] = (GLfloat)((i + 1) % NB_MASSESREEL);
				dataFloatEdge[ i * 4 + 2 ] = 0;
				dataFloatEdge[ i * 4 + 3 ] = 0.01;
			}
			
			for( int i = NB_EDGES ; i < sizeX * sizeY ; i++ ) {
				dataFloatEdge[ i * 4 ]     = 0;
				dataFloatEdge[ i * 4 + 1 ] = 0;
				dataFloatEdge[ i * 4 + 2 ] = 0;
				dataFloatEdge[ i * 4 + 3 ] = 0;
			}
			// Bind the texture to the texture arrays index and init the texture
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA_FLOAT32_ATI ,
						 sizeX, sizeY, 0, GL_RGBA, 
						 GL_FLOAT, (const void *)dataFloatEdge ); 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,  
							GL_NEAREST );
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
							GL_NEAREST );
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glBindTexture( GL_TEXTURE_2D, 0);
		}
			break;
		default:
			printf( "Texture Initialization not defined for texture ID %d\n" , 
				   textureId ); 
	}
	
	// Now we need to free the image data that we loaded since OpenGL 
	// stored it as a texture
	
	if (dataFloat) {	        // If there is texture data
		free(dataFloat);         // Free the texture data, we don't need it anymore
	}
}

///////////////////////////////////////
// ERROR CHECKING
void checkGLErrors(const char *label) {
	GLenum errCode;
	const GLubyte *errStr;
	if ((errCode = glGetError()) != GL_NO_ERROR) {
		errStr = gluErrorString(errCode);
		printf("OpenGL ERROR: ");
		printf((char*)errStr);
		printf("(Label: ");
		printf(label);
		printf(")\n.");
	}
}
bool checkFramebufferStatus() {
	GLenum status;
	status=(GLenum)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	switch(status) {
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			return true;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			printf("Framebuffer incomplete,incomplete attachment\n");
			return false;
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			printf("Unsupported framebuffer format\n");
			return false;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			printf("Framebuffer incomplete,missing attachment\n");
			return false;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			printf("Framebuffer incomplete,attached images \
                   must have same dimensions\n");
			return false;
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			printf("Framebuffer incomplete,attached images \
				   must have same format\n");
			return false;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			printf("Framebuffer incomplete,missing draw buffer\n");
			return false;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			printf("Framebuffer incomplete,missing read buffer\n");
			return false;
	}
	return false;
}

// callback function
void cgErrorCallback(void) {
	CGerror lastError = cgGetError();
	if(lastError) {
		printf(cgGetErrorString(lastError));
		printf(cgGetLastListing(cgContext));
		printf("\n");
	}
}

///////////////////////////////////////
// RENDU IMAGE PAR IMAGE
void render_scene() {
	
	glViewport(0,0,512,512);
	
	// swap de texture entre la texture des positions courantes
	// et celle des positions antŽrieures
	if( previousTexturePosition == POSITION_1_TEXTURE ) {
		previousTexturePosition = POSITION_2_TEXTURE;
		currentTexturePosition = POSITION_3_TEXTURE;
		newTexturePosition = POSITION_1_TEXTURE;
	}
	else if( previousTexturePosition == POSITION_2_TEXTURE ) {
		previousTexturePosition = POSITION_3_TEXTURE;
		currentTexturePosition = POSITION_1_TEXTURE;
		newTexturePosition = POSITION_2_TEXTURE;
	}
	else {
		previousTexturePosition = POSITION_1_TEXTURE;
		currentTexturePosition = POSITION_2_TEXTURE;
		newTexturePosition = POSITION_3_TEXTURE;
	}
	
	//   /////////////////////////////////////////////////////////
	// repassage de la texture de raideur et Žlongation pour modification des caractŽristiques des ressorts
	//   /////////////////////////////////////////////////////////
			for( int i = 0 ; i < NB_EDGES ; i++ ) {
				dataFloatEdge[ i * 4 + 2 ] = raideur[i];
				dataFloatEdge[ i * 4 + 3 ] = elongation[i];
				//raideur[i]=0;
				//elongation[i]=0;
			}


	
	glEnable( GL_TEXTURE_2D );
	glBindTexture(GL_TEXTURE_2D, g_Texture[EDGE_TEXTURE]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA_FLOAT32_ATI ,
				 512, 512, 0, GL_RGBA, 
				 GL_FLOAT, (const void *)dataFloatEdge ); 
	
	// Store the GLUT draw buffer.
	GLint glut_draw_buffer;
	glGetIntegerv(GL_DRAW_BUFFER, &glut_draw_buffer);
	
	/////////////////////////////////////////////////////////
	// pass 1: forces
	// CALCUL DES FORCES
	/////////////////////////////////////////////////////////
	// 
	
	// Attachement d'une texture au FBO
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, g_frameBuffer );
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT , 
							  GL_TEXTURE_RECTANGLE_ARB, 
							  g_Texture[FORCE_TEXTURE], 0);
	
	// reinitialise la projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
//	glOrtho( -1.0 * ratio , 1.0 * ratio , -1.0 , 1.0 , -10.0, 10.00 );
	glOrtho( -1.0  , 1.0  , -1.0 , 1.0 , -10.0, 10.00 );
	
	// no blending
	glDisable(GL_BLEND);
	// glEnable(GL_TEXTURE_RECTANGLE_ARB);
	
	// rend un quad pour le calcul des forces
	glClearColor(0,0,0,1);        
	
	// clears the buffer bits
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	
	
	// toutes les transformations suivantes s«appliquent au modle de vue 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // loads new identity
	
	cgGLEnableProfile(cgVertexProfile);
	cgGLEnableProfile(cgFragmentProfile);
	// forces sur une arte
	cgGLBindProgram(cgQuadVertexProgram);
	cgGLBindProgram(cgForceFragmentProgram);
	
	// lie les paramtres uniformes au programme de vertex
	cgGLSetStateMatrixParameter(cgGetNamedParameter(cgQuadVertexProgram, 
													"ModelViewProj"),
								CG_GL_MODELVIEW_PROJECTION_MATRIX,
								CG_GL_MATRIX_IDENTITY);
	cgGLSetStateMatrixParameter(cgGetNamedParameter(cgQuadVertexProgram, 
													"ModelView"),
								CG_GL_MODELVIEW_MATRIX,
								CG_GL_MATRIX_IDENTITY);
	
	// fait le lien entre les param¶tres de contrúle uniformÚment pris par CG 
	cgGLSetParameter1f(KplusFragmentShaderParam, Kplus);
	cgGLSetParameter1f(LinitFragmentShaderParam, Linit);
	cgGLSetParameter1f(AmortFragmentShaderParam, Amort);
	
	
	checkGLErrors("Pass 1");
	checkFramebufferStatus();
	
	// dessine le quad pour le calcul des forces
	glPushMatrix();
	/////////////////////////////////////////////////////////
	// texture des artes et des masses
	cgGLSetTextureParameter( edge_Index_K_L0_ForceFragmentProgram, 
							g_Texture[EDGE_TEXTURE] );
	cgGLEnableTextureParameter( edge_Index_K_L0_ForceFragmentProgram );
	cgGLSetTextureParameter( mass_Position_ForceFragmentProgram, 
							g_Texture[currentTexturePosition] );
	cgGLEnableTextureParameter( mass_Position_ForceFragmentProgram );
	
	
	glCallList( My_QuadRECT2D );
	
	// dŽsactivation des textures
	cgGLDisableTextureParameter(edge_Index_K_L0_ForceFragmentProgram);
	cgGLDisableTextureParameter(mass_Position_ForceFragmentProgram);
	
	glPopMatrix();
	
	cgGLUnbindProgram(cgVertexProfile);
	cgGLUnbindProgram(cgFragmentProfile);
	
	
	//   /////////////////////////////////////////////////////////
	// Affichage de la texture du ressort raideur et elongation
	//   /////////////////////////////////////////////////////////
	
	/*printf( "Raideur_elongation: " );
	for( int i=0 ; i < 8 * 4 ; i++ ) {
		printf( "%.5f " , dataFloatEdge[i] );
	}
	printf( "\n" );*/
	
	for( int ind=0 ; ind < NB_EDGES ; ind++ ) {
		
		//identifiant de la masse 
		//retour d'info en OSC vers MaxMSP
		osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
		p //<< osc::BeginBundleImmediate
		<< osc::BeginMessage( "/raideur_elongation" ) <<(int) ind <<(float)dataFloatEdge[4 * ind + 2] << (float)dataFloatEdge[4 * ind + 3] << osc::EndMessage;
		//<< osc::EndBundle;
		transmitSocket.Send( p.Data(), p.Size() );
	}
	
	
	
	//
	// Unbind the frame-buffer objects.
	//
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	
	/////////////////////////////////////////////////////////
	// pass 2-3 : cumul des forces
	// CUMUL DES FORCES
	/////////////////////////////////////////////////////////
	// 
	
	//////////////////////////////////////////////////////
	// FBO: frame buffer object 
	// Attach textures to FBO
	// copie les forces dans une texture
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT , g_frameBuffer );
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT , GL_COLOR_ATTACHMENT0_EXT , 
							  GL_TEXTURE_RECTANGLE_ARB , 
							  g_Texture[FORCE_MASS_TEXTURE] , 0 );
	
	// rend un quad pour le cumul des forces
	glClearColor(0,0,0,1);        
	
	// clears the buffer bits
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	
	// toutes les transformations suivantes s«appliquent au modle de vue 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // loads new identity
	
	cgGLEnableProfile(cgVertexProfile);
	cgGLEnableProfile(cgFragmentProfile);
	
	// blend cumulatif pour compositer les forces sur chaque pixel
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	// glEnable(GL_TEXTURE_RECTANGLE_ARB);
	
	checkGLErrors("Pass 2-3");
	checkFramebufferStatus();
	
	// dessine le quad pour le calcul des forces
	glPushMatrix();
	
	/////////////////////////////////////////////////////////
	// pass 2 : cumul des forces gauche
	// FORCES SUR LA PARTICULE DE GAUCHE
	// 
	cgGLBindProgram(cgVertexLeftForcesProgram);
	cgGLBindProgram(cgFragmentLeftCumulForcesProgram);
	
	// lie les paramtres uniformes au programme de vertex
	cgGLSetStateMatrixParameter(cgGetNamedParameter(cgVertexLeftForcesProgram, 
													"ModelViewProj"),
								CG_GL_MODELVIEW_PROJECTION_MATRIX,
								CG_GL_MATRIX_IDENTITY);
	cgGLSetStateMatrixParameter(cgGetNamedParameter(cgVertexLeftForcesProgram, 
													"ModelView"),
								CG_GL_MODELVIEW_MATRIX,
								CG_GL_MATRIX_IDENTITY);
	
	// texture des artes et des masses
	cgGLSetTextureParameter( edge_Index_K_L0_LeftForcesVertexShaderParam, 
							g_Texture[EDGE_TEXTURE] );
	cgGLEnableTextureParameter( edge_Index_K_L0_LeftForcesVertexShaderParam );
	cgGLSetTextureParameter( edge_Force_LeftFragmentShaderParam, 
							g_Texture[FORCE_TEXTURE] );
	cgGLEnableTextureParameter( edge_Force_LeftFragmentShaderParam );
	
	glCallList( My_Points );
	
	// dŽsactivation des textures
	cgGLDisableTextureParameter(edge_Index_K_L0_LeftForcesVertexShaderParam);
	cgGLDisableTextureParameter(edge_Force_LeftFragmentShaderParam);
	
	cgGLUnbindProgram(cgVertexProfile);
	cgGLUnbindProgram(cgFragmentProfile);
	
	/////////////////////////////////////////////////////////
	// pass 3 : cumul des forces droite
	// FORCES SUR LA PARTICULE DE DROITE
	// 
	
	cgGLBindProgram(cgVertexRightForcesProgram);
	cgGLBindProgram(cgFragmentRightCumulForcesProgram);
	
	// lie les paramtres uniformes au programme de vertex
	cgGLSetStateMatrixParameter( cgGetNamedParameter(cgVertexRightForcesProgram, "ModelViewProj"),
								CG_GL_MODELVIEW_PROJECTION_MATRIX,
								CG_GL_MATRIX_IDENTITY );
	cgGLSetStateMatrixParameter( cgGetNamedParameter(cgVertexRightForcesProgram, "ModelView"),
								CG_GL_MODELVIEW_MATRIX,
								CG_GL_MATRIX_IDENTITY );
	
	// texture des artes et des masses
	cgGLSetTextureParameter( edge_Index_K_L0_RightForcesVertexShaderParam, 
							g_Texture[EDGE_TEXTURE] );
	cgGLEnableTextureParameter( edge_Index_K_L0_RightForcesVertexShaderParam );
	cgGLSetTextureParameter( edge_Force_RightFragmentShaderParam, 
							g_Texture[FORCE_TEXTURE] );
	cgGLEnableTextureParameter( edge_Force_RightFragmentShaderParam );
	
	glCallList( My_Points );
	
	// dŽsactivation des textures
	cgGLDisableTextureParameter(edge_Index_K_L0_RightForcesVertexShaderParam);
	cgGLDisableTextureParameter(edge_Force_RightFragmentShaderParam);
	
	glPopMatrix();
	
	cgGLUnbindProgram(cgVertexProfile);
	cgGLUnbindProgram(cgFragmentProfile);
	
	checkGLErrors("Pass 2-3 fin");
	checkFramebufferStatus();
	
	//
	// Unbind the frame-buffer objects.
	//
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	
		
	
	/////////////////////////////////////////////////////////
	// pass 4: positions
	// CALCUL DES POSITIONS
	/////////////////////////////////////////////////////////
	// 
	// rend un quad pour le calcul des positions

	//////////////////////////////////////////////////////
	
	

	// FBO: frame buffer object 
	// Attach textures to FBO
	// copie les nouvelles positions dans une texture
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT , g_frameBuffer );
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT , GL_COLOR_ATTACHMENT0_EXT , 
							  GL_TEXTURE_2D , 
							  g_Texture[newTexturePosition] , 0 );
		
	
	// efface le fond, avoir une sortie opaque pour pouvoir
	// recuperer le resultat des calculs dans la couleur
	glClearColor(0,0,0,1);        
	
	// clears the buffer bits
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	
	// toutes les transformations suivantes s«appliquent au modle de vue 
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // loads new identity
	
	cgGLEnableProfile(cgVertexProfile);
	cgGLEnableProfile(cgFragmentProfile);
	
	// mode decal
	glDisable(GL_BLEND);
	// glEnable(GL_TEXTURE_RECTANGLE_ARB);
	
	// dessine le quad pour le calcul des positions
	cgGLBindProgram(cgQuadVertexProgram);
	cgGLBindProgram(cgPositionsFragmentProgram);
	
	glPushMatrix();
	// lie les paramtres uniformes au programme de vertex
	cgGLSetStateMatrixParameter(cgGetNamedParameter(cgQuadVertexProgram, "ModelViewProj"),
								CG_GL_MODELVIEW_PROJECTION_MATRIX,
								CG_GL_MATRIX_IDENTITY);
	cgGLSetStateMatrixParameter(cgGetNamedParameter(cgQuadVertexProgram, "ModelView"),
								CG_GL_MODELVIEW_MATRIX,
								CG_GL_MATRIX_IDENTITY);
	
	cgGLSetParameter1f(MassePlusFragmentShaderParam, MassePlus);
	cgGLSetParameter1f(muFragmentShaderParam, mu);
	
	// texture des massses et des forces
	cgGLSetTextureParameter( mass_PrevPos_PositionsFragmentShaderParam, 
							g_Texture[previousTexturePosition] );
	cgGLEnableTextureParameter( mass_PrevPos_PositionsFragmentShaderParam );
	
	cgGLSetTextureParameter( mass_CurrPos_PositionsFragmentShaderParam, 
							g_Texture[currentTexturePosition] );
	cgGLEnableTextureParameter( mass_CurrPos_PositionsFragmentShaderParam );
	
	cgGLSetTextureParameter( mass_Forces_PositionsFragmentShaderParam, 
							g_Texture[FORCE_MASS_TEXTURE] );
	cgGLEnableTextureParameter( mass_Forces_PositionsFragmentShaderParam );
	
		
	glCallList( My_QuadRECT2D );
	
		
		
	// dŽsactivation des textures
	cgGLDisableTextureParameter(mass_PrevPos_PositionsFragmentShaderParam);
	cgGLDisableTextureParameter(mass_CurrPos_PositionsFragmentShaderParam);
	cgGLDisableTextureParameter(mass_Forces_PositionsFragmentShaderParam);
	
	glPopMatrix();
	
	cgGLUnbindProgram(cgVertexProfile);
	cgGLUnbindProgram(cgFragmentProfile);
	
	checkGLErrors("Pass 4");
	checkFramebufferStatus();
	
	//   /////////////////////////////////////////////////////////
	// Affichage de la texture de position 
	//   /////////////////////////////////////////////////////////
	glReadPixels( 0 , 
				 0 , 
				 512 , 512, 
				 GL_RGBA, GL_FLOAT, dataFloatTrace );
	
	/*printf( "Positions: " );
	for( int i=0 ; i < 8 * 4 ; i++ ) {
		printf( "%.5f " , dataFloatTrace[i] );
	}
	printf( "\n" );*/
	
	//printf( "%.5f " , mu );
	
		for( int ind=0 ; ind < NB_MASSES ; ind++ ) {
	
		//identifiant de la masse 
		//retour d'info en OSC vers MaxMSP
		osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
		p //<< osc::BeginBundleImmediate
		<< osc::BeginMessage( "/position_xyz" ) <<(int) ind <<(float)dataFloatTrace[4 * ind + 0] << (float)dataFloatTrace[4 * ind + 1] << (float)dataFloatTrace[4 * ind + 2] << (float)dataFloatTrace[4 * ind + 3]<< osc::EndMessage;
		//<< osc::EndBundle;
		transmitSocket.Send( p.Data(), p.Size() );
	}
	
	//
	// Unbind the frame-buffer objects.
	//
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	
	
	//   /////////////////////////////////////////////////////////
	//   /////////////////////////////////////////////////////////
	//   /////////////////////////////////////////////////////////
	//   // rendu auxiliaire
	
	//   cgGLDisableProfile(cgVertexProfile);
	//   cgGLDisableProfile(cgFragmentProfile);
	//   // Now, set up the regular window for rendering...
	//   glClearColor(0,0,0,1);        
	//   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	//   glMatrixMode( GL_MODELVIEW );
	//   glLoadIdentity(); // loads new identity
	
	//   // no blending
	//   glDisable(GL_BLEND);
	//   glEnable(GL_TEXTURE_RECTANGLE_ARB);
	
	//   glBindTexture( GL_TEXTURE_RECTANGLE_ARB, g_Texture[FORCE_MASS_TEXTURE] );
	//   glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, 
	// 		  GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//   glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, 
	// 		  GL_TEXTURE_WRAP_S, GL_CLAMP);
	//   glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, 
	// 		  GL_TEXTURE_WRAP_T, GL_CLAMP);
	//   glCallList( My_QuadRECT2D );
	//   /////////////////////////////////////////////////////////
	//   /////////////////////////////////////////////////////////
	//   /////////////////////////////////////////////////////////
	
	// if( false ) 
	{
		/////////////////////////////////////////////////////////
		// pass 5: dessin
		// TRACƒ DES PARTICULES
		/////////////////////////////////////////////////////////
		// 
		//glViewport(0,0,1440,900);
		
		// efface le fond
		glClearColor(0,0,0,1);        
		
		// clears the buffer bits
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clears the buffer bits
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustum( -0.02 * ratio , 0.02 * ratio , -0.02 , 0.02 , 0.04, 10.00 );
		
		// toutes les transformations suivantes s«appliquent au modle de vue 
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		// blend de transparence
		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, white_transparent );
		//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND); 
		
		// pas d'Žclairement, c'est fait par les shaders
		glDisable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);
		
		// dessine le quad pour le dessin
		cgGLBindProgram(cgDessinVertexProgram);
		cgGLBindProgram(cgDessinFragmentProgram);
		
		// lie les paramtres uniformes au programme de vertex
		cgGLSetStateMatrixParameter(cgGetNamedParameter(cgDessinVertexProgram, "ModelViewProj"),
									CG_GL_MODELVIEW_PROJECTION_MATRIX,
									CG_GL_MATRIX_IDENTITY);
		cgGLSetStateMatrixParameter(cgGetNamedParameter(cgDessinVertexProgram, "ModelView"),
									CG_GL_MODELVIEW_MATRIX,
									CG_GL_MATRIX_IDENTITY);
		
		// dessine les particules et les liens en fonction des dessin
		// calculŽes ˆ l'Žtape prŽcŽdente
		glPushMatrix();
		// texture des massses et des forces
		// previousTexturePosition contient les nouvelles positions des masses
		cgGLSetTextureParameter( mass_Positions_DessinVertexShaderParam, 
								g_Texture[newTexturePosition] );
		cgGLEnableTextureParameter( mass_Positions_DessinVertexShaderParam );
		
		cgGLSetTextureParameter( decal_DessinFragmentShaderParam, 
								g_Texture[DECAL_TEXTURE] );
		cgGLEnableTextureParameter( decal_DessinFragmentShaderParam );
		
		cgGLSetParameter1f( largeurParticlesVertexShaderParam , 0.1 ); 
		cgGLSetParameter1f( profondeurParticlesVertexShaderParam , -5.2 ); 
		
		glCallList( My_Particles );
		
		// dŽsactivation des textures
		cgGLDisableTextureParameter(mass_Positions_DessinVertexShaderParam);
		cgGLDisableTextureParameter(decal_DessinFragmentShaderParam);
		
		glPopMatrix();
		
		// desactive les shaders le temps du dessin de la cube map
		cgGLUnbindProgram(cgVertexProfile);
		cgGLUnbindProgram(cgFragmentProfile);
		cgGLDisableProfile(cgVertexProfile);
		cgGLDisableProfile(cgFragmentProfile);
	}
	
	checkGLErrors("Pass 5");
	
	
	// affiche le frame buffer
	glFlush();
	glutSwapBuffers();
}