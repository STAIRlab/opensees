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
// Description: This file contains the class definition for
// LinearCrdTransf2d.h. LinearCrdTransf2d provides the
// abstraction of a linear transformation for a spatial frame
// between the global and basic coordinate systems
//
// Written: Remo Magalhaes de Souza (rmsouza@ce.berkeley.edu)
// Created: 04/2000
//
#ifndef LinearCrdTransf2d_h
#define LinearCrdTransf2d_h

#include <FrameTransform.h>
#include <Vector.h>
#include <Matrix.h>


class LinearCrdTransf2d: public FrameTransform2d
{
public:
    LinearCrdTransf2d(int tag);
    LinearCrdTransf2d(int tag,
		      const Vector &rigJntOffsetI,
		      const Vector &rigJntOffsetJ);
    
    LinearCrdTransf2d();
    ~LinearCrdTransf2d();
    
    const char *getClassType() const {return "LinearCrdTransf2d";};
    
    int initialize(Node *node1Pointer, Node *node2Pointer);
    int update();
    double getInitialLength();
    double getDeformedLength();
    
    int commitState();
    int revertToLastCommit();        
    int revertToStart();
    
    const Vector &getBasicTrialDisp();
    const Vector &getBasicIncrDisp();
    const Vector &getBasicIncrDeltaDisp();
    const Vector &getBasicTrialVel();
    const Vector &getBasicTrialAccel();
    
    // AddingSensitivity:BEGIN //////////////////////////////////
    const Vector &getBasicDisplTotalGrad(int gradNumber);
    const Vector &getGlobalResistingForceShapeSensitivity(const Vector &basicForce, const Vector &p0);
    const Vector &getBasicDisplFixedGrad();

    // ---MHS
    const Vector & getGlobalResistingForceShapeSensitivity(const Vector &pb,
							   const Vector &p0,
							   int gradNumber);
    bool isShapeSensitivity();
    double getLengthGrad();
    double getd1overLdh();    
    
    // AddingSensitivity:END //////////////////////////////////
    
    const Vector &getGlobalResistingForce(const Vector &basicForce, const Vector &p0);
    const Matrix &getGlobalStiffMatrix(const Matrix &basicStiff, const Vector &basicForce);
    const Matrix &getInitialGlobalStiffMatrix(const Matrix &basicStiff);
    
    CrdTransf *getCopy2d();
    
    int sendSelf(int cTag, Channel &theChannel);
    int recvSelf(int cTag, Channel &theChannel, FEM_ObjectBroker &theBroker);
    
    void Print(OPS_Stream &s, int flag = 0);
    
    // method used to rotate consistent mass matrix
    const Matrix &getGlobalMatrixFromLocal(const Matrix &local);
    
    // methods used in post-processing only
    const Vector &getPointGlobalCoordFromLocal(const Vector &localCoords);
    const Vector &getPointGlobalDisplFromBasic(double xi, const Vector &basicDisps);
    const Vector &getPointLocalDisplFromBasic(double xi, const Vector &basicDisps);    

    int getLocalAxes(Vector &xAxis, Vector &yAxis, Vector &zAxis);
    
private:
    int computeElemtLengthAndOrient();
    void compTransfMatrixLocalGlobal(Matrix &Tlg);
    
    // internal data
    Node *nodeIPtr, *nodeJPtr;  // pointers to the element two endnodes
    
    double *nodeIOffset, *nodeJOffset;	// rigid joint offsets
    
    double cosTheta, sinTheta;  // direction cosines of undeformed element wrt to global system 
    double L;  // undeformed element length

    static Matrix Tlg;  // matrix that transforms from global to local coordinates
    static Matrix kg;   // global stiffness matrix

    double *nodeIInitialDisp, *nodeJInitialDisp;
    bool initialDispChecked;
};

#endif
