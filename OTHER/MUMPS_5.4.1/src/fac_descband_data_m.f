C
C  This file is part of MUMPS 5.4.1, released
C  on Tue Aug  3 09:49:43 UTC 2021
C
C
C  Copyright 1991-2021 CERFACS, CNRS, ENS Lyon, INP Toulouse, Inria,
C  Mumps Technologies, University of Bordeaux.
C
C  This version of MUMPS is provided to you free of charge. It is
C  released under the CeCILL-C license 
C  (see doc/CeCILL-C_V1-en.txt, doc/CeCILL-C_V1-fr.txt, and
C  https://cecill.info/licences/Licence_CeCILL-C_V1-en.html)
C
       MODULE MUMPS_FAC_DESCBAND_DATA_M
       IMPLICIT NONE
#if ! defined(NO_FDM_DESCBAND)
       INTEGER, SAVE :: INODE_WAITED_FOR
       PRIVATE
       PUBLIC :: DESCBAND_STRUC_T, MUMPS_FDBD_INIT, MUMPS_FDBD_END,
     &         MUMPS_FDBD_SAVE_DESCBAND, MUMPS_FDBD_IS_DESCBAND_STORED,
     &         MUMPS_FDBD_RETRIEVE_DESCBAND,
     &         MUMPS_FDBD_FREE_DESCBAND_STRUC,
     &         INODE_WAITED_FOR
       TYPE DESCBAND_STRUC_T
         INTEGER :: INODE, LBUFR
         INTEGER, POINTER, DIMENSION(:) :: BUFR
       END TYPE DESCBAND_STRUC_T
       TYPE (DESCBAND_STRUC_T), POINTER, DIMENSION(:), SAVE::FDBD_ARRAY
       CONTAINS
       SUBROUTINE MUMPS_FDBD_INIT( INITIAL_SIZE, INFO )
       INTEGER, INTENT(IN) :: INITIAL_SIZE
       INTEGER, INTENT(INOUT) :: INFO(2)
       INTEGER :: I, IERR
       ALLOCATE(FDBD_ARRAY( INITIAL_SIZE ), stat=IERR)
       IF (IERR > 0 ) THEN
         INFO(1)=-13
         INFO(2)=INITIAL_SIZE
         RETURN
       ENDIF
       DO I=1, INITIAL_SIZE
         FDBD_ARRAY(I)%INODE=-9999
         FDBD_ARRAY(I)%LBUFR=-9999
         NULLIFY(FDBD_ARRAY(I)%BUFR)
       ENDDO
       INODE_WAITED_FOR = -1
       RETURN
       END SUBROUTINE MUMPS_FDBD_INIT
       FUNCTION MUMPS_FDBD_IS_DESCBAND_STORED( INODE, IWHANDLER )
       LOGICAL :: MUMPS_FDBD_IS_DESCBAND_STORED 
       INTEGER, INTENT(IN) :: INODE
       INTEGER, INTENT(OUT) :: IWHANDLER
       INTEGER :: I
       DO I = 1, size(FDBD_ARRAY)
         IF (FDBD_ARRAY(I)%INODE .EQ. INODE) THEN
           IWHANDLER = I
           MUMPS_FDBD_IS_DESCBAND_STORED = .TRUE.
           RETURN
         ENDIF
       ENDDO
       MUMPS_FDBD_IS_DESCBAND_STORED = .FALSE.
       RETURN
       END FUNCTION MUMPS_FDBD_IS_DESCBAND_STORED
       SUBROUTINE MUMPS_FDBD_SAVE_DESCBAND(INODE, LBUFR, BUFR,
     &                                     IWHANDLER, INFO)
       USE MUMPS_FRONT_DATA_MGT_M, ONLY : MUMPS_FDM_START_IDX
       INTEGER, INTENT(IN) :: INODE, LBUFR, BUFR(LBUFR)
       INTEGER, INTENT(INOUT) :: INFO(2)
       INTEGER, INTENT(OUT) :: IWHANDLER
       TYPE(DESCBAND_STRUC_T), POINTER, DIMENSION(:) :: FDBD_ARRAY_TMP
       INTEGER :: OLD_SIZE, NEW_SIZE, I, IERR
       IWHANDLER = -1
       CALL MUMPS_FDM_START_IDX('A', 'DESCBAND', IWHANDLER, INFO)
       IF (INFO(1) .LT. 0) RETURN
       IF (IWHANDLER > size(FDBD_ARRAY)) THEN
         OLD_SIZE = size(FDBD_ARRAY)
         NEW_SIZE = max( (OLD_SIZE * 3) / 2 + 1, IWHANDLER)
         ALLOCATE(FDBD_ARRAY_TMP(NEW_SIZE),stat=IERR)
         IF (IERR.GT.0) THEN
           INFO(1)=-13
           INFO(2)=NEW_SIZE
           RETURN
         ENDIF
         DO I=1, OLD_SIZE
           FDBD_ARRAY_TMP(I)=FDBD_ARRAY(I)
         ENDDO
         DO I=OLD_SIZE+1, NEW_SIZE
           FDBD_ARRAY_TMP(I)%INODE = -9999
           FDBD_ARRAY_TMP(I)%LBUFR = -9999
           NULLIFY(FDBD_ARRAY_TMP(I)%BUFR)
         ENDDO
         DEALLOCATE(FDBD_ARRAY)
         FDBD_ARRAY=>FDBD_ARRAY_TMP
         NULLIFY(FDBD_ARRAY_TMP)
       ENDIF
       FDBD_ARRAY(IWHANDLER)%INODE = INODE
       FDBD_ARRAY(IWHANDLER)%LBUFR = LBUFR
       ALLOCATE(FDBD_ARRAY(IWHANDLER)%BUFR(LBUFR), stat=IERR)
       IF (IERR > 0 ) THEN
         INFO(1)=-13
         INFO(2)=LBUFR
         RETURN
       ENDIF
       FDBD_ARRAY(IWHANDLER)%BUFR = BUFR
       RETURN
       END SUBROUTINE MUMPS_FDBD_SAVE_DESCBAND
       SUBROUTINE MUMPS_FDBD_RETRIEVE_DESCBAND(IWHANDLER,DESCBAND_STRUC)
       INTEGER, INTENT(IN) :: IWHANDLER
#if defined(MUMPS_F2003)
       TYPE (DESCBAND_STRUC_T), POINTER, INTENT(OUT) :: DESCBAND_STRUC
#else
       TYPE (DESCBAND_STRUC_T), POINTER :: DESCBAND_STRUC
#endif
       DESCBAND_STRUC => FDBD_ARRAY(IWHANDLER)
       RETURN
       END SUBROUTINE MUMPS_FDBD_RETRIEVE_DESCBAND
       SUBROUTINE MUMPS_FDBD_FREE_DESCBAND_STRUC(IWHANDLER)
       USE MUMPS_FRONT_DATA_MGT_M, ONLY : MUMPS_FDM_END_IDX
       INTEGER, INTENT(INOUT) :: IWHANDLER
       TYPE (DESCBAND_STRUC_T), POINTER :: DESCBAND_STRUC
       DESCBAND_STRUC => FDBD_ARRAY(IWHANDLER)
       DESCBAND_STRUC%INODE = -7777 
       DESCBAND_STRUC%LBUFR = -7777
       DEALLOCATE(DESCBAND_STRUC%BUFR)
       NULLIFY(DESCBAND_STRUC%BUFR)
       CALL MUMPS_FDM_END_IDX('A', 'DESCBAND', IWHANDLER)
       RETURN
       END SUBROUTINE MUMPS_FDBD_FREE_DESCBAND_STRUC
       SUBROUTINE MUMPS_FDBD_END(INFO1)
       INTEGER, INTENT(IN) :: INFO1
       INTEGER :: I, IWHANDLER
       IF (.NOT. associated(FDBD_ARRAY)) THEN
         WRITE(*,*) "Internal error 1 in MUMPS_FAC_FDBD_END"
         CALL MUMPS_ABORT()
       ENDIF
       DO I=1, size(FDBD_ARRAY)
         IF (FDBD_ARRAY(I)%INODE .GE. 0) THEN
           IF (INFO1 .GE.0) THEN
             WRITE(*,*) "Internal error 2 in MUMPS_FAC_FDBD_END",I
             CALL MUMPS_ABORT()
           ELSE
             IWHANDLER=I
             CALL MUMPS_FDBD_FREE_DESCBAND_STRUC(IWHANDLER)
           ENDIF
         ENDIF
       ENDDO
       DEALLOCATE(FDBD_ARRAY)
       RETURN
       END SUBROUTINE MUMPS_FDBD_END
#endif
       END MODULE MUMPS_FAC_DESCBAND_DATA_M
