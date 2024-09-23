/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */
//
// Description: This file contains the class definition for TimoshenkoBeamColumn2d.
//
// Interpolation: Displacement
// 
//
// Written: MHS
// Created: Feb 2001
//
#include <TimoshenkoBeamColumn2d.h>
#include <Node.h>
#include <SectionForceDeformation.h>
#include <CrdTransf.h>
#include <Matrix.h>
#include <Vector.h>
#include <ID.h>
#include <Renderer.h>
#include <Domain.h>
#include <string.h>
#include <Information.h>
#include <Parameter.h>
#include <Channel.h>
#include <FEM_ObjectBroker.h>
#include <ElementResponse.h>
#include <CompositeResponse.h>
#include <math.h>
#include <ElementalLoad.h>

Matrix TimoshenkoBeamColumn2d::K(6, 6);
Vector TimoshenkoBeamColumn2d::P(6);
double TimoshenkoBeamColumn2d::workArea[100];

TimoshenkoBeamColumn2d::TimoshenkoBeamColumn2d(int tag, int nd1, int nd2, int numSec,
                                               SectionForceDeformation **s,
                                               BeamIntegration &bi,
                                               CrdTransf &coordTransf, double r)
    : Element(tag, ELE_TAG_TimoshenkoBeamColumn2d), numSections(numSec), theSections(0),
      crdTransf(0), beamInt(0), connectedExternalNodes(2), Q(6), q(3), rho(r),
      parameterID(0)
{
  // Allocate arrays of pointers to SectionForceDeformations
  theSections = new SectionForceDeformation *[numSections];

  for (int i = 0; i < numSections; i++) {

    // Get copies of the material model for each integration point
    theSections[i] = s[i]->getCopy();

    // Check allocation
    if (theSections[i] == 0) {
      opserr << "TimoshenkoBeamColumn2d::TimoshenkoBeamColumn2d -- failed to get a copy "
                "of section model\n";
      exit(-1);
    }
  }

  beamInt = bi.getCopy();

  if (beamInt == 0) {
    opserr << "TimoshenkoBeamColumn2d::TimoshenkoBeamColumn2d - failed to copy beam "
              "integration\n";
    exit(-1);
  }

  crdTransf = coordTransf.getCopy2d();

  if (crdTransf == 0) {
    opserr << "TimoshenkoBeamColumn2d::TimoshenkoBeamColumn2d - failed to copy "
              "coordinate transformation\n";
    exit(-1);
  }

  // Set connected external node IDs
  connectedExternalNodes(0) = nd1;
  connectedExternalNodes(1) = nd2;

  theNodes[0] = 0;
  theNodes[1] = 0;

  q0[0] = 0.0;
  q0[1] = 0.0;
  q0[2] = 0.0;

  p0[0] = 0.0;
  p0[1] = 0.0;
  p0[2] = 0.0;
}

TimoshenkoBeamColumn2d::TimoshenkoBeamColumn2d()
    : Element(0, ELE_TAG_TimoshenkoBeamColumn2d), numSections(0), theSections(0),
      crdTransf(0), beamInt(0), connectedExternalNodes(2), Q(6), q(3), rho(0.0),
      parameterID(0)
{
  q0[0] = 0.0;
  q0[1] = 0.0;
  q0[2] = 0.0;

  p0[0] = 0.0;
  p0[1] = 0.0;
  p0[2] = 0.0;

  theNodes[0] = 0;
  theNodes[1] = 0;
}

TimoshenkoBeamColumn2d::~TimoshenkoBeamColumn2d()
{
  for (int i = 0; i < numSections; i++) {
    if (theSections[i])
      delete theSections[i];
  }

  // Delete the array of pointers to SectionForceDeformation pointer arrays
  if (theSections)
    delete[] theSections;

  if (crdTransf)
    delete crdTransf;

  if (beamInt != 0)
    delete beamInt;
}

int
TimoshenkoBeamColumn2d::getNumExternalNodes() const
{
  return 2;
}

const ID &
TimoshenkoBeamColumn2d::getExternalNodes()
{
  return connectedExternalNodes;
}

Node **
TimoshenkoBeamColumn2d::getNodePtrs()
{
  return theNodes;
}

int
TimoshenkoBeamColumn2d::getNumDOF()
{
  return 6;
}

void
TimoshenkoBeamColumn2d::setDomain(Domain *theDomain)
{
  // Check Domain is not null - invoked when object removed from a domain
  if (theDomain == 0) {
    theNodes[0] = 0;
    theNodes[1] = 0;
    return;
  }

  int Nd1 = connectedExternalNodes(0);
  int Nd2 = connectedExternalNodes(1);

  theNodes[0] = theDomain->getNode(Nd1);
  theNodes[1] = theDomain->getNode(Nd2);

  if (theNodes[0] == nullptr || theNodes[1] == nullptr) {
    opserr << "WARNING TimoshenkoBeamColumn2d (tag: %d), node not found in domain"
           << this->getTag() << endln;
    ;
    return;
  }

  int dofNd1 = theNodes[0]->getNumberDOF();
  int dofNd2 = theNodes[1]->getNumberDOF();

  if (dofNd1 != 3 || dofNd2 != 3) {
    opserr << "ERROR TimoshenkoBeamColumn2d (tag: %d), has differing number of DOFs at its nodes\n";
    //	this->getTag());

    return;
  }

  if (crdTransf->initialize(theNodes[0], theNodes[1])) {
    // Add some error check
  }

  double L = crdTransf->getInitialLength();

  if (L == 0.0) {
    // Add some error check
  }

  this->DomainComponent::setDomain(theDomain);

  this->update();
}

int
TimoshenkoBeamColumn2d::commitState()
{
  int retVal = 0;

  // call element commitState to do any base class stuff
  if ((retVal = this->Element::commitState()) != 0) {
    opserr << "TimoshenkoBeamColumn2d::commitState () - failed in base class";
  }

  // Loop over the integration points and commit the material states
  for (int i = 0; i < numSections; i++)
    retVal += theSections[i]->commitState();

  retVal += crdTransf->commitState();

  return retVal;
}

int
TimoshenkoBeamColumn2d::revertToLastCommit()
{
  int retVal = 0;

  // Loop over the integration points and revert to last committed state
  for (int i = 0; i < numSections; i++)
    retVal += theSections[i]->revertToLastCommit();

  retVal += crdTransf->revertToLastCommit();

  return retVal;
}

int
TimoshenkoBeamColumn2d::revertToStart()
{
  int retVal = 0;

  // Loop over the integration points and revert states to start
  for (int i = 0; i < numSections; i++)
    retVal += theSections[i]->revertToStart();

  retVal += crdTransf->revertToStart();

  return retVal;
}

int
TimoshenkoBeamColumn2d::update(void)
{
  int err = 0;

  // Update the transformation
  crdTransf->update();

  // Get basic deformations
  const Vector &v = crdTransf->getBasicTrialDisp();
  double L        = crdTransf->getInitialLength();
  double oneOverL = 1.0 / L;

  //const Matrix &pts = quadRule.getIntegrPointCoords(numSections);
  double xi[maxNumSections];
  beamInt->getSectionLocations(numSections, L, xi);

  // Loop over the integration points
  for (int i = 0; i < numSections; i++) {

    int order      = theSections[i]->getOrder();
    const ID &code = theSections[i]->getType();

    Vector e(workArea, order);

    double xi6 = 6.0 * xi[i];

    // compute coeficient phi
    const Matrix &ks = theSections[i]->getSectionTangent();
    double EI        = 0.0;
    double GA        = 0.0;
    for (int k = 0; k < order; k++) {
      if (code(k) == SECTION_RESPONSE_MZ)
        EI += ks(k, k);
      if (code(k) == SECTION_RESPONSE_VY)
        GA += ks(k, k);
    }
    double phi = 0.0;
    if (GA != 0.0)
      phi = 12 * EI / (GA * L * L);

    for (int j = 0; j < order; j++) {
      switch (code(j)) {
      case SECTION_RESPONSE_P:
        e(j) = oneOverL * v(0);
        break;
      case SECTION_RESPONSE_MZ:
        e(j) =
            oneOverL / (1 + phi) * ((xi6 - 4.0 - phi)*v(1) + (xi6 - 2.0 + phi)*v(2));
        break;
      case SECTION_RESPONSE_VY:
        e(j) = 0.5*phi/(1 + phi)*v(1) + 0.5*phi/(1 + phi)*v(2);
        break;
      default:
        e(j) = 0.0;
        break;
      }
    }

    // Set the section deformations
    err += theSections[i]->setTrialSectionDeformation(e);
  }

  return 0;
}

const Matrix &
TimoshenkoBeamColumn2d::getTangentStiff()
{
  static Matrix kb(3, 3);

  // Zero for integral
  kb.Zero();
  q.Zero();

  double L        = crdTransf->getInitialLength();
  double oneOverL = 1.0 / L;

  //const Matrix &pts = quadRule.getIntegrPointCoords(numSections);
  //const Vector &wts = quadRule.getIntegrPointWeights(numSections);
  double xi[maxNumSections];
  beamInt->getSectionLocations(numSections, L, xi);
  double wt[maxNumSections];
  beamInt->getSectionWeights(numSections, L, wt);

  // Loop over the integration points
  for (int i = 0; i < numSections; i++) {

    int order      = theSections[i]->getOrder();
    const ID &code = theSections[i]->getType();

    Matrix ka(workArea, order, 3);
    ka.Zero();

    //double xi6 = 6.0*pts(i,0);
    double xi6 = 6.0 * xi[i];

    // Get the section tangent stiffness and stress resultant
    const Vector &s  = theSections[i]->getStressResultant();
    const Matrix &ks = theSections[i]->getSectionTangent();
    double EI        = 0.0;
    double GA        = 0.0;
    for (int k = 0; k < order; k++) {
      if (code(k) == SECTION_RESPONSE_MZ)
        EI += ks(k, k);
      if (code(k) == SECTION_RESPONSE_VY)
        GA += ks(k, k);
    }
    double phi = 0.0;
    if (GA != 0.0)
      phi = 12 * EI / (GA * L * L);

    // Perform numerical integration
    //kb.addMatrixTripleProduct(1.0, *B, ks, wts(i)/L);
    //double wti = wts(i)*oneOverL;
    double wti = wt[i] * oneOverL;
    double tmp;
    for (int j = 0; j < order; j++) {
      switch (code(j)) {
      case SECTION_RESPONSE_P:
        for (int k = 0; k < order; k++)
          ka(k, 0) += ks(k, j) * wti;
        break;
      case SECTION_RESPONSE_MZ:
        for (int k = 0; k < order; k++) {
          tmp = ks(k, j) * wti;
          ka(k, 1) += 1.0 / (1 + phi) * (xi6 - 4.0 - phi) * tmp;
          ka(k, 2) += 1.0 / (1 + phi) * (xi6 - 2.0 + phi) * tmp;
        }
        break;
      case SECTION_RESPONSE_VY:
        for (int k = 0; k < order; k++) {
          tmp = ks(k, j) * wti;
          ka(k, 1) += 0.5 * phi * L / (1 + phi) * tmp;
          ka(k, 2) += 0.5 * phi * L / (1 + phi) * tmp;
        }
        break;
      default:
        break;
      }
    }
    for (int j = 0; j < order; j++) {
      switch (code(j)) {
      case SECTION_RESPONSE_P:
        for (int k = 0; k < 3; k++)
          kb(0, k) += ka(j, k);
        break;
      case SECTION_RESPONSE_MZ:
        for (int k = 0; k < 3; k++) {
          tmp = ka(j, k);
          kb(1, k) += 1.0 / (1 + phi) * (xi6 - 4.0 - phi) * tmp;
          kb(2, k) += 1.0 / (1 + phi) * (xi6 - 2.0 + phi) * tmp;
        }
        break;
      case SECTION_RESPONSE_VY:
        for (int k = 0; k < 3; k++) {
          tmp = ka(j, k);
          kb(1, k) += 0.5 * phi * L / (1 + phi) * tmp;
          kb(2, k) += 0.5 * phi * L / (1 + phi) * tmp;
        }
        break;
      default:
        break;
      }
    }

    //q.addMatrixTransposeVector(1.0, *B, s, wts(i));
    double si;
    for (int j = 0; j < order; j++) {
      //si = s(j)*wts(i);
      si = s[j] * wt[i];
      switch (code(j)) {
      case SECTION_RESPONSE_P:
        q(0) += si;
        break;
      case SECTION_RESPONSE_MZ:
        q(1) += 1.0 / (1 + phi) * (xi6 - 4.0 - phi) * si;
        q(2) += 1.0 / (1 + phi) * (xi6 - 2.0 + phi) * si;
        break;
      case SECTION_RESPONSE_VY:
        q(1) += 0.5 * phi * L / (1 + phi) * si;
        q(2) += 0.5 * phi * L / (1 + phi) * si;
        break;
      default:
        break;
      }
    }
  }

  // Add effects of element loads, q = q(v) + q0
  q(0) += q0[0];
  q(1) += q0[1];
  q(2) += q0[2];

  // Transform to global stiffness
  K = crdTransf->getGlobalStiffMatrix(kb, q);
  return K;
}

const Matrix &
TimoshenkoBeamColumn2d::getInitialBasicStiff()
{
  static Matrix kb(3, 3);

  // Zero for integral
  kb.Zero();

  double L        = crdTransf->getInitialLength();
  double oneOverL = 1.0 / L;

  //const Matrix &pts = quadRule.getIntegrPointCoords(numSections);
  //const Vector &wts = quadRule.getIntegrPointWeights(numSections);
  double xi[maxNumSections];
  beamInt->getSectionLocations(numSections, L, xi);
  double wt[maxNumSections];
  beamInt->getSectionWeights(numSections, L, wt);

  // Loop over the integration points
  for (int i = 0; i < numSections; i++) {

    int order      = theSections[i]->getOrder();
    const ID &code = theSections[i]->getType();

    Matrix ka(workArea, order, 3);
    ka.Zero();

    //double xi6 = 6.0*pts(i,0);
    double xi6 = 6.0 * xi[i];

    // Get the section tangent stiffness and stress resultant
    const Matrix &ks = theSections[i]->getInitialTangent();
    double EI        = 0.0;
    double GA        = 0.0;
    for (int k = 0; k < order; k++) {
      if (code(k) == SECTION_RESPONSE_MZ)
        EI += ks(k, k);
      if (code(k) == SECTION_RESPONSE_VY)
        GA += ks(k, k);
    }
    double phi = 0.0;
    if (GA != 0.0)
      phi = 12 * EI / (GA * L * L);

    // Perform numerical integration
    //kb.addMatrixTripleProduct(1.0, *B, ks, wts(i)/L);
    //double wti = wts(i)*oneOverL;
    double wti = wt[i] * oneOverL;
    double tmp;
    for (int j = 0; j < order; j++) {
      switch (code(j)) {
      case SECTION_RESPONSE_P:
        for (int k = 0; k < order; k++)
          ka(k, 0) += ks(k, j) * wti;
        break;
      case SECTION_RESPONSE_MZ:
        for (int k = 0; k < order; k++) {
          tmp = ks(k, j) * wti;
          ka(k, 1) += 1.0 / (1 + phi) * (xi6 - 4.0 - phi) * tmp;
          ka(k, 2) += 1.0 / (1 + phi) * (xi6 - 2.0 + phi) * tmp;
        }
        break;
      case SECTION_RESPONSE_VY:
        for (int k = 0; k < order; k++) {
          tmp = ks(k, j) * wti;
          ka(k, 1) += 0.5 * phi * L / (1 + phi) * tmp;
          ka(k, 2) += 0.5 * phi * L / (1 + phi) * tmp;
        }
        break;
      default:
        break;
      }
    }
    for (int j = 0; j < order; j++) {
      switch (code(j)) {
      case SECTION_RESPONSE_P:
        for (int k = 0; k < 3; k++)
          kb(0, k) += ka(j, k);
        break;
      case SECTION_RESPONSE_MZ:
        for (int k = 0; k < 3; k++) {
          tmp = ka(j, k);
          kb(1, k) += 1.0 / (1 + phi) * (xi6 - 4.0 - phi) * tmp;
          kb(2, k) += 1.0 / (1 + phi) * (xi6 - 2.0 + phi) * tmp;
        }
        break;
      case SECTION_RESPONSE_VY:
        for (int k = 0; k < 3; k++) {
          tmp = ka(j, k);
          kb(1, k) += 0.5 * phi * L / (1 + phi) * tmp;
          kb(2, k) += 0.5 * phi * L / (1 + phi) * tmp;
        }
        break;
      default:
        break;
      }
    }
  }

  return kb;
}

const Matrix &
TimoshenkoBeamColumn2d::getInitialStiff()
{
  const Matrix &kb = this->getInitialBasicStiff();

  // Transform to global stiffness
  K = crdTransf->getInitialGlobalStiffMatrix(kb);

  return K;
}

const Matrix &
TimoshenkoBeamColumn2d::getMass()
{
  K.Zero();

  if (rho == 0.0)
    return K;

  double L = crdTransf->getInitialLength();
  double m = 0.5 * rho * L;

  K(0, 0) = K(1, 1) = K(3, 3) = K(4, 4) = m;

  return K;
}

void
TimoshenkoBeamColumn2d::zeroLoad(void)
{
  Q.Zero();

  q0[0] = 0.0;
  q0[1] = 0.0;
  q0[2] = 0.0;

  p0[0] = 0.0;
  p0[1] = 0.0;
  p0[2] = 0.0;

  return;
}

int
TimoshenkoBeamColumn2d::addLoad(ElementalLoad *theLoad, double loadFactor)
{
  int type;
  const Vector &data = theLoad->getData(type, loadFactor);
  double L           = crdTransf->getInitialLength();

  if (type == LOAD_TAG_Beam2dUniformLoad) {
    double wt = data(0) * loadFactor; // Transverse (+ve upward)
    double wa = data(1) * loadFactor; // Axial (+ve from node I to J)

    double V = 0.5 * wt * L;
    double M = V * L / 6.0; // wt*L*L/12
    double P = wa * L;

    // Reactions in basic system
    p0[0] -= P;
    p0[1] -= V;
    p0[2] -= V;

    // Fixed end forces in basic system
    q0[0] -= 0.5 * P;
    q0[1] -= M;
    q0[2] += M;
  } else if (type == LOAD_TAG_Beam2dPointLoad) {
    double P      = data(0) * loadFactor;
    double N      = data(1) * loadFactor;
    double aOverL = data(2);

    if (aOverL < 0.0 || aOverL > 1.0)
      return 0;

    double a = aOverL * L;
    double b = L - a;

    // Reactions in basic system
    p0[0] -= N;
    double V1 = P * (1.0 - aOverL);
    double V2 = P * aOverL;
    p0[1] -= V1;
    p0[2] -= V2;

    double L2 = 1.0 / (L * L);
    double a2 = a * a;
    double b2 = b * b;

    // Fixed end forces in basic system
    q0[0] -= N * aOverL;
    double M1 = -a * b2 * P * L2;
    double M2 = a2 * b * P * L2;
    q0[1] += M1;
    q0[2] += M2;
  } else {
    opserr << "TimoshenkoBeamColumn2d::TimoshenkoBeamColumn2d -- load type unknown for "
              "element with tag: "
           << this->getTag() << "TimoshenkoBeamColumn2d::addLoad()\n";

    return -1;
  }

  return 0;
}

int
TimoshenkoBeamColumn2d::addInertiaLoadToUnbalance(const Vector &accel)
{
  // Check for a quick return
  if (rho == 0.0)
    return 0;

  // Get R * accel from the nodes
  const Vector &Raccel1 = theNodes[0]->getRV(accel);
  const Vector &Raccel2 = theNodes[1]->getRV(accel);

  if (3 != Raccel1.Size() || 3 != Raccel2.Size()) {
    opserr << "TimoshenkoBeamColumn2d::addInertiaLoadToUnbalance matrix and vector sizes "
              "are incompatable\n";
    return -1;
  }

  double L = crdTransf->getInitialLength();
  double m = 0.5 * rho * L;

  // Want to add ( - fact * M R * accel ) to unbalance
  // Take advantage of lumped mass matrix
  Q(0) -= m * Raccel1(0);
  Q(1) -= m * Raccel1(1);
  Q(3) -= m * Raccel2(0);
  Q(4) -= m * Raccel2(1);

  return 0;
}

const Vector &
TimoshenkoBeamColumn2d::getResistingForce()
{
  double L = crdTransf->getInitialLength();

  //const Matrix &pts = quadRule.getIntegrPointCoords(numSections);
  //const Vector &wts = quadRule.getIntegrPointWeights(numSections);
  double xi[maxNumSections];
  beamInt->getSectionLocations(numSections, L, xi);
  double wt[maxNumSections];
  beamInt->getSectionWeights(numSections, L, wt);

  // Zero for integration
  q.Zero();

  // Loop over the integration points
  for (int i = 0; i < numSections; i++) {
    int order      = theSections[i]->getOrder();
    const ID &code = theSections[i]->getType();

    const Matrix &ks = theSections[i]->getInitialTangent();
    double EI        = 0.0;
    double GA        = 0.0;
    for (int k = 0; k < order; k++) {
      if (code(k) == SECTION_RESPONSE_MZ)
        EI += ks(k, k);
      if (code(k) == SECTION_RESPONSE_VY)
        GA += ks(k, k);
    }
    double phi = 0.0;
    if (GA != 0.0)
      phi = 12 * EI / (GA * L * L);

    //double xi6 = 6.0*pts(i,0);
    double xi6 = 6.0 * xi[i];

    // Get section stress resultant
    const Vector &s = theSections[i]->getStressResultant();

    // Perform numerical integration on internal force
    //q.addMatrixTransposeVector(1.0, *B, s, wts(i));

    double si;
    for (int j = 0; j < order; j++) {
      //si = s(j)*wts(i);
      si = s(j) * wt[i];
      switch (code(j)) {
      case SECTION_RESPONSE_P:
        q(0) += si;
        break;
      case SECTION_RESPONSE_MZ:
        q(1) += 1.0 / (1 + phi) * (xi6 - 4.0 - phi) * si;
        q(2) += 1.0 / (1 + phi) * (xi6 - 2.0 + phi) * si;
        break;
      case SECTION_RESPONSE_VY:
        q(1) += 0.5 * phi * L / (1 + phi) * si;
        q(2) += 0.5 * phi * L / (1 + phi) * si;
        break;
      default:
        break;
      }
    }
  }

  // Add effects of element loads, q = q(v) + q0
  q(0) += q0[0];
  q(1) += q0[1];
  q(2) += q0[2];

  // Vector for reactions in basic system
  Vector p0Vec(p0, 3);

  P = crdTransf->getGlobalResistingForce(q, p0Vec);

  // Subtract other external nodal loads ... P_res = P_int - P_ext
  //P.addVector(1.0, Q, -1.0);
  P(0) -= Q(0);
  P(1) -= Q(1);
  P(2) -= Q(2);
  P(3) -= Q(3);
  P(4) -= Q(4);
  P(5) -= Q(5);

  return P;
}

const Vector &
TimoshenkoBeamColumn2d::getResistingForceIncInertia()
{

  this->getResistingForce();

  if (rho != 0.0) {
    const Vector &accel1 = theNodes[0]->getTrialAccel();
    const Vector &accel2 = theNodes[1]->getTrialAccel();

    // Compute the current resisting force
    this->getResistingForce();

    double L = crdTransf->getInitialLength();
    double m = 0.5 * rho * L;

    P(0) += m * accel1(0);
    P(1) += m * accel1(1);
    P(3) += m * accel2(0);
    P(4) += m * accel2(1);

    // add the damping forces if rayleigh damping
    if (alphaM != 0.0 || betaK != 0.0 || betaK0 != 0.0 || betaKc != 0.0)
      P += this->getRayleighDampingForces();

  } else {

    // add the damping forces if rayleigh damping
    if (betaK != 0.0 || betaK0 != 0.0 || betaKc != 0.0)
      P += this->getRayleighDampingForces();
  }

  return P;
}

int
TimoshenkoBeamColumn2d::sendSelf(int commitTag, Channel &theChannel)
{
  // place the integer data into an ID

  int dbTag = this->getDbTag();
  int i, j;
  int loc = 0;

  static ID idData(9); // one bigger than needed so no clash later
  idData(0)          = this->getTag();
  idData(1)          = connectedExternalNodes(0);
  idData(2)          = connectedExternalNodes(1);
  idData(3)          = numSections;
  idData(4)          = crdTransf->getClassTag();
  int crdTransfDbTag = crdTransf->getDbTag();
  if (crdTransfDbTag == 0) {
    crdTransfDbTag = theChannel.getDbTag();
    if (crdTransfDbTag != 0)
      crdTransf->setDbTag(crdTransfDbTag);
  }
  idData(5) = crdTransfDbTag;

  if (alphaM != 0 || betaK != 0 || betaK0 != 0 || betaKc != 0)
    idData(6) = 1;
  else
    idData(6) = 0;

  idData(7)        = beamInt->getClassTag();
  int beamIntDbTag = beamInt->getDbTag();
  if (beamIntDbTag == 0) {
    beamIntDbTag = theChannel.getDbTag();
    if (beamIntDbTag != 0)
      beamInt->setDbTag(beamIntDbTag);
  }
  idData(8) = beamIntDbTag;

  if (theChannel.sendID(dbTag, commitTag, idData) < 0) {
    opserr << "TimoshenkoBeamColumn2d::sendSelf() - failed to send ID data\n";
    return -1;
  }

  if (idData(6) == 1) {
    // send damping coefficients
    static Vector dData(4);
    dData(0) = alphaM;
    dData(1) = betaK;
    dData(2) = betaK0;
    dData(3) = betaKc;
    if (theChannel.sendVector(dbTag, commitTag, dData) < 0) {
      opserr << "TimoshenkoBeamColumn2d::sendSelf() - failed to send double data\n";
      return -1;
    }
  }

  // send the coordinate transformation
  if (crdTransf->sendSelf(commitTag, theChannel) < 0) {
    opserr << "TimoshenkoBeamColumn2d::sendSelf() - failed to send crdTranf\n";
    return -1;
  }

  // send the beam integration
  if (beamInt->sendSelf(commitTag, theChannel) < 0) {
    opserr << "TimoshenkoBeamColumn2d::sendSelf() - failed to send beamInt\n";
    return -1;
  }

  //
  // send an ID for the sections containing each sections dbTag and classTag
  // if section ha no dbTag get one and assign it
  //
  ID idSections(2 * numSections);
  loc = 0;
  for (i = 0; i < numSections; i++) {
    int sectClassTag = theSections[i]->getClassTag();
    int sectDbTag    = theSections[i]->getDbTag();
    if (sectDbTag == 0) {
      sectDbTag = theChannel.getDbTag();
      theSections[i]->setDbTag(sectDbTag);
    }

    idSections(loc)     = sectClassTag;
    idSections(loc + 1) = sectDbTag;
    loc += 2;
  }

  if (theChannel.sendID(dbTag, commitTag, idSections) < 0) {
    opserr << "TimoshenkoBeamColumn2d::sendSelf() - failed to send ID data\n";
    return -1;
  }

  //
  // send the sections
  //

  for (j = 0; j < numSections; j++) {
    if (theSections[j]->sendSelf(commitTag, theChannel) < 0) {
      opserr << "TimoshenkoBeamColumn2d::sendSelf() - section " << j
             << "failed to send itself\n";
      return -1;
    }
  }

  return 0;
}

int
TimoshenkoBeamColumn2d::recvSelf(int commitTag, Channel &theChannel,
                                 FEM_ObjectBroker &theBroker)
{
  //
  // receive the integer data containing tag, numSections and coord transformation info
  //
  int dbTag = this->getDbTag();
  int i;

  static ID idData(9); // one bigger than needed so no clash with section ID

  if (theChannel.recvID(dbTag, commitTag, idData) < 0) {
    opserr << "TimoshenkoBeamColumn2d::recvSelf() - failed to recv ID data\n";
    return -1;
  }

  this->setTag(idData(0));
  connectedExternalNodes(0) = idData(1);
  connectedExternalNodes(1) = idData(2);

  int crdTransfClassTag = idData(4);
  int crdTransfDbTag    = idData(5);

  if (idData(6) == 1) {
    // recv damping coefficients
    static Vector dData(4);
    if (theChannel.recvVector(dbTag, commitTag, dData) < 0) {
      opserr << "TimoshenkoBeamColumn2d::sendSelf() - failed to recv double data\n";
      return -1;
    }
    alphaM = dData(0);
    betaK  = dData(1);
    betaK0 = dData(2);
    betaKc = dData(3);
  }

  int beamIntClassTag = idData(7);
  int beamIntDbTag    = idData(8);

  // create a new crdTransf object if one needed
  if (crdTransf == 0 || crdTransf->getClassTag() != crdTransfClassTag) {
    if (crdTransf != 0)
      delete crdTransf;

    crdTransf = theBroker.getNewCrdTransf(crdTransfClassTag);

    if (crdTransf == 0) {
      opserr << "TimoshenkoBeamColumn2d::recvSelf() - failed to obtain a CrdTrans object "
                "with classTag "
             << crdTransfClassTag << endln;
      return -2;
    }
  }
  crdTransf->setDbTag(crdTransfDbTag);

  // invoke recvSelf on the crdTransf object
  if (crdTransf->recvSelf(commitTag, theChannel, theBroker) < 0) {
    opserr << "TimoshenkoBeamColumn2d::sendSelf() - failed to recv crdTranf\n";
    return -3;
  }

  // create a new beamInt object if one needed
  if (beamInt == 0 || beamInt->getClassTag() != beamIntClassTag) {
    if (beamInt != 0)
      delete beamInt;

    beamInt = theBroker.getNewBeamIntegration(beamIntClassTag);

    if (beamInt == 0) {
      opserr << "TimoshenkoBeamColumn2d::recvSelf() - failed to obtain the beam "
                "integration object with classTag"
             << beamIntClassTag << endln;
      exit(-1);
    }
  }

  beamInt->setDbTag(beamIntDbTag);

  // invoke recvSelf on the beamInt object
  if (beamInt->recvSelf(commitTag, theChannel, theBroker) < 0) {
    opserr << "TimoshenkoBeamColumn2d::sendSelf() - failed to recv beam integration\n";
    return -3;
  }

  //
  // recv an ID for the sections containing each sections dbTag and classTag
  //

  ID idSections(2 * idData(3));
  int loc = 0;

  if (theChannel.recvID(dbTag, commitTag, idSections) < 0) {
    opserr << "TimoshenkoBeamColumn2d::recvSelf() - failed to recv ID data\n";
    return -1;
  }

  //
  // now receive the sections
  //

  if (numSections != idData(3)) {

    //
    // we do not have correct number of sections, must delete the old and create
    // new ones before can recvSelf on the sections
    //

    // delete the old
    if (numSections != 0) {
      for (int i = 0; i < numSections; i++)
        delete theSections[i];
      delete[] theSections;
    }

    // create a new array to hold pointers
    theSections = new SectionForceDeformation *[idData(3)];
    if (theSections == 0) {
      opserr << "TimoshenkoBeamColumn2d::recvSelf() - out of memory creating sections "
                "array of size "
             << idData(3) << endln;
      return -1;
    }

    // create a section and recvSelf on it
    numSections = idData(3);
    loc         = 0;

    for (i = 0; i < numSections; i++) {
      int sectClassTag = idSections(loc);
      int sectDbTag    = idSections(loc + 1);
      loc += 2;
      theSections[i] = theBroker.getNewSection(sectClassTag);
      if (theSections[i] == 0) {
        opserr << "TimoshenkoBeamColumn2d::recvSelf() - Broker could not create Section "
                  "of class type "
               << sectClassTag << endln;
        exit(-1);
      }
      theSections[i]->setDbTag(sectDbTag);
      if (theSections[i]->recvSelf(commitTag, theChannel, theBroker) < 0) {
        opserr << "TimoshenkoBeamColumn2d::recvSelf() - section " << i
               << " failed to recv itself\n";
        return -1;
      }
    }

  } else {

    //
    // for each existing section, check it is of correct type
    // (if not delete old & create a new one) then recvSelf on it
    //

    loc = 0;
    for (i = 0; i < numSections; i++) {
      int sectClassTag = idSections(loc);
      int sectDbTag    = idSections(loc + 1);
      loc += 2;

      // check of correct type
      if (theSections[i]->getClassTag() != sectClassTag) {
        // delete the old section[i] and create a new one
        delete theSections[i];
        theSections[i] = theBroker.getNewSection(sectClassTag);
        if (theSections[i] == 0) {
          opserr << "TimoshenkoBeamColumn2d::recvSelf() - Broker could not create "
                    "Section of class type "
                 << sectClassTag << endln;
          exit(-1);
        }
      }

      // recvSelf on it
      theSections[i]->setDbTag(sectDbTag);
      if (theSections[i]->recvSelf(commitTag, theChannel, theBroker) < 0) {
        opserr << "TimoshenkoBeamColumn2d::recvSelf() - section " << i
               << " failed to recv itself\n";
        return -1;
      }
    }
  }

  return 0;
}

void
TimoshenkoBeamColumn2d::Print(OPS_Stream &s, int flag)
{
  s << "\nTimoshenkoBeamColumn2d, element id:  " << this->getTag() << endln;
  s << "\tConnected external nodes:  " << connectedExternalNodes;
  s << "\tCoordTransf: " << crdTransf->getTag() << endln;
  s << "\tmass density:  " << rho << endln;

  double L  = crdTransf->getInitialLength();
  double P  = q(0);
  double M1 = q(1);
  double M2 = q(2);
  double V  = (M1 + M2) / L;
  s << "\tEnd 1 Forces (P V M): " << -P + p0[0] << " " << V + p0[1] << " " << M1 << endln;
  s << "\tEnd 2 Forces (P V M): " << P << " " << -V + p0[2] << " " << M2 << endln;

  beamInt->Print(s, flag);

  for (int i = 0; i < numSections; i++)
    theSections[i]->Print(s, flag);
}

int
TimoshenkoBeamColumn2d::displaySelf(Renderer &theViewer, int displayMode, float fact)
{
  static Vector v1(3);
  static Vector v2(3);

  theNodes[0]->getDisplayCrds(v1, fact, displayMode);
  theNodes[1]->getDisplayCrds(v2, fact, displayMode);

  return theViewer.drawLine(v1, v2, 1.0, 1.0, this->getTag());
}

Response *
TimoshenkoBeamColumn2d::setResponse(const char **argv, int argc, OPS_Stream &output)
{
  Response *theResponse = 0;

  output.tag("ElementOutput");
  output.attr("eleType", "TimoshenkoBeamColumn2d");
  output.attr("eleTag", this->getTag());
  output.attr("node1", connectedExternalNodes[0]);
  output.attr("node2", connectedExternalNodes[1]);

  // global force -
  if (strcmp(argv[0], "forces") == 0 || strcmp(argv[0], "force") == 0 ||
      strcmp(argv[0], "globalForce") == 0 || strcmp(argv[0], "globalForces") == 0) {

    output.tag("ResponseType", "Px_1");
    output.tag("ResponseType", "Py_1");
    output.tag("ResponseType", "Mz_1");
    output.tag("ResponseType", "Px_2");
    output.tag("ResponseType", "Py_2");
    output.tag("ResponseType", "Mz_2");

    theResponse = new ElementResponse(this, 1, P);

    // local force -
  } else if (strcmp(argv[0], "localForce") == 0 || strcmp(argv[0], "localForces") == 0) {

    output.tag("ResponseType", "N1");
    output.tag("ResponseType", "V1");
    output.tag("ResponseType", "M1");
    output.tag("ResponseType", "N2");
    output.tag("ResponseType", "V2");
    output.tag("ResponseType", "M2");

    theResponse = new ElementResponse(this, 2, P);

    // basic force -
  } else if (strcmp(argv[0], "basicForce") == 0 || strcmp(argv[0], "basicForces") == 0) {

    output.tag("ResponseType", "N");
    output.tag("ResponseType", "M1");
    output.tag("ResponseType", "M2");

    theResponse = new ElementResponse(this, 9, Vector(3));

    // chord rotation -
  } else if (strcmp(argv[0], "chordRotation") == 0 ||
             strcmp(argv[0], "chordDeformation") == 0 ||
             strcmp(argv[0], "basicDeformation") == 0) {

    output.tag("ResponseType", "eps");
    output.tag("ResponseType", "theta1");
    output.tag("ResponseType", "theta2");

    theResponse = new ElementResponse(this, 3, Vector(3));

    // plastic rotation -
  } else if (strcmp(argv[0], "plasticRotation") == 0 ||
             strcmp(argv[0], "plasticDeformation") == 0) {

    output.tag("ResponseType", "epsP");
    output.tag("ResponseType", "theta1P");
    output.tag("ResponseType", "theta2P");

    theResponse = new ElementResponse(this, 4, Vector(3));

  } else if (strcmp(argv[0], "RayleighForces") == 0 ||
             strcmp(argv[0], "rayleighForces") == 0) {

    theResponse = new ElementResponse(this, 12, P);
  }

  // section response -
  else if (strstr(argv[0], "sectionX") != 0) {
    if (argc > 2) {
      float sectionLoc = atof(argv[1]);

      double xi[maxNumSections];
      double L = crdTransf->getInitialLength();
      beamInt->getSectionLocations(numSections, L, xi);

      sectionLoc /= L;

      float minDistance = fabs(xi[0] - sectionLoc);
      int sectionNum    = 0;
      for (int i = 1; i < numSections; i++) {
        if (fabs(xi[i] - sectionLoc) < minDistance) {
          minDistance = fabs(xi[i] - sectionLoc);
          sectionNum  = i;
        }
      }

      output.tag("GaussPointOutput");
      output.attr("number", sectionNum + 1);
      output.attr("eta", xi[sectionNum] * L);

      theResponse = theSections[sectionNum]->setResponse(&argv[2], argc - 2, output);
    }
  } else if (strstr(argv[0], "section") != 0) {

    if (argc > 1) {

      int sectionNum = atoi(argv[1]);

      if (sectionNum > 0 && sectionNum <= numSections && argc > 2) {

        output.tag("GaussPointOutput");
        output.attr("number", sectionNum);
        double xi[maxNumSections];
        double L = crdTransf->getInitialLength();
        beamInt->getSectionLocations(numSections, L, xi);
        output.attr("eta", xi[sectionNum - 1] * L);

        theResponse =
            theSections[sectionNum - 1]->setResponse(&argv[2], argc - 2, output);

        output.endTag();

      } else if (sectionNum == 0) { // argv[1] was not an int, we want all sections,

        CompositeResponse *theCResponse = new CompositeResponse();
        int numResponse                 = 0;
        double xi[maxNumSections];
        double L = crdTransf->getInitialLength();
        beamInt->getSectionLocations(numSections, L, xi);

        for (int i = 0; i < numSections; i++) {

          output.tag("GaussPointOutput");
          output.attr("number", i + 1);
          output.attr("eta", xi[i] * L);

          Response *theSectionResponse =
              theSections[i]->setResponse(&argv[1], argc - 1, output);

          output.endTag();

          if (theSectionResponse != 0) {
            numResponse = theCResponse->addResponse(theSectionResponse);
          }
        }

        if (numResponse == 0) // no valid responses found
          delete theCResponse;
        else
          theResponse = theCResponse;
      }
    }
  }

  // curvature sensitivity along element length
  else if (strcmp(argv[0], "dcurvdh") == 0)
    return new ElementResponse(this, 5, Vector(numSections));

  // basic deformation sensitivity
  else if (strcmp(argv[0], "dvdh") == 0)
    return new ElementResponse(this, 6, Vector(3));

  else if (strcmp(argv[0], "integrationPoints") == 0)
    return new ElementResponse(this, 7, Vector(numSections));

  else if (strcmp(argv[0], "integrationWeights") == 0)
    return new ElementResponse(this, 8, Vector(numSections));

  else if (strcmp(argv[0], "sectionTags") == 0)
    theResponse = new ElementResponse(this, 110, ID(numSections));

  output.endTag();
  return theResponse;
}

int
TimoshenkoBeamColumn2d::getResponse(int responseID, Information &eleInfo)
{
  double V;
  double L = crdTransf->getInitialLength();

  if (responseID == 1)
    return eleInfo.setVector(this->getResistingForce());

  else if (responseID == 12)
    return eleInfo.setVector(this->getRayleighDampingForces());

  else if (responseID == 2) {
    P(3) = q(0);
    P(0) = -q(0) + p0[0];
    P(2) = q(1);
    P(5) = q(2);
    V    = (q(1) + q(2)) / L;
    P(1) = V + p0[1];
    P(4) = -V + p0[2];
    return eleInfo.setVector(P);
  }

  else if (responseID == 9) {
    return eleInfo.setVector(q);
  }

  // Chord rotation
  else if (responseID == 3)
    return eleInfo.setVector(crdTransf->getBasicTrialDisp());

  // Plastic rotation
  else if (responseID == 4) {
    static Vector vp(3);
    static Vector ve(3);
    const Matrix &kb = this->getInitialBasicStiff();
    kb.Solve(q, ve);
    vp = crdTransf->getBasicTrialDisp();
    vp -= ve;
    return eleInfo.setVector(vp);
  }

  // Curvature sensitivity
  else if (responseID == 5) {
    /*
      Vector curv(numSections);
      const Vector &v = crdTransf->getBasicDispGradient(1);
      
      double L = crdTransf->getInitialLength();
      double oneOverL = 1.0/L;
      //const Matrix &pts = quadRule.getIntegrPointCoords(numSections);
      double pts[2];
      pts[0] = 0.0;
      pts[1] = 1.0;
      
      // Loop over the integration points
      for (int i = 0; i < numSections; i++) {
	int order = theSections[i]->getOrder();
	const ID &code = theSections[i]->getType();
	//double xi6 = 6.0*pts(i,0);
	double xi6 = 6.0*pts[i];
	curv(i) = oneOverL*((xi6-4.0)*v(1) + (xi6-2.0)*v(2));
      }
      
      return eleInfo.setVector(curv);
    */

    Vector curv(numSections);

    /*
    // Loop over the integration points
    for (int i = 0; i < numSections; i++) {
      int order = theSections[i]->getOrder();
      const ID &code = theSections[i]->getType();
      const Vector &dedh = theSections[i]->getdedh();
      for (int j = 0; j < order; j++) {
	if (code(j) == SECTION_RESPONSE_MZ)
	  curv(i) = dedh(j);
      }
    }
    */

    return eleInfo.setVector(curv);
  }

  // Basic deformation sensitivity
  else if (responseID == 6) {
    const Vector &dvdh = crdTransf->getBasicDisplTotalGrad(1);
    return eleInfo.setVector(dvdh);
  }

  else if (responseID == 7) {
    //const Matrix &pts = quadRule.getIntegrPointCoords(numSections);
    double xi[maxNumSections];
    beamInt->getSectionLocations(numSections, L, xi);
    Vector locs(numSections);
    for (int i = 0; i < numSections; i++)
      locs(i) = xi[i] * L;
    return eleInfo.setVector(locs);
  }

  else if (responseID == 8) {
    //const Vector &wts = quadRule.getIntegrPointWeights(numSections);
    double wt[maxNumSections];
    beamInt->getSectionWeights(numSections, L, wt);
    Vector weights(numSections);
    for (int i = 0; i < numSections; i++)
      weights(i) = wt[i] * L;
    return eleInfo.setVector(weights);
  }

  else if (responseID == 110) {
    ID tags(numSections);
    for (int i = 0; i < numSections; i++)
      tags(i) = theSections[i]->getTag();
    return eleInfo.setID(tags);
  }

  else
    return -1;
}

// AddingSensitivity:BEGIN ///////////////////////////////////
int
TimoshenkoBeamColumn2d::setParameter(const char **argv, int argc, Parameter &param)
{
  if (argc < 1)
    return -1;

  // If the parameter belongs to the element itself
  if (strcmp(argv[0], "rho") == 0)
    return param.addObject(1, this);

  if (strstr(argv[0], "sectionX") != 0) {
    if (argc < 3)
      return -1;

    float sectionLoc = atof(argv[1]);

    double xi[maxNumSections];
    double L = crdTransf->getInitialLength();
    beamInt->getSectionLocations(numSections, L, xi);

    sectionLoc /= L;

    float minDistance = fabs(xi[0] - sectionLoc);
    int sectionNum    = 0;
    for (int i = 1; i < numSections; i++) {
      if (fabs(xi[i] - sectionLoc) < minDistance) {
        minDistance = fabs(xi[i] - sectionLoc);
        sectionNum  = i;
      }
    }
    return theSections[sectionNum]->setParameter(&argv[2], argc - 2, param);
  }
  // If the parameter belongs to a section or lower
  if (strstr(argv[0], "section") != 0) {

    if (argc < 3)
      return -1;

    // Get section number: 1...Np
    int sectionNum = atoi(argv[1]);

    if (sectionNum > 0 && sectionNum <= numSections)
      return theSections[sectionNum - 1]->setParameter(&argv[2], argc - 2, param);

    else
      return -1;
  }

  else if (strstr(argv[0], "integration") != 0) {

    if (argc < 2)
      return -1;

    return beamInt->setParameter(&argv[1], argc - 1, param);
  }
  int result = -1;
  // Default, send to every object
  int ok = 0;
  for (int i = 0; i < numSections; i++) {
    ok = theSections[i]->setParameter(argv, argc, param);
    if (ok != -1)
      result = ok;
  }
  ok = beamInt->setParameter(argv, argc, param);
  if (ok != -1)
    result = ok;
  return result;
}

int
TimoshenkoBeamColumn2d::updateParameter(int parameterID, Information &info)
{
  if (parameterID == 1) {
    rho = info.theDouble;
    return 0;
  } else
    return -1;
}

int
TimoshenkoBeamColumn2d::activateParameter(int passedParameterID)
{
  parameterID = passedParameterID;

  return 0;
}

const Matrix &
TimoshenkoBeamColumn2d::getKiSensitivity(int gradNumber)
{
  K.Zero();
  return K;
}

const Matrix &
TimoshenkoBeamColumn2d::getMassSensitivity(int gradNumber)
{
  K.Zero();
  return K;
}

const Vector &
TimoshenkoBeamColumn2d::getResistingForceSensitivity(int gradNumber)
{
  double L        = crdTransf->getInitialLength();
  double oneOverL = 1.0 / L;

  //const Matrix &pts = quadRule.getIntegrPointCoords(numSections);
  //const Vector &wts = quadRule.getIntegrPointWeights(numSections);
  double xi[maxNumSections];
  beamInt->getSectionLocations(numSections, L, xi);
  double wt[maxNumSections];
  beamInt->getSectionWeights(numSections, L, wt);

  // Zero for integration
  static Vector dqdh(3);
  dqdh.Zero();

  // Loop over the integration points
  for (int i = 0; i < numSections; i++) {

    int order      = theSections[i]->getOrder();
    const ID &code = theSections[i]->getType();

    //double xi6 = 6.0*pts(i,0);
    double xi6 = 6.0 * xi[i];
    //double wti = wts(i);
    double wti = wt[i];

    // Some extra declarations
    double dLdh = crdTransf->getLengthGrad();

    const Matrix &ks = theSections[i]->getSectionTangent();
    double EI        = 0.0;
    double GA        = 0.0;
    for (int k = 0; k < order; k++) {
      if (code(k) == SECTION_RESPONSE_MZ) {
        EI += ks(k, k);
      }
      if (code(k) == SECTION_RESPONSE_VY) {
        GA += ks(k, k);
      }
    }

    const Matrix &sens = theSections[i]->getSectionTangentSensitivity(gradNumber);
    double dk11dh      = 0.0;
    double dk22dh      = 0.0;

    for (int k = 0; k < order; k++) {
      if (code(k) == SECTION_RESPONSE_MZ) {
        dk11dh += sens(k, k);
      }
      if (code(k) == SECTION_RESPONSE_VY) {
        dk22dh += sens(k, k);
      }
    }

    double phi    = 0.0;
    double dphidh = 0.0;
    if (GA != 0.0) {
      phi    = 12 * EI / (GA * L * L);
      dphidh = 12 * (dk11dh * GA * L - EI * (dk22dh * L + 2 * dLdh * GA)) / (GA * GA) /
               (L * L * L);
    }

    Vector s(workArea, order);
    s = theSections[i]->getStressResultant();

    // Get section stress resultant gradient
    const Vector &dsdh = theSections[i]->getStressResultantSensitivity(gradNumber, true);

    // Perform numerical integration on internal force gradient
    double sensi, si;
    for (int j = 0; j < order; j++) {
      sensi = dsdh(j) * wti;
      si    = s(j) * wti;
      switch (code(j)) {
      case SECTION_RESPONSE_P:
        dqdh(0) += sensi;
        break;
      case SECTION_RESPONSE_MZ:
        dqdh(1) += 1.0 / (1 + phi) * (xi6 - 4.0 - phi) * sensi -
                   dphidh / pow((1 + phi), 2) * (xi6 - 4.0 - phi) * si +
                   1.0 / (1 + phi) * (-dphidh) * si;
        dqdh(2) += 1.0 / (1 + phi) * (xi6 - 2.0 + phi) * sensi -
                   dphidh / pow((1 + phi), 2) * (xi6 - 2.0 + phi) * si +
                   1.0 / (1 + phi) * (+dphidh) * si;
        break;
      case SECTION_RESPONSE_VY:
        dqdh(1) += 0.5 * phi * L / (1 + phi) * sensi +
                   0.5 * dphidh / pow((1 + phi), 2) * L * si +
                   0.5 * phi * dLdh / (1 + phi) * si;
        dqdh(2) += 0.5 * phi * L / (1 + phi) * sensi +
                   0.5 * dphidh / pow((1 + phi), 2) * L * si +
                   0.5 * phi * dLdh / (1 + phi) * si;
        break;
      default:
        break;
      }
    }
  }

  // Transform forces
  static Vector dp0dh(3); // No distributed loads

  P.Zero();

  //////////////////////////////////////////////////////////////

  if (crdTransf->isShapeSensitivity()) {
    // Perform numerical integration to obtain basic stiffness matrix
    // Some extra declarations
    static Matrix kbmine(3, 3);
    kbmine.Zero();
    q.Zero();

    double tmp;

    for (int i = 0; i < numSections; i++) {

      int order      = theSections[i]->getOrder();
      const ID &code = theSections[i]->getType();

      //double xi6 = 6.0*pts(i,0);
      double xi6 = 6.0 * xi[i];
      //double wti = wts(i);
      double wti = wt[i];

      const Vector &s  = theSections[i]->getStressResultant();
      const Matrix &ks = theSections[i]->getSectionTangent();
      double EI        = 0.0;
      double GA        = 0.0;
      for (int k = 0; k < order; k++) {
        if (code(k) == SECTION_RESPONSE_MZ)
          EI += ks(k, k);
        if (code(k) == SECTION_RESPONSE_VY)
          GA += ks(k, k);
      }
      double phi = 0.0;
      if (GA != 0.0)
        phi = 12 * EI / (GA * L * L);

      Matrix ka(workArea, order, 3);
      ka.Zero();

      double si;
      for (int j = 0; j < order; j++) {
        si = s(j) * wti;
        switch (code(j)) {
        case SECTION_RESPONSE_P:
          q(0) += si;
          for (int k = 0; k < order; k++) {
            ka(k, 0) += ks(k, j) * wti;
          }
          break;
        case SECTION_RESPONSE_MZ:
          q(1) += 1.0 / (1 + phi) * (xi6 - 4.0 - phi) * si;
          q(2) += 1.0 / (1 + phi) * (xi6 - 2.0 + phi) * si;
          for (int k = 0; k < order; k++) {
            tmp = ks(k, j) * wti;
            ka(k, 1) += 1.0 / (1 + phi) * (xi6 - 4.0 - phi) * tmp;
            ka(k, 2) += 1.0 / (1 + phi) * (xi6 - 2.0 + phi) * tmp;
          }
          break;
        case SECTION_RESPONSE_VY:
          q(1) += 0.5 * phi * L / (1 + phi) * si;
          q(2) += 0.5 * phi * L / (1 + phi) * si;
          for (int k = 0; k < order; k++) {
            tmp = ks(k, j) * wti;
            ka(k, 1) += 0.5 * phi * L / (1 + phi) * tmp;
            ka(k, 2) += 0.5 * phi * L / (1 + phi) * tmp;
          }
          break;
        default:
          break;
        }
      }
      for (int j = 0; j < order; j++) {
        switch (code(j)) {
        case SECTION_RESPONSE_P:
          for (int k = 0; k < 3; k++) {
            kbmine(0, k) += ka(j, k);
          }
          break;
        case SECTION_RESPONSE_MZ:
          for (int k = 0; k < 3; k++) {
            tmp = ka(j, k);
            kbmine(1, k) += 1.0 / (1 + phi) * (xi6 - 4.0 - phi) * tmp;
            kbmine(2, k) += 1.0 / (1 + phi) * (xi6 - 2.0 + phi) * tmp;
          }
          break;
        case SECTION_RESPONSE_VY:
          for (int k = 0; k < 3; k++) {
            tmp = ka(j, k);
            kbmine(1, k) += 0.5 * phi * L / (1 + phi) * tmp;
            kbmine(2, k) += 0.5 * phi * L / (1 + phi) * tmp;
          }
          break;
        default:
          break;
        }
      }
    }

    const Vector &A_u = crdTransf->getBasicTrialDisp();
    double dLdh       = crdTransf->getLengthGrad();
    double d1overLdh  = -dLdh / (L * L);
    // a^T k_s dadh v
    dqdh.addMatrixVector(1.0, kbmine, A_u, d1overLdh);

    // k dAdh u
    const Vector &dAdh_u = crdTransf->getBasicDisplFixedGrad();
    dqdh.addMatrixVector(1.0, kbmine, dAdh_u, oneOverL);

    // dAdh^T q
    P += crdTransf->getGlobalResistingForceShapeSensitivity(q, dp0dh, gradNumber);
  }

  // A^T (dqdh + k dAdh u)
  P += crdTransf->getGlobalResistingForce(dqdh, dp0dh);

  return P;
}

// NEW METHOD
int
TimoshenkoBeamColumn2d::commitSensitivity(int gradNumber, int numGrads)
{
  // Get basic deformation and sensitivities
  const Vector &v = crdTransf->getBasicTrialDisp();

  static Vector dvdh(3);
  dvdh = crdTransf->getBasicDisplTotalGrad(gradNumber);

  double L        = crdTransf->getInitialLength();
  double oneOverL = 1.0 / L;
  //const Matrix &pts = quadRule.getIntegrPointCoords(numSections);
  double xi[maxNumSections];
  beamInt->getSectionLocations(numSections, L, xi);

  // Some extra declarations
  double d1oLdh = crdTransf->getd1overLdh();
  double dLdh   = crdTransf->getLengthGrad();

  // Loop over the integration points
  for (int i = 0; i < numSections; i++) {

    int order      = theSections[i]->getOrder();
    const ID &code = theSections[i]->getType();

    Vector e(workArea, order);

    //double xi6 = 6.0*pts(i,0);
    double xi6 = 6.0 * xi[i];

    const Matrix &ks = theSections[i]->getSectionTangent();
    double EI        = 0.0;
    double GA        = 0.0;
    for (int k = 0; k < order; k++) {
      if (code(k) == SECTION_RESPONSE_MZ) {
        EI += ks(k, k);
      }
      if (code(k) == SECTION_RESPONSE_VY) {
        GA += ks(k, k);
      }
    }

    const Matrix &sens = theSections[i]->getSectionTangentSensitivity(gradNumber);
    double dk11dh      = 0.0;
    double dk22dh      = 0.0;
    for (int k = 0; k < order; k++) {
      if (code(k) == SECTION_RESPONSE_MZ) {
        dk11dh += sens(k, k);
      }
      if (code(k) == SECTION_RESPONSE_VY) {
        dk22dh += sens(k, k);
      }
    }

    double phi    = 0.0;
    double dphidh = 0.0;
    if (GA != 0.0) {
      phi    = 12 * EI / (GA * L * L);
      dphidh = 12 * (dk11dh * GA * L - EI * (dk22dh * L + 2 * dLdh * GA)) / (GA * GA) /
               (L * L * L);
    }

    for (int j = 0; j < order; j++) {

      switch (code(j)) {
      case SECTION_RESPONSE_P:
        e(j) = oneOverL * dvdh(0) + d1oLdh * v(0);
        break;
      case SECTION_RESPONSE_MZ:
        e(j) = oneOverL * 1.0 / (1 + phi) *
                   ((xi6 - 4.0 - phi) * dvdh(1) + (xi6 - 2.0 + phi) * dvdh(2)) +
               d1oLdh * 1.0 / (1 + phi) *
                   ((xi6 - 4.0 - phi) * v(1) + (xi6 - 4.0 - phi) * v(2)) -
               oneOverL * dphidh / pow((1 + phi), 2) *
                   ((xi6 - 4.0 - phi) * v(1) + (xi6 - 4.0 - phi) * v(2)) +
               oneOverL * 1.0 / (1 + phi) * (-dphidh * v(1) + dphidh * v(2));
        break;
      case SECTION_RESPONSE_VY:
        e(j) = 0.5 * phi / (1 + phi) * (dvdh(1) + dvdh(2)) +
               0.5 * (v(1) + v(2)) * dphidh / pow((1 + phi), 2);
        break;
      default:
        e(j) = 0.0;
        break;
      }
    }

    // Set the section deformations
    theSections[i]->commitSensitivity(e, gradNumber, numGrads);
  }

  return 0;
}

// AddingSensitivity:END /////////////////////////////////////////////
