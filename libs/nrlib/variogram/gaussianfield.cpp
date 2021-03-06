// $Id: gaussianfield.cpp 1046 2012-08-14 11:42:12Z anner $

// Copyright (c)  2011, Norwegian Computing Center
// All rights reserved.
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// �  Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// �  Redistributions in binary form must reproduce the above copyright notice, this list of
//    conditions and the following disclaimer in the documentation and/or other materials
//    provided with the distribution.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <cmath>
#include "gaussianfield.hpp"

#include "variogram.hpp"
#include "../grid/grid2d.hpp"
#include "../random/random.hpp"
#include "../fft/fftgrid2d.hpp"
#include "../fft/fft.hpp"



using namespace NRLib;

// Local function, not accessible elsewhere.
// Using int instead of size_t, since we want negative differences.
Grid2D<double> CovGridForFFT2D(const Variogram& variogram,
                               int nx, double dx,
                               int ny, double dy)
{
  Grid2D<double> cov(nx, ny);
  int nxm = (nx + 1)/2;
  int nym = (ny + 1)/2;
  double ddx, ddy;
  int i, j;
  for (j = 0; j < nym; j++) {
    ddy = j * dy;
    for (i = 0; i < nxm; i++) {
      ddx = i * dx;
      cov(i, j) = variogram.GetCov(ddx,ddy);
    }
  }
  // -1 - i quadrant
  for (j = 0; j < nym; j++) {
    ddy = j * dy;
    for (i = nxm; i < nx; i++) {
      ddx = (i - nx) * dx;
      cov(i,j) = variogram.GetCov(ddx, ddy);
    }
  }

  // 1 + i quadrant
  for (j = nym; j < ny; j++) {
    ddy = (j - ny) * dy;
    for (i = nxm; i < nx; i++) {
      ddx = (i - nx)* dx;
      cov(i,j) = variogram.GetCov(ddx, ddy);
    }
  }

  // -1 + i quadrant
  for (j = nym; j < ny; j++) {
    ddy = (j - ny) * dy;
    for (i = 0; i < nxm; i++) {
      ddx = i*dx;
      cov(i,j) = variogram.GetCov(ddx, ddy);
    }
  }

  return cov;
}


void NRLib::Simulate2DGaussianField(const Variogram& variogram,
                                    size_t nx, double dx,
                                    size_t ny, double dy,
                                    // double padding_fraction,
                                    // bool   user_defined_padding,
                                    Grid2D<double>& grid_out)
{
  // Find grid size.
  double range_x, range_y;
  if (variogram.GetAzimuthAngle() == 0.0) {
    range_x = variogram.GetRangeX();
    range_y = variogram.GetRangeY();
  }
  else {
    range_x = std::max(variogram.GetRangeX(), variogram.GetRangeY());
    range_y = range_x;
  }
  size_t n_pad_x = std::max(static_cast<size_t>(4.0 * range_x / dx), 2*nx) - nx;
  size_t n_pad_y = std::max(static_cast<size_t>(4.0 * range_y / dy), 2*ny) - ny;

  FFTGrid2D<double> fftgrid(nx, ny, n_pad_x, n_pad_y, true);

  size_t nx_tot = fftgrid.GetNItot();
  size_t ny_tot = fftgrid.GetNJtot();

  // Covariance grid.
  Grid2D<double> cov = CovGridForFFT2D(variogram,
                                       static_cast<int>(nx_tot), dx,
                                       static_cast<int>(ny_tot), dy);

  // Grid with white noice.
  Grid2D<double> noise(nx_tot,ny_tot);
  size_t i,j;
  for (i = 0; i < nx_tot; i++)
    for (j = 0; j < ny_tot; j++)
      noise(i,j) = NRLib::Random::Norm01();

  fftgrid.Initialize(noise);
  FFTGrid2D<double> filter(nx, ny, n_pad_x, n_pad_y, false);
  filter.Initialize(cov);
  fftgrid.ConvolveCovariance(filter);
  grid_out = fftgrid.GetRealGrid();
}

void
NRLib::Simulate1DGaussianField(const Variogram& variogram,
                               size_t nx, double dx,
                               std::vector<double> & grid_out)
{
  double range = variogram.GetRangeX();
  size_t nx_pad = std::max(static_cast<size_t>(4.0 * range / dx), 2*nx) - nx;

  std::vector<double> cov(nx_pad);
  size_t nxm = (nx_pad+1)/2;
  for(size_t i = 0; i < nxm; i++){
    cov[i] = variogram.GetCov(i*dx);
  }
  for(size_t i=nxm; i < nx_pad; i++){
    double ddx = dx*(nx_pad - i);
    cov[i] = variogram.GetCov(ddx);
  }

  std::vector<double> noise(2*nx_pad);
  for(size_t i = 0; i < 2*nx_pad; i++)
    noise[i] = NRLib::Random::Norm01();
  std::vector<std::complex<double> > cov_fft;
  std::vector<std::complex<double> > noise_fft;
  NRLib::ComputeFFT1D(cov, cov_fft, true);
  NRLib::ComputeFFT1D(noise, noise_fft, true);
  std::vector<std::complex<double> > convolve(cov_fft.size());

  for(size_t i = 0; i < cov_fft.size(); i++)
    convolve[i] = std::sqrt(cov_fft[i])*noise_fft[i];


  NRLib::ComputeFFTInv1D(convolve, grid_out, true);
}


/*
void
NRLib::Simulate1DGaussianField(NRLib::Matrix cov_in,
                               size_t nx,
                               std::vector<double> & grid_out)
{
 // double range = variogram.GetRangeX();
 // size_t nx_pad = std::max(static_cast<size_t>(4.0 * range / dx), 2*nx) - nx;
  size_t nx_pad = 2*nx;
  std::vector<double> cov(nx_pad);
 // size_t nxm = (nx_pad+1)/2;
  for(size_t i = 0; i < nx; i++){
    cov[i] = cov_in(static_cast<int> (i),0);
  }
  for(size_t i=nx; i < nx_pad; i++){
    cov[i] = cov_in(static_cast<int> (nx_pad-i),0);
  }

  std::vector<double> noise(2*nx_pad);
  for(size_t i = 0; i < 2*nx_pad; i++)
    noise[i] = NRLib::Random::Norm01();
  std::vector<std::complex<double> > cov_fft;
  std::vector<std::complex<double> > noise_fft;
  NRLib::ComputeFFT1D(cov, cov_fft, true);
  NRLib::ComputeFFT1D(noise, noise_fft, true);
  std::vector<std::complex<double> > convolve(cov_fft.size());

  for(size_t i = 0; i < cov_fft.size(); i++)
    convolve[i] = std::sqrt(cov_fft[i])*noise_fft[i];


  NRLib::ComputeFFTInv1D(convolve, grid_out, true);
}
*/
