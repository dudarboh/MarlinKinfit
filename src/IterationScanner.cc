/*! \file 
 *  \brief Implements class IterationScanner
 *
 * \b Changelog:
 * - 
 *
 * \b CVS Log messages:
 * - $Log: IterationScanner.cc,v $
 * - Revision 1.1  2011/03/03 15:03:02  blist
 * - Latest version, with NewFitterGSL
 * -
 *
 */ 
#ifdef MARLIN_USE_ROOT

#include "IterationScanner.h"

#include "BaseFitter.h"
#include "NewFitterGSL.h"
#include "BaseFitObject.h"
#include "BaseHardConstraint.h"

#include <TString.h>
#include <TH2F.h>

#include <gsl/gsl_vector.h>

#undef NDEBUG
#include <cassert>
#include <iostream>

using namespace std;

IterationScanner::IterationScanner (BaseFitter& fitter_)
: fitter (fitter_)
{}

void IterationScanner::doScan (int xglobal, 
            int nx,
            double xstart,
            double xstop,
            int yglobal, 
            int ny,
            double ystart,
            double ystop,
            const char *idprefix,
            const char *titleprefix) {
  
  NewFitterGSL *newfitter = dynamic_cast<NewFitterGSL *>(&fitter);
            
  FitObjectContainer* fitobjects = fitter.getFitObjects();
  if (fitobjects == 0) return;
  ConstraintContainer* constraints = fitter.getConstraints();
  if (constraints == 0) return;
  
  FitObjectContainer fitobjects_backup(fitobjects->size());
  for (unsigned int i = 0; i < fitobjects->size();  ++i) {
    BaseFitObject *fo = (*fitobjects)[i];
    assert (fo);
    fitobjects_backup[i] = fo->copy();
  }
  // Get largest global parameter number,
  // find parameter names
  TString xname ("");
  TString yname ("");
  
  int idim = 1;
  for (FitObjectIterator i = fitobjects->begin(); i != fitobjects->end(); ++i) {
    BaseFitObject *fo = *i;
    assert (fo);
    for (int ilocal = 0; ilocal < fo->getNPar(); ++ilocal) {
      int iglobal = fo->getGlobalParNum (ilocal);
      if (iglobal >= idim) idim = iglobal+1;
      if (iglobal == xglobal) xname = fo->getParamName (ilocal);
      if (iglobal == yglobal) yname = fo->getParamName (ilocal);
    }
  }
  for (ConstraintIterator i = constraints->begin(); i != constraints->end(); ++i) {
    BaseHardConstraint *c = *i;
    assert (c);
    int iglobal = c->getGlobalNum();
      if (iglobal >= idim) idim = iglobal+1;
  }
  
  if (xglobal >= idim) return;
  if (yglobal >= idim) return;
  
  double *parsave  = new double [idim];
  double *par  = new double [idim];
  
  // Get starting values
  for (FitObjectIterator i = fitobjects->begin(); i != fitobjects->end(); ++i) {
    BaseFitObject *fo = *i;
    assert (fo);
    for (int ilocal = 0; ilocal < fo->getNPar(); ++ilocal) {
      int iglobal = fo->getGlobalParNum (ilocal);
      assert (iglobal >= 0 && iglobal < idim);
      parsave[iglobal] = fo->getParam (ilocal);
    }
  }

  // Book Histograms
  
  TString idpostfix("");
  idpostfix += xglobal;
  idpostfix += "_";
  idpostfix += yglobal;
  
  TString titlepostfix(" vs ");
  titlepostfix += xname;
  titlepostfix += " and ";
  titlepostfix += yname;
  titlepostfix += ";";
  titlepostfix += xname;
  titlepostfix += ";";
  titlepostfix += yname;
  
  // Chi2 Histogram
  
  TString id (idprefix);
  id += "nit_";
  id += idpostfix;
  
  TString title (titleprefix);
  title += "Iterations ";
  title += titlepostfix;
  
  TH2F *hnit = new TH2F (id, title, nx, xstart, xstop, ny, ystart, ystop);
  cout << "Booking Histo '" << id << "': '" << title << "'" << endl;
    
  
  // Do the scan
  
  for (int ix = 1; ix <= nx; ++ix) {
    double x = (ix - 0.5)*(xstop-xstart)/nx + xstart;
    for (int iy = 1; iy <= ny; ++iy) {
      double y = (iy - 0.5)*(ystop-ystart)/ny + ystart;
      
      // Set parameters
      for (int i = 0; i < idim; ++i) par[i] = parsave[i];
      par[xglobal] = x;
      par[yglobal] = y;
      for (FitObjectIterator i = fitobjects->begin(); i != fitobjects->end(); ++i) {
        BaseFitObject *fo = *i;
        assert (fo);
        fo->updateParams(par, idim);
      }
      
      for (unsigned int i = 0; i < fitobjects->size();  ++i) {
        BaseFitObject *fo = (*fitobjects)[i];
        BaseFitObject *fobu = fitobjects_backup[i];
        assert (fo);
        assert (fobu);
        for (int j = 0; j < fobu->getNPar(); ++j) {
          for (int k = 0; k < fobu->getNPar(); ++k) {
            fo->setCov (j, k, fobu->getCov(j, k));
          }
        }
      }
      
      // Calculate nit
      
      double fprob = fitter.fit();
      double nit = fitter.getIterations();
      
      cout << "IterationScanner::doScan: x=" << x;
      if (newfitter) cout << " -> " << gsl_vector_get (newfitter->x, xglobal);
      cout << ", y=" << y;
      if (newfitter) cout << " -> " << gsl_vector_get (newfitter->x, yglobal);
      cout << ", fitprob=" << fprob << ", nit=" << nit << endl;
      
      // Fill Histos
      hnit->SetBinContent (ix, iy, nit);      
    }
  }
  
  // Write histos;
  hnit->Write();

  for (unsigned int i = 0; i < fitobjects_backup.size();  ++i) {
    delete fitobjects_backup[i];
  }

  delete par;
  delete parsave;
}

#endif // MARLIN_USE_ROOT
