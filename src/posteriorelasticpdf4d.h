#ifndef POSTERIORELASTICPDF4D_H
#define POSTERIORELASTICPDF4D_H


#include "nrlib/grid/grid2d.hpp"
#include "nrlib/trend/trend.hpp"
#include <src/posteriorelasticpdf.h>
#include "src/simbox.h"

class FFTGrid;

// The 4D empirical PDF is found by performing an automatic dimension reduction of the
// elastic parameters from 3D to 2D.
// Denote the resulting 4D grid a density grid. We count the number of data points that are placed in each cell in the
// density grid.

class PosteriorElasticPDF4D : public PosteriorElasticPDF {
public:

  PosteriorElasticPDF4D(const std::vector<double>                   & d1, // first dimension of data points
                        const std::vector<double>                   & d2, // second dimension of data points
                        const std::vector<double>                   & d3, // third dimension of data points
                        const std::vector<int>                      & t1, // fourth dimension (trend parameters)
                        const std::vector<int>                      & t2, // fourth dimension (trend parameters)
                        const std::vector<std::vector<double> >     & v,  // Transformation of elastic variables from 3D to 2D
                        const double                     *const*const sigma, // Gaussian smoothing kernel in 2D
                        int                                           n1,    // resolution of density grid in elastic dimension 1
                        int                                           n2,    // resolution of density grid in elastic dimension 2
                        int                                           nt1,    // resolution of density grid in trend dimension 1
                        int                                           nt2,    // resolution of density grid in trend dimension 2
                        double                                        d1_min,
                        double                                        d1_max,
                        double                                        d2_min,
                        double                                        d2_max,
                        double                                        t1_min,
                        double                                        t1_max,
                        double                                        t2_min,
                        double                                        t2_max,
                        int                                           ind);

  PosteriorElasticPDF4D(int nx,                       // resolution of density grid in dimension 1
                        int ny,                       // resolution of density grid in dimension 2
                        int nt1,                      // resolution of density grid in dimension 3 (trend parameter 1)
                        int nt2);                     // resolution of density grid in dimension 4 (trend parameter 2)

  ~PosteriorElasticPDF4D();

  // Returns missing if values are outside definition area.
  virtual double Density(const double & vp,
                         const double & vs,
                         const double & rho,
                         const double & s1,
                         const double & s2) const;

  virtual double Density(const double & vp,
                         const double & vs,
                         const double & rho,
                         const double & s1) const;

  virtual double Density(const double & vp,
                         const double & vs,
                         const double & rho) const;

  virtual void ResampleAndWriteDensity(const std::string & fileName,
                                       const Simbox      * origVol,
                                       Simbox            * volume,
                                       int                 gridNo,
                                       bool                writeSurface) const;

  virtual double FindDensity(const double & vp,
                             const double & vs,
                             const double & rho,
                             const double & s1 = 0,
                             const double & s2 = 0,
                             const Simbox * const volume = 0) const;

  void WriteAsciiFile(std::string filename,
                      int         i,
                      int         j) const;


  // Get functions for the limits of the variables
  double GetXMin() const { return x_min_; }
  double GetXMax() const { return x_max_; }
  double GetdX()   const { return dx_; }

  double GetYMin() const { return y_min_; }
  double GetYMax() const { return y_max_; }
  double GetdY()   const { return dy_; }

private:

  // Density grid
  NRLib::Grid2D<FFTGrid *> histogram_ ;

  //Linear transform 1 and 2
  std::vector<double> v1_;
  std::vector<double> v2_;

  // Grid resolution for the transformed variables.
  int nx_;
  int ny_;

  // Grid resolution for the trend variables
  int nt1_;
  int nt2_;

  //Limits and grid size for variables 1 and 2
  double x_min_;
  double x_max_;
  double dx_;

  double y_min_;
  double y_max_;
  double dy_;

  // Limits and grid size for trend 1 and 2
  double t1_min_;
  double t1_max_;
  double dt1_;

  double t2_min_;
  double t2_max_;
  double dt2_;

};

#endif
