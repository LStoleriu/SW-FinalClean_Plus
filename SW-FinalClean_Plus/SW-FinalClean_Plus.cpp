// SW-FinalClean_Plus.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <vector>
#include <queue>
#include <algorithm>
#include <numeric>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_roots.h>

#define min(a,b) ( (a) < (b) ? (a) : (b) )

//#define SP4
#define TITAN

//#define ONE_SWITCH
//#undef ONE_SWITCH

#define MHL
//#undef MHL

//#define FORC
//#undef FORC

#define npart 1

#define nstep 500

using namespace std;

const int n_max_vec = 100;

const double Pi = 3.1415926535897932384626433832795;
const double Pis2 = 1.5707963267948966192313216916398;
const double Pix2 = 6.283185307179586476925286766559;

const double uround = 1.0e-8;

struct SW_function_params
{
public: double Hk_fcn, H_fcn, theta0_fcn;
};
struct sAnizo
{
public: double ax, ay, az;
};
struct sReadData
{
public: double x, y, z, theta_ea, phi_ea, volume, k, Msat, theta_sol, phi_sol;
};
struct sCoef
{
public: int vecin;
		double C, coef, xx, xy, xz, yx, yy, yz, zx, zy, zz;
};
struct  Camp
{
public: double Hampl, theta, phi, Hx, Hy, Hz;
};
struct Moment
{
public: double M, theta, phi, Mx, My, Mz;
};

typedef vector<double> dbls;
dbls ugs_ini, ugs_fin, tLLG_mx, tLLG_mn;
dbls ugs_ini_RAW, ugs_fin_RAW, tLLG_mx_RAW, tLLG_mn_RAW;

struct Camp H_de_t(double t);
void position_coeficients(struct sReadData D1, struct sReadData D2, struct sCoef *Pos_Coef, double *dist);
void function_neighbours(void);
void anisotropy_coef(void);
void SW_angle_single_full(int part, double *sol, struct sReadData Medium_single, struct Camp H);
void Convert_sol_to_loco(const double *sol, double *loco_angle);
void Convert_loco_to_sol(const double *loco_angle, double *sol, struct Camp H);
double SW_fcn(double x, void *params);
void Add_interactions(int part, struct Camp *H);
double interpolare(double loco_angle_with_ea_old, double loco_angle_with_ea_target);
double interpolare_RAW(double loco_angle_with_ea_old, double loco_angle_with_ea_target);
double timp_w_speed(double t, double h, double *sol_old, double *sol_target, double *sol_new);
double timp_2D_3D(double t, double h, double *sol_old, double *sol_target, double *sol_new);
double stability_test(double solutie[], double solutie_old[]);

static struct sAnizo A[npart];
struct sReadData Medium[npart], Medium_single;
static struct Camp H[nstep], Hext;
static struct Moment /*M[nstep][npart],*/ Msys;
static double Hx_part[npart];
static double Hy_part[npart];
static double Hz_part[npart];

static int neighbours[npart];
static struct sCoef Position_Coef[npart][n_max_vec];

double Hk_fcn, H_fcn, theta0_fcn;

double y[2 * npart], y_target[2 * npart], y_old[2 * npart];

//******************************************************

const double alpha = 0.01;
const double miu0 = 4.0e-7*Pi;

const double Ms = 795774.7154594767; // 2.668e5; //
const double K1 = 1.0e5;

const double gamma = 2.210173e5;
const double time_norm = (1.0 + alpha * alpha) / gamma / Ms;

double theta_ea = 0.1 * Pi / 180.0;
double phi_ea = 0.1 * Pi / 180.0;

double theta_h = 10.0 * Pi / 180.0 + 1.0e-4;
double phi_h = 0.0 * Pi / 180.0 + 1.0e-4;

double theta_0 = 10.0 * Pi / 180.0 + 1.0e-4;
double phi_0 = 0.0 * Pi / 180.0 + 1.0e-4;

const double Hk = 2.0*fabs(K1) / miu0 / Ms / Ms;
double VolumTotal = 0.0;

const double Exch = 0.1;

const double SPEED = 300.0;

const double perioada = 20000.0; // picosec

//const double FieldMax = +1193662.0 / Ms;
//const double FieldMin = -1193662.0 / Ms;

const double FieldMax = +5.0 * Hk;
const double FieldMin = -5.0 * Hk;

// Js = 1T, K1 = 1e5 -> Hk = 200000 A/m

//const double FieldMax = +1193662.0 / Ms;	//+3580986.0 / Ms;   [ A/m  /  A/m ] -> 15000 Oe  -> 1.5 T
//const double FieldMin = -1193662.0 / Ms;	//-3580986.0 / Ms;

const double prag_vecini = 1.0e-4;
double interaction_multiplier = 1.0;

//******************************************************

int main()
{
	int i, j;
	FILE *fp;
	clock_t begin = clock();

	//////////////////////////////////////////////////////////////////////////	////////////////////////////////////////////////////////////////////////// OUTPUT

#ifdef SP4
	char save_file[500] = "D:\\Stoleriu\\C\\special\\3d\\res\\2018\\SW---LLG\\_ONE_SWITCH_\\MHL\\SW_time_Js1-K1e5_t30_th10_timp2D.dat"; //SP4
	char save_file_FORC[500] = "D:\\Stoleriu\\C\\special\\3d\\res\\2017\\SW---LLG\\SW-full\\SW_FORC.dat"; //SP4
#else
	char save_file[500] = "E:\\Stoleriu\\C\\special\\3d\\res\\2018\\SW---LLG\\NewRunsValidation\\MHL\\SW_time_Js1-K1e5_th10_T20000_H50Hk_t30-MHL.dat";
#endif

	fp = fopen(save_file, "w");
	fclose(fp);

	double	t = 0.0;

	//////////////////////////////////////////////////////////////////////////	////////////////////////////////////////////////////////////////////////// FIELD

	for (i = 0; i < nstep; i++)
	{
		H[i].Hampl = FieldMax - (FieldMax - FieldMin) * i / (nstep - 1);
		H[i].theta = theta_h;
		H[i].phi = phi_h;
		H[i].Hx = H[i].Hampl * sin(H[i].theta) * cos(H[i].phi);
		H[i].Hy = H[i].Hampl * sin(H[i].theta) * sin(H[i].phi);
		H[i].Hz = H[i].Hampl * cos(H[i].theta);
	}

	//////////////////////////////////////////////////////////////////////////	////////////////////////////////////////////////////////////////////////// INPUT

#ifdef SP4
	fp = fopen("D:\\Stoleriu\\C\\special\\3d\\res\\2017\\SW---LLG\\SW-full\\200x200x15_2000_singlelayer_mixed_xz.dat", "r");
#else
	fp = fopen("E:\\Stoleriu\\C\\special\\3d\\res\\2017\\SW---LLG\\SW-full\\300x300x15_10000_singlelayer_disk_orient_xz.dat", "r");
#endif
	for (i = 0; i < npart; i++)
	{
		//fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n", &Medium[i].x, &Medium[i].y, &Medium[i].z, &Medium[i].theta_ea, &Medium[i].phi_ea, &Medium[i].volume, &Medium[i].k, &Medium[i].theta_sol, &Medium[i].phi_sol, &dummy);
		Medium[i].x = 0.0;
		Medium[i].y = 0.0;
		Medium[i].z = 0.0;
		Medium[i].volume = 1.0;

		VolumTotal += Medium[i].volume;
		Medium[i].Msat = Ms;

		Medium[i].k = 1.0;
		Medium[i].theta_ea = theta_ea;
		Medium[i].phi_ea = phi_ea;
		Medium[i].theta_sol = theta_0;
		Medium[i].phi_sol = phi_0;
	}
	fclose(fp);

	//////////////////////////////////////////////////////////////////////////	////////////////////////////////////////////////////////////////////////// TIMPI

	double ugs_ini_tmp, ugs_fin_tmp, tLLG_tmp_mn, tLLG_tmp_mx;

#ifdef SP4
	fp = fopen("D:\\Stoleriu\\C\\special\\3d\\res\\2018\\SW---LLG\\_ONE_SWITCH_\\LLG_time_polar_full_0.01_AllAnglesK_360.dat", "r");
#else
	fp = fopen("E:\\Stoleriu\\C\\special\\3d\\res\\2018\\SW---LLG\\_ONE_SWITCH_\\LLG_time_polar_full_0.01_AllAnglesK_360.dat", "r");
#endif
	for (double ug0 = 0; ug0 < 180; ug0++)	// 180 de unghiuri START
	{
		for (double ug1 = -180; ug1 < 180; ug1++)	// 360 de unghiuri TARGET pentru fiecare unghi START
		{
			//fscanf(fp, "%lf %lf %lf %lf\n", &ugs_ini_tmp, &ugs_fin_tmp, &tLLG_tmp_mn, &tLLG_tmp_mx);
			fscanf(fp, "%lf %lf %lf\n", &ugs_ini_tmp, &ugs_fin_tmp, &tLLG_tmp_mx);
			ugs_ini.push_back(ugs_ini_tmp);
			ugs_fin.push_back(ugs_fin_tmp);
			//tLLG_mn.push_back(tLLG_tmp_mn);
			tLLG_mx.push_back(tLLG_tmp_mx);
		}
	}
	for (int ug0 = 180 * 360 - 360; ug0 < 180 * 360; ug0++)
	{
		ugs_ini[ug0] = Pi + 1.0e-7;
	}
	fclose(fp);


#ifdef SP4
	fp = fopen("D:\\Stoleriu\\C\\special\\3d\\res\\2018\\SW---LLG\\_ONE_SWITCH_\\LLG_time_polar_full_0.01_AllAnglesK_360.dat", "r");
#else
	fp = fopen("E:\\Stoleriu\\C\\special\\3d\\res\\2018\\SW---LLG\\_ONE_SWITCH_\\LLG_time_polar_full_0.01_AllAnglesK_360_RAW.dat", "r");
#endif
	for (double ug0 = 0; ug0 < 180; ug0 += 2.5)	// 180 de unghiuri START
	{
		for (double ug1 = -180; ug1 < 180; ug1 += 2.5)	// 360 de unghiuri TARGET pentru fiecare unghi START
		{
			//fscanf(fp, "%lf %lf %lf %lf\n", &ugs_ini_tmp, &ugs_fin_tmp, &tLLG_tmp_mn, &tLLG_tmp_mx);
			fscanf(fp, "%lf %lf %lf\n", &ugs_ini_tmp, &ugs_fin_tmp, &tLLG_tmp_mx);
			ugs_ini_RAW.push_back(ugs_ini_tmp);
			ugs_fin_RAW.push_back(ugs_fin_tmp);
			//tLLG_mn_RAW.push_back(tLLG_tmp_mn);
			tLLG_mx_RAW.push_back(tLLG_tmp_mx);
		}
	}
	for (int ug0 = 72 * 144 - 144/*180 * 360 - 360*/; ug0 < 72 * 144/*180 * 360*/; ug0++)
	{
		ugs_ini_RAW[ug0] = Pi + 1.0e-7;
	}
	fclose(fp);

	//////////////////////////////////////////////////////////////////////////	////////////////////////////////////////////////////////////////////////// END

	anisotropy_coef();
	function_neighbours();

	//////////////////////////////////////////////////////////////////////////
	// ONE SWITCH
	//////////////////////////////////////////////////////////////////////////
#ifdef ONE_SWITCH

	fp = fopen(save_file, "w");
	fclose(fp);

	//////////////////////////////////////////////////////////////////////////	////////////////////////////////////////////////////////////////////////// INIT
	for (i = 0; i < npart; i++)
	{
		Medium[i].theta_sol = 1.0e-4;
		Medium[i].phi_sol = 1.0e-4;
		y[2 * i + 0] = Medium[i].theta_sol;
		y[2 * i + 1] = Medium[i].phi_sol;
		y_old[2 * i + 0] = y[2 * i + 0];
		y_old[2 * i + 1] = y[2 * i + 1];
	}

	//////////////////////////////////////////////////////////////////////////	////////////////////////////////////////////////////////////////////////// 

	double MHL_projection;
	fp = fopen(save_file, "w");


	//////////////////////////////////////////////////////////////////////////	////////////////////////////////////////////////////////////////////////// SWITCH
	//for (i = 0; i < nstep; i++)
	{
		i = 0;
		H[i].Hampl = -2.0*Hk;

		H[i].theta = theta_h;
		H[i].phi = phi_h;
		H[i].Hx = H[i].Hampl * sin(H[i].theta) * cos(H[i].phi);
		H[i].Hy = H[i].Hampl * sin(H[i].theta) * sin(H[i].phi);
		H[i].Hz = H[i].Hampl * cos(H[i].theta);

		t = 30.1372;

		Hext = H[i];

		for (j = 0; j < npart; j++)
		{
			y[2 * j + 0] = Medium[j].theta_sol;
			y[2 * j + 1] = Medium[j].phi_sol;
			y_old[2 * j + 0] = y[2 * j + 0];
			y_old[2 * j + 1] = y[2 * j + 1];
		}

		for (j = 0; j < npart; j++)
		{
			Medium_single = Medium[j];
			SW_angle_single_full(j, &y[2 * j + 0], Medium_single, Hext);
		}
		/////////////////////////////////////////////////////////////
		for (j = 0; j < npart; j++) {
			y_target[2 * j + 0] = y[2 * j + 0];
			y_target[2 * j + 1] = y[2 * j + 1];
			y[2 * j + 0] = y_old[2 * j + 0];
			y[2 * j + 1] = y_old[2 * j + 1];
		}

		int numLLG = 0;
		double torq_dif = 1.0;

		while (((torq_dif = stability_test(y, y_old)) > 1.0e-4) || (numLLG < 100))
		{
			printf("%d  :   %lf\n", numLLG, torq_dif);
			numLLG++;

			//timp_w_speed(t, Hext.Hampl, y_old, y_target, y);
			//timp_2D(t, Hext.Hampl, y_old, y_target, y);
			timp_2D_3D(t, Hext.Hampl, y_old, y_target, y);
			///////////////////////////////////////////////////////////////

			Msys.Mx = 0.0; Msys.My = 0.0; Msys.Mz = 0.0;
			for (j = 0; j < npart; j++)
			{
				Medium[j].theta_sol = y[2 * j + 0];
				Medium[j].phi_sol = y[2 * j + 1];
				Msys.Mx += Medium[j].volume * sin(y[2 * j + 0])*cos(y[2 * j + 1]);
				Msys.My += Medium[j].volume * sin(y[2 * j + 0])*sin(y[2 * j + 1]);
				Msys.Mz += Medium[j].volume * cos(y[2 * j + 0]);
			}

			Msys.Mx /= VolumTotal;
			Msys.My /= VolumTotal;
			Msys.Mz /= VolumTotal;

			MHL_projection = (Msys.Mx*sin(H[i].theta)*cos(H[i].phi) + Msys.My*sin(H[i].theta)*sin(H[i].phi) + Msys.Mz*cos(H[i].theta));

			for (j = 0; j < npart; j++)
			{
				y_old[2 * j + 0] = y[2 * j + 0];
				y_old[2 * j + 1] = y[2 * j + 1];
			}

			fprintf(fp, "%20.16lf %20.16lf %20.16lf %20.16lf %20.16lg %20.16lg %20.16lg %20.16lf %20.16lf\n", numLLG*t, H[i].Hx, H[i].Hy, H[i].Hz, Msys.Mx, Msys.My, Msys.Mz, (Hext.Hampl*miu0*Ms), MHL_projection);
		}

		printf("[%3d] : %07.4lf -> %+7.4lf %+7.4lf \n", i, Hext.Hampl, y[0], MHL_projection);
	}

	fclose(fp);
#endif

	//////////////////////////////////////////////////////////////////////////
	// MHL
	//////////////////////////////////////////////////////////////////////////
#ifdef MHL

	fp = fopen(save_file, "w");
	fclose(fp);

	//////////////////////////////////////////////////////////////////////////	////////////////////////////////////////////////////////////////////////// SATURARE
	for (i = 0; i < npart; i++)
	{
		Medium[i].theta_sol = H[0].theta;
		Medium[i].phi_sol = H[0].phi;
		y[2 * i + 0] = Medium[i].theta_sol;
		y[2 * i + 1] = Medium[i].phi_sol;
		y_old[2 * i + 0] = y[2 * i + 0];
		y_old[2 * i + 1] = y[2 * i + 1];
	}
	Hext = H_de_t(0); // H[0];
	for (j = 0; j < npart; j++)
	{
		Medium_single = Medium[j];
		SW_angle_single_full(j, &y[2 * j + 0], Medium_single, Hext);
	}
	for (j = 0; j < npart; j++) {
		Medium[i].theta_sol = y[2 * j + 0];
		Medium[i].phi_sol = y[2 * j + 1];
		y_old[2 * j + 0] = y[2 * j + 0];
		y_old[2 * j + 1] = y[2 * j + 1];
	}
	//////////////////////////////////////////////////////////////////////////	////////////////////////////////////////////////////////////////////////// END SATURARE

	double MHL_projection;
	fp = fopen(save_file, "w");

	t = 31.0;
	i = 0;
	//////////////////////////////////////////////////////////////////////////	////////////////////////////////////////////////////////////////////////// MHL DOWN
	//for (i = 0; i < nstep; i++)
	while (i*t < 2.0*perioada)
	{
		Hext = H_de_t(t*i); // H[i];

		for (j = 0; j < npart; j++)
		{
			y[2 * j + 0] = Medium[j].theta_sol;
			y[2 * j + 1] = Medium[j].phi_sol;
			y_old[2 * j + 0] = y[2 * j + 0];
			y_old[2 * j + 1] = y[2 * j + 1];
		}

		for (j = 0; j < npart; j++)
		{
			Medium_single = Medium[j];
			SW_angle_single_full(j, &y[2 * j + 0], Medium_single, Hext);
		}

		/////////////////////////////////////////////////////////////
		for (j = 0; j < npart; j++) {
			y_target[2 * j + 0] = y[2 * j + 0];
			y_target[2 * j + 1] = y[2 * j + 1];
		}

		//timp_w_speed(t, Hext.Hampl, y_old, y_target, y);
		//timp_2D(t, Hext.Hampl, y_old, y_target, y);
		timp_2D_3D(t, Hext.Hampl, y_old, y_target, y);
		///////////////////////////////////////////////////////////////

		Msys.Mx = 0.0; Msys.My = 0.0; Msys.Mz = 0.0;
		for (j = 0; j < npart; j++)
		{
			Medium[j].theta_sol = y[2 * j + 0];
			Medium[j].phi_sol = y[2 * j + 1];
			Msys.Mx += Medium[j].volume * sin(y[2 * j + 0])*cos(y[2 * j + 1]);
			Msys.My += Medium[j].volume * sin(y[2 * j + 0])*sin(y[2 * j + 1]);
			Msys.Mz += Medium[j].volume * cos(y[2 * j + 0]);
		}

		Msys.Mx /= VolumTotal;
		Msys.My /= VolumTotal;
		Msys.Mz /= VolumTotal;

		MHL_projection = (Msys.Mx*sin(Hext.theta)*cos(Hext.phi) + Msys.My*sin(Hext.theta)*sin(Hext.phi) + Msys.Mz*cos(Hext.theta));

		fprintf(fp, "%20.16lf %20.16lf %20.16lf %20.16lf %20.16lg %20.16lg %20.16lg %20.16lf %20.16lf\n", i*t, Hext.Hx, Hext.Hy, Hext.Hz, Msys.Mx, Msys.My, Msys.Mz, (Hext.Hampl*miu0*Ms), MHL_projection);

		printf("[%3d] : %07.4lf -> %+7.4lf %+7.4lf \n", i, Hext.Hampl, y[0], MHL_projection);
		i++;
	}

	// 	//////////////////////////////////////////////////////////////////////////	////////////////////////////////////////////////////////////////////////// MHL UP
	// 	for (i = nstep - 1; i > 0; i--)
	// 	{
	// 		Hext = H_de_t( t * (nstep-1) + t * (nstep-i) ); // H[i];
	// 
	// 		for (j = 0; j < npart; j++)
	// 		{
	// 			y[2 * j + 0] = Medium[j].theta_sol;
	// 			y[2 * j + 1] = Medium[j].phi_sol;
	// 			y_old[2 * j + 0] = y[2 * j + 0];
	// 			y_old[2 * j + 1] = y[2 * j + 1];
	// 		}
	// 
	// 		for (j = 0; j < npart; j++)
	// 		{
	// 			Medium_single = Medium[j];
	// 			SW_angle_single_full(j, &y[2 * j + 0], Medium_single, Hext);
	// 		}
	// 
	// 		/////////////////////////////////////////////////////////////
	// 		for (j = 0; j < npart; j++) {
	// 			y_target[2 * j + 0] = y[2 * j + 0];
	// 			y_target[2 * j + 1] = y[2 * j + 1];
	// 		}
	// 
	// 		//timp_w_speed(t, Hext.Hampl, y_old, y_target, y);
	// 		//timp_2D(t, Hext.Hampl, y_old, y_target, y);
	// 		timp_2D_3D(t, Hext.Hampl, y_old, y_target, y);
	// 		/////////////////////////////////////////////////////////////
	// 
	// 
	// 		Msys.Mx = 0.0; Msys.My = 0.0; Msys.Mz = 0.0;
	// 		for (j = 0; j < npart; j++)
	// 		{
	// 			Medium[j].theta_sol = y[2 * j + 0];
	// 			Medium[j].phi_sol = y[2 * j + 1];
	// 			Msys.Mx += Medium[j].volume * sin(y[2 * j + 0])*cos(y[2 * j + 1]);
	// 			Msys.My += Medium[j].volume * sin(y[2 * j + 0])*sin(y[2 * j + 1]);
	// 			Msys.Mz += Medium[j].volume * cos(y[2 * j + 0]);
	// 		}
	// 
	// 		Msys.Mx /= VolumTotal;
	// 		Msys.My /= VolumTotal;
	// 		Msys.Mz /= VolumTotal;
	// 
	// 		MHL_projection = (Msys.Mx*sin(H[i].theta)*cos(H[i].phi) + Msys.My*sin(H[i].theta)*sin(H[i].phi) + Msys.Mz*cos(H[i].theta));
	// 
	// 		fprintf(fp, "%20.16lf %20.16lf %20.16lf %20.16lf %20.16lf %20.16lf %20.16lf %20.16lf %20.16lf\n", (nstep + (nstep - i))*t, Hext.Hx, Hext.Hy, Hext.Hz, Msys.Mx, Msys.My, Msys.Mz, (Hext.Hampl*miu0*Ms), MHL_projection);
	// 
	// 		printf("%07.4lf -> %+7.4lf %+7.4lf \n", Hext.Hampl, y[0], MHL_projection);
	// 	}
	// 
	// 	fclose(fp);
#endif


	clock_t end = clock();
	double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("****************************************************\n");
	printf("ran %lf\n", time_spent);
	printf("****************************************************\n");
	return 0;
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
//**************************************************************************
//**************************************************************************
//**************************************************************************

struct Camp H_de_t(double t)
{
	struct Camp H;

	H.theta = theta_h;
	H.phi = phi_h;

	double ampl = FieldMax;

	H.Hampl = ampl * cos(Pix2 * t / perioada);

	H.Hx = H.Hampl * sin(H.theta) * cos(H.phi);
	H.Hy = H.Hampl * sin(H.theta) * sin(H.phi);
	H.Hz = H.Hampl * cos(H.theta);


	return H;
}

//**************************************************************************

void position_coeficients(struct sReadData D1, struct sReadData D2, sCoef *Pos_Coef, double *dist)
{
	*dist = sqrt((D2.x - D1.x)*(D2.x - D1.x) + (D2.y - D1.y)*(D2.y - D1.y) + (D2.z - D1.z)*(D2.z - D1.z));
	double r = (double)rand() / RAND_MAX;

	if (*dist == 0)
	{
		Pos_Coef->C = r * 0.1;

		Pos_Coef->coef = 0.0;
		Pos_Coef->xx = 0.0;
		Pos_Coef->xy = 0.0;
		Pos_Coef->xz = 0.0;
		Pos_Coef->yx = 0.0;
		Pos_Coef->yy = 0.0;
		Pos_Coef->yz = 0.0;
		Pos_Coef->zx = 0.0;
		Pos_Coef->zy = 0.0;
		Pos_Coef->zz = 0.0;
	}
	else
	{
		double rx = (D2.x - D1.x) / *dist;
		double ry = (D2.y - D1.y) / *dist;
		double rz = (D2.z - D1.z) / *dist;

		Pos_Coef->C = 0.0;

		Pos_Coef->coef = D2.volume * D2.Msat / 4.0 / Pi / *dist / *dist / *dist / Ms;

		Pos_Coef->xx = 3.0 * rx * rx - 1.0;
		Pos_Coef->xy = 3.0 * rx * ry;
		Pos_Coef->xz = 3.0 * rx * rz;
		Pos_Coef->yx = Pos_Coef->xy;
		Pos_Coef->yy = 3.0 * ry * ry - 1.0;
		Pos_Coef->yz = 3.0 * ry * rz;
		Pos_Coef->zx = Pos_Coef->xz;
		Pos_Coef->zy = Pos_Coef->yz;
		Pos_Coef->zz = 3.0 * rz * rz - 1.0;
	}
}

//**************************************************************************

void function_neighbours(void)
{
	sReadData Data1, Data2;
	sCoef Pos_Coef;
	double distance;
	int neighbours_max = 0, neighbours_med = 0;
	int f;

	for (int i = 0; i < npart; i++)
	{
		neighbours[i] = 0;
		Data1 = Medium[i];

		for (f = 0; f < npart; f++)
		{
			Data2 = Medium[f];

			if ((i != f))
			{
				position_coeficients(Data1, Data2, &Pos_Coef, &distance);
				// 				if (distance < 3.0e-7)
				// 				{
				// 					if (neighbours[i] + 1 > n_max_vec - 1)
				// 					{
				// 						printf("\n\n PREA MULTI VECINI !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n\n");
				// 						getchar();
				// 						break;
				// 					}
				// 					else
				// 					{
				// 						neighbours[i]++;
				// 						Pos_Coef.vecin = f;  //<<<---- asta e vecin
				// 						Position_Coef[i][neighbours[i] - 1] = Pos_Coef;
				// 					}
				// 					continue; // din acelasi cub
				// 				}

				if (Pos_Coef.coef > prag_vecini)
				{
					if (neighbours[i] + 1 > n_max_vec - 1)
					{
						printf("\n\n PREA MULTI VECINI !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n\n");
						getchar();
						break;
					}
					else
					{
						neighbours[i]++;
						Pos_Coef.vecin = f;  //<<<---- asta e vecin
						Position_Coef[i][neighbours[i] - 1] = Pos_Coef;
					}
				}
			}
		}
		printf("particula: %d  cu  %d  vecini \n", i, neighbours[i]);
		neighbours_med += neighbours[i];
		if (neighbours[i] > neighbours_max) neighbours_max = neighbours[i];
	}
	printf("Numar maxim de vecini: %d      Numar mediu de vecini: %f \n", neighbours_max, (float)neighbours_med / npart);
}

//**************************************************************************

void anisotropy_coef(void)
{
	for (int i = 0; i < npart; i++)
	{
		A[i].ax = sin(Medium[i].theta_ea)*cos(Medium[i].phi_ea);
		A[i].ay = sin(Medium[i].theta_ea)*sin(Medium[i].phi_ea);
		A[i].az = cos(Medium[i].theta_ea);
	}
}

//**************************************************************************

void SW_angle_single_full(int part, double *sol, struct sReadData Medium_single, struct Camp H)
{
	static double loco_angle_M, loco_old_angle_M;
	static double loco_angle_H;
	static double Hc;
	int n_found = 0, state = 1;
	double g, temp_angle_H, temp_Hea, temp_Hha;
	double hx, hy, hz;
	double mx, my, mz;

	int status, n_sols, ram = 0;
	int iter = 0, max_iter = 100;

	double *sols, *limits;

	const gsl_root_fsolver_type *T;
	gsl_root_fsolver *s;
	double r = 0;
	double x_lo, x_hi;
	gsl_function F;

	Add_interactions(part, &H);

	hx = sin(H.theta)*cos(H.phi);
	hy = sin(H.theta)*sin(H.phi);
	hz = cos(H.theta);

	//	Convert_sol_to_loco(sol, loco_angle_M);
	{
		mx = sin(sol[0])*cos(sol[1]);
		my = sin(sol[0])*sin(sol[1]);
		mz = cos(sol[0]);
		loco_angle_M = acos(A[part].ax * mx + A[part].ay * my + A[part].az * mz);
	}

	loco_angle_H = acos(A[part].ax*hx + A[part].ay*hy + A[part].az*hz);
	double its_a_sin = sin(loco_angle_H);
	double its_a_cos = cos(loco_angle_H);
	double aux1 = pow(sin(loco_angle_H)*sin(loco_angle_H), (1.0 / 3.0));
	double aux2 = pow(cos(loco_angle_H)*cos(loco_angle_H), (1.0 / 3.0));
	g = pow((pow(sin(loco_angle_H)*sin(loco_angle_H), (1.0 / 3.0)) + pow(cos(loco_angle_H)*cos(loco_angle_H), (1.0 / 3.0))), -1.5);
	Hc = Medium_single.k * Hk * g;

	//parametrii functiei de rezolvat (in coordonate loco)
	Hk_fcn = Medium_single.k * Hk;
	H_fcn = H.Hampl;
	theta0_fcn = loco_angle_H;
	struct SW_function_params params = { Medium_single.k * Hk, H.Hampl, loco_angle_H };
	////////////////////////////////////////////////////
	F.function = &SW_fcn;
	F.params = &params;
	T = gsl_root_fsolver_brent;
	/////////////////////////////////////////////////////////

	loco_old_angle_M = acos(A[part].ax*sin(sol[0])*cos(sol[1]) + A[part].ay*sin(sol[0])*sin(sol[1]) + A[part].az*cos(sol[0]));
	if (loco_old_angle_M < Pis2)
	{
		state = 1;
	}
	else
	{
		state = -1;
	}

	if (H.Hampl == 0.0)
	{
		sols = (double *)calloc(1, sizeof(double));
		limits = (double *)calloc(1, sizeof(double));
		if (state > 0)
		{
			loco_angle_M = 0.0;
		}
		else
		{
			loco_angle_M = Pi;
		}
	}
	else
	{
		temp_Hea = H.Hampl * cos(loco_angle_H);
		temp_Hha = H.Hampl * sin(loco_angle_H);
		if ((temp_Hea >= 0.0) && (temp_Hha >= 0.0))
		{
			temp_angle_H = Pi + atan(-pow(tan(loco_angle_H)*tan(loco_angle_H), (1.0 / 6.0)));	//C1
		}
		if ((temp_Hea >= 0.0) && (temp_Hha < 0.0))
		{
			temp_angle_H = Pi + atan(pow(tan(loco_angle_H)*tan(loco_angle_H), (1.0 / 6.0)));	//C4
		}
		if ((temp_Hea < 0.0) && (temp_Hha < 0.0))
		{
			temp_angle_H = Pix2 + atan(-pow(tan(loco_angle_H)*tan(loco_angle_H), (1.0 / 6.0)));	//C3
		}
		if ((temp_Hea < 0.0) && (temp_Hha >= 0.0))
		{
			temp_angle_H = atan(pow(tan(loco_angle_H)*tan(loco_angle_H), (1.0 / 6.0)));	//C2
		}

		if (H.Hampl >= Hc)
			n_sols = 2;
		else
			n_sols = 4;

		sols = (double *)calloc(n_sols, sizeof(double));
		limits = (double *)calloc(2 * n_sols, sizeof(double));

		//printf("%d: temp_angle: %lf      (Hea, Hha):  (%lf, %lf)\n", n_sols, temp_angle_H, temp_Hea, temp_Hha);

		if (fabs(H.Hampl) >= Hc)
		{
			if ((temp_Hea >= 0.0) && (temp_Hha >= 0.0))
			{
				limits[0] = 0.0; limits[1] = Pis2; limits[2] = Pi; limits[3] = 3.0*Pis2;	//C1
			}
			if ((temp_Hea >= 0.0) && (temp_Hha < 0.0))
			{
				limits[0] = Pis2; limits[1] = Pi; limits[2] = 3.0*Pis2; limits[3] = Pix2;	//C4
			}
			if ((temp_Hea < 0.0) && (temp_Hha < 0.0))
			{
				limits[0] = 0.0; limits[1] = Pis2; limits[2] = Pi; limits[3] = 3.0*Pis2;	//C3
			}
			if ((temp_Hea < 0.0) && (temp_Hha >= 0.0))
			{
				limits[0] = Pis2; limits[1] = Pi; limits[2] = 3.0*Pis2; limits[3] = Pix2;	//C2
			}
		}
		else
		{
			if ((temp_Hea >= 0.0) && (temp_Hha >= 0.0))
			{
				limits[0] = 0.0; limits[1] = Pis2; limits[2] = Pi; limits[3] = 3.0*Pis2; limits[4] = Pis2; limits[5] = temp_angle_H; limits[6] = temp_angle_H; limits[7] = Pi;
			}
			if ((temp_Hea >= 0.0) && (temp_Hha < 0.0))
			{
				limits[0] = Pis2; limits[1] = Pi; limits[2] = 3.0*Pis2; limits[3] = Pix2; limits[4] = Pi; limits[5] = temp_angle_H; limits[6] = temp_angle_H; limits[7] = 3.0*Pis2;
			}
			if ((temp_Hea < 0.0) && (temp_Hha < 0.0))
			{
				limits[0] = 0.0; limits[1] = Pis2; limits[2] = Pi; limits[3] = 3.0*Pis2; limits[4] = 3.0*Pis2; limits[5] = temp_angle_H; limits[6] = temp_angle_H; limits[7] = Pix2;
			}
			if ((temp_Hea < 0.0) && (temp_Hha >= 0.0))
			{
				limits[0] = Pis2; limits[1] = Pi; limits[2] = 3.0*Pis2; limits[3] = Pix2; limits[4] = 0.0; limits[5] = temp_angle_H; limits[6] = temp_angle_H; limits[7] = Pis2;
			}
		}

		for (int scontor = 0; scontor < n_sols; scontor++)
		{
			s = gsl_root_fsolver_alloc(T);
			x_lo = limits[2 * scontor];
			x_hi = limits[2 * scontor + 1];
			gsl_root_fsolver_set(s, &F, x_lo, x_hi);
			do
			{
				iter++;
				status = gsl_root_fsolver_iterate(s);
				r = gsl_root_fsolver_root(s);
				x_lo = gsl_root_fsolver_x_lower(s);
				x_hi = gsl_root_fsolver_x_upper(s);
				status = gsl_root_test_interval(x_lo, x_hi, 0, 1.0e-7);
			} while (status == GSL_CONTINUE && iter < max_iter);
			sols[scontor] = r;
			gsl_root_fsolver_free(s);
		}

		if (H.Hampl >= Hc)
		{
			for (int scontor = 0; scontor < n_sols; scontor++)
			{
				if ((Hk_fcn * cos(2.0*sols[scontor]) + H_fcn * cos(sols[scontor] - theta0_fcn)) > 0)
					loco_angle_M = sols[scontor];
			}
		}
		else
		{
			int contor_sol_bune = 0;
			double sol_bune[2];
			for (int scontor = 0; scontor < n_sols; scontor++)
			{
				if ((Hk_fcn * cos(2.0*sols[scontor]) + H_fcn * cos(sols[scontor] - theta0_fcn)) > 0)
				{
					sol_bune[contor_sol_bune] = sols[scontor];
					contor_sol_bune++;
				}
			}

			if (contor_sol_bune != 2)
			{
				printf("SOMETHING FISHY!\n");
			}

			if (fabs(loco_old_angle_M - sol_bune[0]) > fabs(loco_old_angle_M - sol_bune[1]))
				loco_angle_M = sol_bune[1];
			else
				loco_angle_M = sol_bune[0];
		}
	}

	//	Convert_loco_to_sol(loco_angle_M, sol, H);
	{
		double ug_ea_H, temp_ea, temp_H;
		hx = sin(H.theta)*cos(H.phi);
		hy = sin(H.theta)*sin(H.phi);
		hz = cos(H.theta);
		if (H.Hampl < 0.0)
		{
			hx *= (-1.0);
			hy *= (-1.0);
			hz *= (-1.0);
		}

		ug_ea_H = acos(A[part].ax*hx + A[part].ay*hy + A[part].az*hz);

		if (loco_angle_M <= Pi)
		{
			temp_ea = sin(ug_ea_H - loco_angle_M) / sin(ug_ea_H);
			temp_H = sin(loco_angle_M) / sin(ug_ea_H);
		}
		else
		{
			temp_ea = sin(ug_ea_H - (Pix2 - loco_angle_M)) / sin(ug_ea_H);
			temp_H = sin(Pix2 - loco_angle_M) / sin(ug_ea_H);
		}

		mx = (temp_ea*A[part].ax + temp_H * hx);
		my = (temp_ea*A[part].ay + temp_H * hy);
		mz = (temp_ea*A[part].az + temp_H * hz);

		sol[0] = acos(mz);

		if ((mx == 0.0) && (my == 0.0))
		{
			sol[1] = 0.0;
		}
		else
		{
			sol[1] = acos(mx / sqrt(mx*mx + my * my));
			if (my < 0)
			{
				sol[1] = Pix2 - sol[1];
			}
		}
	}
	free(sols);
	free(limits);
}

//**************************************************************************

void Convert_sol_to_loco(const double *sol, double *loco_angle)
{
	double mx, my, mz;

	for (int i = 0; i < npart; i++)
	{
		mx = sin(sol[2 * i + 0])*cos(sol[2 * i + 1]);
		my = sin(sol[2 * i + 0])*sin(sol[2 * i + 1]);
		mz = cos(sol[2 * i + 0]);
		loco_angle[i] = acos(A[i].ax * mx + A[i].ay * my + A[i].az * mz);
	}
}

//**************************************************************************

void Convert_loco_to_sol(const double *loco_angle, double *sol, struct Camp H)
{
	double hx, hy, hz, mx, my, mz, ug_ea_H, temp_ea, temp_H;
	hx = sin(H.theta)*cos(H.phi);
	hy = sin(H.theta)*sin(H.phi);
	hz = cos(H.theta);
	if (H.Hampl < 0.0)
	{
		hx *= (-1.0);
		hy *= (-1.0);
		hz *= (-1.0);
	}

	for (int i = 0; i < npart; i++)
	{
		ug_ea_H = acos(A[i].ax*hx + A[i].ay*hy + A[i].az*hz);

		if (loco_angle[i] <= Pi)
		{
			temp_ea = sin(ug_ea_H - loco_angle[i]) / sin(ug_ea_H);
			temp_H = sin(loco_angle[i]) / sin(ug_ea_H);
		}
		else
		{
			temp_ea = sin(ug_ea_H - (Pix2 - loco_angle[i])) / sin(ug_ea_H);
			temp_H = sin(Pix2 - loco_angle[i]) / sin(ug_ea_H);
		}

		mx = (temp_ea*A[i].ax + temp_H * hx);
		my = (temp_ea*A[i].ay + temp_H * hy);
		mz = (temp_ea*A[i].az + temp_H * hz);

		sol[2 * i + 0] = acos(mz);

		if ((mx == 0.0) && (my == 0.0))
		{
			sol[2 * i + 1] = 0.0;
		}
		else
		{
			sol[2 * i + 1] = acos(mx / sqrt(mx*mx + my * my));
			if (my < 0)
			{
				sol[2 * i + 1] = Pix2 - sol[2 * i + 1];
			}
		}
	}
}

//**************************************************************************

double SW_fcn(double x, void *params)
{
	struct SW_function_params *p = (struct SW_function_params *) params;

	double Hk_fcn = p->Hk_fcn;
	double H_fcn = p->H_fcn;
	double theta0_fcn = p->theta0_fcn;

	return Hk_fcn * cos(x)*sin(x) + H_fcn * sin(x - theta0_fcn);
}

//**************************************************************************

void Add_interactions(int part, struct Camp *H)
{
	int kk;
	double mp_x, mp_y, mp_z;

	for (int j = 0; j < neighbours[part]; j++)
	{
		kk = 2 * Position_Coef[part][j].vecin;
		mp_x = sin(y[kk])*cos(y[kk + 1]);
		mp_y = sin(y[kk])*sin(y[kk + 1]);
		mp_z = cos(y[kk]);

		H->Hx += interaction_multiplier * Position_Coef[part][j].coef * (Position_Coef[part][j].xx*mp_x + Position_Coef[part][j].xy*mp_y + Position_Coef[part][j].xz*mp_z);
		H->Hy += interaction_multiplier * Position_Coef[part][j].coef * (Position_Coef[part][j].yx*mp_x + Position_Coef[part][j].yy*mp_y + Position_Coef[part][j].yz*mp_z);
		H->Hz += interaction_multiplier * Position_Coef[part][j].coef * (Position_Coef[part][j].zx*mp_x + Position_Coef[part][j].zy*mp_y + Position_Coef[part][j].zz*mp_z);
	}

	H->Hampl = sqrt(H->Hx*H->Hx + H->Hy*H->Hy + H->Hz*H->Hz);

	H->theta = acos(H->Hz / H->Hampl);

	H->phi = atan2(H->Hy, H->Hx);
}

//**************************************************************************

double interpolare(double loco_angle_with_ea_old, double loco_angle_with_ea_target)
{
	double t_ugini_mic_ugfin_mic, t_ugini_mic_ugfin_mare, t_ugini_mare_ugfin_mic, t_ugini_mare_ugfin_mare;
	int i, j;

	for (i = 0; i < 180 * 360; i++)
	{
		if (ugs_ini[i] < loco_angle_with_ea_old)
		{
			continue;
		}

		for (j = 0; j < 360; j++)
		{
			if (ugs_fin[(i - 360) + j] < loco_angle_with_ea_target)
			{
				continue;
			}
			else
			{
				break;
			}
		}
		break;
	}

	//	printf("%lf  %lf  %lf\n", ugs_ini[i - 180 + j - 1], ugs_fin[i - 180 + j - 1], tLLG[i - 180 + j - 1]);
	//	printf("%lf  %lf  %lf\n", ugs_ini[i - 180 + j    ], ugs_fin[i - 180 + j    ], tLLG[i - 180 + j    ]);
	//	printf("%lf  %lf  %lf\n", ugs_ini[ i      + j - 1], ugs_fin[ i      + j - 1], tLLG[ i      + j - 1]);
	//	printf("%lf  %lf  %lf\n", ugs_ini[ i      + j    ], ugs_fin[ i      + j    ], tLLG[ i      + j    ]);

	if (i >= 180 * 360)
	{
		/*printf("AAAAAAAAAAAAAAAAAAAA!\n");*/
		i = 180 * 360 - 360;
	}
	if (j >= 360)
	{
		/*printf("BBBBBBBBBBBBBBBBBBBB!\n");*/
		j = 360 - 1;
	}

	t_ugini_mic_ugfin_mic = tLLG_mx[i - 360 + j - 1];
	t_ugini_mic_ugfin_mare = tLLG_mx[i - 360 + j];
	t_ugini_mare_ugfin_mic = tLLG_mx[i + j - 1];
	t_ugini_mare_ugfin_mare = tLLG_mx[i + j];

	double aux1 = t_ugini_mic_ugfin_mic + (t_ugini_mare_ugfin_mic - t_ugini_mic_ugfin_mic) * (loco_angle_with_ea_old - ugs_ini[i - 360 + j - 1]) / (ugs_ini[i + j - 1] - ugs_ini[i - 360 + j - 1]);

	double aux2 = t_ugini_mic_ugfin_mare + (t_ugini_mare_ugfin_mare - t_ugini_mic_ugfin_mare) * (loco_angle_with_ea_old - ugs_ini[i - 360 + j]) / (ugs_ini[i + j] - ugs_ini[i - 360 + j]);

	double rez = aux1 + (aux2 - aux1) * (loco_angle_with_ea_target - ugs_fin[i + j - 1]) / (ugs_fin[i - 144/*360*/ + j] - ugs_fin[i + j - 1]);

	return(rez);
}

//**************************************************************************

double interpolare_RAW(double loco_angle_with_ea_old, double loco_angle_with_ea_target)
{
	double t_ugini_mic_ugfin_mic, t_ugini_mic_ugfin_mare, t_ugini_mare_ugfin_mic, t_ugini_mare_ugfin_mare;
	int i, j;

	for (i = 0; i < 72 * 144/*180 * 360*/; i++)
	{
		if (ugs_ini_RAW[i] < loco_angle_with_ea_old)
		{
			continue;
		}

		for (j = 1; j < 144/*360*/; j++)
		{
			if (ugs_fin_RAW[(i - 144/*360*/) + j] < loco_angle_with_ea_target)
			{
				continue;
			}
			else
			{
				break;
			}
		}
		break;
	}

	//	printf("%lf  %lf  %lf\n", ugs_ini[i - 180 + j - 1], ugs_fin[i - 180 + j - 1], tLLG[i - 180 + j - 1]);
	//	printf("%lf  %lf  %lf\n", ugs_ini[i - 180 + j    ], ugs_fin[i - 180 + j    ], tLLG[i - 180 + j    ]);
	//	printf("%lf  %lf  %lf\n", ugs_ini[ i      + j - 1], ugs_fin[ i      + j - 1], tLLG[ i      + j - 1]);
	//	printf("%lf  %lf  %lf\n", ugs_ini[ i      + j    ], ugs_fin[ i      + j    ], tLLG[ i      + j    ]);

	if (i >= 72 * 144/*180 * 360*/)
	{
		/*printf("AAAAAAAAAAAAAAAAAAAA!\n");*/
		i = 72 * 144 - 144/*180 * 360 - 360*/;
	}
	if (j >= 144/*360*/)
	{
		/*printf("BBBBBBBBBBBBBBBBBBBB!\n");*/
		j = 144 - 1/*360 - 1*/;
	}

	t_ugini_mic_ugfin_mic = tLLG_mx_RAW[i - 144/*360*/ + j - 1];
	t_ugini_mic_ugfin_mare = tLLG_mx_RAW[i - 144/*360*/ + j];
	t_ugini_mare_ugfin_mic = tLLG_mx_RAW[i + j - 1];
	t_ugini_mare_ugfin_mare = tLLG_mx_RAW[i + j];

	double aux1 = t_ugini_mic_ugfin_mic + (t_ugini_mare_ugfin_mic - t_ugini_mic_ugfin_mic) * (loco_angle_with_ea_old - ugs_ini_RAW[i - 144/*360*/ + j - 1]) / (ugs_ini_RAW[i + j - 1] - ugs_ini_RAW[i - 144/*360*/ + j - 1]);

	double aux2 = t_ugini_mic_ugfin_mare + (t_ugini_mare_ugfin_mare - t_ugini_mic_ugfin_mare) * (loco_angle_with_ea_old - ugs_ini_RAW[i - 144/*360*/ + j]) / (ugs_ini_RAW[i + j] - ugs_ini_RAW[i - 144/*360*/ + j]);

	double rez = aux1 + (aux2 - aux1) * (loco_angle_with_ea_target - ugs_fin_RAW[i + j - 1]) / (ugs_fin_RAW[i - 144/*360*/ + j] - ugs_fin_RAW[i + j - 1]);

	return(rez);
}

//**************************************************************************

double timp_w_speed(double t, double h, double *sol_old, double *sol_target, double *sol_new)
{
	double mx0, mx1, my0, my1, mz0, mz1, mxf, myf, mzf, modulus_help;
	double loco_angle_with_ea_old, loco_angle_with_ea_target, angle_target, angle_new;
	int i;

	double norm, k_norm_x, k_norm_y, k_norm_z, j_norm_x, j_norm_y, j_norm_z;
	double s0, c0, s1, c1;
	double term1, term2, term3, factor;
	double TORQx, TORQy, TORQz, TORQxx, TORQyy, TORQzz, TORQ;

	// h, modulul campului, trebuie sa fie pozitiv
	h = fabs(h);

	for (i = 0; i < npart; i++)
	{
		mx0 = sin(sol_old[2 * i + 0]) * cos(sol_old[2 * i + 1]);
		my0 = sin(sol_old[2 * i + 0]) * sin(sol_old[2 * i + 1]);
		mz0 = cos(sol_old[2 * i + 0]);

		mx1 = sin(sol_target[2 * i + 0]) * cos(sol_target[2 * i + 1]);
		my1 = sin(sol_target[2 * i + 0]) * sin(sol_target[2 * i + 1]);
		mz1 = cos(sol_target[2 * i + 0]);

		modulus_help = A[i].ax*mx0 + A[i].ay*my0 + A[i].az*mz0;
		if (modulus_help > 1.0) modulus_help = 1.0 - 1.0e-5;
		if (modulus_help < -1.0) modulus_help = -1.0 + 1.0e-5;
		loco_angle_with_ea_old = acos(modulus_help);				// unghi OLD fata de EA

		modulus_help = mx0 * mx1 + my0 * my1 + mz0 * mz1;
		if (modulus_help > 1.0) modulus_help = 1.0 - 1.0e-5;
		if (modulus_help < -1.0) modulus_help = -1.0 + 1.0e-5;
		angle_target = acos(modulus_help);							// angle_target - unghiul dintre OLD si TARGET

		modulus_help = A[i].ax*mx1 + A[i].ay*my1 + A[i].az*mz1;
		if (modulus_help > 1.0) modulus_help = 1.0 - 1.0e-5;
		if (modulus_help < -1.0) modulus_help = -1.0 + 1.0e-5;		// unghi TARGET fata de EA
		loco_angle_with_ea_target = acos(modulus_help);

		// determinare camp pentru MxMxH si directie de giratie

		double H_part;
		s0 = sin(sol_old[2 * i + 0]);
		c0 = cos(sol_old[2 * i + 0]);
		s1 = sin(sol_old[2 * i + 1]);
		c1 = cos(sol_old[2 * i + 1]);

		term1 = A[i].ax*s0*c1 + A[i].ay*s0*s1 + A[i].az*c0;
		term2 = -A[i].ax*s0*s1 + A[i].ay*s0*c1;
		term3 = A[i].ax*c0*c1 + A[i].ay*c0*s1 - A[i].az*s0;
		factor = 1.0 /*+ 2.0*K2*term1*term1 / K1 + 3.0*K3*term1*term1*term1*term1 / K1*/;

		Hx_part[i] = Hext.Hx + Medium[i].k * Hk * factor * (-term1 * term2*s1 / s0 + term1 * term3*c0*c1);
		Hy_part[i] = Hext.Hy + Medium[i].k * Hk * factor * (term1*term2*c1 / s0 + term1 * term3*c0*s1);
		Hz_part[i] = Hext.Hz + Medium[i].k * Hk * factor * (-term1 * term3*s0);

		H_part = sqrt(Hx_part[i] * Hx_part[i] + Hy_part[i] * Hy_part[i] + Hz_part[i] * Hz_part[i]);

		double T_Larmor = ((2.0*Pi) / (gamma * (H_part*Ms))*1.0e12);

		TORQx = (my0*Hz_part[i] - mz0 * Hy_part[i]);
		TORQy = -(mx0*Hz_part[i] - mz0 * Hx_part[i]);
		TORQz = (mx0*Hy_part[i] - my0 * Hx_part[i]);
		//TORQ = sqrt(TORQx * TORQx + TORQy * TORQy + TORQz * TORQz);

		TORQxx = (my0*TORQz - mz0 * TORQy);
		TORQyy = -(mx0*TORQz - mz0 * TORQx);
		TORQzz = (mx0*TORQy - my0 * TORQx);

		TORQ = sqrt(TORQxx * TORQxx + TORQyy * TORQyy + TORQzz * TORQzz);

		// se roteste pozitia veche a lui H astfel incat acesta sa fie dupa Oz si se afla noul M
		double R1[3][3], R2[3][3];

		double th_Hext = acos(Hext.Hz / fabs(Hext.Hampl));
		double unghi_intre_Hext_si_H;

		if (angle_target > Pis2)
			unghi_intre_Hext_si_H = th_Hext;
		else
			if (angle_target < 0.275)
				unghi_intre_Hext_si_H = sol_target[2 * i + 0];
			else
				unghi_intre_Hext_si_H = sol_target[2 * i + 0] - (angle_target - 0.275)*(sol_target[2 * i + 0] - th_Hext) / (Pis2 - 0.275);

		c0 = cos(-sol_target[2 * i + 1]); s0 = sin(-sol_target[2 * i + 1]);
		c1 = cos(-unghi_intre_Hext_si_H); s1 = sin(-unghi_intre_Hext_si_H);

		R1[0][0] = c0 * c1;  R1[0][1] = -s0 * c1;  R1[0][2] = s1;
		R1[1][0] = s0;       R1[1][1] = c0;       R1[1][2] = 0.0;
		R1[2][0] = -c0 * s1;  R1[2][1] = s0 * s1;  R1[2][2] = c1;

		R2[0][0] = R1[0][0]; R2[0][1] = R1[1][0]; R2[0][2] = R1[2][0];
		R2[1][0] = R1[0][1]; R2[1][1] = R1[1][1]; R2[1][2] = R1[2][1];
		R2[2][0] = R1[0][2]; R2[2][1] = R1[1][2]; R2[2][2] = R1[2][2];

		double mx0r = R1[0][0] * mx0 + R1[0][1] * my0 + R1[0][2] * mz0;
		double my0r = R1[1][0] * mx0 + R1[1][1] * my0 + R1[1][2] * mz0;
		double mz0r = R1[2][0] * mx0 + R1[2][1] * my0 + R1[2][2] * mz0;

		double th_m0r = acos(mz0r);
		double ph_m0r = atan2(my0r, mx0r);

		// se modifica th_m0r si ph_m0r conform regulilor

		angle_new = t * fabs(TORQ) / (SPEED * 0.01 / alpha);
		double th_m1r = th_m0r - angle_new;
		if (th_m1r < 0.0) th_m1r = 0.0;
		double ph_m1r = ph_m0r - t * Pi / T_Larmor;

		// se roteste totul inapoi rezultand noua solutie

		double mx1r = sin(th_m1r)*cos(ph_m1r);
		double my1r = sin(th_m1r)*sin(ph_m1r);
		double mz1r = cos(th_m1r);

		mxf = R2[0][0] * mx1r + R2[0][1] * my1r + R2[0][2] * mz1r;
		myf = R2[1][0] * mx1r + R2[1][1] * my1r + R2[1][2] * mz1r;
		mzf = R2[2][0] * mx1r + R2[2][1] * my1r + R2[2][2] * mz1r;


		sol_new[2 * i + 0] = acos(mzf);
		sol_new[2 * i + 1] = atan2(myf, mxf);
	}
	return TORQ;
}

//**************************************************************************

double timp_2D_3D(double t, double h, double *sol_old, double *sol_target, double *sol_new)
{
	double mx0, mx1, my0, my1, mz0, mz1, mxf, myf, mzf, modulus_help;
	double timp_max, loco_angle_with_ea_old, loco_angle_with_ea_target, angle_target, angle_new;
	int i;

	double norm, k_norm_x, k_norm_y, k_norm_z, j_norm_x, j_norm_y, j_norm_z;
	double s0, c0, s1, c1;
	double term1, term2, term3, factor;
	double TORQx, TORQy, TORQz, TORQxx, TORQyy, TORQzz, TORQ;

	// h, modulul campului, trebuie sa fie pozitiv
	h = fabs(h);

	for (i = 0; i < npart; i++)
	{
		mx0 = sin(sol_old[2 * i + 0]) * cos(sol_old[2 * i + 1]);
		my0 = sin(sol_old[2 * i + 0]) * sin(sol_old[2 * i + 1]);
		mz0 = cos(sol_old[2 * i + 0]);

		mx1 = sin(sol_target[2 * i + 0]) * cos(sol_target[2 * i + 1]);
		my1 = sin(sol_target[2 * i + 0]) * sin(sol_target[2 * i + 1]);
		mz1 = cos(sol_target[2 * i + 0]);

		modulus_help = A[i].ax*mx0 + A[i].ay*my0 + A[i].az*mz0;
		if (modulus_help > 1.0) modulus_help = 1.0 - 1.0e-5;
		if (modulus_help < -1.0) modulus_help = -1.0 + 1.0e-5;
		loco_angle_with_ea_old = acos(modulus_help);				// unghi OLD fata de EA

		modulus_help = mx0 * mx1 + my0 * my1 + mz0 * mz1;
		if (modulus_help > 1.0) modulus_help = 1.0 - 1.0e-5;
		if (modulus_help < -1.0) modulus_help = -1.0 + 1.0e-5;
		angle_target = acos(modulus_help);							// angle_target - unghiul dintre OLD si TARGET

		modulus_help = A[i].ax*mx1 + A[i].ay*my1 + A[i].az*mz1;
		if (modulus_help > 1.0) modulus_help = 1.0 - 1.0e-5;
		if (modulus_help < -1.0) modulus_help = -1.0 + 1.0e-5;		// unghi TARGET fata de EA
		loco_angle_with_ea_target = acos(modulus_help);

		// determinare camp pentru MxMxH si directie de giratie

		double H_part;
		s0 = sin(sol_old[2 * i + 0]);
		c0 = cos(sol_old[2 * i + 0]);
		s1 = sin(sol_old[2 * i + 1]);
		c1 = cos(sol_old[2 * i + 1]);

		term1 = A[i].ax*s0*c1 + A[i].ay*s0*s1 + A[i].az*c0;
		term2 = -A[i].ax*s0*s1 + A[i].ay*s0*c1;
		term3 = A[i].ax*c0*c1 + A[i].ay*c0*s1 - A[i].az*s0;
		factor = 1.0 /*+ 2.0*K2*term1*term1 / K1 + 3.0*K3*term1*term1*term1*term1 / K1*/;

		Hx_part[i] = Hext.Hx + Medium[i].k * Hk * factor * (-term1 * term2*s1 / s0 + term1 * term3*c0*c1);
		Hy_part[i] = Hext.Hy + Medium[i].k * Hk * factor * (term1*term2*c1 / s0 + term1 * term3*c0*s1);
		Hz_part[i] = Hext.Hz + Medium[i].k * Hk * factor * (-term1 * term3*s0);

		H_part = sqrt(Hx_part[i] * Hx_part[i] + Hy_part[i] * Hy_part[i] + Hz_part[i] * Hz_part[i]);

		//double T_Larmor = ((2.0*Pi) / (gamma * (2.0*Hk*Ms))*1.0e12);							////////////////////////////////////////////////////////////////////////// ???
		double t_Larmor_used_2D = ((2.0*Pi) / (gamma * (1.5*(2.0*fabs(1.0e5) / miu0 / 795774.7154594767)))*1.0e12);
		double T_Larmor = ((2.0*Pi) / (gamma * (H_part*Ms))*1.0e12);

		//high field?

// 		double slope_ini_tTL_vs_K1M = 0.7474752590e-2;// (20 - 2) / (2454.29958 - 46.19312);
// 		double slope_log_log = -1.1736;
// 		double slope_correct = pow(10, (log10(0.7474752590e-2) + slope_log_log * (4.0 - 5.0)));
// 
// 		T_Larmor *= slope_correct;



		TORQx = (my0*Hz_part[i] - mz0 * Hy_part[i]);
		TORQy = -(mx0*Hz_part[i] - mz0 * Hx_part[i]);
		TORQz = (mx0*Hy_part[i] - my0 * Hx_part[i]);
		//TORQ = sqrt(TORQx * TORQx + TORQy * TORQy + TORQz * TORQz);

		TORQxx = (my0*TORQz - mz0 * TORQy);
		TORQyy = -(mx0*TORQz - mz0 * TORQx);
		TORQzz = (mx0*TORQy - my0 * TORQx);

		TORQ = sqrt(TORQxx * TORQxx + TORQyy * TORQyy + TORQzz * TORQzz);

		// se roteste pozitia veche a lui H astfel incat acesta sa fie dupa Oz si se afla noul M
		double R1[3][3], R2[3][3];

		double th_Hext = acos(Hext.Hz / fabs(Hext.Hampl));
		double unghi_intre_Hext_si_H;


		////////////////////////////////////////////////////////////////////////// ////////////////////////////////////////////////////////////////////////// CE-I ASTA???
		if (angle_target > Pis2)
			unghi_intre_Hext_si_H = th_Hext;
		else
			if (angle_target < 0.275)
				unghi_intre_Hext_si_H = sol_target[2 * i + 0];
			else
				unghi_intre_Hext_si_H = sol_target[2 * i + 0] - (angle_target - 0.275)*(sol_target[2 * i + 0] - th_Hext) / (Pis2 - 0.275);
		////////////////////////////////////////////////////////////////////////// //////////////////////////////////////////////////////////////////////////

		c0 = cos(-sol_target[2 * i + 1]); s0 = sin(-sol_target[2 * i + 1]);
		c1 = cos(-unghi_intre_Hext_si_H); s1 = sin(-unghi_intre_Hext_si_H);

		R1[0][0] = c0 * c1;  R1[0][1] = -s0 * c1;  R1[0][2] = s1;
		R1[1][0] = s0;       R1[1][1] = c0;       R1[1][2] = 0.0;
		R1[2][0] = -c0 * s1;  R1[2][1] = s0 * s1;  R1[2][2] = c1;

		R2[0][0] = R1[0][0]; R2[0][1] = R1[1][0]; R2[0][2] = R1[2][0];
		R2[1][0] = R1[0][1]; R2[1][1] = R1[1][1]; R2[1][2] = R1[2][1];
		R2[2][0] = R1[0][2]; R2[2][1] = R1[1][2]; R2[2][2] = R1[2][2];

		double mx0r = R1[0][0] * mx0 + R1[0][1] * my0 + R1[0][2] * mz0;
		double my0r = R1[1][0] * mx0 + R1[1][1] * my0 + R1[1][2] * mz0;
		double mz0r = R1[2][0] * mx0 + R1[2][1] * my0 + R1[2][2] * mz0;

		double th_m0r = acos(mz0r);
		double ph_m0r = atan2(my0r, mx0r);

		// se modifica th_m0r si ph_m0r conform regulilor

		//if (cos(sol_old[2 * i + 1]) * cos(sol_target[2 * i + 1]) < 0.0)
		//	timp_max = T_Larmor * interpolare_RAW(loco_angle_with_ea_old, loco_angle_with_ea_target);
		//else
		timp_max = /*t_Larmor_used_2D*/T_Larmor * interpolare(loco_angle_with_ea_old, loco_angle_with_ea_target);

		if (t < timp_max)
		{
			angle_new = angle_target * t / (timp_max /*/ (2.0 * (1.0+1.0*fabs(cos(angle_target))) )*/);
		}
		else
		{
			angle_new = angle_target;
		}

		//angle_new = angle_target - angle_new;
		double th_m1r = th_m0r - angle_new;
		if (th_m1r < 0.0) th_m1r = 0.0;
		double ph_m1r = ph_m0r + t * Pix2 / T_Larmor;

		// se roteste totul inapoi rezultand noua solutie

		double mx1r = sin(th_m1r)*cos(ph_m1r);
		double my1r = sin(th_m1r)*sin(ph_m1r);
		double mz1r = cos(th_m1r);

		mxf = R2[0][0] * mx1r + R2[0][1] * my1r + R2[0][2] * mz1r;
		myf = R2[1][0] * mx1r + R2[1][1] * my1r + R2[1][2] * mz1r;
		mzf = R2[2][0] * mx1r + R2[2][1] * my1r + R2[2][2] * mz1r;

		sol_new[2 * i + 0] = acos(mzf);
		sol_new[2 * i + 1] = atan2(myf, mxf);
	}
	return TORQ;
}

//**************************************************************************

double stability_test(double solutie[], double solutie_old[])
{
	double diference = 0.0;
	double torq, torq_old;
	double proj, proj_old;
	double th, ph, th_old, ph_old;

	for (int i = 0; i < npart; i++)
	{
		if (fabs(solutie_old[2 * i + 0] - solutie[2 * i + 0]) > diference)
			diference = fabs(solutie_old[2 * i + 0] - solutie[2 * i + 0]);

		if (fabs(solutie_old[2 * i + 1] - solutie[2 * i + 1]) > diference)
			diference = fabs(solutie_old[2 * i + 1] - solutie[2 * i + 1]);

		th = atan(tan(solutie[2 * i + 0]));
		ph = atan(tan(solutie[2 * i + 1]));

		torq = (th * Hext.phi - ph * Hext.theta) * (th * Hext.phi - ph * Hext.theta) + (ph - Hext.phi) * (ph - Hext.phi) + (Hext.theta - th) * (Hext.theta - th);
		proj = (sin(th)*cos(ph)*sin(Hext.theta)*cos(Hext.phi) + sin(th)*sin(ph)*sin(Hext.theta)*sin(Hext.phi) + cos(th)*cos(Hext.theta));

		th_old = atan(tan(solutie_old[2 * i + 0]));
		ph_old = atan(tan(solutie_old[2 * i + 1]));

		torq_old = (th_old * Hext.phi - ph_old * Hext.theta) * (th_old * Hext.phi - ph_old * Hext.theta) + (ph_old - Hext.phi) * (ph_old - Hext.phi) + (Hext.theta - th_old) * (Hext.theta - th_old);
		proj_old = (sin(th_old)*cos(ph_old)*sin(Hext.theta)*cos(Hext.phi) + sin(th_old)*sin(ph_old)*sin(Hext.theta)*sin(Hext.phi) + cos(th_old)*cos(Hext.theta));

		if (fabs(torq_old - torq) > diference)
			diference = fabs(torq_old - torq);

		//  		if (fabs(proj_old - proj) > diference)
		//  			diference = fabs(proj_old - proj);
	}

	return diference;
}

//**************************************************************************